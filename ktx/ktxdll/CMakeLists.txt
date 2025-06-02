# Copyright 2022-2023 The Khronos Group Inc.
# Copyright 2022-2023 RasterGrid Kft.
# SPDX-License-Identifier: Apache-2.0


add_library(ktxdll
    command.cpp
    command.h
    command_compare.cpp
    command_create.cpp
    command_deflate.cpp
    command_encode.cpp
    command_extract.cpp
    command_help.cpp
    command_info.cpp
    command_transcode.cpp
    command_validate.cpp
    deflate_utils.h
    encode_utils_astc.h
    encode_utils_basis.h
    encode_utils_common.h
    format_descriptor.h
    formats.h
    fragment_uri.h
    ktx_main.cpp
    ktx_main.h
    metrics_utils.h
    transcode_utils.cpp
    transcode_utils.h
    utility.h
    validate.cpp
    validate.h
    validation_messages.h
)

create_version_header(tools/ktxdll ktxdll)

set_target_properties(
    ktxdll
    PROPERTIES
    OUTPUT_NAME ktxdll
    PDB_NAME ktxdll # Avoid conflict with libktx's .pdb.
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)


target_include_directories(
    ktxdll
    PRIVATE
    .
    $<TARGET_PROPERTY:imageio,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
)

target_include_directories(
    ktxdll
    SYSTEM
    PRIVATE
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/ktx/other_include
    "${PROJECT_SOURCE_DIR}/external"
)

target_link_libraries(
    ktxdll
    PRIVATE
    imageio
    objUtil
    ktx
    ${ASTCENC_LIB_TARGET}
    $<IF:$<BOOL:${WIN32}>,Pathcch,> # For PathCchRemoveFileSpec on Windows
    fmt::fmt
    cxxopts::cxxopts
)

target_compile_definitions(
    ktxdll
    PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

target_compile_features(
    ktxdll
    PUBLIC
    cxx_std_17
)

if(KTX_DEVELOPER_FEATURE_PATCH)
    target_sources(
        ktxdll
        PRIVATE
        command_patch.cpp
    )
    target_compile_definitions(
        ktxdll
        PRIVATE
        KTX_DEVELOPER_FEATURE_PATCH=1
    )
endif()

set_tool_properties(ktxdll)
set_code_sign(ktxdll)
