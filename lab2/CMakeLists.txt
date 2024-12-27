cmake_minimum_required(VERSION 3.5.0)
project(pa2 VERSION 0.1.0 LANGUAGES C)

add_library(libruntime SHARED IMPORTED)
set_property(TARGET libruntime PROPERTY IMPORTED_LOCATION "/home/jjj/Documents/ITMO/DistibutedSystems/lab2/lib64/libruntime.so")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS On)

add_executable(pa2
            main.c chanel.c process.c ipc.c ipc_message.c bank_robbery.c bank_client.c bank_server.c
            )

target_compile_options(pa2 PUBLIC 
-Wall -pedantic
-fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak -fsanitize=undefined -fsanitize-address-use-after-scope -fsanitize=address
)

target_link_libraries(pa2 
            asan ubsan
            libruntime
)

