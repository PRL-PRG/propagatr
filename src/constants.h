#ifndef PROMISEDYNTRACER_CONSTANTS_H
#define PROMISEDYNTRACER_CONSTANTS_H

#include "definitions.h"

#include <string>
#include <vector>

extern const char UNIT_SEPARATOR;
extern const char RECORD_SEPARATOR;

extern const eval_depth_t ESCAPED_PROMISE_EVAL_DEPTH;
extern const eval_depth_t UNASSIGNED_PROMISE_EVAL_DEPTH;

extern const std::size_t PROMISE_MAPPING_BUCKET_COUNT;
extern const std::size_t FUNCTION_MAPPING_BUCKET_SIZE;

extern const std::vector<std::string> ENVIRONMENT_VARIABLES;

extern const timestamp_t UNDEFINED_TIMESTAMP;

extern const denoted_value_id_t UNASSIGNED_DENOTED_VALUE_ID;

extern const function_id_t UNASSIGNED_FUNCTION_ID;

extern const std::string UNASSIGNED_CLASS_NAME;

extern const call_id_t UNASSIGNED_CALL_ID;

extern const int UNASSIGNED_FORMAL_PARAMETER_POSITION;

extern const int UNASSIGNED_ACTUAL_ARGUMENT_POSITION;

extern const int UNASSIGNED_FORMAL_PARAMETER_COUNT;

extern const unsigned int OBJECT_TYPE_TABLE_COUNT;

extern const scope_t UNASSIGNED_SCOPE;
extern const scope_t TOP_LEVEL_SCOPE;

extern const gc_cycle_t UNDEFINED_GC_CYCLE;
#endif /* PROMISEDYNTRACER_CONSTANTS_H */
