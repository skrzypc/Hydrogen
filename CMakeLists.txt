cmake_minimum_required(VERSION 4.2)

project(Hydrogen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "Limited configs" FORCE)

set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/x64)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${OUTPUT_DIR}/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR}/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_DIR}/RelWithDebInfo)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${OUTPUT_DIR}/Debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR}/Release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_DIR}/RelWithDebInfo)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${OUTPUT_DIR}/Debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR}/Release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_DIR}/RelWithDebInfo)

file(GLOB_RECURSE SRC_FILES src/*.cpp)
file(GLOB_RECURSE INCLUDE_FILES include/*.h)
file(GLOB_RECURSE SHADER_FILES shaders/*)

add_executable(Hydrogen WIN32 ${SRC_FILES} ${INCLUDE_FILES} ${SHADER_FILES})

if(SHADER_FILES)
    target_sources(Hydrogen PRIVATE ${SHADER_FILES})
    set_source_files_properties(${SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_FILES} ${INCLUDE_FILES} ${SHADER_FILES})

target_include_directories(Hydrogen PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/thirdParty
)

set(VCPKG_INSTALLED_DIR "${CMAKE_SOURCE_DIR}/thirdParty/vcpkg_installed/x64-windows")
target_include_directories(Hydrogen PRIVATE ${VCPKG_INSTALLED_DIR}/include)

target_compile_definitions(Hydrogen PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:_RELEASE;_NDEBUG>
    $<$<CONFIG:RelWithDebInfo>:_RELDBGINFO;_NDEBUG>
)

target_compile_options(Hydrogen PRIVATE /W3 /permissive- /Zc:__cplusplus /FC)

if(MSVC)
    target_compile_options(Hydrogen PRIVATE /MP)
    target_compile_definitions(Hydrogen PRIVATE UNICODE _UNICODE)

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Hydrogen)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")
endif()

find_package(directxtk12 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

target_link_libraries(Hydrogen PRIVATE
    d3d12
    dxgi
    d3dcompiler
    dxguid
    dxcompiler
    Microsoft::DirectXTK12
    assimp::assimp
	imgui::imgui
    ${VCPKG_INSTALLED_DIR}/lib/dxcompiler.lib
    ${VCPKG_INSTALLED_DIR}/lib/DirectXTex.lib
)

set_target_properties(Hydrogen PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
    set_target_properties(Hydrogen PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY_EXE};bugprone-*;cppcoreguidelines-*;clang-analyzer-*;modernize-*;concurrency-*;performance-*;-modernize-use-trailing-return-type;-modernize-use-nodiscard"
    )
endif()
