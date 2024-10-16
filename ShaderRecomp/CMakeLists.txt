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
target_link_libraries(ShaderRecomp PRIVATE Microsoft::DirectXShaderCompiler)

target_precompile_headers(ShaderRecomp PRIVATE pch.h)

add_compile_definitions(ShaderRecomp _CRT_SECURE_NO_WARNINGS)