
#include "probes.h"

inline TracerState& tracer_state(dyntracer_t* dyntracer) {
    return *(static_cast<TracerState*>(dyntracer->state));
}

static inline void set_dispatch(Call* call,
                               const dyntrace_dispatch_t dispatch) {
   if (dispatch == DYNTRACE_DISPATCH_S3) {
       call->set_S3_method();
   } else if (dispatch == DYNTRACE_DISPATCH_S4) {
       call->set_S4_method();
   }
}

void dyntrace_entry(dyntracer_t* dyntracer, SEXP expression, SEXP environment) {
    TracerState& state = tracer_state(dyntracer);

    /* we do not do state.enter_probe() in this function because this is a
       pseudo probe that executes before the tracing actually starts. this is
       only for initialization purposes. */

    state.initialize();

    // search_promises(dyntracer, R_BaseEnv);

    /* probe_exit here ensures we start the timer for timing argument execution.
     */
    state.exit_probe(Event::DyntraceEntry);
}

void dyntrace_exit(dyntracer_t* dyntracer,
                   SEXP expression,
                   SEXP environment,
                   SEXP result,
                   int error) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::DyntraceExit);

    /* Force an imaginary GC cycle at program end */
    state.enter_gc();

    state.cleanup(error);

    // serialize??
    state.serialize_and_output();

    /* we do not do start.exit_probe() because the tracer has finished
       executing and we don't need to resume the timer. */
}

void closure_entry(dyntracer_t* dyntracer,
                   const SEXP call,
                   const SEXP op,
                   const SEXP args,
                   const SEXP rho,
                   const dyntrace_dispatch_t dispatch) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::ClosureEntry);

    Call* function_call = state.create_call(call, op, args, rho);

    // forin C++
    /*
    function_id_t fn_id = function_call->get_function()->get_id();
    for (Argument* argument: call->get_arguments()) {
      SEXP arg_val = argument->get_denoted_value()->get_expression();

      // check if arg is in the table
      state.add_argument_dependency(arg_val, fn_id, argument->get_formal_parameter_position());
    }
    */

    set_dispatch(function_call, dispatch);

    state.push_stack(function_call);

    state.exit_probe(Event::ClosureEntry);
}

// for typr stuff
CallTrace deal_with_function_call(Call* function_call, SEXP return_value) {
    CallTrace trace_for_this_call = CallTrace(  function_call->get_function()->get_namespace(), 
                                                function_call->get_function_name());

    for (Argument* argument: function_call->get_arguments()) {
        int param_pos = argument->get_formal_parameter_position();
        DenotedValue * arg_val = argument->get_denoted_value();

        SEXP raw_obj = arg_val->get_raw_object();
        
        // if raw_obj is a promise, this will be recorded as 'unused'
        trace_for_this_call.add_to_call_trace(param_pos, Type(raw_obj));
    }

    trace_for_this_call.add_to_call_trace(-1, Type(return_value));

    return trace_for_this_call;
}

CallTrace deal_with_builtin_and_special(Call* function_call, SEXP args, SEXP return_value, TracerState* state) {
    CallTrace trace_for_this_call = CallTrace(  function_call->get_function()->get_namespace(), 
                                                function_call->get_function_name());

    int i = 0;
    
    for(SEXP cons = args; cons != R_NilValue; cons = CDR(cons)) {
        SEXP el = CAR(cons);

        // build up call trace
        trace_for_this_call.add_to_call_trace(i, Type(el));

        // dependencies
        state->get_dependencies().add_argument(el, function_call->get_function()->get_id(), i);

        i++;
    }

    // return value
    trace_for_this_call.add_to_call_trace(-1, Type(return_value));
    state->get_dependencies().add_argument(return_value, function_call->get_function()->get_id(), -1);

    return trace_for_this_call;

}

void closure_exit(dyntracer_t* dyntracer,
                  const SEXP call,
                  const SEXP op,
                  const SEXP args,
                  const SEXP rho,
                  const dyntrace_dispatch_t dispatch,
                  const SEXP return_value) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::ClosureExit);

    ExecutionContext exec_ctxt = state.pop_stack();

    if (!exec_ctxt.is_closure()) {
        dyntrace_log_error("Not found matching closure on stack");
    }

    Call* function_call = exec_ctxt.get_closure();

    /*
    function_id_t fn_id = function_call->get_function()->get_id();
    state.add_return_dependency(return_value, fn_id);
    */

    function_id_t fn_id = function_call->get_function()->get_id();

    for (Argument* argument: function_call->get_arguments()) {
      int param_pos = argument->get_formal_parameter_position();
      DenotedValue * arg_val = argument->get_denoted_value();

      SEXP raw_obj = arg_val->get_raw_object();

      // TODO should arg_val or raw_obj here be swapped?
      if (arg_val->is_promise()) {
        SEXP val = dyntrace_get_promise_value(raw_obj);
        // all will get forced, so yeah.
        while (type_of_sexp(val) == PROMSXP) {
          val = dyntrace_get_promise_value(val);
        }
        if (val != R_UnboundValue) {
          // its forced
          state.get_dependencies().add_argument(val, fn_id, param_pos);
        } else {
          // don't have to do anything, nothing to do
        }
      } else {
        state.get_dependencies().add_argument(raw_obj, fn_id, param_pos);
      }

    }

    state.deal_with_call_trace(deal_with_function_call(function_call, return_value));

    state.get_dependencies().add_return(return_value, function_call->get_function()->get_id());

    function_call->set_return_value_type(type_of_sexp(return_value));

    state.notify_caller(function_call);

    state.destroy_call(function_call);

    state.exit_probe(Event::ClosureExit);
}

// TODO deal with builtins for propagatr
void builtin_entry(dyntracer_t* dyntracer,
                  const SEXP call,
                  const SEXP op,
                  const SEXP args,
                  const SEXP rho,
                  const dyntrace_dispatch_t dispatch) {
   TracerState& state = tracer_state(dyntracer);
   state.enter_probe(Event::BuiltinEntry);
   Call* function_call = state.create_call(call, op, args, rho);
   set_dispatch(function_call, dispatch);
   state.push_stack(function_call);
   state.exit_probe(Event::BuiltinEntry);
}

void builtin_exit(dyntracer_t* dyntracer,
                 const SEXP call,
                 const SEXP op,
                 const SEXP args,
                 const SEXP rho,
                 const dyntrace_dispatch_t dispatch,
                 const SEXP return_value) {
   TracerState& state = tracer_state(dyntracer);
   state.enter_probe(Event::BuiltinExit);
   ExecutionContext exec_ctxt = state.pop_stack();
   if (!exec_ctxt.is_builtin()) {
       dyntrace_log_error("Not found matching builtin on stack");
   }
   Call* function_call = exec_ctxt.get_builtin();
   state.deal_with_call_trace(deal_with_builtin_and_special(function_call, args, return_value, &state));

   // TODO: dependencies

   function_call->set_return_value_type(type_of_sexp(return_value));
   state.notify_caller(function_call);
   state.destroy_call(function_call);
   state.exit_probe(Event::BuiltinExit);
}

// TODO propagatr do this
void special_entry(dyntracer_t* dyntracer,
                  const SEXP call,
                  const SEXP op,
                  const SEXP args,
                  const SEXP rho,
                  const dyntrace_dispatch_t dispatch) {
   TracerState& state = tracer_state(dyntracer);
   state.enter_probe(Event::SpecialEntry);
   Call* function_call = state.create_call(call, op, args, rho);
   set_dispatch(function_call, dispatch);
   state.push_stack(function_call);
   state.exit_probe(Event::SpecialEntry);
}

void special_exit(dyntracer_t* dyntracer,
                 const SEXP call,
                 const SEXP op,
                 const SEXP args,
                 const SEXP rho,
                 const dyntrace_dispatch_t dispatch,
                 const SEXP return_value) {
   TracerState& state = tracer_state(dyntracer);
   state.enter_probe(Event::SpecialExit);
   ExecutionContext exec_ctxt = state.pop_stack();
   if (!exec_ctxt.is_special()) {
       dyntrace_log_error("Not found matching special object on stack");
   }
   Call* function_call = exec_ctxt.get_special();

   state.deal_with_call_trace(deal_with_builtin_and_special(function_call, args, return_value, &state));

   function_call->set_return_value_type(type_of_sexp(return_value));
   state.notify_caller(function_call);
   state.destroy_call(function_call);
   state.exit_probe(Event::SpecialExit);
}


void promise_force_entry(dyntracer_t* dyntracer, const SEXP promise) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::PromiseForceEntry);

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    /* Force promise in the end. This is important to get correct force order
       from the call object. */
    promise_state->force();

    state.push_stack(promise_state);

    state.exit_probe(Event::PromiseForceEntry);
}

void promise_force_exit(dyntracer_t* dyntracer, const SEXP promise) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::PromiseForceExit);

    ExecutionContext exec_ctxt = state.pop_stack();

    if (!exec_ctxt.is_promise()) {
        dyntrace_log_error("unable to find matching promise on stack");
    }

    DenotedValue* promise_state = exec_ctxt.get_promise();

    const SEXP value = dyntrace_get_promise_value(promise);

    promise_state->set_value_type(type_of_sexp(value));

    /* if promise is not an argument, then don't process it. */
    if (promise_state->is_argument()) {
        // we know that the call is valid because this is an argument promise

        for (Argument * arg : promise_state->get_arguments()) {
          // get info
          function_id_t fn_id = arg->get_call()->get_function()->get_id();
          int param_pos = arg->get_formal_parameter_position();

          // put it
          state.get_dependencies().add_argument(value, fn_id, param_pos);
        }
    }

    state.exit_probe(Event::PromiseForceExit);
}

static void gc_promise_unmark(TracerState& state, const SEXP promise) {
    DenotedValue* promise_state = state.lookup_promise(promise, true);

    state.remove_promise(promise, promise_state);
}

static void gc_closure_unmark(TracerState& state, const SEXP function) {
    state.remove_function(function);
}

void gc_unmark(dyntracer_t* dyntracer, const SEXP object) {
   TracerState& state = tracer_state(dyntracer);
   state.enter_probe(Event::GcUnmark);

   // try to remove anytime the gc unmarks
   state.get_dependencies().remove_value(object);

   switch (TYPEOF(object)) {
   case PROMSXP:
       gc_promise_unmark(state, object);
       break;
   case CLOSXP:
       gc_closure_unmark(state, object);
       break;
   default:
       break;
   }
   state.exit_probe(Event::GcUnmark);
}

void context_entry(dyntracer_t* dyntracer, const RCNTXT* cptr) {
   TracerState& state = tracer_state(dyntracer);
   state.enter_probe(Event::ContextEntry);
   state.push_stack(cptr);
   state.exit_probe(Event::ContextEntry);
}

void jump_single_context(TracerState& state,
                        ExecutionContext& exec_ctxt,
                        bool returned,
                        const sexptype_t return_value_type,
                        const SEXP rho) {
   if (exec_ctxt.is_call()) {
       Call* call = exec_ctxt.get_call();
       call->set_jumped();
       call->set_return_value_type(return_value_type);
       state.notify_caller(call);
       state.destroy_call(call);
   }
   else if (exec_ctxt.is_promise()) {
       DenotedValue* promise = exec_ctxt.get_promise();
       promise->set_value_type(JUMPSXP);
       if (returned && promise->is_argument() &&
           (promise->get_environment() == rho)) {
           promise->set_non_local_return();
       }
   }
}

void context_jump(dyntracer_t* dyntracer,
                 const RCNTXT* context,
                 const SEXP return_value,
                 int restart) {
   TracerState& state = tracer_state(dyntracer);
   state.enter_probe(Event::ContextJump);
   /* Identify promises that do non local return. First, check if
  this special is a 'return', then check if the return happens
  right after a promise is forced, then walk back in the stack
  to the promise with the same environment as the return. This
  promise is the one that does non local return. Note that the
  loop breaks after the first such promise is found. This is
  because only one promise can be held responsible for non local
  return, the one that invokes the return function. */
   execution_contexts_t exec_ctxts(state.unwind_stack(context));
   const SEXP rho = context->cloenv;
   std::size_t context_count = exec_ctxts.size();
   if (context_count == 0) {
   } else if (context_count == 1) {
       jump_single_context(state, exec_ctxts.front(), false, JUMPSXP, rho);
   } else {
       auto begin_iter = exec_ctxts.begin();
       auto end_iter = --exec_ctxts.end();
       bool returned =
           (begin_iter->is_special() &&
            begin_iter->get_special()->get_function()->is_return());
       for (auto iter = begin_iter; iter != end_iter; ++iter) {
           jump_single_context(state, *iter, returned, JUMPSXP, rho);
       }
       jump_single_context(
           state, *end_iter, returned, type_of_sexp(return_value), rho);
   }
   state.exit_probe(Event::ContextJump);
}

void context_exit(dyntracer_t* dyntracer, const RCNTXT* cptr) {
   TracerState& state = tracer_state(dyntracer);
   state.enter_probe(Event::ContextExit);
   ExecutionContext exec_ctxt = state.pop_stack();
   if (!exec_ctxt.is_r_context()) {
       dyntrace_log_error("Nonmatching r context on stack");
   }
   state.exit_probe(Event::ContextExit);
}
