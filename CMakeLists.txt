cmake_minimum_required(VERSION 4.2)
project(Hydrogen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "Limited configs" FORCE)

set(HYDROGEN_ANALYSIS_PROPS "${CMAKE_BINARY_DIR}/HydrogenAnalysis.props")
file(GENERATE
    OUTPUT "${HYDROGEN_ANALYSIS_PROPS}"
    CONTENT [=[
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <EnableClangTidyCodeAnalysis>true</EnableClangTidyCodeAnalysis>
    <EnableMicrosoftCodeAnalysis>false</EnableMicrosoftCodeAnalysis>
    <RunCodeAnalysisOnThisProject>false</RunCodeAnalysisOnThisProject>
  </PropertyGroup>
</Project>
]=]
)

set(OUTPUT_ROOT "${CMAKE_BINARY_DIR}")
foreach(CONFIG Debug Release RelWithDebInfo)
    string(TOUPPER "${CONFIG}" CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_ROOT}/bin/${CONFIG}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_ROOT}/lib/${CONFIG}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_ROOT}/lib/${CONFIG}")
endforeach()

add_library(HydrogenBuildOptions INTERFACE)
target_compile_features(HydrogenBuildOptions INTERFACE cxx_std_23)
target_compile_definitions(HydrogenBuildOptions INTERFACE
    UNICODE
    _UNICODE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:_RELEASE;NDEBUG>
    $<$<CONFIG:RelWithDebInfo>:_RELDBGINFO;NDEBUG>
)
target_compile_options(HydrogenBuildOptions INTERFACE
    /W3
    /permissive-
    /Zc:__cplusplus
    $<$<CONFIG:Debug>:/Z7>
    /FC
    $<$<BOOL:${MSVC}>:/MP>
)

file(GLOB_RECURSE HYDROGEN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp"
)
file(GLOB_RECURSE HYDROGEN_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)
file(GLOB_RECURSE HYDROGEN_SHADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*"
)

add_library(Hydrogen STATIC
    ${HYDROGEN_SOURCES}
    ${HYDROGEN_HEADERS}
    ${HYDROGEN_SHADERS}
)

set_source_files_properties(${HYDROGEN_SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${HYDROGEN_SOURCES}
    ${HYDROGEN_HEADERS}
    ${HYDROGEN_SHADERS}
)

find_package(directxtk12 CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)
find_package(directx-dxc CONFIG REQUIRED)
find_package(directx12-agility CONFIG REQUIRED)
find_package(fastgltf CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(winpixevent CONFIG REQUIRED)

target_link_libraries(Hydrogen
    PUBLIC
        HydrogenBuildOptions
        d3d12
        dxgi
        dxguid
        Microsoft::DirectXTK12
        Microsoft::DirectXTex
        Microsoft::DirectXShaderCompiler
        fastgltf::fastgltf
        imgui::imgui
        Microsoft::WinPixEventRuntime
        Microsoft::DirectX12-Agility
)

target_include_directories(Hydrogen
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/thirdParty"
        "${CMAKE_SOURCE_DIR}/thirdParty/vcpkg_installed/x64-windows/include"
)

add_subdirectory(apps/DevelopmentApp)

if(MSVC)
    set_target_properties(Hydrogen PROPERTIES
        VS_PROJECT_IMPORT
            "${HYDROGEN_ANALYSIS_PROPS}"
    )
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT DevelopmentApp)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")
endif()
