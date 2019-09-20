#ifndef PROMISEDYNTRACER_PROBES_H
#define PROMISEDYNTRACER_PROBES_H

#include "TracerState.h"
#include "utilities.h"

#define R_USE_SIGNALS 1
#include "Defn.h"

extern "C" {

  void dyntrace_entry(dyntracer_t* dyntracer, SEXP expression, SEXP environment);

  void dyntrace_exit(dyntracer_t* dyntracer,
                     SEXP expression,
                     SEXP environment,
                     SEXP result,
                     int error);

  void promise_force_exit(dyntracer_t* dyntracer, 
                          const SEXP promise);

  void closure_entry(dyntracer_t* dyntracer,
                     const SEXP call,
                     const SEXP op,
                     const SEXP args,
                     const SEXP rho,
                     const dyntrace_dispatch_t dispatch);

  void closure_exit(dyntracer_t* dyntracer,
                    const SEXP call,
                    const SEXP op,
                    const SEXP args,
                    const SEXP rho,
                    const dyntrace_dispatch_t dispatch,
                    const SEXP return_value);

  void builtin_entry(dyntracer_t* dyntracer,
                     const SEXP call,
                     const SEXP op,
                     const SEXP args,
                     const SEXP rho,
                     const dyntrace_dispatch_t dispatch);

  void special_entry(dyntracer_t* dyntracer,
                     const SEXP call,
                     const SEXP op,
                     const SEXP args,
                     const SEXP rho,
                     const dyntrace_dispatch_t dispatch);

  void builtin_exit(dyntracer_t* dyntracer,
                    const SEXP call,
                    const SEXP op,
                    const SEXP args,
                    const SEXP rho,
                    const dyntrace_dispatch_t dispatch,
                    const SEXP return_value);

  void special_exit(dyntracer_t* dyntracer,
                    const SEXP call,
                    const SEXP op,
                    const SEXP args,
                    const SEXP rho,
                    const dyntrace_dispatch_t dispatch,
                    const SEXP return_value);

  void gc_unmark(dyntracer_t* dyntracer, const SEXP object);

  void context_entry(dyntracer_t* dyntracer, const RCNTXT*);

  void context_jump(dyntracer_t* dyntracer,
                    const RCNTXT*,
                    SEXP return_value,
                    int restart);

  void context_exit(dyntracer_t* dyntracer, const RCNTXT*);
}
#endif /* __PROBES_H__ */
