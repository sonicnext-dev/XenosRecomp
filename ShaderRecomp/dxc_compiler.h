#pragma once

struct DxcCompiler
{
    IDxcCompiler3* dxcCompiler = nullptr;

    DxcCompiler();
    ~DxcCompiler();

    IDxcBlob* compile(const std::string& shaderSource, bool isPixelShader, bool compileSpirv);
};