#ifndef PROMISEDYNTRACER_SEXPTYPES_H
#define PROMISEDYNTRACER_SEXPTYPES_H

#include "stdlibs.h"

typedef unsigned int sexptype_t;

extern const sexptype_t UNBOUNDSXP;
extern const sexptype_t UNASSIGNEDSXP;
extern const sexptype_t MISSINGSXP;
extern const sexptype_t JUMPSXP;
extern const sexptype_t CONTEXTSXP;

sexptype_t type_of_sexp(SEXP value);
std::string sexptype_to_string(sexptype_t);
std::string value_type_to_string(SEXP value);

#endif /* PROMISEDYNTRACER_SEXPTYPES_H */
