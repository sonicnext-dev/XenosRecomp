#pragma once

#include <string>
#include <vector>
#include <objc/objc.h>

struct AirCompiler
{
    id mtlDevice;

    AirCompiler();
    ~AirCompiler();

    [[nodiscard]] std::vector<uint8_t> compile(const std::string& shaderSource) const;
};
