#ifndef PROMISEDYNTRACER_FUNCTION_H
#define PROMISEDYNTRACER_FUNCTION_H

#include "Call.h"
#include "Rinternals.h"
#include "sexptypes.h"

#include <fstream>

class Function {
  public:
    explicit Function(const SEXP op,
                      const std::string& package_name,
                      const std::string& definition,
                      const function_id_t& id)
        : formal_parameter_count_(0)
        , wrapper_(true)
        , namespace_(package_name)
        , definition_(definition)
        , id_(id) {
        type_ = type_of_sexp(op);

        if (type_ == CLOSXP) {
            for (SEXP formal = FORMALS(op); formal != R_NilValue;
                 formal = CDR(formal)) {
                ++formal_parameter_count_;
            }
            primitive_offset_ = -1;
            byte_compiled_ = TYPEOF(BODY(op)) == BCODESXP;
        } else {
            formal_parameter_count_ = dyntrace_get_c_function_arity(op);
            primitive_offset_ = dyntrace_get_primitive_offset(op);
            byte_compiled_ = false;
        }
    }

    bool is_byte_compiled() const {
        return byte_compiled_;
    }

    int get_formal_parameter_count() const {
        return formal_parameter_count_;
    }

    sexptype_t get_type() const {
        return type_;
    }

    bool is_closure() const {
        return type_ == CLOSXP;
    }

    bool is_special() const {
        return type_ == SPECIALSXP;
    }

    bool is_builtin() const {
        return type_ == BUILTINSXP;
    }

    int get_primitive_offset() const {
        return primitive_offset_;
    }

    bool is_return() const {
        return (get_primitive_offset() == PRIMITIVE_RETURN_OFFSET_);
    }

    bool is_curly_bracket() const {
        return (get_primitive_offset() == PRIMITIVE_CURLY_BRACKET_OFFSET_);
    }

    bool is_dot_internal() const {
        return (get_primitive_offset() == PRIMITIVE_DOT_INTERNAL_OFFSET_);
    }

    bool is_dot_primitive() const {
        return (get_primitive_offset() == PRIMITIVE_DOT_PRIMITIVE_OFFSET_);
    }

    bool is_dot_c() const {
        return (get_primitive_offset() == PRIMITIVE_DOT_C_OFFSET_);
    }

    bool is_dot_fortran() const {
        return (get_primitive_offset() == PRIMITIVE_DOT_FORTRAN_OFFSET_);
    }

    bool is_dot_external() const {
        return (get_primitive_offset() == PRIMITIVE_DOT_EXTERNAL_OFFSET_);
    }

    bool is_dot_external2() const {
        return (get_primitive_offset() == PRIMITIVE_DOT_EXTERNAL2_OFFSET_);
    }

    bool is_dot_call() const {
        return (get_primitive_offset() == PRIMITIVE_DOT_CALL_OFFSET_);
    }

    bool is_dot_external_graphics() const {
        return (get_primitive_offset() ==
                PRIMITIVE_DOT_EXTERNAL_GRAPHICS_OFFSET_);
    }

    bool is_dot_call_graphics() const {
        return (get_primitive_offset() == PRIMITIVE_DOT_CALL_GRAPHICS_OFFSET_);
    }

    const function_id_t& get_id() const {
        return id_;
    }

    const std::string& get_definition() const {
        return definition_;
    }

    const std::string& get_namespace() const {
        return namespace_;
    }

    const std::vector<std::string>& get_names() const {
        return names_;
    }

    void add_summary(Call* call) {
        int i;

        for (i = 0; i < names_.size(); ++i) {
            if (names_[i] == call->get_function_name()) {
                break;
            }
        }

        if (i == names_.size()) {
            names_.push_back(call->get_function_name());
        }

    }

    std::string get_name_string() const {
        const std::string& package = get_namespace();
        const std::vector<std::string>& names = get_names();

        std::string all_names = "(";

        if (names.size() >= 1) {
            all_names += package + "::" + names[0];
        }

        for (std::size_t i = 1; i < names.size(); ++i) {
            all_names += " " + package + "::" + names[i];
        }

        all_names += ")";

        return all_names;
    }

    static std::string find_namespace(const SEXP op);

    static std::tuple<std::string, std::string, function_id_t>
    compute_definition_and_id(const SEXP op);

  private:
    sexptype_t type_;
    std::size_t formal_parameter_count_;
    bool wrapper_;
    std::string namespace_;
    std::string definition_;
    function_id_t id_;
    int primitive_offset_;
    bool byte_compiled_;

    std::vector<std::string> names_;

    static const int PRIMITIVE_RETURN_OFFSET_ = 6;
    static const int PRIMITIVE_CURLY_BRACKET_OFFSET_ = 11;
    static const int PRIMITIVE_DOT_INTERNAL_OFFSET_ = 26;
    static const int PRIMITIVE_DOT_PRIMITIVE_OFFSET_ = 27;
    static const int PRIMITIVE_DOT_C_OFFSET_ = 426;
    static const int PRIMITIVE_DOT_FORTRAN_OFFSET_ = 427;
    static const int PRIMITIVE_DOT_EXTERNAL_OFFSET_ = 428;
    static const int PRIMITIVE_DOT_EXTERNAL2_OFFSET_ = 429;
    static const int PRIMITIVE_DOT_CALL_OFFSET_ = 430;
    static const int PRIMITIVE_DOT_EXTERNAL_GRAPHICS_OFFSET_ = 431;
    static const int PRIMITIVE_DOT_CALL_GRAPHICS_OFFSET_ = 432;
};

#endif /* PROMISEDYNTRACER_FUNCTION_H */
