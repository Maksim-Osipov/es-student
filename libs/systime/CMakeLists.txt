add_library(systime STATIC src/systime.c)
target_include_directories(systime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(systime PRIVATE -Wall -Wextra)
target_link_libraries(systime PRIVATE pico_time)
