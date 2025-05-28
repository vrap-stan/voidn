cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

# set(CMAKE_GENERATOR "Ninja" CACHE STRING "Ninja generator")
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type")

project(VCProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# vcpkg 설정 (OpenImageIO)
find_package(OpenImageIO CONFIG REQUIRED) #위의 fmt가 필요
# add_subdirectory(thirdparty/OpenImageIO-3.0.6.1) #위의 fmt가 필요
find_package(draco CONFIG REQUIRED)
find_package(tbb CONFIG REQUIRED)


add_library(vcpp_core SHARED
    core/core.cpp
    core/core.h
)

# if debug, "$ENV{VCPKG_ROOT}/x64-windows/debug/lib", else "$ENV{VCPKG_ROOT}/installed/x64-windows/lib"

# target_link_directories(vcpp_core PRIVATE
#     $ENV{VCPKG_ROOT}/installed/x64-windows/lib
# )

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_directories(vcpp_core PRIVATE
        $ENV{VCPKG_ROOT}/installed/x64-windows/debug/lib
    )
else()
    target_link_directories(vcpp_core PRIVATE
        $ENV{VCPKG_ROOT}/installed/x64-windows/lib
    )
endif()

# OIDN 설정
set(OIDN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/oidn")
target_include_directories(vcpp_core PRIVATE ${OIDN_DIR}/include)

#Autodesk FBX SDK
set(FBX_SDK_DIR "C:/Program Files/Autodesk/FBX/FBX SDK/2020.3.7")
target_include_directories(vcpp_core SYSTEM PRIVATE ${FBX_SDK_DIR}/include)
# link_directories(${FBX_SDK_DIR}/lib/x64/debug)

# ktx
set(KTX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/ktx")
target_include_directories(vcpp_core PRIVATE ${KTX_DIR}/include)

# ktx fmt, openimageio는 fmt::v11을 쓰지만 ktx는 fmt::v10을 사용함
# add_subdirectory(thirdparty/fmt)
# find_package(fmt CONFIG REQUIRED)

#ofbx
# find_package(libdeflate CONFIG REQUIRED)
# find_package(unofficial-openfbx CONFIG REQUIRED)
# # set(OFBX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/openfbx")
# # target_include_directories(vcpp_core ${OFBX_DIR}/include)
# # link_directories(${OFBX_DIR}/lib)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(FBX_LIB_DIR ${FBX_SDK_DIR}/lib/x64/debug)
    set(KTX_LIB_DIR ${KTX_DIR}/lib/Debug)
    set(OIDN_LIB_DIR ${OIDN_DIR}/lib/Debug)

    set(TBB_LIBS
        tbb12_debug.lib
        tbbmalloc_debug.lib
        tbbmalloc_proxy_debug.lib
    )

else()
    set(FBX_LIB_DIR ${FBX_SDK_DIR}/lib/x64/release)
    set(KTX_LIB_DIR ${KTX_DIR}/lib/Release)
    set(OIDN_LIB_DIR ${OIDN_DIR}/lib/Release)

    set(TBB_LIBS
        tbb12.lib
        tbbmalloc.lib
        tbbmalloc_proxy.lib
    )
endif()

# oidn 및 OpenImageIO 링크
target_link_libraries(vcpp_core PRIVATE
    draco::draco
    fmt::fmt
    ${TBB_LIBS}
    # libdeflate
    # openfbx
    OpenImageIO::OpenImageIO
    ${OIDN_LIB_DIR}/OpenImageDenoise_core.lib
    ${OIDN_LIB_DIR}/OpenImageDenoise.lib
    ${OIDN_LIB_DIR}/OpenImageDenoise_device_cpu.lib
    ${OIDN_LIB_DIR}/OpenImageDenoise_device_cuda.lib

    #KTX
    Pathcch
    # ${KTX_DIR}/lib/Debug/ktx.lib
    # ${KTX_DIR}/lib/Debug/ktx_read.lib
    # ${KTX_DIR}/lib/Debug/objUtil.lib
    # ${KTX_DIR}/lib/Debug/ktxdll.lib
    # ${KTX_DIR}/lib/Debug/fmtdv10.lib
    # ${KTX_DIR}/lib/Debug/astcenc-avx2-static.lib
    # ${KTX_DIR}/lib/Debug/imageio.lib

    # ${KTX_DIR}/lib/Release/ktx.lib
    # ${KTX_DIR}/lib/Release/ktx_read.lib
    # ${KTX_DIR}/lib/Release/objUtil.lib
    # ${KTX_DIR}/lib/Release/ktxdll.lib
    # ${KTX_DIR}/lib/Release/fmtv10.lib
    # ${KTX_DIR}/lib/Release/astcenc-avx2-static.lib
    # ${KTX_DIR}/lib/Release/imageio.lib

    ${KTX_LIB_DIR}/ktxdll.lib
    ${KTX_LIB_DIR}/ktx.lib
    ${KTX_LIB_DIR}/ktx_read.lib
    ${KTX_LIB_DIR}/objUtil.lib
    ${KTX_LIB_DIR}/ktxdll.lib
    ${KTX_LIB_DIR}/fmtv10.lib
    ${KTX_LIB_DIR}/astcenc-avx2-static.lib
    ${KTX_LIB_DIR}/imageio.lib

    # Autodesk FBX
    # ${FBX_SDK_DIR}/lib/x64/debug/alembic.lib
    # ${FBX_SDK_DIR}/lib/x64/debug/libfbxsdk.lib
    # ${FBX_SDK_DIR}/lib/x64/debug/libxml2.lib
    # ${FBX_SDK_DIR}/lib/x64/debug/zlib.lib

    # ${FBX_SDK_DIR}/lib/x64/debug/alembic-md.lib
    # ${FBX_SDK_DIR}/lib/x64/debug/libfbxsdk-md.lib
    # ${FBX_SDK_DIR}/lib/x64/debug/libxml2-md.lib
    # ${FBX_SDK_DIR}/lib/x64/debug/zlib-md.lib

    # ${FBX_SDK_DIR}/lib/x64/debug/alembic-mt.lib
    # ${FBX_SDK_DIR}/lib/x64/debug/libfbxsdk-mt.lib
    # ${FBX_SDK_DIR}/lib/x64/debug/libxml2-mt.lib
    # ${FBX_SDK_DIR}/lib/x64/debug/zlib-mt.lib

    # ${FBX_SDK_DIR}/lib/x64/release/alembic.lib
    # ${FBX_SDK_DIR}/lib/x64/release/libfbxsdk.lib
    # ${FBX_SDK_DIR}/lib/x64/release/libxml2.lib
    # ${FBX_SDK_DIR}/lib/x64/release/zlib.lib

    #${FBX_SDK_DIR}/lib/x64/release/alembic.lib
    #${FBX_SDK_DIR}/lib/x64/release/libfbxsdk.lib
    #${FBX_SDK_DIR}/lib/x64/release/libxml2.lib
    #${FBX_SDK_DIR}/lib/x64/release/zlib.lib

    ${FBX_LIB_DIR}/alembic.lib
    ${FBX_LIB_DIR}/libfbxsdk.lib
    ${FBX_LIB_DIR}/libxml2.lib
    ${FBX_LIB_DIR}/zlib.lib
)

#############################################################
## test program

add_executable(vcpp main.cpp)

target_link_libraries(vcpp PRIVATE vcpp_core)

target_include_directories(vcpp_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(vcpp PRIVATE vcpp_core)
