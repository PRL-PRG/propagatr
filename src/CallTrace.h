#ifndef TYPEDYNTRACER_CALL_TRACE_H
#define TYPEDYNTRACER_CALL_TRACE_H

#include "Type.h"
#include <iostream>
// NOTE for mac need : export LIBRARY_PATH=/usr/local/opt/openssl/lib/

class CallTrace {

    public:
    explicit CallTrace(std::string pname, std::string fname, function_id_t fn_id, dyntrace_dispatch_t dispatch, int uid) :
    pkg_name_(pname), fun_name_(fname), fn_id_(fn_id), dispatch_(dispatch), uid_(uid) { }

    CallTrace(CallTrace* ct) {
        pkg_name_ = ct->get_package_name();
        has_dots_ = ct->get_has_dots();
        fun_name_ = ct->get_function_name();
        fn_id_ = ct->get_fn_id();
        dispatch_ = ct->get_dispatch_type();
        uid_ = ct->get_uid();  
        call_trace_ = ct->get_call_trace(); // potentially bad?
    }

    CallTrace() {
        pkg_name_ = "";
        fun_name_ = "";
        fn_id_ = "";
        dispatch_ = DYNTRACE_DISPATCH_NONE;
    }

    std::string get_function_name() const {
        return fun_name_;
    }

    void set_function_name(std::string fname) {
        fun_name_ = fname;
    }

    std::string get_package_name() const {
        return pkg_name_;
    }

    void set_package_name(std::string pname) {
        pkg_name_ = pname;
    }

    function_id_t get_fn_id() {
        return fn_id_;
    }

    void set_fn_id(function_id_t fn_id) {
        fn_id_ = fn_id;
    }

    dyntrace_dispatch_t get_dispatch_type() const {
        return dispatch_;
    }

    void set_dispatch_type(dyntrace_dispatch_t n) {
        dispatch_ = n;
    }

    std::unordered_map<int, Type> & get_call_trace() {
        return call_trace_;
    }

    void add_to_call_trace(int ppos, Type ptype) {
        auto i = call_trace_.find(ppos);
        if (i == call_trace_.end()) {
            call_trace_.insert(std::make_pair(ppos, ptype));
        } else {
            i->second = ptype;
        }
    }

    bool operator==(const CallTrace & trace) const {
        return this->compute_hash() == trace.compute_hash();
    }
    
    bool operator!=(const CallTrace & trace) const {
        return this->compute_hash() == trace.compute_hash();
    }

    bool operator<(const CallTrace & trace) const {
        return this->compute_hash() < trace.compute_hash();
    }

    std::size_t compute_hash() const {
        size_t the_hash = ((std::hash<std::string>()(fun_name_)
                           ^ (std::hash<std::string>()(pkg_name_) << 1)) >> 1)
                           ^ (std::hash<std::string>()(fn_id_) << 1) >> 1
                           ^ (std::hash<dyntrace_dispatch_t>()(dispatch_) << 1) >> 1;
        
        // need to do a commutative operation cause we arent guaranteed the order here
        the_hash *= compute_hash_just_for_types();

        // TODO do we want to do anything else with the hash here?

        return the_hash;
    }

    std::size_t compute_hash_just_for_types() const {
        size_t the_hash = 1;
        for (auto i = call_trace_.begin(); i != call_trace_.end(); ++i) {
            the_hash *= std::hash<int>()(i->first) ^ i->second.hash_type();
        }

        return the_hash;
    }

    int get_uid() {
        return uid_;
    }

    bool get_has_dots() const {
        return has_dots_;
    }

    void set_has_dots(bool new_has_dots) {
        has_dots_ = new_has_dots;
    }

    private:
    int uid_;
    bool has_dots_ = false;
    std::string pkg_name_;
    std::string fun_name_;
    function_id_t fn_id_;
    dyntrace_dispatch_t dispatch_;
    std::unordered_map<int, Type> call_trace_;

};

struct CallTraceHasher
{
    std::size_t operator()(const CallTrace& ct) const
    {
        return ct.compute_hash();
    }
};

#endif /* TYPEDYNTRACER_CALL_TRACE_H */
