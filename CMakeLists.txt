cmake_minimum_required(VERSION 3.15)
project(ApiTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Указываем CMake, что папка include содержит наши переиспользуемые классы

# -----------------------------------------------------------------------------
# 1. Скачиваем Google Test (GTest) — Ссылка исправлена на ZIP-архив
# -----------------------------------------------------------------------------
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
FetchContent_MakeAvailable(googletest)

# -----------------------------------------------------------------------------
# 2. Настройка подмодулей перед загрузкой gRPC
# -----------------------------------------------------------------------------
# Отключаем встроенные тесты для ускорения сборки
set(protobuf_BUILD_TESTS OFF CACHE INTERNAL "")
set(gRPC_BUILD_TESTS OFF CACHE INTERNAL "")
set(gRPC_BUILD_BLOATY OFF CACHE INTERNAL "")

# ВАЖНО: Отключаем правила создания установочных файлов (решает проблему с export set)
set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
set(gRPC_INSTALL OFF CACHE BOOL "" FORCE)
set(utf8_range_ENABLE_INSTALL OFF CACHE BOOL "" FORCE) 

# Принудительно заставляем gRPC собирать зависимости из своих папок
set(gRPC_PROTOBUF_PROVIDER "module" CACHE INTERNAL "")
set(gRPC_ZLIB_PROVIDER "module" CACHE INTERNAL "")
set(gRPC_CARES_PROVIDER "module" CACHE INTERNAL "")
set(gRPC_RE2_PROVIDER "module" CACHE INTERNAL "")
set(gRPC_SSL_PROVIDER "module" CACHE INTERNAL "")

# -----------------------------------------------------------------------------
# 3. Скачиваем gRPC и Protobuf — Ссылка исправлена на ZIP-архив
# -----------------------------------------------------------------------------
FetchContent_Declare(
  grpc
  GIT_REPOSITORY https://github.com/grpc/grpc
  GIT_TAG        v1.83.0
  GIT_SHALLOW    ON
)
set(FETCHCONTENT_QUIET OFF)
# Внимание: Эта команда будет выполняться долго при первой конфигурации!
FetchContent_MakeAvailable(grpc)

# -----------------------------------------------------------------------------
# 4. НАСТРОЙКА АВТОМАТИЧЕСКОЙ ГЕНЕРАЦИИ ФАЙЛОВ ИЗ .PROTO (Перенесено наверх)
# -----------------------------------------------------------------------------
# Путь к вашему proto-файлу относительно корня проекта
set(PROTO_FILE "proto/service.proto")

# Создаем папку, куда CMake сгенерирует .cc и .h файлы
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})

# Описываем правила компиляции .proto файла с помощью protoc
add_custom_command(
    OUTPUT "${GENERATED_DIR}/service.pb.cc" "${GENERATED_DIR}/service.grpc.pb.cc"
    COMMAND protobuf::protoc
    ARGS --cpp_out=${GENERATED_DIR} --grpc_out=${GENERATED_DIR}
         --plugin=protoc-gen-grpc=$<TARGET_FILE:grpc_cpp_plugin>
         -I ${CMAKE_CURRENT_SOURCE_DIR}/proto
         ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE}
    DEPENDS ${PROTO_FILE} protobuf::protoc grpc_cpp_plugin
)

set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
# =============================================================================
# 5. Создание исполняемых файлов (Раздельные тесты)
# =============================================================================

# -----------------------------------------------------------------------------
# ТЕСТ 1: Проверка утечек памяти (Будет собран файл grpc_memory_tests.exe)
# -----------------------------------------------------------------------------
add_executable(grpc_memory_tests "tests/grpc_memory_leaks_test.cpp")

# Подключаем к первому тесту сгенерированные файлы кодогенерации
target_sources(grpc_memory_tests PRIVATE 
    "${GENERATED_DIR}/service.pb.cc" 
    "${GENERATED_DIR}/service.grpc.pb.cc"
)
# Открываем для первого теста доступ к заголовочным файлам из папки generated
target_include_directories(grpc_memory_tests PRIVATE ${GENERATED_DIR} ${INCLUDE_DIR})

# Линкуем общие библиотеки к первому тесту
target_link_libraries(grpc_memory_tests PRIVATE 
    gtest_main 
    protobuf::libprotobuf 
    grpc++
)

# -----------------------------------------------------------------------------
# ТЕСТ 2: Нагрузочный бенчмарк RPS (Будет собран файл grpc_cpu_tests.exe)
# -----------------------------------------------------------------------------
add_executable(grpc_cpu_tests "tests/grpc_CPU_RPS_test.cpp")

# Подключаем ко второму тесту те же самые сгенерированные файлы gRPC
target_sources(grpc_cpu_tests PRIVATE 
    "${GENERATED_DIR}/service.pb.cc" 
    "${GENERATED_DIR}/service.grpc.pb.cc"
)
# Открываем для второго теста доступ к заголовочным файлам из папки generated
target_include_directories(grpc_cpu_tests PRIVATE ${GENERATED_DIR} ${INCLUDE_DIR})

# Линкуем общие библиотеки ко второму тесту
target_link_libraries(grpc_cpu_tests PRIVATE 
    gtest_main 
    protobuf::libprotobuf 
    grpc++
)

# -----------------------------------------------------------------------------
# ТЕСТ 3: Тестирование таймаутов (Будет собран файл grpc_timeout_tests.exe)
# -----------------------------------------------------------------------------
add_executable(grpc_timeout_tests "tests/grpc_timeout_test.cpp")

# Подключаем сгенерированные файлы gRPC
target_sources(grpc_timeout_tests PRIVATE 
    "${GENERATED_DIR}/service.pb.cc" 
    "${GENERATED_DIR}/service.grpc.pb.cc"
)
# Открываем доступ к заголовочным файлам
target_include_directories(grpc_timeout_tests PRIVATE ${GENERATED_DIR} ${INCLUDE_DIR})

# Линкуем общие библиотеки
target_link_libraries(grpc_timeout_tests PRIVATE 
    gtest_main 
    protobuf::libprotobuf 
    grpc++
)

# -----------------------------------------------------------------------------
# ТЕСТ 4: Тест стабильности и замера времени (Будет собран файл grpc_soak_tests.exe)
# -----------------------------------------------------------------------------
add_executable(grpc_soak_tests "tests/grpc_soak_test.cpp")

target_sources(grpc_soak_tests PRIVATE 
    "${GENERATED_DIR}/service.pb.cc" 
    "${GENERATED_DIR}/service.grpc.pb.cc"
)

target_include_directories(grpc_soak_tests PRIVATE ${GENERATED_DIR} ${INCLUDE_DIR})

target_link_libraries(grpc_soak_tests PRIVATE gtest_main protobuf::libprotobuf grpc++)
