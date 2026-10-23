add_library(stdio_text_protocol STATIC src/stdio-text-protocol.c)
target_include_directories(stdio_text_protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(stdio_text_protocol PRIVATE -Wall -Wextra)
target_link_libraries(stdio_text_protocol PRIVATE pico_stdlib)
