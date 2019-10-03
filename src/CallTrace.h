#ifndef TYPEDYNTRACER_CALL_TRACE_H
#define TYPEDYNTRACER_CALL_TRACE_H

#include "Type.h"

// NOTE for mac need : export LIBRARY_PATH=/usr/local/opt/openssl/lib/

class CallTrace {

    public:
    explicit CallTrace(std::string pname, std::string fname) :
    pkg_name_(pname), fun_name_(fname) {}

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

    std::unordered_map<int, Type> get_call_trace() const {
        return call_trace_;
    }

    void add_to_call_trace(int ppos, Type ptype) {
        call_trace_.insert(std::make_pair(ppos, ptype));
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
                           ^ (std::hash<std::string>()(pkg_name_) << 1)) >> 1);
        
        // need to do a commutative operation cause we arent guaranteed the order here
        for (auto i = call_trace_.begin(); i != call_trace_.end(); ++i) {
            the_hash *= std::hash<int>()(i->first) ^ i->second.hash_type();
        }

        // TODO do we want to do anything else with the hash here?

        return the_hash;
    }

    private:
    std::string pkg_name_;
    std::string fun_name_;
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