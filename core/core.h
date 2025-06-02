#pragma once

#ifdef _WIN32
#define VCPP_API __declspec(dllexport)
#else
#define VCPP_API
#endif

extern "C"
{
    VCPP_API int vcpp_ktx(int argc, char *argv[], const char *options = nullptr);

    VCPP_API int vcpp_fbx(int argc, char *argv[], const char *options = nullptr);

    VCPP_API int vcpp_image_denoise(const char *options = nullptr);

    VCPP_API int vcpp_test(const char *options = nullptr);
}