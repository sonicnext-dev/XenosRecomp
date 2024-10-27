project(ShaderRecomp)

include(bin2h.cmake)
bin2h(SOURCE_FILE shader_common.hlsli HEADER_FILE shader_common.hlsli.h VARIABLE_NAME SHADER_COMMON_HLSLI)

add_executable(ShaderRecomp 
    constant_table.h
    dxc_compiler.cpp
    dxc_compiler.h
    main.cpp
    pch.h
    shader.h
    shader_code.h
    shader_common.hlsli.h
    shader_recompiler.cpp
    shader_recompiler.h)

find_package(directx-dxc CONFIG REQUIRED)
find_package(xxhash CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)

target_link_libraries(ShaderRecomp PRIVATE
    Microsoft::DirectXShaderCompiler
    xxHash::xxhash
    $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static>)

target_precompile_headers(ShaderRecomp PRIVATE pch.h)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(ShaderRecomp PRIVATE -Wno-switch -Wno-unused-variable)
endif()

target_compile_definitions(ShaderRecomp PRIVATE _CRT_SECURE_NO_WARNINGS)

file(COPY ${PACKAGE_PREFIX_DIR}/bin/dxil.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
