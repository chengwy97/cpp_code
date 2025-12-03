#pragma once

#ifdef _WIN32
    #ifdef SHARED_LIB_EXPORTS
        #define SHARED_LIB_API __declspec(dllexport)
    #else
        #define SHARED_LIB_API __declspec(dllimport)
    #endif
#else
    #define SHARED_LIB_API
#endif

namespace shared_lib {
    SHARED_LIB_API int divide(int a, int b);
    SHARED_LIB_API void print_shared();
}

