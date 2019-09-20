#include "utilities.h"

#include "base64.h"

#include <algorithm>

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
