#ifndef PROMISEDYNTRACER_CALL_H
#define PROMISEDYNTRACER_CALL_H

#include "Argument.h"
#include "definitions.h"
#include "stdlibs.h"
#include "sexptypes.h"

class Function;

class Call {
  public:
    /* defined in cpp file to get around cyclic dependency issues. */
    explicit Call(const call_id_t id,
                  const std::string& function_name,
                  const SEXP environment,
                  Function* function);

    call_id_t get_id() const {
        return id_;
    }

    void add_argument(Argument* argument) {
        arguments_.push_back(argument);
        ++actual_argument_count_;
    }

    const std::string& get_function_name() const {
        return function_name_;
    }

    Function* get_function() {
        return function_;
    }

    const Function* get_function() const {
        return function_;
    }

    void set_actual_argument_count(int actual_argument_count) {
        actual_argument_count_ = actual_argument_count;
    }

    int get_actual_argument_count() const {
        return actual_argument_count_;
    }

    SEXP get_environment() const {
        return environment_;
    }

    void set_return_value_type(sexptype_t return_value_type) {
        return_value_type_ = return_value_type;
    }

    sexptype_t get_return_value_type() const {
        return return_value_type_;
    }

    void set_jumped() {
        jumped_ = true;
    }

    bool is_jumped() const {
        return jumped_;
    }

    void set_S3_method() {
            S3_method_ = true;
        }

        void set_S4_method() {
            S4_method_ = true;
        }

    void set_force_order(int force_order) {
        force_order_ = {force_order};
    }

    std::vector<Argument*>& get_arguments() {
        return arguments_;
    }

  private:
    const call_id_t id_;
    const std::string function_name_;
    int actual_argument_count_;
    const SEXP environment_;
    Function* function_;
    sexptype_t return_value_type_;
    std::vector<Argument*> arguments_;
    bool jumped_;
    pos_seq_t force_order_;
    bool S3_method_;
    bool S4_method_;
};

#endif /* PROMISEDYTRACER_CALL_H */
