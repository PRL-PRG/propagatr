#ifndef PROMISEDYNTRACER_DEFINITIONS_H
#define PROMISEDYNTRACER_DEFINITIONS_H

#include <string>
#include <vector>

typedef int call_id_t;
typedef std::string function_id_t;

typedef int env_id_t;
typedef int var_id_t;

typedef long long int timestamp_t;
typedef int denoted_value_id_t;

struct eval_depth_t {
    int call_depth;
    int promise_depth;
    int nested_promise_depth;
    int forcing_actual_argument_position;
};

typedef std::vector<int> pos_seq_t;

struct lifecycle_t {
    std::string action;
    std::vector<int> count;
};

typedef std::string scope_t;

typedef int gc_cycle_t;

#endif /* PROMISEDYNTRACER_DEFINITIONS_H */
