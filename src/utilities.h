#ifndef PROMISEDYNTRACER_UTILITIES_H
#define PROMISEDYNTRACER_UTILITIES_H

// #include "constants.h"
#include "definitions.h"
#include "stdlibs.h"

#include <openssl/evp.h>
#include <type_traits>

#include <sys/stat.h>
#include <errno.h>

int mkdir_p(const char *path, mode_t mode);

#define failwith(format, ...) \
    failwith_impl(__FILE__, __LINE__, format, __VA_ARGS__)

#define failwith_impl(file, line, format, ...)                             \
    do {                                                                   \
        fprintf(stderr, "ERROR [%s:%d] " format, file, line, __VA_ARGS__); \
        exit(EXIT_FAILURE);                                                \
    } while (0)

/* getting types */
std::string get_type_of_sexp(SEXP thing);

int get_file_size(std::ifstream& file);

std::string readfile(std::ifstream& file);

bool file_exists(const std::string& filepath);

char* copy_string(char* destination, const char* source, size_t buffer_size);

bool sexp_to_bool(SEXP value);

int sexp_to_int(SEXP value);

std::string sexp_to_string(SEXP value);

std::string compute_hash(const char* data);

const char* get_name(SEXP sexp);

std::string serialize_r_expression(SEXP e);

std::string clock_ticks_to_string(clock_t ticks);
std::string to_string(const char* str);

inline std::string check_string(const char* s) {
    return s == NULL ? "<unknown>" : s;
}

inline void* malloc_or_die(std::size_t size) {
    void* data = std::malloc(size);
    if (data == nullptr) {
        failwith("memory allocation error: unable to allocate %zu bytes.\n",
                 size);
    }
    return data;
}

inline void* calloc_or_die(std::size_t num, std::size_t size) {
    void* data = std::calloc(num, size);
    if (data == nullptr) {
        failwith("memory allocation error: unable to allocate %zu bytes.\n",
                 size);
    }
    return data;
}

inline void* realloc_or_die(void* ptr, std::size_t size) {
    void* data = std::realloc(ptr, size);
    if (data == nullptr) {
        failwith("memory allocation error: unable to reallocate %zu bytes.\n",
                 size);
    }
    return data;
}

std::string pos_seq_to_string(const pos_seq_t& pos_seq);

inline bool is_dots_symbol(const SEXP symbol) {
    return symbol == R_DotsSymbol;
}

inline std::string symbol_to_string(const SEXP symbol) {
    return CHAR(PRINTNAME(symbol));
}

template <typename T>
inline void copy_and_reset(T& left, T& right) {
    left = right;
    right = 0;
}

/* is env_a parent of env_b */
inline bool is_parent_environment(SEXP env_a, SEXP env_b) {
    if (env_a == env_b)
        return false;
    for (SEXP env_cur = ENCLOS(env_b); env_cur != R_NilValue;
         env_cur = ENCLOS(env_cur)) {
        if (env_cur == env_a)
            return true;
    }
    return false;
}

template <typename E>
constexpr auto to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}

#endif /* PROMISEDYNTRACER__UTILITIES_H */
