cmake_minimum_required(VERSION 3.20)
project(VCProject LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Toolchain (vcpkg)
set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")



# Dependencies (vcpkg)
find_package(OpenImageIO CONFIG REQUIRED)
find_package(draco CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

# External: FBX SDK
set(FBX_SDK_DIR "C:/Program Files/Autodesk/FBX/FBX SDK/2020.3.7")

# External: OIDN
set(OIDN_DIR "${CMAKE_SOURCE_DIR}/thirdparty/oidn")

# External: KTX
set(KTX_DIR "${CMAKE_SOURCE_DIR}/thirdparty/ktx")
add_library(ktx STATIC IMPORTED GLOBAL)
set_target_properties(ktx PROPERTIES
    IMPORTED_CONFIGURATIONS "Debug;Release"
    IMPORTED_LOCATION_DEBUG "${KTX_DIR}/lib/Debug/ktx.lib"
    IMPORTED_LOCATION_RELEASE "${KTX_DIR}/lib/Release/ktx.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${KTX_DIR}/include"
)
add_subdirectory(external/cxxopts) # KTXDLL을 위함
add_subdirectory(ktx) # KTXDLL을 위함

add_library(vcpp-core SHARED
    core/core.cpp
    core/core.h
)

target_include_directories(vcpp-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(vcpp-core PRIVATE
    ${OIDN_DIR}/include
    SYSTEM ${FBX_SDK_DIR}/include
)

# FBX libraries
set(FBX_LIB_DIR ${FBX_SDK_DIR}/lib/x64)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(FBX_LIB_DIR "${FBX_LIB_DIR}/debug")
else()
    set(FBX_LIB_DIR "${FBX_LIB_DIR}/release")
endif()

set(FBX_LIBS
    ${FBX_LIB_DIR}/alembic.lib
    ${FBX_LIB_DIR}/libfbxsdk.lib
    ${FBX_LIB_DIR}/libxml2.lib
    ${FBX_LIB_DIR}/zlib.lib
)

# OIDN libraries
set(OIDN_LIB_DIR ${OIDN_DIR}/lib/${CMAKE_BUILD_TYPE})
set(OIDN_LIBS
    OpenImageIO::OpenImageIO
    ${OIDN_LIB_DIR}/curtn.lib
    ${OIDN_LIB_DIR}/OpenImageDenoise_core.lib
    ${OIDN_LIB_DIR}/OpenImageDenoise.lib
    ${OIDN_LIB_DIR}/OpenImageDenoise_device_cpu.lib
    ${OIDN_LIB_DIR}/OpenImageDenoise_device_cuda.lib
)

# Additional KTX components (manually link others)
set(KTX_EXTRA_LIBS
    # ${KTX_DIR}/lib/${CMAKE_BUILD_TYPE}/ktxdll.lib
    # ${KTX_DIR}/lib/${CMAKE_BUILD_TYPE}/ktx_read.lib
    # ${KTX_DIR}/lib/${CMAKE_BUILD_TYPE}/objUtil.lib
    # ${KTX_DIR}/lib/${CMAKE_BUILD_TYPE}/imageio.lib
    ${KTX_DIR}/lib/${CMAKE_BUILD_TYPE}/astcenc-avx2-static.lib
    Pathcch
)

# Link libraries
set(COMMON_LIBS
    nlohmann_json::nlohmann_json
    draco::draco
    fmt::fmt
    TBB::tbb
    ktx
    ktxdll
    ${KTX_EXTRA_LIBS}
    ${FBX_LIBS}
    ${OIDN_LIBS}
)

target_link_libraries(vcpp-core PRIVATE ${COMMON_LIBS})

add_executable(vcpp-test test/vcpp-test.cpp)
target_include_directories(vcpp-test PRIVATE ${OIDN_DIR}/include)
target_link_libraries(vcpp-test PRIVATE vcpp-core ${COMMON_LIBS})

# DLL Copy
set(THIRDPARTY_DLLS_DIR "${CMAKE_SOURCE_DIR}/thirdparty/dlls")
set(OUTPUT_DLL_DIR "$<TARGET_FILE_DIR:vcpp-core>")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    file(GLOB DLL_FILES "${THIRDPARTY_DLLS_DIR}/Debug/*.dll")
else()
    file(GLOB DLL_FILES "${THIRDPARTY_DLLS_DIR}/Release/*.dll")
endif()

foreach(DLL_FILE IN LISTS DLL_FILES)
    add_custom_command(TARGET vcpp-core POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DLL_FILE}" "${OUTPUT_DLL_DIR}"
        COMMENT "Copying: ${DLL_FILE} -> ${OUTPUT_DLL_DIR}"
        VERBATIM
    )
endforeach()

add_custom_command(TARGET vcpp-core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.9/bin/nvrtc64_120_0.dll"
    $<TARGET_FILE_DIR:vcpp-core>
    COMMENT "Copying CUDA DLL for nvrtc to output directory"
)

# suppress deprecated <codecvt> warnings for test target
target_compile_definitions(vcpp-test PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

# suppress nontrivial memcpy/memset warnings for FBX SDK
target_compile_options(vcpp-core PRIVATE
    -Wno-nontrivial-memcall
    -Wno-deprecated-builtins
)

# suppress missing enum case warnings in ktxdll
target_compile_options(ktxdll PRIVATE -Wno-switch)
