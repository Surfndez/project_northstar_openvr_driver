#pragma once

#if defined(_WIN32)
#define HMD_EXPORT extern "C" __declspec( dllexport )
#define HMD_IMPORT extern "C" __declspec( dllimport )
#elif defined(__GNUC__) || defined(COMPILER_GCC) || defined(__APPLE__)
#define HMD_EXPORT extern "C" __attribute__((visibility("default")))
#define HMD_IMPORT extern "C" 
#else
#error "Unsupported Platform."
#endif
