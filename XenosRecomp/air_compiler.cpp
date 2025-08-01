#include "air_compiler.h"

#include <fstream>
#include <iterator>
#include <unistd.h>

#ifdef UNLEASHED_RECOMP
#define COMPILER_EXTRA_OPTIONS "-DUNLEASHED_RECOMP"
#elif defined(MARATHON_RECOMP)
#define COMPILER_EXTRA_OPTIONS "-DMARATHON_RECOMP"
#else
#define COMPILER_EXTRA_OPTIONS
#endif

struct TemporaryPath
{
    const std::string path;

    explicit TemporaryPath(std::string_view path) : path(path) {}

    ~TemporaryPath()
    {
        unlink(path.c_str());
    }
};

std::vector<uint8_t> AirCompiler::compile(const std::string& shaderSource)
{
    // Save source to a location on disk for the compiler to read.
    char sourcePathTemplate[PATH_MAX] = "/tmp/xenos_metal_XXXXXX.metal";
    const int sourceFd = mkstemps(sourcePathTemplate, 6);
    if (sourceFd == -1)
    {
        fmt::println("Failed to create temporary file for shader source: {}", strerror(errno));
        std::exit(1);
    }

    const TemporaryPath sourcePath(sourcePathTemplate);
    const TemporaryPath irPath(sourcePath.path + ".ir");
    const TemporaryPath metalLibPath(sourcePath.path + ".metallib");

    const ssize_t sourceWritten = write(sourceFd, shaderSource.data(), shaderSource.size());
    close(sourceFd);
    if (sourceWritten < 0)
    {
        fmt::println("Failed to write shader source to disk: {}", strerror(errno));
        std::exit(1);
    }

    const std::string compileCommand = fmt::format("xcrun -sdk macosx metal " COMPILER_EXTRA_OPTIONS " -D__air__ -Wno-unused-variable -frecord-sources -gline-tables-only -o \"{}\" -c \"{}\"", irPath.path, sourcePath.path);
    if (const int compileStatus = std::system(compileCommand.c_str()); compileStatus != 0)
    {
        fmt::println("Metal compiler exited with status: {}", compileStatus);
        fmt::println("Generated source:\n{}", shaderSource);
        std::exit(1);
    }

    const std::string linkCommand = fmt::format(R"(xcrun -sdk macosx metallib -o "{}" "{}")", metalLibPath.path, irPath.path);
    if (const int linkStatus = std::system(linkCommand.c_str()); linkStatus != 0)
    {
        fmt::println("Metal linker exited with status: {}", linkStatus);
        fmt::println("Generated source:\n{}", shaderSource);
        std::exit(1);
    }

    std::ifstream libStream(metalLibPath.path, std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator(libStream)), std::istreambuf_iterator<char>());
    return data;
}
