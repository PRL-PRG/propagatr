
#include "probes.h"

inline TracerState& tracer_state(dyntracer_t* dyntracer) {
    return *(static_cast<TracerState*>(dyntracer->state));
}

// Set the dispatch type.
static inline void set_dispatch(Call* call,
                               const dyntrace_dispatch_t dispatch) {
   if (dispatch == DYNTRACE_DISPATCH_S3) {
       call->set_S3_method();
   } else if (dispatch == DYNTRACE_DISPATCH_S4) {
       call->set_S4_method();
   }
}

// On entry for the tracer.
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

// On exit of the tracer.
void dyntrace_exit(dyntracer_t* dyntracer,
                   SEXP expression,
                   SEXP environment,
                   SEXP result,
                   int error) {
    std::cout << "dyntrace exiting...\n\n";

    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::DyntraceExit);

    /* Force an imaginary GC cycle at program end */
    state.enter_gc();

    state.cleanup(error);

    // Serialize the traces and write them out.
    state.serialize_and_output();

    /* we do not do start.exit_probe() because the tracer has finished
       executing and we don't need to resume the timer. */
}

// Old functionality for dealing with closures.
// Kept for posterity, though ideally TODO: modify to match current closure_, builtin_, and special_ entry logic
// and use instead for less code redundancy.
CallTrace deal_with_function_call(Call* function_call, SEXP return_value, dyntrace_dispatch_t dispatch, TracerState* state) {
    CallTrace trace_for_this_call = CallTrace(  function_call->get_function()->get_namespace(), 
                                                function_call->get_function_name(),
                                                function_call->get_function()->get_id(),
                                                dispatch, 42);

    for (Argument* argument: function_call->get_arguments()) {
        int param_pos = argument->get_formal_parameter_position();
        DenotedValue * arg_val = argument->get_denoted_value();

        SEXP raw_obj = arg_val->get_raw_object();
        std::vector<std::string> tags;

        // to get types, if the top level is a promise... its a promise!
        // TODO: don't go all the way in, look at the expression slot of needed
        if (arg_val->is_promise()) {
            
            SEXP val = dyntrace_get_promise_value(raw_obj);
            SEXP old_expr = dyntrace_get_promise_expression(raw_obj);
            // all will get forced, so yeah.
            while (type_of_sexp(val) == PROMSXP) {
            // while (TYPEOF(val) == PROMSXP) {
                old_expr = dyntrace_get_promise_expression(val);
                val = dyntrace_get_promise_value(val);
            }

            auto the_type = type_of_sexp(val);

            // TODO test this with real promises
            if (val != R_UnboundValue) {
                // std::cout << param_pos << ": something was passed and it was used.\n";

                // if (the_type == CLOSXP || the_type == SPECIALSXP || the_type == BUILTINSXP) {
                //     // put tag with the function id
                //     // tags.push_back("fn-id;" + state->lookup_function(val)->get_id());
                //     tags.push_back(state->lookup_function(val)->get_id());
                // }

                trace_for_this_call.add_to_call_trace(param_pos, Type(val, tags));
            } else {
                // std::cout << param_pos << ": passed and not used.\n";

                // if (the_type == CLOSXP || the_type == SPECIALSXP || the_type == BUILTINSXP) {
                //     // put tag with the function id
                //     // tags.push_back("fn-id;" + state->lookup_function(val)->get_id());
                //     tags.push_back(state->lookup_function(val)->get_id());
                // }

                tags.push_back("missing");

                trace_for_this_call.add_to_call_trace(param_pos, Type(old_expr, tags));
            }
        } else {
            // std::cout << param_pos << ": nothing was passed.\n";
            trace_for_this_call.add_to_call_trace(param_pos, Type(R_MissingArg));
        }

        // if raw_obj is a promise, this will be recorded as 'unused'
        // trace_for_this_call.add_to_call_trace(param_pos, Type(raw_obj));
    }

    auto the_type = type_of_sexp(return_value);
    std::vector<std::string> tags;
    // if (the_type == CLOSXP || the_type == SPECIALSXP || the_type == BUILTINSXP) {
    //     // put tag with the function id
    //     // tags.push_back("fn-id;" + state->lookup_function(return_value)->get_id());
    //     tags.push_back(state->lookup_function(return_value)->get_id());
    // }

    trace_for_this_call.add_to_call_trace(-1, Type(return_value, tags));

    return trace_for_this_call;
}

// Old functionality for dealing with builtins and specials.
// See TODO above.
CallTrace deal_with_builtin_and_special(Call* function_call, SEXP args, SEXP return_value, TracerState* state, dyntrace_dispatch_t dispatch) {
    CallTrace trace_for_this_call = state->create_call_trace(  function_call->get_function()->get_namespace(), 
                                                function_call->get_function_name(),
                                                function_call->get_function()->get_id(),
                                                dispatch);

    int i = 0;
    
    for(SEXP cons = args; cons != R_NilValue; cons = CDR(cons)) {
        SEXP el = CAR(cons);

        // build up call trace
        trace_for_this_call.add_to_call_trace(i, Type(el));

        // dependencies
        // state->get_dependencies().add_argument(el, function_call->get_function()->get_id(), i);

        i++;
    }

    // return value
    trace_for_this_call.add_to_call_trace(-1, Type(return_value));
    // state->get_dependencies().add_argument(return_value, function_call->get_function()->get_id(), -1, trace_for_this_call.compute_hash());

    return trace_for_this_call;

}

// For GDB breakpoint debugging.
// There's an issue with how GDB interacts with R-dyntrace and using these is the best way
// to establish reliable breakpoints.
void exampleFun() {
    
}

void exampleFun2() {
    
}

// Called when closures are entered.
// Here, we establish an initial guess at the types, and the types themselves are filled in
// once the promises are forced in the function context. We collect information on the values
// at that time.
void closure_entry(dyntracer_t* dyntracer,
                   const SEXP call,
                   const SEXP op,
                   const SEXP args,
                   const SEXP rho,
                   const dyntrace_dispatch_t dispatch) {

    // General R-dyntrace preamble.
    TracerState& state = tracer_state(dyntracer);
    state.enter_probe(Event::ClosureEntry);
    Call* function_call = state.create_call(call, op, args, rho);
    set_dispatch(function_call, dispatch);

    // Set up the call trace of the function call.
    function_call->set_call_trace(state.create_call_trace(function_call->get_function()->get_namespace(), 
                                  function_call->get_function_name(), function_call->get_function()->get_id(),
                                  dispatch));

    // Find ... (vararg) arguments, and deal with dispatch cases.
    for (Argument * arg : function_call->get_arguments()) {

        // If doing S3 dispatch, some of the arguments may be preevaluated, and we won't be able to 
        // rely on promise_force_* to tie them to this function, so we do that here.
        if (dispatch == DYNTRACE_DISPATCH_S3) {
            // We care about the argument. Look for pre-evaluated arguments, and add them to ct.
            if (arg->get_denoted_value()->is_forced() || arg->get_denoted_value()->is_preforced()) {
                // Add it to the CT.
                std::vector<std::string> tags;
                SEXP value = arg->get_denoted_value()->get_raw_object();
                auto the_type = type_of_sexp(value);

                while (the_type == PROMSXP) {
                    value = dyntrace_get_promise_value(value);
                    the_type = type_of_sexp(value);
                }

                // We used to do this to track which functions were being passed around, but that
                // turns out to have been ill-advised due to some issues with get_id being used
                // in this context. TODO: do this, and fix get_id.
                // if (the_type == CLOSXP || the_type == SPECIALSXP || the_type == BUILTINSXP) {
                //     // put tag with the function id
                //     // tags.push_back("fn-id;" + state->lookup_function(val)->get_id());
                //     tags.push_back(state.lookup_function(value)->get_id());
                //

                // add the type to the call trace.
                function_call->get_call_trace()->add_to_call_trace(arg->get_formal_parameter_position(), Type(value, tags));
            }
        }

        // Deal with ... args. We just record that the position is vararg, and promise_force_* will
        // not generate types for them. 
        if (arg->is_dot_dot_dot()) {
            function_call->get_call_trace()->set_has_dots(true);
            function_call->get_call_trace()->add_to_call_trace(arg->get_formal_parameter_position(), Type(DOTSXP));
            // break; // We only need one. But in case things are out of order...
        } else {
            // In this case, we establish the initial guess of the type.
            int param_pos = arg->get_formal_parameter_position();
            DenotedValue * arg_val = arg->get_denoted_value();

            SEXP raw_obj = arg_val->get_raw_object();
            std::vector<std::string> tags;

            // We are not forcing promises. This is good.
            if (arg_val->is_promise()) {
                
                SEXP val = dyntrace_get_promise_value(raw_obj);
                SEXP old_expr = dyntrace_get_promise_expression(raw_obj);
                
                while (type_of_sexp(val) == PROMSXP) {
                    old_expr = dyntrace_get_promise_expression(val);
                    val = dyntrace_get_promise_value(val);
                }

                auto the_type = type_of_sexp(val);

                // TODO test this with real promises
                if (val != R_UnboundValue) {
                    // std::cout << param_pos << ": something was passed and it was used.\n";

                    // if (the_type == CLOSXP || the_type == SPECIALSXP || the_type == BUILTINSXP) {
                    //     // put tag with the function id
                    //     // tags.push_back("fn-id;" + state->lookup_function(val)->get_id());
                    //     tags.push_back(state.lookup_function(val)->get_id());
                    // }

                    function_call->get_call_trace()->add_to_call_trace(param_pos, Type(val, tags));
                } else {
                    // If the type of old_expr is symbol or language, then it's missing.
                    the_type = type_of_sexp(old_expr);

                    // if (the_type == CLOSXP || the_type == SPECIALSXP || the_type == BUILTINSXP) {
                    //     // put tag with the function id
                    //     // tags.push_back("fn-id;" + state->lookup_function(val)->get_id());
                    //     tags.push_back(state.lookup_function(val)->get_id());
                    if (the_type == SYMSXP || the_type == LANGSXP) {
                        function_call->get_call_trace()->add_to_call_trace(param_pos, Type(MISSINGSXP));
                    } else {
                        function_call->get_call_trace()->add_to_call_trace(param_pos, Type(old_expr, {}));
                    }
                }
            } else {
                // std::cout << param_pos << ": nothing was passed.\n";
                function_call->get_call_trace()->add_to_call_trace(param_pos, Type(R_MissingArg));
            }
        }
    }

    state.push_stack(function_call);

    // Create an initial call trace for this function, and push it onto the CallTract stack.
    // CallTrace trace = CallTrace(function_call->get_function()->get_namespace(), 
    //                             function_call->get_function_name(),
    //                             function_call->get_function()->get_id(),
    //                             dispatch); 

    // state.push_trace(trace);

    state.exit_probe(Event::ClosureEntry);
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

    function_id_t fn_id = function_call->get_function()->get_id();

    // Dependencies?
    // for (Argument* argument: function_call->get_arguments()) {
    //   int param_pos = argument->get_formal_parameter_position();
    //   DenotedValue * arg_val = argument->get_denoted_value();

    //   SEXP raw_obj = arg_val->get_raw_object();

    //   if (arg_val->is_promise()) {
    //     SEXP val = dyntrace_get_promise_value(raw_obj);
    //     // all will get forced, so yeah.
    //     while (type_of_sexp(val) == PROMSXP) {
    //     // while (TYPEOF(val) == PROMSXP) {
    //       val = dyntrace_get_promise_value(val);
    //     }
    //     if (val != R_UnboundValue) {
    //       // its forced
    //       state.get_dependencies().add_argument(val, fn_id, param_pos);
    //     } else {
    //       // don't have to do anything, nothing to do
    //     }
    //   } else {
    //     state.get_dependencies().add_argument(raw_obj, fn_id, param_pos);
    //   }

    // }

    // Old way of dealing with function calls.
    // CallTrace ct = deal_with_function_call(function_call, return_value, dispatch, &state);

    // state.deal_with_call_trace(ct); // COMMENT THIS WHEN CHANGING BACK

    // state.get_dependencies().add_return(return_value, function_call->get_function()->get_id(), ct.compute_hash());

    SEXP val = return_value;
    // all will get forced, so yeah.
    while (type_of_sexp(val) == PROMSXP) {
        val = dyntrace_get_promise_value(val);
    }

    auto the_type = type_of_sexp(val);

    function_call->set_return_value_type(type_of_sexp(val));

    // NOTE: This code is duplicated in jump_single_context.
    // If you change one, change both.

    CallTrace ct = function_call->get_call_trace();

    // auto the_type = type_of_sexp(val);
    std::vector<std::string> tags;
    // if (the_type == CLOSXP || the_type == SPECIALSXP || the_type == BUILTINSXP) {
    //     // put tag with the function id
    //     // tags.push_back("fn-id;" + state->lookup_function(return_value)->get_id());
    //     tags.push_back(state.lookup_function(val)->get_id());
    // }

    ct.add_to_call_trace(-1, Type(val, tags));

    // state.get_dependencies().add_return(val, function_call->get_function()->get_id(), ct.compute_hash());

    state.deal_with_call_trace(ct); 

    // Done dealing with return.
    state.notify_caller(function_call);

    state.destroy_call(function_call);

    state.exit_probe(Event::ClosureExit);
}

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

    function_call->set_call_trace(state.create_call_trace(function_call->get_function()->get_namespace(), 
                                  function_call->get_function_name(), function_call->get_function()->get_id(),
                                  dispatch));

    state.push_stack(function_call);

    // Create an initial call trace for this function, and push it onto the CallTract stack.
    // CallTrace trace = CallTrace(function_call->get_function()->get_namespace(), 
    //                             function_call->get_function_name(),
    //                             function_call->get_function()->get_id(),
    //                             dispatch); 

    // state.push_trace(trace);

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
    state.deal_with_call_trace(deal_with_builtin_and_special(function_call, args, return_value, &state, dispatch));

    function_call->set_return_value_type(type_of_sexp(return_value));
    state.notify_caller(function_call);

    // CallTrace ct = state.pop_trace_stack();
    // Deal with return.
    // ct.add_to_call_trace(-1, Type(return_value));
    // state.get_dependencies().add_argument(return_value, function_call->get_function()->get_id(), -1, ct.compute_hash());

    state.destroy_call(function_call);
    // state.deal_with_call_trace(ct);

    state.exit_probe(Event::BuiltinExit);
}

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

    function_call->set_call_trace(state.create_call_trace(function_call->get_function()->get_namespace(), 
                                  function_call->get_function_name(), function_call->get_function()->get_id(),
                                  dispatch));
                                  
    state.push_stack(function_call);

    // Create an initial call trace for this function, and push it onto the CallTract stack.
    // CallTrace trace = CallTrace(function_call->get_function()->get_namespace(), 
    //                             function_call->get_function_name(),
    //                             function_call->get_function()->get_id(),
    //                             dispatch); 

    // state.push_trace(trace);

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
    state.deal_with_call_trace(deal_with_builtin_and_special(function_call, args, return_value, &state, dispatch));

    function_call->set_return_value_type(type_of_sexp(return_value));
    state.notify_caller(function_call);

    // CallTrace ct = state.pop_trace_stack();

    // Add the return?
    // ct.add_to_call_trace(-1, Type(return_value));
    // state.get_dependencies().add_argument(return_value, function_call->get_function()->get_id(), -1, ct.compute_hash());

    state.destroy_call(function_call);
    // state.deal_with_call_trace(ct);

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

    auto the_type = type_of_sexp(value);
    promise_state->set_value_type(the_type);

    /* if promise is not an argument, then don't process it. */
    if (promise_state->is_argument()) {
        // we know that the call is valid because this is an argument promise

        // Look at the argument(s, if it's ...)
        for (Argument * arg : promise_state->get_arguments()) {

            // We don't want ... arguments, but let's warn the function
            // that it has ... arguments.
            if (arg->is_dot_dot_dot()) {
                // This is dealt with in a prepass phase in closure_entry.
                // CallTrace * ct = arg->get_call()->get_call_trace();
                // ct->set_has_dots(true);
                // Doesn't work here:
                // arg->get_call()->get_call_trace()->add_to_call_trace(arg->get_formal_parameter_position(), Type(DOTSXP));
                continue;
            }

            // get info
            function_id_t fn_id = arg->get_call()->get_function()->get_id();
            int param_pos = arg->get_formal_parameter_position();
            
            CallTrace * ct = arg->get_call()->get_call_trace();

            // if (arg->get_call()->get_function_name() == "h")
            //     std::cout << "uid: " << ct->get_uid() << "\n";

            std::vector<std::string> tags;

            // if (the_type == CLOSXP || the_type == SPECIALSXP || the_type == BUILTINSXP) {
            //     // put tag with the function id
            //     // tags.push_back("fn-id;" + state->lookup_function(val)->get_id());
            //     tags.push_back(state.lookup_function(value)->get_id());
            // }

            // add the type to the call trace.
            ct->add_to_call_trace(param_pos, Type(value, tags));

            // DEBUG:
            // std::cout << ct->get_function_name() << " " << ct->get_call_trace().at(param_pos).get_top_level_type() << "\n";

            // put it
            // state.get_dependencies().add_argument(value, fn_id, param_pos);
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
   // state.get_dependencies().remove_value(object);

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
                        const SEXP return_value,
                        const SEXP rho) {
   if (exec_ctxt.is_call()) {
       Call* call = exec_ctxt.get_call();
       call->set_jumped();
       call->set_return_value_type(return_value_type);

       /* Duplicate closure_exit call trace processing, deals with S3 cases.
       */
       if(call->get_function()->is_closure()) {

           // NOTE: This is a duplicate of the code in closure_exit.
           // If you change one, change both.
            function_id_t fn_id = call->get_function()->get_id();

            // NOTE: Dependency code used to be here.

            // Deal with return.
            // CallTrace ct = state.pop_trace_stack();

            CallTrace ct = call->get_call_trace();

            if (return_value_type == JUMPSXP || return_value == NULL) {
                ct.add_to_call_trace(-1, Type(return_value_type));

                // We have no dependencies to add in this case.
                // state.get_dependencies().add_return(return_value, call->get_function()->get_id(), ct.compute_hash());
            } else {
                auto the_type = type_of_sexp(return_value);
                std::vector<std::string> tags;
                // if (the_type == CLOSXP || the_type == SPECIALSXP || the_type == BUILTINSXP) {
                //     // put tag with the function id
                //     // tags.push_back("fn-id;" + state->lookup_function(return_value)->get_id());
                //     tags.push_back(state.lookup_function(return_value)->get_id());
                // }

                ct.add_to_call_trace(-1, Type(return_value, tags));

                // state.get_dependencies().add_return(return_value, call->get_function()->get_id(), ct.compute_hash());
            }

            state.deal_with_call_trace(ct); 

       }


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
       jump_single_context(state, exec_ctxts.front(), false, JUMPSXP, return_value, rho); // 
   } else {
       auto begin_iter = exec_ctxts.begin();
       auto end_iter = --exec_ctxts.end();
       bool returned =
           (begin_iter->is_special() &&
            begin_iter->get_special()->get_function()->is_return());
       for (auto iter = begin_iter; iter != end_iter; ++iter) {
           jump_single_context(state, *iter, returned, JUMPSXP, return_value, rho);
       }
       jump_single_context(
           state, *end_iter, returned, type_of_sexp(return_value), return_value, rho);
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
