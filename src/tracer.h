#ifndef PROMISEDYNTRACER_TRACER_H
#define PROMISEDYNTRACER_TRACER_H

#include <Rinternals.h>
#undef TRUE
#undef FALSE
#undef length
#undef eval
#undef error

#ifdef __cplusplus
extern "C" {
#endif

SEXP create_dyntracer(SEXP output_dirpath,
                      SEXP analyzed_file_name,
                      SEXP verbose,
                      SEXP truncate,
                      SEXP binary,
                      SEXP compression_level);

SEXP destroy_dyntracer(SEXP dyntracer_sexp);

#ifdef __cplusplus
}
#endif

#endif /* PROMISEDYNTRACER_TRACER_H */
