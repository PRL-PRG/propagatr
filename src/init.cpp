// #include "table.h"
#include "tracer.h"

#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

#ifdef __cplusplus
extern "C" {
#endif

static const R_CallMethodDef CallEntries[] = {
    {"create_dyntracer", (DL_FUNC) &create_dyntracer, 6},
    {"destroy_dyntracer", (DL_FUNC) &destroy_dyntracer, 1},
    // {"write_data_table", (DL_FUNC) &write_data_table, 5},
    // {"read_data_table", (DL_FUNC) &read_data_table, 3},
    {NULL, NULL, 0}};

void attribute_visible R_init_propagatr(DllInfo* dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE);
}

#ifdef __cplusplus
}
#endif
