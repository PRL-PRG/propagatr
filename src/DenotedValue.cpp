#include "DenotedValue.h"

#include "Argument.h"

/* this function is defined here instead of .h file to break circular dependency
   issues that arise due to accessing get_environment method of call */
void DenotedValue::remove_argument(const call_id_t call_id,
                                   const function_id_t& function_id,
                                   const sexptype_t return_value_type,
                                   const int formal_parameter_count,
                                   const Argument* argument) {
    if (argument != argument_stack_.back()) {
        dyntrace_log_error("removed argument does not match the argument in "
                           "promise argument stack.");
    }

    previous_call_id_ = call_id;
    previous_function_id_ = function_id;
    previous_call_return_value_type_ = return_value_type;
    previous_formal_parameter_count_ = formal_parameter_count;
    previous_default_argument_ = argument->is_default_argument();
    previous_formal_parameter_position_ =
        argument->get_formal_parameter_position();
    previous_actual_argument_position_ =
        argument->get_actual_argument_position();

    argument_stack_.pop_back();
    was_argument_ = true;
    // get_lifecycle().add_event(PromiseEvent::Type::Unbound);
}

void DenotedValue::metaprogram() {
    check_and_set_escape_();
    ++metaprogram_count_;
    if (is_argument()) {
        argument_stack_.back()->direct_metaprogram();
        for (int i = argument_stack_.size() - 2; i >= 0; --i) {
            argument_stack_[i]->indirect_metaprogram();
        }
    }
}

void DenotedValue::force() {
    check_and_set_escape_();
    ++force_count_;

    if (context_sensitive_lookup_) {
        context_sensitive_force_ = true;
    }

    if (is_argument()) {
        argument_stack_.back()->direct_force();
        for (int i = argument_stack_.size() - 2; i >= 0; --i) {
            argument_stack_[i]->indirect_force();
        }
    }
}

void DenotedValue::lookup_value() {
    check_and_set_escape_();
    ++value_lookup_count_;
    if (is_argument()) {
        argument_stack_.back()->direct_lookup();
        for (int i = argument_stack_.size() - 2; i >= 0; --i) {
            argument_stack_[i]->indirect_lookup();
        }
    }
}

void DenotedValue::set_evaluation_depth(const eval_depth_t& eval_depth) {
    eval_depth_ = eval_depth;
    int position = eval_depth.forcing_actual_argument_position;
    if (is_argument() && position != UNASSIGNED_ACTUAL_ARGUMENT_POSITION) {
        argument_stack_.back()->set_forcing_actual_argument_position(position);
    }
}

void DenotedValue::used_for_S3_dispatch() {
    ++S3_dispatch_count_;

    // get_lifecycle().add_event(PromiseEvent::Type::S3Dispatch);

    if (is_argument()) {
        argument_stack_.back()->set_used_for_S3_dispatch();
    }
}

void DenotedValue::used_for_S4_dispatch() {
    ++S4_dispatch_count_;

    // get_lifecycle().add_event(PromiseEvent::Type::S4Dispatch);

    if (is_argument()) {
        argument_stack_.back()->set_used_for_S4_dispatch();
    }
}

void DenotedValue::set_non_local_return() {
    non_local_return_ = true;
    if (is_argument()) {
        argument_stack_.back()->set_non_local_return();
    }
}
