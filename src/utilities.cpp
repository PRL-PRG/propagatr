#include "utilities.h"

#include "base64.h"

#include <algorithm>

#include "sexptypes.h"

const bool SHORTEN_ENV = true;

int mkdir_p(const char *path, mode_t mode)
{
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    const size_t len = strlen(path);
    char _path[PATH_MAX];
    char *p; 

    errno = 0;

    /* Copy string so its mutable */
    if (len > sizeof(_path)-1) {
        errno = ENAMETOOLONG;
        return -1; 
    }   
    strcpy(_path, path);

    /* Iterate the string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';

            if (mkdir(_path, mode) != 0) {
                if (errno != EEXIST)
                    return -1; 
            }

            *p = '/';
        }
    }   

    if (mkdir(_path, mode) != 0) {
        if (errno != EEXIST)
            return -1; 
    }   

    return 0;
}

int get_file_size(std::ifstream& file) {
    int position = file.tellg();
    file.seekg(0, std::ios_base::end);
    int length = file.tellg();
    file.seekg(position, std::ios_base::beg);
    return length;
}

std::string readfile(std::ifstream& file) {
    std::string contents;
    file.seekg(0, std::ios::end);
    contents.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    contents.assign(std::istreambuf_iterator<char>(file),
                    std::istreambuf_iterator<char>());
    return contents;
}

bool file_exists(const std::string& filepath) {
    return std::ifstream(filepath).good();
}

// typr stuff

std::string get_language_class(SEXP object) {
    SEXP head = CAR(object);
    if (type_of_sexp(head) == SYMSXP) {
        std::string name(CHAR(PRINTNAME(head)));
        if (name == "if" || name == "while" || name == "for" || name == "=" ||
            name == "<-" || name == "(" || name == "{") {
            return name;
        }
    }
    return "call";
}
std::vector<std::string> get_class_names(SEXP object) {
    std::vector<std::string> class_names;
    SEXP klass = getAttrib(object, R_ClassSymbol);
    /* class attribute not present  */
    if (klass == R_NilValue) {
        SEXP dim = getAttrib(object, R_DimSymbol);
        int ndim = Rf_length(dim);
        /* dimension attribute not present or of length 0  */
        if (ndim == 0) {
            SEXPTYPE t = TYPEOF(object);
            switch (t) {
            case CLOSXP:
            case SPECIALSXP:
            case BUILTINSXP:
                class_names.push_back("function");
                break;
            case REALSXP:
                /* NOTE: this is handled separately as ^double[]  */
                /* class_names.push_back("numeric"); */
                break;
            case SYMSXP:
                class_names.push_back("name");
                break;
            case LANGSXP:
                class_names.push_back(get_language_class(object));
                break;
            default:
                /* NOTE: these are handled separately */
                /* class_names.push_back(sexptype_to_string(type_of_sexp(object))); */
                break;
            }
        }
        /* two dimensions  */
        else if (ndim == 2) {
            class_names.push_back("matrix");
        }
        /* not two dimensions  */
        else {
            class_names.push_back("array");
        }
    }
    /* class attribute present  */
    else if (type_of_sexp(klass) == STRSXP) {
        for (int index = 0; index < LENGTH(klass); ++index) {
            class_names.push_back(CHAR(STRING_ELT(klass, index)));
        }
    }
    return class_names;
}

std::string deal_with_promise(SEXP thing) {
    
    // We know its a promise.
    SEXP val = dyntrace_get_promise_value(thing);
    SEXP old_expr = dyntrace_get_promise_expression(thing);
    // all will get forced, so yeah.
    while (TYPEOF(val) == PROMSXP) {
    // while (TYPEOF(val) == PROMSXP) {
        old_expr = dyntrace_get_promise_expression(val);
        val = dyntrace_get_promise_value(val);
    }

    auto the_type = TYPEOF(val);
    
    if (val != R_UnboundValue) {
        // std::cout << param_pos << ": something was passed and it was used.\n";

        return(get_type_of_sexp(val));
    } else {
        // If the type of old_expr is symbol or language, then it's missing.
        return(get_type_of_sexp(old_expr));
    }
}

std::string simple_type_of_value(SEXP val) {
    switch (TYPEOF(val)) {
        case NILSXP:
            return "NULL";
        case SYMSXP:
            return "symbol";
        case LISTSXP:
            return "pairlist";
        case CLOSXP:
            return "closure";
        case ENVSXP:
            return "env";
        case PROMSXP:
            return deal_with_promise(val);
        case LANGSXP:
            return "LANGSXP";
        case SPECIALSXP:
            return "special";
        case BUILTINSXP:
            return "builtin";
        case CHARSXP:
            return "CHARSXP";
        case LGLSXP:
            return "logical";
        case INTSXP:
            return "integer";
        case REALSXP:
            return "double";
        case CPLXSXP:
            return "complex";
        case STRSXP:
            return "character";
        case DOTSXP:
            return "dots";
        case ANYSXP:
            return "any";
        case VECSXP:
            return "list";
        case EXPRSXP:
            return "expression";
        case BCODESXP:
            return "BCODESXP";
        case EXTPTRSXP:
            return "EXTPTRSXP";
        case WEAKREFSXP:
            return "WEAKREFSXP";
        case RAWSXP:
            return "raw";
        case S4SXP:
            return "S4";
        case NEWSXP:
            return "NEWSXP";
        case FREESXP:
            return "FREESXP";
        /*
        case FUNSXP:
            return "function";
        */
    }

    return "ERROR?";
}

std::string vector_logic(std::string vec_type, SEXP vec_sexp) {
    int len = LENGTH(vec_sexp);
    bool has_na = false;
    int i = 0;
    std::string ret_str = vec_type;

    // deal with the possiblity that its a matrix
    if (Rf_isMatrix(vec_sexp)) {
        int n_row = Rf_nrows(vec_sexp);
        int n_col = Rf_ncols(vec_sexp);

        ret_str.append("[" + std::to_string(n_row) + "-" + std::to_string(n_col) + "]");

        return ret_str;
    }

    switch(TYPEOF(vec_sexp)) {
        case STRSXP: {
            for (i = 0; i < len; ++i) {
                if (STRING_ELT(vec_sexp, i) == NA_STRING) {
                    has_na = true;
                    break;
                }
            }
            break;
        }
        case LGLSXP: {
            auto as_bool_vec = LOGICAL(vec_sexp);
            for (i = 0; i < len; ++i) {
                if (as_bool_vec[i] == NA_LOGICAL) {
                    has_na = true;
                    break;
                }
            }
            break;
        }
        case INTSXP: {
            auto as_int_vec = INTEGER(vec_sexp);
            for (i = 0; i < len; ++i) {
                if (as_int_vec[i] == NA_INTEGER) {
                    has_na = true;
                    break;
                }
            }
            break;
        }
        case REALSXP: {
            auto as_real_vec = REAL(vec_sexp);
            for (i = 0; i < len; ++i) {
                if (ISNA(as_real_vec[i])) {
                    has_na = true;
                    break;
                }
            }
            break;
        }
        case CPLXSXP: {
            auto as_cplx_vec = COMPLEX(vec_sexp);
            for (i = 0; i < len; ++i) {
                if (ISNA(as_cplx_vec[i].r) || ISNA(as_cplx_vec[i].i)) {
                    has_na = true;
                    break;
                }
            }
            break;
        }
        case RAWSXP: {
            /* there won't be NAs here b/c raws are just raw bytes */
            break;
        }
    }

    if (len == 1) {
        // scalar
        // we deal with them separately, just put the length in
    }

    std::string len_portion = "[" + std::to_string(len) + "]";
    ret_str.append(len_portion);

    SEXP names = getAttrib(vec_sexp, R_NamesSymbol);
    std::string names_tag = "@names[";

    if (names != R_NilValue) {
        for(int i = 0; i < len; ++i) {
            // Sanitize name.
            std::string the_name_as_string(CHAR(VECTOR_ELT(names, i)));

            // std::replace(the_name_as_string.begin(), the_name_as_string.end(), ',', '#');
            // std::replace(the_name_as_string.begin(), the_name_as_string.end(), '`', '#');

            std::string oldComma(",");
            std::string oldBacktick("`");

            int pos;

            while ((pos = the_name_as_string.find(oldComma)) != std::string::npos)
                the_name_as_string.replace(pos, oldComma.length(), "[PROPAGATR-COMMA]");

            while ((pos = the_name_as_string.find(oldBacktick)) != std::string::npos)
                the_name_as_string.replace(pos, oldComma.length(), "[PROPAGATR-BACKTICK]");

            names_tag.append("`"); // delimits names
            names_tag.append(the_name_as_string);
            names_tag.append("`"); // ^
            if (i != len - 1)
                names_tag.append("~");
            else 
                names_tag.append("]");
        }

        if (len == 0) {
            names_tag.append("]");
        }
    
        ret_str.append(names_tag);

    }

    if (!has_na) {
        // append NA-less tag 

        ret_str.append("@NA-free");
    } else {
        // has NA, treat as NULL
        // if (len == 1)
        //     ret_str = "NULL";
    }

    return ret_str;
}

std::string list_logic(SEXP list_sxp) {
    
    if (Rf_isFrame(list_sxp)) {
        SEXP col_names = getAttrib(list_sxp, R_NamesSymbol);
        int num_cols = LENGTH(col_names);
        int num_rows = LENGTH(Rf_GetRowNames(list_sxp));

        std::string ret_df = "data.frame";
        ret_df.append("[" + std::to_string(num_rows) + "-" + std::to_string(num_cols) + "]");
        
        int len = LENGTH(col_names);
        if (len > 0) {
            ret_df.append("@cols[");
            for (int i = 0; i < len; ++i) {
                // NAMES :: column names
                std::string the_name_as_string(CHAR(STRING_ELT(col_names, i)));

                // std::replace(the_name_as_string.begin(), the_name_as_string.end(), ',', '#');
                // std::replace(the_name_as_string.begin(), the_name_as_string.end(), '`', '#');

                std::string oldComma(",");
                std::string oldBacktick("`");

                int pos;

                while ((pos = the_name_as_string.find(oldComma)) != std::string::npos)
                    the_name_as_string.replace(pos, oldComma.length(), "[PROPAGATR-COMMA]");

                while ((pos = the_name_as_string.find(oldBacktick)) != std::string::npos)
                    the_name_as_string.replace(pos, oldComma.length(), "[PROPAGATR-BACKTICK]");

                // Deal with the type of the column
                SEXP elt = VECTOR_ELT(list_sxp, i);
                // This is if we want simple types. To get full types, do the other thing.
                // std::string this_type = simple_type_of_value(elt);
                // The other thing, for full types.
                std::string this_type = get_type_of_sexp(elt);

                ret_df.append("`"); // backticks to delimit names
                ret_df.append(the_name_as_string);
                ret_df.append("`:");
                ret_df.append(this_type);

                if (i != len - 1)
                    ret_df.append("~");
            }
            ret_df.append("]");
        }

        return ret_df;
    }

    bool has_null = false;

    std::string ret_str = "list<";
    std::string this_type = "empty";
    int len = LENGTH(list_sxp);

    // NAMES :: list names 
    SEXP names = getAttrib(list_sxp, R_NamesSymbol);
    // std::string names_tag = "@names[";
    std::string inner_names = "";
    std::string overall_type = "";

    // 
    for(int i = 0; i < len; ++i) {
        SEXP elt = VECTOR_ELT(list_sxp, i);
        // This is if we want simple types. To get full types, do the other thing.
        // this_type = simple_type_of_value(elt);
        // The other thing, for full types.
        this_type = get_type_of_sexp(elt);

        // This is if we just want one inner type.
        // To build tuples, do the next thing.
        // Check if the type was consistent throughout the list.
        // if (i == 0) {
        //     overall_type = this_type;
        // } else {
        //     if (this_type.compare(overall_type) != 0) {
        //         overall_type = "any";
        //     }
        // }

        // For tuples:
        overall_type.append(this_type);
        if (i != len - 1)
            overall_type.append("~");
        
        if (names != R_NilValue) {
            // If there are names, make it a struct.
            ret_str = "struct<";
            std::string the_name_as_string(CHAR(VECTOR_ELT(names, i)));

            // std::replace(the_name_as_string.begin(), the_name_as_string.end(), ',', '#');
            // std::replace(the_name_as_string.begin(), the_name_as_string.end(), '`', '#');

            std::string oldComma(",");
            std::string oldBacktick("`");

            int pos;

            while ((pos = the_name_as_string.find(oldComma)) != std::string::npos)
                the_name_as_string.replace(pos, oldComma.length(), "[PROPAGATR-COMMA]");

            while ((pos = the_name_as_string.find(oldBacktick)) != std::string::npos)
                the_name_as_string.replace(pos, oldComma.length(), "[PROPAGATR-BACKTICK]");

            inner_names.append("`"); // backticks to delimit names
            inner_names.append(the_name_as_string);
            inner_names.append("`:");
            inner_names.append(this_type);
            if (i != len - 1)
                inner_names.append("~");
            // else 
            //     inner_names.append("]");
        }

        if (elt == R_NilValue)
            has_null = true;
    }

    if (names == R_NilValue)
        ret_str.append(overall_type);
    else
        ret_str.append(inner_names);

    ret_str.append(">");
    std::string len_str = "[" + std::to_string(len) + "]";
    ret_str.append(len_str);

    // if (names != R_NilValue)
    //     ret_str.append(names_tag);

    if (!has_null) {
        ret_str.append("@NULL-free");
    }

    return ret_str;
}

std::string env_logic(SEXP env_sxp) {
    std::string ret_str = "environment";

    if (SHORTEN_ENV) {
        return (ret_str);
    }

    // these environments are big so we just shorten them
    if (env_sxp == R_GlobalEnv) {
        ret_str.append("{global}");
    } else if (env_sxp == R_BaseEnv || env_sxp == R_BaseNamespace) {
        ret_str.append("{base}");
    } else {
        // TRUE is Rboolean
        // var_names will be a character vector
        SEXP var_names = R_lsInternal(env_sxp, TRUE);

        int len = LENGTH(var_names);
        std::string bindings = "{";
        for (int i = 0; i < len; ++i) {
            bindings.append(CHAR(STRING_ELT(var_names, i)));
            if (i != len - 1) {
                bindings.append("~");
            }
        }
        bindings.append("}");
        ret_str.append(bindings);
    }

    return ret_str;
}

/* typr */
std::string get_type_of_sexp(SEXP thing) {

    // Start by checking to see if thing has a class.
    std::vector<std::string> class_names = get_class_names(thing);

    if (class_names.size() > 0) {
        // In this case, just return class<fold> as the type.
        std::sort(class_names.begin(), class_names.end());
        std::string ret_str = "class<`";
        for (int i = 0; i < class_names.size(); i++) {
            if (i != 0)
                ret_str.append("`, `");

            ret_str.append(class_names[i]);
        }
        ret_str.append("`>");
        return ret_str;
    }

    int len = 1;
    switch (TYPEOF(thing)) {
        case NILSXP:
            return "NULL";
        case SYMSXP:
            return "symbol";
        case LISTSXP:
            // NOT A LIST
            return "pairlist";
        case CLOSXP:
            return "closure";
        case ENVSXP:
            // TODO replace this when ready
            return env_logic(thing);
            // return "environment";
        case PROMSXP:
            return deal_with_promise(thing);
        case LANGSXP:
            return "LANGSXP";
        case SPECIALSXP:
            return "special";
        case BUILTINSXP:
            return "builtin";
        case CHARSXP:
            return "CHARSXP";
        case LGLSXP:
            return vector_logic("logical", thing);
        case INTSXP:
            return vector_logic("integer", thing);
        case REALSXP:
            return vector_logic("double", thing);
        case CPLXSXP:
            return vector_logic("complex", thing);
        case STRSXP:
            return vector_logic("character", thing);
        case DOTSXP:
            return "list<any>";
        case ANYSXP:
            return "any";
        case VECSXP:
            return list_logic(thing);
        case EXPRSXP:
            return "expression";
        case BCODESXP:
            return "BCODESXP";
        case EXTPTRSXP:
            return "EXTPTRSXP";
        case WEAKREFSXP:
            return "WEAKREFSXP";
        case RAWSXP:
            return vector_logic("raw", thing);
        case S4SXP:
            return "S4";
        case NEWSXP:
            return "NEWSXP";
        case FREESXP:
            return "FREESXP";
        /*
        case FUNSXP:
            return "function";
        */
    }

    return "ERROR?";
}

char* copy_string(char* destination, const char* source, size_t buffer_size) {
    size_t l = strlen(source);
    if (l >= buffer_size) {
        strncpy(destination, source, buffer_size - 1);
        destination[buffer_size - 1] = '\0';
    } else {
        strcpy(destination, source);
    }
    return destination;
}

bool sexp_to_bool(SEXP value) {
    return LOGICAL(value)[0] == TRUE;
}

int sexp_to_int(SEXP value) {
    return (int) *INTEGER(value);
}

std::string sexp_to_string(SEXP value) {
    return std::string(CHAR(STRING_ELT(value, 0)));
}

const char* get_name(SEXP sexp) {
    const char* s = NULL;

    switch (TYPEOF(sexp)) {
    case CHARSXP:
        s = CHAR(sexp);
        break;
    case LANGSXP:
        s = get_name(CAR(sexp));
        break;
    case BUILTINSXP:
    case SPECIALSXP:
        s = CHAR(PRIMNAME(sexp));
        break;
    case SYMSXP:
        s = CHAR(PRINTNAME(sexp));
        break;
    }

    return s == NULL ? "" : s;
}

#include <Rinternals.h>

std::string serialize_r_expression(SEXP e) {
    std::string expression;
    int linecount = 0;
    SEXP strvec = serialize_sexp(e, &linecount);
    for (int i = 0; i < linecount - 1; ++i) {
        expression.append(CHAR(STRING_ELT(strvec, i))).append("\n");
    }
    if (linecount >= 1) {
        expression.append(CHAR(STRING_ELT(strvec, linecount - 1)));
    }
    return expression;
}

std::string compute_hash(const char* data) {
    const EVP_MD* md = EVP_md5();
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX mdctx;
    EVP_MD_CTX_init(&mdctx);
    EVP_DigestInit_ex(&mdctx, md, NULL);
    EVP_DigestUpdate(&mdctx, data, strlen(data));
    EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
    EVP_MD_CTX_cleanup(&mdctx);
#else
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    EVP_MD_CTX_init(mdctx);
    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, data, strlen(data));
    EVP_DigestFinal_ex(mdctx, md_value, &md_len);
    EVP_MD_CTX_free(mdctx);
#endif
    std::string result{base64_encode(
        reinterpret_cast<const unsigned char*>(md_value), md_len)};

    // This replacement is done so that the hash can be directly used
    // as a filename. If this is not done, the / in the hash prevents
    // it from being used as the name of the file which contains the
    // function which is hashed.
    std::replace(result.begin(), result.end(), '/', '#');
    return result;
}

const char* remove_null(const char* value) {
    return value ? value : "";
}

std::string to_string(const char* str) {
    return str ? std::string(str) : std::string("");
}

std::string pos_seq_to_string(const pos_seq_t& pos_seq) {
    if (pos_seq.size() == 0) {
        return "()";
    }

    std::string str = "(" + std::to_string(pos_seq[0]);

    for (auto i = 1; i < pos_seq.size(); ++i) {
        str.append(" ").append(std::to_string(pos_seq[i]));
    }

    return str + ")";
}
