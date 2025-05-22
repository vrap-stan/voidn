cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
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

#Autodesk FBX SDK
set(FBX_SDK_DIR "C:/Program Files/Autodesk/FBX/FBX SDK/2020.3.7")
include_directories(${FBX_SDK_DIR}/include)
# link_directories(${FBX_SDK_DIR}/lib/x64/debug)


#ofbx
# find_package(libdeflate CONFIG REQUIRED)
# find_package(unofficial-openfbx CONFIG REQUIRED)
# # set(OFBX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/openfbx")
# # include_directories(${OFBX_DIR}/include)
# # link_directories(${OFBX_DIR}/lib)

# vcpkg 설정 (OpenImageIO)
find_package(OpenImageIO CONFIG REQUIRED)
find_package(draco CONFIG REQUIRED)


add_executable(oidn_app main.cpp)

# oidn 및 OpenImageIO 링크
target_link_libraries(oidn_app PRIVATE
    draco::draco
    # libdeflate
    # openfbx
    OpenImageIO::OpenImageIO
    OpenImageDenoise_core.lib
    OpenImageDenoise.lib
    OpenImageDenoise_device_cpu.lib
    OpenImageDenoise_device_cuda.lib

    # Autodesk FBX
    ${FBX_SDK_DIR}/lib/x64/debug/alembic.lib
    ${FBX_SDK_DIR}/lib/x64/debug/libfbxsdk.lib
    ${FBX_SDK_DIR}/lib/x64/debug/libxml2.lib
    ${FBX_SDK_DIR}/lib/x64/debug/zlib.lib

    # ${FBX_SDK_DIR}/lib/x64/debug/alembic-md.lib
    # ${FBX_SDK_DIR}/lib/x64/debug/libfbxsdk-md.lib
    # ${FBX_SDK_DIR}/lib/x64/debug/libxml2-md.lib
    # ${FBX_SDK_DIR}/lib/x64/debug/zlib-md.lib

    # ${FBX_SDK_DIR}/lib/x64/debug/alembic-mt.lib
    # ${FBX_SDK_DIR}/lib/x64/debug/libfbxsdk-mt.lib
    # ${FBX_SDK_DIR}/lib/x64/debug/libxml2-mt.lib
    # ${FBX_SDK_DIR}/lib/x64/debug/zlib-mt.lib
)
