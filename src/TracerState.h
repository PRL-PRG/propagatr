
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
#include "CallTrace.h"

#include <iostream>
#include <set>
#include <sstream> // for serializing
#include <string>  // for serializing
#include <sys/types.h>
#include <sys/stat.h>

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

  void initialize() const { 
    serialize_configuration_();
  }

  std::unordered_map<SEXP, DenotedValue *> promises_;
  denoted_value_id_t denoted_value_id_counter_;

  ExecutionContextStack &get_stack_() { return stack_; }

  TracerState(const std::string &output_dirpath, const std::string &package_under_analysis, const std::string &analyzed_file_name, 
              bool verbose, bool truncate, bool binary, int compression_level)
      : output_dirpath_(output_dirpath), package_under_analysis_(package_under_analysis), analyzed_file_name_(analyzed_file_name), 
        verbose_(verbose), truncate_(truncate), binary_(binary), compression_level_(compression_level), timestamp_(0),
        event_counter_(to_underlying(Event::COUNT), 0) {  }

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

  void cleanup(int error) {
        
        for (auto const& binding: promises_) {
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

    int num_traces = 0;

    CallTrace * create_call_trace(std::string pname, 
                                  std::string fname, 
                                  function_id_t fn_id, 
                                  dyntrace_dispatch_t dispatch) {

      CallTrace * ct = new CallTrace(pname, fname, fn_id, dispatch, num_traces++);
      return ct;
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

    /*
    *
            typer stuff!!
    *
    */

    void deal_with_call_trace(CallTrace a_trace) {
        if (traces_.count(a_trace) == 1) {
            // its in
            counts_.insert_or_assign(a_trace, counts_.at(a_trace) + 1);
        } else {
            // its not in yet
            traces_.insert(std::make_pair(a_trace, a_trace));
            counts_.insert(std::pair<CallTrace, int>(a_trace, 1));
        }
    }

    // makes the string "type, {classes}, {attrs}"
    // std::string serialize_for_param_pos(std::unordered_map<int, Type> * trace_map, int pos) {
    std::string serialize_for_param_pos(Type type) {
      std::stringstream out;

      // type
      out << "\"" << type.get_top_level_type();

      // tags
      std::vector<std::string> * tags = type.get_tags();
      if (tags->size() != 0) {
        for (auto it = tags->begin(); it != tags->end(); ++it) {
          out << "@" << * it;
        }
      }
      
      out << "\",\"{";

      // classes
      std::vector<std::string> classes = type.get_classes();
      int size_as_int = classes.size();
      for (int i = 0; i < size_as_int; ++i) {
        
        out << classes[i];

        if (i != size_as_int - 1) {
          out << "-";
        } 
      }

      out << "}\",\"{";

        // attrs
      std::vector<std::string> attrs = type.get_attr_names();
      size_as_int = attrs.size();
      for (int i = 0; i < size_as_int; ++i) {
        out << attrs[i];

        if (i != size_as_int - 1) {
          out << "-";
        } 
      }

      out << "}\"";

      return out.str();
    }

    void serialize_traces_list() {

      // 1. init stringstream
      std::stringstream out;

      // make directories - this always runs before writing dependencies
      // TODO: ensure results directory exists or make
      struct stat info;
      if (stat(output_dirpath_.c_str(), &info) != 0) {
        // DNE, create
        mkdir_p(output_dirpath_.c_str(), S_IRWXU);
      } else {
        // exists, nothing to do
      }

      std::ofstream out_file(output_dirpath_ + "/traces_" + analyzed_file_name_ + ".txt");

      int max_of_max = 0;

      // 2. iterate through keys \in traces_
      //    print the trace + counts to file
      for (std::pair<CallTrace, CallTrace> element : traces_) {
        // what format do we want?
        CallTrace el = element.second;

        std::string dispatch_type = "None";
        switch(el.get_dispatch_type()) {
          case DYNTRACE_DISPATCH_S3:
            dispatch_type = "S3";
            break;
          case DYNTRACE_DISPATCH_S4:
            dispatch_type = "S4";
            break;
        }

        std::unordered_map<int, Type> trace_map = el.get_call_trace();
        out << package_under_analysis_ << "," << el.get_package_name() << "," << el.get_function_name() << ",\"" << el.get_fn_id() << "\"," << el.compute_hash() << "," << el.compute_hash_just_for_types() << "," << dispatch_type << "," << el.get_has_dots() << "," << counts_[el] << ",";

        std::vector<int> keys;
        keys.reserve(trace_map.size());
        for (auto kv : trace_map) {
          keys.push_back(kv.first);
        }

        int max_ = keys[0];
        for (int v : keys) {
          if (max_ < v) 
            max_ = v;
        }

        max_of_max = fmax(max_, max_of_max);

        for (int i = -1; i <= max_; ++i) {
          if (std::find(keys.begin(), keys.end(), i) != keys.end()) {
            // found
            Type type_to_serialize = trace_map.at(i);
            out << serialize_for_param_pos(type_to_serialize);
          } else {
            // put nothing
            out << "???,{},{}";
          }

          if (i != max_) {
              out << ",";
            } else {
              out << "\n";
            }
        }

        // get all (ret is first @ -1)
        /*
        int size_as_int = trace_map.size();
        for (int i = -1; i < (size_as_int - 1); ++i) {
          // TODO pass this by reference?
          std::cout << "i: " << i << "\n";

          Type type_to_serialize = trace_map.at(i);
          out << serialize_for_param_pos(type_to_serialize);

          if (i != size_as_int - 2) {
            out << ", ";
          } else {
            out << "\n";
          }
        }
        */

        // write to file and empty
        // TODO TODO TODO do we want to do this??
        // or write incrementally...
        // will these files ever get so big
        // out_file << out.rdbuf();
        // out.clear();
      }
      
      // generate the header, and TODO write it to the beginning of the file...
      // might involve having to copy the file

      int max_num_of_args = max_of_max;
      std::string init_header_string = "package_being_analyzed,package,fun_name,fun_id,trace_hash,type_hash,dispatch,has_dots,count,arg_t_r,arg_c_r,arg_a_r";
      for (int i = 0; i <= max_num_of_args; ++i) {
        std::string elt = ",arg_t" + std::to_string(i) + ",arg_c" + std::to_string(i) + ",arg_a" + std::to_string(i);
        init_header_string.append(elt);
      }

      // close file when finished
      out_file << init_header_string << "\n";
      out_file << out.rdbuf();
      out_file.close();
    }

    void serialize_dependencies() {
      std::stringstream out;

      // first, print the graph in the following form:
      out << dependencies_.serialize().rdbuf();

      std::ofstream out_file_2(output_dirpath_ + "/dependency_graph_" + analyzed_file_name_ + ".txt");
      out_file_2 << out.rdbuf();
      out_file_2.close();

      // then, print the fun_ids mapping to function names and packages
    }

    void serialize_and_output() {
      
      std::cout << "begin: serialize traces...\n\n";

      serialize_traces_list();

      std::cout << "begin: serialize dependencies...\n\n";

      serialize_dependencies();

      std::cout << "end: serialize...\n\n";
    }

  private:
    ExecutionContextStack stack_;
    const std::string output_dirpath_;
    const std::string package_under_analysis_;
    const std::string analyzed_file_name_;
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

    // this is for typr
    // traces_ should have a list of traces, we can look for hash collisions
    // and call it an already seen trace
    std::unordered_map<CallTrace, CallTrace, CallTraceHasher> traces_;
    // ^ is to see if we have already seen the calltrace (in a way that doesnt suck)
    std::unordered_map<CallTrace, int, CallTraceHasher> counts_;

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
            if (std::string(CHAR(name)) == "...") {
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
            } else {
                ++actual_argument_position;
                process_closure_argument_(call,
                                          formal_parameter_position,
                                          actual_argument_position,
                                          name,
                                          argument,
                                          is_dots_symbol(name));
            }
        }
    }
};

#endif /* PROMISEDYNTRACER_TRACER_STATE_H */
