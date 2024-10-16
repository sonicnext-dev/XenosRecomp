#include "shader.h"
#include "shader_recompiler.h"
#include "dxc_compiler.h"

static std::unique_ptr<uint8_t[]> readAllBytes(const char* filePath)
{
    FILE* file = fopen(filePath, "rb");
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
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

int main(int argc, char** argv)
{
    if (std::filesystem::is_directory(argv[1]))
    {
        std::vector<std::string> filePaths;

        for (auto& file : std::filesystem::directory_iterator(argv[1]))
        {
            auto extension = file.path().extension();
            if (extension == ".xpu" || extension == ".xvu")
                filePaths.push_back(file.path().string());
        }

        std::for_each(std::execution::par_unseq, filePaths.begin(), filePaths.end(), [&](auto& filePath)
            {
                printf("%s\n", filePath.c_str());

                thread_local ShaderRecompiler recompiler;
                recompiler = {};
                recompiler.recompile(readAllBytes(filePath.c_str()).get());
                writeAllBytes((filePath + ".hlsl").c_str(), recompiler.out.data(), recompiler.out.size());

                thread_local DxcCompiler dxcCompiler;

                auto dxil = dxcCompiler.compile(recompiler.out, recompiler.isPixelShader, false);
                auto spirv = dxcCompiler.compile(recompiler.out, recompiler.isPixelShader, true);

                assert(dxil != nullptr && spirv != nullptr);
                assert(*(reinterpret_cast<uint32_t*>(dxil->GetBufferPointer()) + 1) != 0 && "DXIL was not signed properly!");

                std::string outFilePath = argv[2];
                outFilePath += '/';
                outFilePath += filePath.substr(filePath.find_last_of("\\/") + 1);

                FILE* file = fopen(outFilePath.c_str(), "wb");

                struct
                {
                    uint32_t version;
                    uint32_t dxilSize;
                    uint32_t spirvSize;
                } header;

                header.version = 0;
                header.dxilSize = uint32_t(dxil->GetBufferSize());
                header.spirvSize = uint32_t(spirv->GetBufferSize());

                fwrite(&header, sizeof(header), 1, file);
                fwrite(dxil->GetBufferPointer(), 1, dxil->GetBufferSize(), file);
                fwrite(spirv->GetBufferPointer(), 1, spirv->GetBufferSize(), file);

                fclose(file);
            });
    }
    else
    {
    }

    return 0;
}