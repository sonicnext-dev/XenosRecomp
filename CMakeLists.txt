cmake_minimum_required(VERSION 3.20)

include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_CXX_STANDARD 17)

project(ShaderRecomp-ALL)

add_subdirectory(ShaderRecomp)
