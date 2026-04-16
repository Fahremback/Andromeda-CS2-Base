cmake_minimum_required(VERSION 3.15)

project(AimbotExampleModule VERSION 1.0.0)

# Configurações
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directory para modules/
if(NOT DEFINED OUTPUT_DIR)
    set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/../../modules")
endif()

set(RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

# Arquivos do módulo
set(SOURCES
    aimbot_example.cpp
)

# Headers (para IDE)
set(HEADERS
)

# Criar DLL
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})

# Incluir diretórios do projeto principal
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/../.."
    "${CMAKE_SOURCE_DIR}/../../Andromeda-CS2-Base"
)

# Definir nome de saída
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "aimbot_example"
    RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT_DIR}"
)

# Preprocessor definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    _WINDOWS
    _USRDLL
    UNICODE
    _UNICODE
)

# Compiler options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /WX-
        /permissive-
    )
    
    # Otimizações para Release
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/GL>
    )
endif()

# Link libraries (se necessário)
# target_link_libraries(${PROJECT_NAME} PRIVATE
#     SomeLibrary
# )

# Post-build event (opcional)
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E echo "Module built successfully!"
# )
