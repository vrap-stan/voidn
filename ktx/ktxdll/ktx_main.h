// ktx_main.h (Your existing version is fine)
#pragma once

#ifdef _WIN32
    #define KTX_API __declspec(dllexport)
#else
    #define KTX_API
#endif

extern "C" {
KTX_API int ktx_main(int argc, char* argv[]);
// No need for ktx_run_command_with_args if we modify InitUTF8CLI behavior conditionally
}