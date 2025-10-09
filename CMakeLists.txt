cmake_minimum_required(VERSION 3.15)
project(SDM VERSION 1.0.0 LANGUAGES CXX)

# Configuração C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Adiciona SQLiteCpp
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/SQLiteCpp)

# Define a biblioteca do core
add_library(sdm_core STATIC)

# Configura sources do core
target_sources(sdm_core PRIVATE
    src/main.cpp
    # src/core/arquivo2.cpp
    # Adicione todos os arquivos .cpp do core aqui
)

# Configura includes - agora pode usar #include "core/header.h"
target_include_directories(sdm_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

# Dependências
target_link_libraries(sdm_core PUBLIC SQLiteCpp)

# Garante C++11 para todos que usarem sdm_core
target_compile_features(sdm_core PUBLIC cxx_std_11)

# Para usar o core em outros lugares:
# add_executable(meu_app src/main.cpp)
# target_link_libraries(meu_app sdm_core)
