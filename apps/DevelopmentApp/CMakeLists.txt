file(GLOB_RECURSE DEV_APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
)
file(GLOB_RECURSE DEV_APP_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

add_executable(DevelopmentApp WIN32
    ${DEV_APP_SOURCES}
    ${DEV_APP_HEADERS}
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${DEV_APP_SOURCES}
    ${DEV_APP_HEADERS}
)

target_link_libraries(DevelopmentApp PRIVATE Hydrogen)
target_compile_features(DevelopmentApp PRIVATE cxx_std_23)

target_include_directories(DevelopmentApp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_target_properties(DevelopmentApp PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

if(MSVC)
    set_target_properties(DevelopmentApp PROPERTIES
        VS_PROJECT_IMPORT
            "${HYDROGEN_ANALYSIS_PROPS}"
    )
endif()

foreach(CONFIG Debug Release RelWithDebInfo)
    string(TOUPPER "${CONFIG}" CONFIG_UPPER)
    set_target_properties(DevelopmentApp PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER}
            "${CMAKE_BINARY_DIR}/bin/${CONFIG}/DevelopmentApp"
    )
endforeach()

if(TARGET Microsoft::DirectX12-Agility)
    add_custom_command(TARGET DevelopmentApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:DevelopmentApp>/D3D12"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_PROPERTY:Microsoft::DirectX12-Core,IMPORTED_LOCATION_RELEASE>
            "$<TARGET_FILE_DIR:DevelopmentApp>/D3D12"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_PROPERTY:Microsoft::DirectX12-Layers,IMPORTED_LOCATION_DEBUG>
            "$<TARGET_FILE_DIR:DevelopmentApp>/D3D12"
        COMMAND_EXPAND_LISTS
    )
endif()

if(TARGET Microsoft::DirectXShaderCompiler)
    add_custom_command(TARGET DevelopmentApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Microsoft::DirectXShaderCompiler>
            "$<TARGET_FILE_DIR:DevelopmentApp>"
        COMMAND_EXPAND_LISTS
    )
endif()
