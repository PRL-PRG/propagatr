
#include "Function.h"
#include "utilities.h"

std::string Function::find_namespace(const SEXP op) {
    if (TYPEOF(op) == SPECIALSXP || TYPEOF(op) == BUILTINSXP) {
        return "base";
    }

    SEXP namesym = R_NilValue;
    const char* packprefix = "package:";
    size_t pplen = strlen(packprefix);

    void (*probe)(dyntracer_t*, SEXP, SEXP, SEXP);
    probe = dyntrace_active_dyntracer->probe_environment_variable_lookup;
    dyntrace_active_dyntracer->probe_environment_variable_lookup = NULL;

    SEXP env = CLOENV(op);
    const char* name = "empty";

    while (TYPEOF(env) == ENVSXP && env != R_EmptyEnv) {
        if (env == R_GlobalEnv) {
            name = "global";
            break;
        }

        if (env == R_BaseEnv || env == R_BaseNamespace) {
            name = "base";
            break;
        }

        namesym = R_NamespaceEnvSpec(env);

        if (namesym != R_NilValue) {
            if (TYPEOF(namesym) == STRSXP && LENGTH(namesym) > 0) {
                name = CHAR(STRING_ELT(namesym, 0));
                break;
            } else if (TYPEOF(namesym) == CHARSXP) {
                name = CHAR(namesym);
                break;
            }
        }

        namesym = getAttrib(env, R_NameSymbol);

        if (namesym != R_NilValue && TYPEOF(namesym) == STRSXP &&
            Rf_length(namesym) > 0 &&
            !strncmp(packprefix, CHAR(STRING_ELT(namesym, 0)), pplen)) {
            name = CHAR(STRING_ELT(namesym, 0)) + pplen;
            break;
        }

        namesym = findVar(R_dot_packageName, env);

        if (TYPEOF(namesym) == STRSXP && LENGTH(namesym) > 0) {
            name = CHAR(STRING_ELT(namesym, 0));
            break;
        } else if (TYPEOF(namesym) == CHARSXP) {
            name = CHAR(namesym);
            break;
        }

        env = ENCLOS(env);
    }

    dyntrace_active_dyntracer->probe_environment_variable_lookup = probe;

    return name;
}

std::tuple<std::string, std::string, function_id_t>
Function::compute_definition_and_id(const SEXP op) {
    std::string definition;
    std::string package_name;
    function_id_t id;

    if (type_of_sexp(op) == CLOSXP) {
        definition = serialize_r_expression(op);
        package_name = find_namespace(op);
        id = compute_hash((package_name + definition).c_str());
    } else {
        definition = "function body not extracted for non closures";
        package_name = "base";
        id = dyntrace_get_c_function_name(op);
    }

    return make_tuple(package_name, definition, id);
}
