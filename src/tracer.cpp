#include "tracer.h"

#include <Rdyntrace.h>

#include <csignal>

#include "probes.h"

extern "C" {

dyntracer_t* dyntracer_;

void handleSignal(int signum) {
    if (signum == SIGTERM) {
        (static_cast<TracerState*>(dyntracer_->state))->serialize_and_output();
    }
}

SEXP create_dyntracer(SEXP output_dirpath,
                      SEXP package_under_analysis,
                      SEXP analyzed_file_name,
                      SEXP verbose,
                      SEXP truncate,
                      SEXP binary,
                      SEXP compression_level) {
    void* state = new TracerState(sexp_to_string(output_dirpath),
                                  sexp_to_string(package_under_analysis),
                                  sexp_to_string(analyzed_file_name),
                                  sexp_to_bool(verbose),
                                  sexp_to_bool(truncate),
                                  sexp_to_bool(binary),
                                  sexp_to_int(compression_level));

    std::cout << "creating dyntracer, and tracing...\n\n";

    /* calloc initializes the memory to zero. This ensures that probes not
       attached will be NULL. Replacing calloc with malloc will cause
       segfaults. */
    dyntracer_t* dyntracer = (dyntracer_t*) calloc(1, sizeof(dyntracer_t));
    /* TODO Uncomment some of these for the hooks that you want. */
    dyntracer->probe_dyntrace_entry = dyntrace_entry;
    dyntracer->probe_dyntrace_exit = dyntrace_exit;
    dyntracer->probe_closure_entry = closure_entry;
    dyntracer->probe_builtin_entry = builtin_entry;
    dyntracer->probe_special_entry = special_entry;
    dyntracer->probe_gc_unmark = gc_unmark;
    dyntracer->probe_context_entry = context_entry;
    dyntracer->probe_context_jump = context_jump;
    dyntracer->probe_context_exit = context_exit;
    dyntracer->probe_closure_exit = closure_exit;
    dyntracer->probe_builtin_exit = builtin_exit;
    dyntracer->probe_special_exit = special_exit;
    dyntracer->probe_promise_force_entry = promise_force_entry;
    dyntracer->probe_promise_force_exit = promise_force_exit;
    // dyntracer->probe_S3_dispatch_entry = S3_dispatch_entry;
    // dyntracer->probe_S4_dispatch_argument = S4_dispatch_argument;
    dyntracer->state = state;

    /* Move these up as needed.
    dyntracer->probe_deserialize_object = deserialize_object;
    dyntracer->probe_eval_entry = eval_entry;
    dyntracer->probe_closure_argument_list_creation_entry =
        closure_argument_list_creation_entry;
    dyntracer->probe_closure_argument_list_creation_exit =
        closure_argument_list_creation_exit;
    dyntracer->probe_gc_entry = gc_entry;
    dyntracer->probe_promise_force_entry = promise_force_entry;
    dyntracer->probe_gc_allocate = gc_allocate;
    dyntracer->probe_promise_value_lookup = promise_value_lookup;
    dyntracer->probe_promise_expression_lookup = promise_expression_lookup;
    dyntracer->probe_promise_environment_lookup = promise_environment_lookup;
    dyntracer->probe_promise_value_assign = promise_value_assign;
    dyntracer->probe_promise_expression_assign = promise_expression_assign;
    dyntracer->probe_promise_environment_assign = promise_environment_assign;
    dyntracer->probe_promise_substitute = promise_substitute;
    dyntracer->probe_environment_variable_define = environment_variable_define;
    dyntracer->probe_environment_variable_assign = environment_variable_assign;
    dyntracer->probe_environment_variable_remove = environment_variable_remove;
    dyntracer->probe_environment_variable_lookup = environment_variable_lookup;
    dyntracer->probe_environment_context_sensitive_promise_eval_entry =
        environment_context_sensitive_promise_eval_entry;
    dyntracer->probe_environment_context_sensitive_promise_eval_exit =
        environment_context_sensitive_promise_eval_exit;
    dyntracer->probe_substitute_call = substitute_call;
    */
    dyntracer_ = dyntracer;

    signal(SIGTERM, handleSignal);

    return dyntracer_to_sexp(dyntracer, "dyntracer.promise");
}

static void destroy_promise_dyntracer(dyntracer_t* dyntracer) {
    /* free dyntracer iff it has not already been freed.
       this check ensures that multiple calls to destroy_dyntracer on the same
       object do not crash the process. */
    if (dyntracer) {
        delete (static_cast<TracerState*>(dyntracer->state));
        free(dyntracer);
    }
}

SEXP destroy_dyntracer(SEXP dyntracer_sexp) {
    return dyntracer_destroy_sexp(dyntracer_sexp, destroy_promise_dyntracer);
}

} // extern "C"
