cmake_minimum_required(VERSION 3.5.0)
project(pa5 VERSION 0.1.0 LANGUAGES C)

add_library(libruntime SHARED IMPORTED)
set_property(TARGET libruntime PROPERTY IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib64/libruntime.so")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS On)

add_executable(pa5
            main.c
            chanel.c process.c ipc.c ipc_message.c lamport_time.c critical_sections.c
            )

target_compile_options(pa5 PUBLIC 
-Wall -pedantic
-fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak -fsanitize=undefined -fsanitize-address-use-after-scope -fsanitize=address
)

target_link_libraries(pa5
            asan ubsan
            libruntime
)

