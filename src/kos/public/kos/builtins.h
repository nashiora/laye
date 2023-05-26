#ifndef KOS_BUILTINS_H
#define KOS_BUILTINS_H

#include <stdarg.h>
#include <stdbool.h>

#ifndef KOS_NO_SHORT_NAMES
#  define raise_fatal_error kos_raise_fatal_error
#endif // KOS_NO_SHORT_NAMES

#ifndef _MSC_VER
#  define NORETURN __attribute__((noreturn))
#  define PRETTY_FUNCTION __PRETTY_FUNCTION__
#  define BUILTIN_UNREACHABLE() __builtin_unreachable()
#else
#  define NORETURN __declspec(noreturn)
#  define PRETTY_FUNCTION __FUNCSIG__
#  define BUILTIN_UNREACHABLE() __assume(0)
#endif

#if __STDC_VERSION__ >= 201112L
#define static_assert(...) _Static_assert(__VA_ARGS__)
#else
#define static_assert(...) ASSERT(__VA_ARGS__)
#endif

#ifdef assert
#undef assert
#endif
#define assert(cond, ...) ((cond) ? cast(void) 0 : (kos_raise_fatal_error(__FILE__, PRETTY_FUNCTION, __LINE__, false, false, #cond, "" __VA_ARGS__), BUILTIN_UNREACHABLE()))

#define PPSTR_(x) #x
#define PPSTR(x) PPSTR_(x)

#define CAT_(a, b) a ## b
#define CAT(a, b) CAT_(a, b)

#ifndef noreturn
#define noreturn NORETURN
#endif

#ifndef cast
#define cast(T) (T)
#endif

#ifndef nullptr
#define nullptr NULL
#endif

#define KOS_INTERNAL_PROGRAM_ERROR(...) (kos_raise_fatal_error(__FILE__, PRETTY_FUNCTION, __LINE__, false, false, NULL, __VA_ARGS__), BUILTIN_UNREACHABLE())

#ifndef unimplemented
#define unimplemented KOS_INTERNAL_PROGRAM_ERROR("unimplemented")
#endif

#ifndef unreachable
#define unreachable KOS_INTERNAL_PROGRAM_ERROR("unreachable")
#endif

#ifndef list
#define list(T) T*
#endif

#ifndef hashmap
#define hashmap(K, V) struct { K key; V value; }
#endif

#ifndef strmap
#define strmap(V) hashmap(char*, V)
#endif

noreturn void kos_raise_fatal_error(
    const char* file, const char* func, int line,
    bool isSignalOrException, bool isSorry,
    const char* assertCondition,
    const char* fmt, ...);

#endif // KOS_BUILTINS_H
