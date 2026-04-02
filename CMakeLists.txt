cmake_minimum_required(VERSION 4.2)
project(Hydrogen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "Limited configs" FORCE)

# Output directories
set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/x64)
foreach(CONFIG Debug Release RelWithDebInfo)
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${OUTPUT_DIR}/${CONFIG})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${OUTPUT_DIR}/${CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${OUTPUT_DIR}/${CONFIG})
endforeach()

# Sources
file(GLOB_RECURSE SRC_FILES     src/*.cpp)
file(GLOB_RECURSE INCLUDE_FILES include/*.h)
file(GLOB_RECURSE SHADER_FILES  shaders/*)

add_executable(Hydrogen WIN32 ${SRC_FILES} ${INCLUDE_FILES} ${SHADER_FILES})

# Shaders are just for IDE visibility, not compiled by CMake
set_source_files_properties(${SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_FILES} ${INCLUDE_FILES} ${SHADER_FILES})

# Includes
target_include_directories(Hydrogen PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/thirdParty
)

# vcpkg
set(VCPKG_INSTALLED_DIR "${CMAKE_SOURCE_DIR}/thirdParty/vcpkg_installed/x64-windows")
target_include_directories(Hydrogen PRIVATE ${VCPKG_INSTALLED_DIR}/include)

# Compile definitions
target_compile_definitions(Hydrogen PRIVATE
    UNICODE
    _UNICODE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:_RELEASE;NDEBUG>
    $<$<CONFIG:RelWithDebInfo>:_RELDBGINFO;NDEBUG>
)

# Compile options
target_compile_options(Hydrogen PRIVATE
    /W3
    /permissive-
    /Zc:__cplusplus
    /FC
    $<$<BOOL:${MSVC}>:/MP>
)

# MSVC-specific
if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Hydrogen)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")
endif()

# Dependencies
find_package(directxtk12    CONFIG REQUIRED)
find_package(directxtex     CONFIG REQUIRED)
find_package(directx-dxc    CONFIG REQUIRED)
find_package(directx12-agility CONFIG REQUIRED)
find_package(assimp         CONFIG REQUIRED)
find_package(imgui          CONFIG REQUIRED)
find_package(winpixevent    CONFIG REQUIRED)

target_link_libraries(Hydrogen PRIVATE
    d3d12
    dxgi
    dxguid
    Microsoft::DirectXTK12
    Microsoft::DirectXTex
    Microsoft::DirectXShaderCompiler
    assimp::assimp
    imgui::imgui
    Microsoft::WinPixEventRuntime
	Microsoft::DirectX12-Agility
)

# Agility SDK — copy DLLs to D3D12/ next to the exe, per-config
if(TARGET Microsoft::DirectX12-Agility)
    add_custom_command(TARGET Hydrogen POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Hydrogen>/D3D12"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_PROPERTY:Microsoft::DirectX12-Core,IMPORTED_LOCATION_RELEASE>
            "$<TARGET_FILE_DIR:Hydrogen>/D3D12"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_PROPERTY:Microsoft::DirectX12-Layers,IMPORTED_LOCATION_DEBUG>
            "$<TARGET_FILE_DIR:Hydrogen>/D3D12"
        COMMAND_EXPAND_LISTS
    )
    target_link_libraries(Hydrogen PRIVATE Microsoft::DirectX12-Agility)
endif()

# DXC runtime DLL — copy next to exe
if(TARGET Microsoft::DirectXShaderCompiler)
    add_custom_command(TARGET Hydrogen POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Microsoft::DirectXShaderCompiler>
            "$<TARGET_FILE_DIR:Hydrogen>"
        COMMAND_EXPAND_LISTS
    )
endif()

# Working directory for VS debugger
set_target_properties(Hydrogen PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# clang-tidy
find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
    set_target_properties(Hydrogen PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY_EXE};bugprone-*;cppcoreguidelines-*;clang-analyzer-*;modernize-*;concurrency-*;performance-*;-modernize-use-trailing-return-type;-modernize-use-nodiscard"
    )
endif()