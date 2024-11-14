cmake_minimum_required(VERSION 3.5.0)
project(pa2 VERSION 0.1.0 LANGUAGES C)

add_library(libruntime SHARED IMPORTED)
set_property(TARGET libruntime PROPERTY IMPORTED_LOCATION "/home/jjj/Documents/ITMO/DistibutedSystems/lab2/lib64/libruntime.so")

add_executable(pa2
            main.c chanel.c process.c ipc.c ipc_message.c bank_robbery.c bank_client.c bank_server.c
            )

target_link_libraries(pa2 libruntime)

