#ifndef PROMISEDYNTRACER_ARGUMENT_H
#define PROMISEDYNTRACER_ARGUMENT_H

#include "DenotedValue.h"
#include "definitions.h"
#include "sexptypes.h"
#include "utilities.h"

class Call;

class Argument {
  public:
    explicit Argument(Call* call,
                      int formal_parameter_position,
                      int actual_argument_position,
                      bool default_argument,
                      bool dot_dot_dot)
        : call_(call)
        , formal_parameter_position_(formal_parameter_position)
        , actual_argument_position_(actual_argument_position)
        , default_argument_(default_argument)
        , dot_dot_dot_(dot_dot_dot)
        , direct_force_(false)
        , indirect_force_(false)
        , direct_lookup_count_(0)
        , indirect_lookup_count_(0)
        , direct_metaprogram_count_(0)
        , indirect_metaprogram_count_(0)
        , S3_dispatch_(false)
        , S4_dispatch_(false)
        , non_local_return_(false)
        , denoted_value_(nullptr)
        , forcing_actual_argument_position_(
              UNASSIGNED_ACTUAL_ARGUMENT_POSITION) {
    }

    Call* get_call() {
        return call_;
    }

    int get_formal_parameter_position() const {
        return formal_parameter_position_;
    }

    int get_actual_argument_position() const {
        return actual_argument_position_;
    }

    bool is_default_argument() const {
        return default_argument_;
    }

    bool is_dot_dot_dot() const {
        return dot_dot_dot_;
    }

    void direct_force() {
        direct_force_ = true;
    }

    bool is_directly_forced() const {
        return direct_force_;
    }

    void indirect_force() {
        indirect_force_ = true;
    }

    bool is_indirectly_forced() const {
        return indirect_force_;
    }

    void direct_lookup() {
        direct_lookup_count_++;
    }

    int get_direct_lookup_count() const {
        return direct_lookup_count_;
    }

    void indirect_lookup() {
        indirect_lookup_count_++;
    }

    int get_indirect_lookup_count() const {
        return indirect_lookup_count_;
    }

    void direct_metaprogram() {
        direct_metaprogram_count_++;
    }

    int get_direct_metaprogram_count() const {
        return direct_metaprogram_count_;
    }

    void indirect_metaprogram() {
        indirect_metaprogram_count_++;
    }

    int get_indirect_metaprogram_count() const {
        return indirect_metaprogram_count_;
    }

    void set_used_for_S3_dispatch() {
        S3_dispatch_ = true;
    }

    bool used_for_S3_dispatch() const {
        return S3_dispatch_;
    }

    void set_used_for_S4_dispatch() {
        S4_dispatch_ = true;
    }

    bool used_for_S4_dispatch() const {
        return S4_dispatch_;
    }

    void set_non_local_return() {
        non_local_return_ = true;
    }

    bool does_non_local_return() const {
        return non_local_return_;
    }

    void set_denoted_value(DenotedValue* denoted_value) {
        denoted_value_ = denoted_value;
    }

    DenotedValue* get_denoted_value() {
        return denoted_value_;
    }

    void
    set_forcing_actual_argument_position(int forcing_actual_argument_position) {
        forcing_actual_argument_position_ = forcing_actual_argument_position;
    }

    int get_forcing_actual_argument_position() const {
        return forcing_actual_argument_position_;
    }

  private:
    Call* call_;
    const int formal_parameter_position_;
    const int actual_argument_position_;
    const bool default_argument_;
    const bool dot_dot_dot_;
    bool direct_force_;
    bool indirect_force_;
    int direct_lookup_count_;
    int indirect_lookup_count_;
    int direct_metaprogram_count_;
    int indirect_metaprogram_count_;
    bool S3_dispatch_;
    bool S4_dispatch_;
    bool non_local_return_;
    DenotedValue* denoted_value_;
    int forcing_actual_argument_position_;
};

#endif /* PROMISEDYNTRACER_ARGUMENT_H */
