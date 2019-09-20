#ifndef PROMISEDYNTRACER_EXECUTION_CONTEXT_H
#define PROMISEDYNTRACER_EXECUTION_CONTEXT_H

#include "sexptypes.h"

/* forward declarations to prevent cyclic dependencies */
class DenotedValue;
class Call;

class ExecutionContext {
  public:
    explicit ExecutionContext(DenotedValue* promise_state)
        : type_(PROMSXP), promise_state_(promise_state), execution_time_(0) {
    }

    explicit ExecutionContext(const RCNTXT* r_context)
        : type_(CONTEXTSXP), r_context_(r_context), execution_time_(0) {
    }

    /* defined in cpp file to get around cyclic dependency issues. */
    explicit ExecutionContext(Call* call);

    bool is_promise() const {
        return (type_ == PROMSXP);
    }

    bool is_builtin() const {
        return (type_ == BUILTINSXP);
    }

    bool is_special() const {
        return (type_ == SPECIALSXP);
    }

    bool is_closure() const {
        return (type_ == CLOSXP);
    }

    bool is_call() const {
        return is_builtin() || is_special() || is_closure();
    }

    bool is_r_context() const {
        return (type_ == CONTEXTSXP);
    }

    DenotedValue* get_promise() const {
        return promise_state_;
    }

    Call* get_builtin() const {
        return call_;
    }

    Call* get_special() const {
        return call_;
    }

    Call* get_closure() const {
        return call_;
    }

    Call* get_call() const {
        return call_;
    }

    const RCNTXT* get_r_context() const {
        return r_context_;
    }

    void increment_execution_time(const std::uint64_t increment) {
        execution_time_ += increment;
    }

    std::uint64_t get_execution_time() const {
        return execution_time_;
    }

  private:
    sexptype_t type_;
    union {
        DenotedValue* promise_state_;
        Call* call_;
        const RCNTXT* r_context_;
    };
    std::uint64_t execution_time_;
};

using execution_contexts_t = std::vector<ExecutionContext>;
using execution_context_iterator = execution_contexts_t::iterator;
using reverse_execution_context_iterator =
    execution_contexts_t::reverse_iterator;
using const_execution_context_iterator = execution_contexts_t::const_iterator;
using const_reverse_execution_context_iterator =
    execution_contexts_t::const_reverse_iterator;

#endif /* PROMISEDYNTRACER_EXECUTION_CONTEXT_H */
