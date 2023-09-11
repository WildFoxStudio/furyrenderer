// Copyright RedFox Studio 2022

#pragma once

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)

#include <intrin.h>
// Stops the debugger - checkout link on how to stop debugger
// https://docs.microsoft.com/en-us/cpp/intrinsics/debugbreak?view=msvc-160
#define SIGNALBREAK() (__nop(), __debugbreak())
#define DEPRECATED(FUNCNAME, COMMENT) __declspec(deprecated)
#elif defined(__APPLE__) && defined(__aarch64__)
#include <unistd.h>
#define SIGNALBREAK() \
    __asm__ __volatile__("   mov    x0, %x0;    \n" /* pid                */ \
                         "   mov    x1, #0x11;  \n" /* SIGSTOP            */ \
                         "   mov    x16, #0x25; \n" /* syscall 37 = kill  */ \
                         "   svc    #0x80       \n" /* software interrupt */ \
                         "   mov    x0, x0      \n" /* nop                */ \
                         ::"r"(getpid()) \
                         : "x0", "x1", "x16", "memory")
#elif defined(__APPLE__) && defined(__arm__)
#include <unistd.h>
#define SIGNALBREAK() \
    __asm__ __volatile__("   mov    r0, %0;     \n" /* pid                */ \
                         "   mov    r1, #0x11;  \n" /* SIGSTOP            */ \
                         "   mov    r12, #0x25; \n" /* syscall 37 = kill  */ \
                         "   svc    #0x80       \n" /* software interrupt */ \
                         "   mov    r0, r0      \n" /* nop                */ \
                         ::"r"(getpid()) \
                         : "r0", "r1", "r12", "memory")
#elif defined(__APPLE__) && (defined(__i386__) || defined(__x86_64__))
#include <unistd.h>
#define SIGNALBREAK() __asm__ __volatile__("int $3; mov %eax, %eax")
#else pragma error "Incompatible architecture"
#endif



#include <iostream>

// Define debug functions here

// Log print - temporary for now will use the console output
inline void
_fprint(const char* _condition, const char* file, const int line)
{
    std::cerr << "check(" << _condition << ") IN FILE " << file << " LINE " << line << "\n" << std::flush;
};

#define ERRORLOG(msg) std::cerr << "Error:" << msg << " IN FILE " << __FILE__ << " LINE " << __LINE__ << "\n" << std::flush;

// Use check only to trigger an exception in debug mode
// if condition = false then the debugger will stop
// clang-format off
#if defined(_DEBUG)
#define check(condition) \
        { \
            if (!(condition)) \
                { \
                    _fprint(#condition, __FILE__, __LINE__); \
                    SIGNALBREAK(); \
                } \
        };
#else
#define check(condition) 
#endif

// Use critical it will throw a runtime error if the condition is false even in release builds
#if !defined(_DEBUG)

#define critical(condition) \
    {\
        if(!(condition))\
            {\
            _fprint(#condition, __FILE__, __LINE__);\
            throw std::runtime_error("The program has encountered an error\n");\
            }\
    };

#else

#define critical(condition) \
    {\
        if(!(condition))\
            {\
            _fprint(#condition, __FILE__, __LINE__);\
            SIGNALBREAK(); \
            }\
    }
#endif

//clang-format on
