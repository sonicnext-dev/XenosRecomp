#include "air_compiler.h"

#import <Metal/Metal.h>

AirCompiler::AirCompiler()
{
    mtlDevice = MTLCreateSystemDefaultDevice();
    if (@available(macOS 13.3, *))
    {
        static_cast<id<MTLDevice>>(mtlDevice).shouldMaximizeConcurrentCompilation = true;
    }
}

AirCompiler::~AirCompiler()
{
    [mtlDevice release];
}

std::vector<uint8_t> AirCompiler::compile(const std::string& shaderSource) const
{
    @autoreleasepool {
        NSString *sourceStr = [NSString stringWithCString:shaderSource.c_str() encoding:[NSString defaultCStringEncoding]];
        MTLCompileOptions *options = [MTLCompileOptions new];
        options.libraryType = MTLLibraryTypeExecutable;
        options.preprocessorMacros = @{
            @"__air__" : @"1"
        };

        NSError *error;
        id<MTLLibrary> lib = [mtlDevice newLibraryWithSource:sourceStr options:options error:&error];
        if (!lib && error)
        {
            fmt::println("Failed to create MTLLibrary: {}", [[error localizedDescription] UTF8String]);
            exit(1);
        }

        if (![lib respondsToSelector: @selector(serializeToURL:error:)])
        {
            fmt::println("MTLLibrary does not support serializeToURL");
            exit(1);
        }

        NSFileManager *fileManager = [NSFileManager defaultManager];
        NSURL *url = [[fileManager temporaryDirectory] URLByAppendingPathComponent:@"xenos_metal_lib.metallib"];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-method-access"
        [lib serializeToURL:url error:&error];
#pragma clang diagnostic pop
        if (error)
        {
            fmt::println("Failed to serialize MTLLibrary: {}", [[error localizedDescription] UTF8String]);
            exit(1);
        }

        NSData *data = [NSData dataWithContentsOfURL:url options:0 error:&error];
        if (error)
        {
            fmt::println("Failed to read MTLLibrary data: {}", [[error localizedDescription] UTF8String]);
            exit(1);
        }

        [fileManager removeItemAtURL:url error:&error];

        const auto* bytes = static_cast<const uint8_t*>([data bytes]);
        size_t len = [data length];
        return {bytes, bytes + len};
    }
}