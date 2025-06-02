cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

# set(CMAKE_GENERATOR "Ninja" CACHE STRING "Ninja generator")
# set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type")

project(VCProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# vcpkg 설정 (OpenImageIO)
find_package(OpenImageIO CONFIG REQUIRED) #위의 fmt가 필요
# add_subdirectory(thirdparty/OpenImageIO-3.0.6.1) #위의 fmt가 필요
find_package(draco CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)


add_library(vcpp_core SHARED
    core/core.cpp
    core/core.h
)

target_include_directories(vcpp_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
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

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(FBX_LIB_DIR ${FBX_SDK_DIR}/lib/x64/debug)
    set(FBX_LIBS
        ${FBX_LIB_DIR}/alembic.lib
        ${FBX_LIB_DIR}/libfbxsdk.lib
        ${FBX_LIB_DIR}/libxml2.lib
        ${FBX_LIB_DIR}/zlib.lib
    )

    set(OIDN_LIB_DIR ${OIDN_DIR}/lib/Debug)
    set(OIDN_LIBS
        OpenImageIO::OpenImageIO
        ${OIDN_LIB_DIR}/curtn.lib
        ${OIDN_LIB_DIR}/OpenImageDenoise_core.lib
        ${OIDN_LIB_DIR}/OpenImageDenoise.lib
        ${OIDN_LIB_DIR}/OpenImageDenoise_device_cpu.lib
        ${OIDN_LIB_DIR}/OpenImageDenoise_device_cuda.lib
    )

    set(TBB_LIB_DIR $ENV{VCPKG_ROOT}/installed/x64-windows/debug/lib)
    set(TBB_LIBS
        ${TBB_LIB_DIR}/tbb12_debug.lib
        ${TBB_LIB_DIR}/tbbmalloc_debug.lib
        ${TBB_LIB_DIR}/tbbmalloc_proxy_debug.lib
    )

    set(KTX_LIB_DIR ${KTX_DIR}/lib/Debug)
    set(KTX_LIBS
        Pathcch

        ${KTX_LIB_DIR}/ktxdll.lib
        ${KTX_LIB_DIR}/ktx.lib
        ${KTX_LIB_DIR}/ktx_read.lib
        ${KTX_LIB_DIR}/objUtil.lib
        ${KTX_LIB_DIR}/imageio.lib
        ${KTX_LIB_DIR}/astcenc-avx2-static.lib
    )

else()
    set(FBX_LIB_DIR ${FBX_SDK_DIR}/lib/x64/release)
    set(FBX_LIBS
        ${FBX_LIB_DIR}/alembic.lib
        ${FBX_LIB_DIR}/libfbxsdk.lib
        ${FBX_LIB_DIR}/libxml2.lib
        ${FBX_LIB_DIR}/zlib.lib
    )

    set(OIDN_LIB_DIR ${OIDN_DIR}/lib/Release)
    set(OIDN_LIBS
        OpenImageIO::OpenImageIO
        ${OIDN_LIB_DIR}/curtn.lib
        ${OIDN_LIB_DIR}/OpenImageDenoise_core.lib
        ${OIDN_LIB_DIR}/OpenImageDenoise.lib
        ${OIDN_LIB_DIR}/OpenImageDenoise_device_cpu.lib
        ${OIDN_LIB_DIR}/OpenImageDenoise_device_cuda.lib
        # ${OIDN_LIB_DIR}/OpenEXR-3_3.lib
        # ${OIDN_LIB_DIR}/OpenEXRCore-3_3.lib
        # ${OIDN_LIB_DIR}/OpenEXRUtil-3_3.lib
    )

    set(TBB_LIB_DIR $ENV{VCPKG_ROOT}/installed/x64-windows/lib)
    set(TBB_LIBS
        ${TBB_LIB_DIR}/tbb12.lib
        ${TBB_LIB_DIR}/tbbmalloc.lib
        ${TBB_LIB_DIR}/tbbmalloc_proxy.lib
    )

    set(KTX_LIB_DIR ${KTX_DIR}/lib/Release)
    set(KTX_LIBS
        Pathcch

        ${KTX_LIB_DIR}/ktxdll.lib
        ${KTX_LIB_DIR}/ktx.lib
        ${KTX_LIB_DIR}/ktx_read.lib
        ${KTX_LIB_DIR}/objUtil.lib
        ${KTX_LIB_DIR}/imageio.lib
        ${KTX_LIB_DIR}/astcenc-avx2-static.lib
    )

endif()

# message TBB_LIBS
message(STATUS "TBB_LIBS: ${TBB_LIBS}")

# oidn 및 OpenImageIO 링크
target_link_libraries(vcpp_core PRIVATE
    nlohmann_json::nlohmann_json
    draco::draco
    fmt::fmt

    # ${TBB_LIBS}
    # tbb
    TBB::tbb
    ${OIDN_LIBS}
    ${KTX_LIBS}
    ${FBX_LIBS}
)

#############################################################
## test program



add_executable(vcpp main.cpp)

target_link_libraries(vcpp PRIVATE vcpp_core)
target_include_directories(vcpp PRIVATE ${OIDN_DIR}/include)
target_link_libraries(vcpp PRIVATE
    nlohmann_json::nlohmann_json
    draco::draco
    fmt::fmt
    TBB::tbb

    # ${TBB_LIBS}
    ${OIDN_LIBS}
    ${KTX_LIBS}
    ${FBX_LIBS}
)


#############################################################
## DLL 복사
set(THIRDPARTY_DLLS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/dlls")
set(OUTPUT_DLL_DIR "$<TARGET_FILE_DIR:vcpp_core>")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    file(GLOB DLL_FILES "${THIRDPARTY_DLLS_DIR}/Debug/*.dll")
else()
    file(GLOB DLL_FILES "${THIRDPARTY_DLLS_DIR}/Release/*.dll")
endif()

foreach(DLL_FILE IN LISTS DLL_FILES)
    add_custom_command(TARGET vcpp_core POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DLL_FILE}" "${OUTPUT_DLL_DIR}"
        COMMENT "Copying: ${DLL_FILE} -> ${OUTPUT_DLL_DIR}"
        VERBATIM
    )
endforeach()

# CUDA nvrtc DLL 복사
add_custom_command(TARGET vcpp_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.9/bin/nvrtc64_120_0.dll"
    $<TARGET_FILE_DIR:vcpp_core>
    COMMENT "Copying CUDA DLL for nvrtc to output directory"
)
