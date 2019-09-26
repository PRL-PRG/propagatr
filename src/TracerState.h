
#ifndef PROMISEDYNTRACER_TRACER_STATE_H
#define PROMISEDYNTRACER_TRACER_STATE_H

#include "Argument.h"
#include "Call.h"
#include "DenotedValue.h"
#include "DependencyNodeGraph.h"
#include "Event.h"
#include "ExecutionContextStack.h"
#include "Function.h"
#include "sexptypes.h"
#include "stdlibs.h"

#include <iostream>
#include <set>
#include <unordered_map>

class TracerState {
  /***************************************************************************
   * Function API
   ***************************************************************************/
public:
  const std::string &get_output_dirpath() const { return output_dirpath_; }

  execution_contexts_t unwind_stack(const RCNTXT *context) {
    return get_stack_().unwind(ExecutionContext(context));
  }

  void remove_promise(const SEXP promise, DenotedValue *promise_state) {
    promises_.erase(promise);
    destroy_promise(promise_state);
  }

  bool get_truncate() const { return truncate_; }

  bool is_verbose() const { return verbose_; }

  bool is_binary() const { return binary_; }

  int get_compression_level() const { return compression_level_; }

  void exit_probe(const Event event) { resume_execution_timer(); }

  void enter_probe(const Event event) {
    pause_execution_timer();
    increment_timestamp_();
    ++event_counter_[to_underlying(event)];
  }

  void enter_gc() { ++gc_cycle_; }

  void pause_execution_timer() {
    auto execution_pause_time = std::chrono::high_resolution_clock::now();
    std::uint64_t execution_time =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            execution_pause_time - execution_resume_time_)
            .count();
    ExecutionContextStack &stack(get_stack_());
    if (!stack.is_empty()) {
      stack.peek(1).increment_execution_time(execution_time);
    }
  }

  void resume_execution_timer() {
    execution_resume_time_ = std::chrono::high_resolution_clock::now();
  }

  void initialize() const { serialize_configuration_(); }

  std::unordered_map<SEXP, DenotedValue *> promises_;
  denoted_value_id_t denoted_value_id_counter_;

  ExecutionContextStack &get_stack_() { return stack_; }

  TracerState(const std::string &output_dirpath, bool verbose, bool truncate,
              bool binary, int compression_level)
      : output_dirpath_(output_dirpath), verbose_(verbose), truncate_(truncate),
        binary_(binary), compression_level_(compression_level), timestamp_(0),
        event_counter_(to_underlying(Event::COUNT), 0) {}

  Function *lookup_function(const SEXP op) {
    Function *function = nullptr;
    auto iter = functions_.find(op);
    if (iter != functions_.end()) {
      return iter->second;
    }
    const auto [package_name, function_definition, function_id] =
        Function::compute_definition_and_id(op);
    auto iter2 = function_cache_.find(function_id);
    if (iter2 == function_cache_.end()) {
      function =
          new Function(op, package_name, function_definition, function_id);
      function_cache_.insert({function_id, function});
    } else {
      function = iter2->second;
    }
    functions_.insert({op, function});
    return function;
  }

  void remove_function(const SEXP op) {
    auto it = functions_.find(op);
    if (it != functions_.end()) {
      functions_.erase(it);
    }
  }

  DenotedValue *lookup_promise(const SEXP promise, bool create = false,
                               bool local = false) {
    static int printed = 0;
    auto iter = promises_.find(promise);

    /* all promises encountered are added to the map. Its not possible for
       a promise id to be encountered which is not already mapped.
       If this happens, possibly, the mapper methods are not the first to
       be called in the analysis. Hence, they are not able to update the
       mapping. */
    if (iter == promises_.end()) {
      if (create) {
        DenotedValue *promise_state(create_raw_promise_(promise, local));
        promises_.insert({promise, promise_state});
        return promise_state;
      } else {
        return nullptr;
      }
    }
    return iter->second;
  }

  DenotedValue *create_raw_promise_(const SEXP promise, bool local) {
    SEXP rho = dyntrace_get_promise_environment(promise);

    DenotedValue *promise_state =
        new DenotedValue(get_next_denoted_value_id_(), promise, local);

    promise_state->set_creation_scope(infer_creation_scope());

    /* Setting this bit tells us that the promise is currently in the
       promises table. As long as this is set, the call holding a reference
       to it will not delete it. */
    promise_state->set_active();
    return promise_state;
  }

  scope_t infer_creation_scope() {
    ExecutionContextStack &stack = get_stack_();

    for (auto iter = stack.crbegin(); iter != stack.crend(); ++iter) {
      if (iter->is_call()) {
        const Function *const function = iter->get_call()->get_function();
        /* '{' function as promise creation source is not very
           insightful. We want to keep going back until we find
           something meaningful. */
        if (!function->is_curly_bracket()) {
          return function->get_id();
        }
      }
    }
    return TOP_LEVEL_SCOPE;
  }

  template <typename T> void push_stack(T *context) {
    get_stack_().push(context);
  }

  void cleanup(int error) {for (auto const& binding: promises_) {
            destroy_promise(binding.second);
        }

        promises_.clear();

        for (auto const& binding: function_cache_) {
            destroy_function_(binding.second);
        }

        functions_.clear();

        function_cache_.clear();

        if (!get_stack_().is_empty()) {
            dyntrace_log_error("stack not empty on tracer exit.")
        }

        if (error) {
            std::ofstream error_file(get_output_dirpath() + "/ERROR");
            error_file << "ERROR";
            error_file.close();
        } else {
            std::ofstream noerror_file(get_output_dirpath() + "/NOERROR");
            noerror_file << "NOERROR";
            noerror_file.close();
        }
    }

    void destroy_promise(DenotedValue* promise_state) {
        /* here we delete a promise iff we are the only one holding a
           reference to it. A promise can be simultaneously held by
           a call and this promise map. While it is held by the promise
           map, the active flag is set and while it is held by the call
           the argument flag is set. So, to decide if we have to delete
           the promise at this point, we first unset the active flag
           (because we are interesting in removing the promise) and then,
           we check the argument flag. If argument flag is unset, it means
           the promise is not held by a call and can be deleted. If the
           argument flag is set, it means the promise is held by a call
           and when that call gets deleted, it will delete this promise */
        promise_state->set_destruction_gc_cycle(get_current_gc_cycle_());

        promise_state->set_inactive();

        if (!promise_state->is_argument()) {
            delete promise_state;
        }
    }

    gc_cycle_t get_current_gc_cycle_() {
        return gc_cycle_;
    }

    Call* create_call(const SEXP call,
                      const SEXP op,
                      const SEXP args,
                      const SEXP rho) {
        Function* function = lookup_function(op);
        Call* function_call = nullptr;
        call_id_t call_id = get_next_call_id_();
        const std::string function_name = get_name(call);

        function_call = new Call(call_id, function_name, rho, function);

        if (TYPEOF(op) == CLOSXP) {
            process_closure_arguments_(function_call, op);
        } else {
            int eval = dyntrace_get_c_function_argument_evaluation(op);
            function_call->set_force_order(eval);
        }

        return function_call;
    }

    void destroy_call(Call* call) {
        Function* function = call->get_function();

        function->add_summary(call);

        for (Argument* argument: call->get_arguments()) {
            DenotedValue* value = argument->get_denoted_value();

            if (!value->is_active()) {
                delete value;
            } else {
                value->remove_argument(
                    call->get_id(),
                    call->get_function()->get_id(),
                    call->get_return_value_type(),
                    call->get_function()->get_formal_parameter_count(),
                    argument);
            }

            argument->set_denoted_value(nullptr);

            delete argument;
        }

        delete call;
    }

    void notify_caller(Call* callee) {
        ExecutionContextStack& stack = get_stack_();

        if (!stack.is_empty()) {
            ExecutionContext exec_ctxt = stack.peek(1);

            if (!exec_ctxt.is_call()) {
                return;
            }

            Call* caller = exec_ctxt.get_call();
            Function* function = caller->get_function();
        }
    }

    ExecutionContext pop_stack() {
        ExecutionContextStack& stack(get_stack_());
        ExecutionContext exec_ctxt(stack.pop());
        if (!stack.is_empty()) {
            stack.peek(1).increment_execution_time(
                exec_ctxt.get_execution_time());
        }
        return exec_ctxt;
    }

    // propagatr logic is in DependencyNodeGraph

    DependencyNodeGraph& get_dependencies() {
        return dependencies_;
    }

  private:
    ExecutionContextStack stack_;
    const std::string output_dirpath_;
    gc_cycle_t gc_cycle_;
    const bool verbose_;
    const bool truncate_;
    const bool binary_;
    const int compression_level_;
    std::chrono::time_point<std::chrono::high_resolution_clock>
        execution_resume_time_;
    std::vector<unsigned long int> event_counter_;
    timestamp_t timestamp_;
    call_id_t call_id_counter_;

    // this is for propagatr specifically
    DependencyNodeGraph dependencies_;

    call_id_t get_next_call_id_() {
        return ++call_id_counter_;
    }

    timestamp_t increment_timestamp_() {
        return timestamp_++;
    }

    void serialize_configuration_() const {
        std::ofstream fout(get_output_dirpath() + "/CONFIGURATION",
                           std::ios::trunc);

        auto serialize_row = [&fout](const std::string& key,
                                     const std::string& value) {
            fout << key << "=" << value << std::endl;
        };

        for (const std::string& envvar: ENVIRONMENT_VARIABLES) {
            serialize_row(envvar, to_string(getenv(envvar.c_str())));
        }

        serialize_row("GIT_COMMIT_INFO", GIT_COMMIT_INFO);
        serialize_row("truncate", std::to_string(get_truncate()));
        serialize_row("verbose", std::to_string(is_verbose()));
        serialize_row("binary", std::to_string(is_binary()));
        serialize_row("compression_level",
                      std::to_string(get_compression_level()));
    }

    denoted_value_id_t get_next_denoted_value_id_() {
        return denoted_value_id_counter_++;
    }

    void destroy_function_(Function* function) {
        delete function;
    }

    std::unordered_map<SEXP, Function*> functions_;
    std::unordered_map<function_id_t, Function*> function_cache_;

    void process_closure_argument_(Call* call,
                                   int formal_parameter_position,
                                   int actual_argument_position,
                                   const SEXP name,
                                   const SEXP argument,
                                   bool dot_dot_dot) {
        DenotedValue* value = nullptr;
        /* only add to promise map if the argument is a promise */
        if (type_of_sexp(argument) == PROMSXP) {
            value = lookup_promise(argument, true);
        } else {
            value =
                new DenotedValue(get_next_denoted_value_id_(), argument, false);
            value->set_creation_scope(infer_creation_scope());
        }
        bool default_argument = true;
        if (value->is_promise()) {
            default_argument =
                call->get_environment() == value->get_environment();
        }
        Argument* arg = new Argument(call,
                                     formal_parameter_position,
                                     actual_argument_position,
                                     default_argument,
                                     dot_dot_dot);
        arg->set_denoted_value(value);
        value->add_argument(arg);
        call->add_argument(arg);
    }

    void process_closure_arguments_(Call* call, const SEXP op) {
        SEXP formal = nullptr;
        SEXP name = nullptr;
        SEXP argument = nullptr;
        SEXP rho = call->get_environment();
        int formal_parameter_position = -1;
        int actual_argument_position = -1;
        for (formal = FORMALS(op); formal != R_NilValue; formal = CDR(formal)) {
            ++formal_parameter_position;
            /* get argument name */
            name = TAG(formal);
            /* lookup argument in environment by name */
            argument = dyntrace_lookup_environment(rho, name);
            switch (type_of_sexp(argument)) {
            case DOTSXP:
                for (SEXP dot_dot_dot_arguments = argument;
                     dot_dot_dot_arguments != R_NilValue;
                     dot_dot_dot_arguments = CDR(dot_dot_dot_arguments)) {
                    ++actual_argument_position;
                    name = TAG(dot_dot_dot_arguments);
                    SEXP dot_dot_dot_argument = CAR(dot_dot_dot_arguments);
                    process_closure_argument_(call,
                                              formal_parameter_position,
                                              actual_argument_position,
                                              name,
                                              dot_dot_dot_argument,
                                              true);
                }
                break;
            default:
                ++actual_argument_position;
                process_closure_argument_(call,
                                          formal_parameter_position,
                                          actual_argument_position,
                                          name,
                                          argument,
                                          is_dots_symbol(name));
                break;
            }
        }
    }
};

#endif /* PROMISEDYNTRACER_TRACER_STATE_H */
