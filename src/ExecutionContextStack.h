#ifndef PROMISEDYNTRACER_EXECUTION_CONTEXT_STACK_H
#define PROMISEDYNTRACER_EXECUTION_CONTEXT_STACK_H

#include "ExecutionContext.h"

#include <vector>

using execution_contexts_t = std::vector<ExecutionContext>;

class ExecutionContextStack {
  public:
    using iterator = execution_contexts_t::iterator;
    using reverse_iterator = execution_contexts_t::reverse_iterator;
    using const_iterator = execution_contexts_t::const_iterator;
    using const_reverse_iterator = execution_contexts_t::const_reverse_iterator;

    explicit ExecutionContextStack(): stack_() {
    }

    size_t size() const {
        return stack_.size();
    }

    bool is_empty() const {
        return stack_.empty();
    }

    iterator begin() {
        return stack_.begin();
    }

    iterator end() {
        return stack_.end();
    }

    reverse_iterator rbegin() {
        return stack_.rbegin();
    }

    reverse_iterator rend() {
        return stack_.rend();
    }

    const_iterator cbegin() const {
        return stack_.cbegin();
    }

    const_iterator cend() const {
        return stack_.cend();
    }

    const_reverse_iterator crbegin() const {
        return stack_.crbegin();
    }

    const_reverse_iterator crend() const {
        return stack_.crend();
    }

    template <typename T>
    void push(T* context) {
        stack_.push_back(ExecutionContext(context));
    }

    ExecutionContext pop() {
        ExecutionContext context{peek(1)};
        stack_.pop_back();
        return context;
    }

    const ExecutionContext& peek(std::size_t n = 1) const {
        return stack_.at(stack_.size() - n);
    }

    ExecutionContext& peek(std::size_t n = 1) {
        return stack_.at(stack_.size() - n);
    }

    execution_contexts_t unwind(const ExecutionContext& context) {
        execution_contexts_t unwound_contexts;

        while (size() > 0) {
            ExecutionContext& temp_context{stack_.back()};

            if (temp_context.is_r_context() &&
                (temp_context.get_r_context() == context.get_r_context())) {
                return unwound_contexts;
            }
            stack_.pop_back();
            unwound_contexts.push_back(temp_context);
        }
        dyntrace_log_error("cannot find matching context while unwinding\n");
    }

  private:
    execution_contexts_t stack_;
};

#endif
