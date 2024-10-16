#include "shader.h"
#include "shader_recompiler.h"
#include "dxc_compiler.h"

static std::unique_ptr<uint8_t[]> readAllBytes(const char* filePath, size_t& fileSize)
{
    FILE* file = fopen(filePath, "rb");
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    auto data = std::make_unique<uint8_t[]>(fileSize);
    fread(data.get(), 1, fileSize, file);
    fclose(file);
    return data;
}

static void writeAllBytes(const char* filePath, const void* data, size_t dataSize)
{
    FILE* file = fopen(filePath, "wb");
    fwrite(data, 1, dataSize, file);
    fclose(file);
}

struct ShaderCacheEntry
{
    XXH64_hash_t hash = 0;
    uint32_t dxilOffset = 0;
    uint32_t dxilSize = 0;
    uint32_t spirvOffset = 0;
    uint32_t spirvSize = 0;
    uint64_t reserved = 0;
};

struct ShaderHolder
{
    std::string filePath;
    size_t fileSize = 0;
    std::unique_ptr<uint8_t[]> fileData;
    IDxcBlob* dxil = nullptr;
    IDxcBlob* spirv = nullptr;
    ShaderCacheEntry entry;
};

struct ShaderCacheHeader
{
    uint32_t version = 0;
    uint32_t count = 0;
    uint32_t reserved0 = 0;
    uint32_t reserved1 = 0;
};

int main(int argc, char** argv)
{
    if (std::filesystem::is_directory(argv[1]))
    {
        std::map<XXH64_hash_t, ShaderHolder> shaders;

        for (auto& file : std::filesystem::directory_iterator(argv[1]))
        {
            auto extension = file.path().extension();
            if (extension == ".xpu" || extension == ".xvu")
            {
                ShaderHolder shader;
                shader.filePath = file.path().string();
                shader.fileData = readAllBytes(shader.filePath.c_str(), shader.fileSize);
                shader.entry.hash = XXH3_64bits(shader.fileData.get(), shader.fileSize);

                shaders.emplace(shader.entry.hash, std::move(shader));
            }
        }

        std::for_each(std::execution::par_unseq, shaders.begin(), shaders.end(), [&](auto& hashShaderPair)
            {
                auto& shader = hashShaderPair.second;
                printf("%s\n", shader.filePath.c_str());

                thread_local ShaderRecompiler recompiler;
                recompiler = {};
                recompiler.recompile(shader.fileData.get());
                writeAllBytes((shader.filePath + ".hlsl").c_str(), recompiler.out.data(), recompiler.out.size());

                thread_local DxcCompiler dxcCompiler;

                shader.dxil = dxcCompiler.compile(recompiler.out, recompiler.isPixelShader, false);
                shader.spirv = dxcCompiler.compile(recompiler.out, recompiler.isPixelShader, true);

                assert(shader.dxil != nullptr && shader.spirv != nullptr);
                assert(*(reinterpret_cast<uint32_t*>(shader.dxil->GetBufferPointer()) + 1) != 0 && "DXIL was not signed properly!");
            });

        FILE* file = fopen(argv[2], "wb");

        ShaderCacheHeader header{};
        header.version = 0;
        header.count = shaders.size();
        fwrite(&header, sizeof(header), 1, file);

        uint32_t headerSize = sizeof(ShaderCacheHeader) + shaders.size() * sizeof(ShaderCacheEntry);
        uint32_t dxilOffset = headerSize;
        uint32_t spirvOffset = 0;

        for (auto& [_, shader] : shaders)
        {
            shader.entry.dxilOffset = dxilOffset;
            shader.entry.dxilSize = uint32_t(shader.dxil->GetBufferSize());
            shader.entry.spirvOffset = spirvOffset;
            shader.entry.spirvSize = uint32_t(shader.spirv->GetBufferSize());
            dxilOffset += shader.entry.dxilSize;
            spirvOffset += shader.entry.spirvSize;
        }

        for (auto& [_, shader] : shaders)
        {
            shader.entry.spirvOffset += dxilOffset;
            fwrite(&shader.entry, sizeof(shader.entry), 1, file);
        }

        for (auto& [_, shader] : shaders)
            fwrite(shader.dxil->GetBufferPointer(), 1, shader.dxil->GetBufferSize(), file);

        for (auto& [_, shader] : shaders)
            fwrite(shader.spirv->GetBufferPointer(), 1, shader.spirv->GetBufferSize(), file);

        fclose(file);
    }
    else
    {
    }

    return 0;
}