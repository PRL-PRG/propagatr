#ifndef TYPEDYNTRACER_TYPE_H
#define TYPEDYNTRACER_TYPE_H

#include "utilities.h"

class Type {

    public:
    explicit Type(std::string top_level_type) :
    top_level_type_(top_level_type) {}

    explicit Type(SEXP get_my_type, const std::vector<std::string> tags = {}) {
        
        /* type */

        // tags first
        tags_ = tags;

        if (get_my_type == R_MissingArg) {
            top_level_type_ = "missing";
        } else {
            // split if necessary
            std::string type = get_type_of_sexp(get_my_type);
            auto loc_of_at = type.find("@");
            
            if (loc_of_at == std::string::npos) {
                // not in
                top_level_type_ = type;
            } else {
                top_level_type_ = type.substr(0, loc_of_at);
                tags_.push_back(type.substr(loc_of_at+1));
            }
        }

        /* class(es) */ /* TODO is this the right way to do this? */
        // SEXP class_as_charsxp = Rf_S3Class(get_my_type);
        SEXP class_as_charsxp = Rf_getAttrib(get_my_type, R_ClassSymbol);

        // TODO do i have to check for null here?
        for (int i = 0; i < LENGTH(class_as_charsxp); i++) {
            classes_.push_back(CHAR(STRING_ELT(class_as_charsxp, i)));
        }

        /* attributes */
        SEXP attrs_as_sxp = ATTRIB(get_my_type);
        if (attrs_as_sxp == R_NilValue) {
            // attributes are null
        } else {
            SEXP attr_names = getAttrib(attrs_as_sxp, R_NamesSymbol);
            for (int i = 0; i < LENGTH(attr_names); i++) {
                attr_names_.push_back(CHAR(STRING_ELT(attr_names, i)));
            }
        }
    }

    std::string get_top_level_type() const {
        return top_level_type_;
    }

    void set_top_level_type(std::string new_one) {
        top_level_type_ = new_one;
    }

    std::vector<std::string> get_attr_names() const {
        return attr_names_;
    }

    void set_attr_names(std::vector<std::string> new_one) {
        attr_names_ = new_one;
    }

    std::vector<std::string> get_classes() const {
        return classes_;
    }

    void set_classes(std::vector<std::string> new_one) {
        classes_ = new_one;
    }

    /* use this when hashing CallTrace */
    std::size_t hash_type() const {
        std::size_t the_hash = std::hash<std::string>()(top_level_type_);
        
        // need to do a commutative operation cause we arent guaranteed the order here
        for (int i = 0; i < attr_names_.size(); ++i) {
            the_hash = the_hash * std::hash<std::string>()(attr_names_[i]);
        }

        for (int i = 0; i < classes_.size(); ++i) {
            the_hash = the_hash * std::hash<std::string>()(classes_[i]);
        }

        for (int i = 0; i < tags_.size(); ++i) {
            the_hash = the_hash * std::hash<std::string>()(tags_[i]);
        }

        return the_hash;
    }

    std::vector<std::string> * get_tags() {
        return & tags_;
    }

    private:
    std::string top_level_type_;
    std::vector<std::string> attr_names_;
    std::vector<std::string> classes_;
    std::vector<std::string> tags_;

};

#endif /* TYPEDYNTRACER_TYPE_H */