#include "ExecutionContext.h"

#include "Function.h"

ExecutionContext::ExecutionContext(Call* call)
    : type_(call->get_function()->get_type()), call_(call), execution_time_(0) {
}
