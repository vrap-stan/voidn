cmake_minimum_required(VERSION 3.20)
set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

set(CMAKE_GENERATOR "Ninja" CACHE STRING "Ninja generator")
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type")

project(OIDNProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# OIDN 설정
set(OIDN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/oidn")
include_directories(${OIDN_DIR}/include)
link_directories(${OIDN_DIR}/lib)

# vcpkg 설정 (OpenImageIO)
find_package(OpenImageIO CONFIG REQUIRED)

add_executable(oidn_app main.cpp)

# oidn 및 OpenImageIO 링크
target_link_libraries(oidn_app PRIVATE
    OpenImageIO::OpenImageIO
    OpenImageDenoise_core.lib
    OpenImageDenoise.lib
    OpenImageDenoise_device_cpu.lib
    OpenImageDenoise_device_cuda.lib
    )
