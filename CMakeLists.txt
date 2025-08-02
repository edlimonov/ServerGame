cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Создаем статическую библиотеку с основной логикой
add_library(game_lib STATIC
    src/model.cpp
    src/model.h
    src/tagged.h
    src/loot_generator.cpp
    src/loot_generator.h
    src/extra_data.cpp
    src/extra_data.h
    src/collision_detector.cpp
    src/collision_detector.h
    src/geom.h
    src/model_serialization.h
    src/connection_pool.h
    src/postgres.cpp
    src/postgres.h
)

# Подключаем зависимости к библиотеке
target_include_directories(game_lib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    CONAN_PKG::boost
    CONAN_PKG::libpqxx
)
target_link_libraries(game_lib PUBLIC
    CONAN_PKG::boost
    CONAN_PKG::libpqxx
)

# Основной исполняемый файл
add_executable(game_server
    # Файлы, связанные с HTTP и инфраструктурой
    src/main.cpp
    src/http_server.cpp
    src/http_server.h
    src/sdk.h
    src/boost_json.cpp
    src/json_loader.h
    src/json_loader.cpp
    src/request_handler.cpp
    src/request_handler.h
    src/log.h
    src/log.cpp
    src/logging_request_handler.h
    src/api_handler.cpp
    src/api_handler.h
    src/file_handler.cpp
    src/file_handler.h
    src/application.cpp
    src/application.h
    src/common_handler.h
    src/common_handler.cpp
    src/ticker.h
)

# Подключаем библиотеку и зависимости к исполняемому файлу
target_link_libraries(game_server
    game_lib
    Threads::Threads
    CONAN_PKG::boost
    CONAN_PKG::libpqxx
)
