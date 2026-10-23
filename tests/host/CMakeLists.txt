cmake_minimum_required(VERSION 3.15)
project(firmware_host_tests C)
set(CMAKE_C_STANDARD 11)
set(ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
set(PROJECT ${ROOT}/231-firmware-template)
add_executable(test_firmware test_firmware.c
    ${PROJECT}/logic/led-task/led-task.c ${PROJECT}/logic/button-task/button-task.c
    ${PROJECT}/logic/pi-task/pi-task.c ${ROOT}/libs/systime/src/systime.c
    ${ROOT}/libs/profiling/src/profiling.c ${ROOT}/libs/stdio-text-protocol/src/stdio-text-protocol.c)
target_include_directories(test_firmware PRIVATE . ${PROJECT} ${PROJECT}/logic
    ${ROOT}/libs/systime/include ${ROOT}/libs/profiling/include ${ROOT}/libs/stdio-text-protocol/include)
if(MSVC)
    target_compile_options(test_firmware PRIVATE /W4 /UNDEBUG)
else()
    target_compile_options(test_firmware PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    target_link_libraries(test_firmware PRIVATE m)
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME api_boundaries COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_api.py $<TARGET_FILE:example_runtime>)
add_executable(test_buttons test_buttons.c
    ${ROOT}/239-two-buttons/logic/button-task/button-task.c
    ${ROOT}/239-two-buttons/logic/led-task/led-task.c ${ROOT}/libs/systime/src/systime.c)
target_include_directories(test_buttons PRIVATE . ${ROOT}/239-two-buttons
    ${ROOT}/239-two-buttons/logic ${ROOT}/libs/systime/include)
if(MSVC)
    target_compile_options(test_buttons PRIVATE /W4 /UNDEBUG)
else()
    target_compile_options(test_buttons PRIVATE -Wall -Wextra -Werror -UNDEBUG)
endif()
add_test(NAME independent_buttons COMMAND test_buttons)
enable_testing()
add_test(NAME firmware_logic COMMAND test_firmware)

add_executable(example_runtime example_runtime.c
    ${PROJECT}/logic/api/api.c ${PROJECT}/logic/api/api-callbacks.c
    ${PROJECT}/logic/led-task/led-task.c ${PROJECT}/logic/button-task/button-task.c
    ${PROJECT}/logic/pi-task/pi-task.c ${ROOT}/libs/systime/src/systime.c
    ${ROOT}/libs/profiling/src/profiling.c ${ROOT}/libs/stdio-text-protocol/src/stdio-text-protocol.c)
target_include_directories(example_runtime PRIVATE . ${PROJECT} ${PROJECT}/logic
    ${ROOT}/libs/systime/include ${ROOT}/libs/profiling/include
    ${ROOT}/libs/device/include ${ROOT}/libs/stdio-text-protocol/include)
if(MSVC)
    target_compile_options(example_runtime PRIVATE /FI${CMAKE_CURRENT_SOURCE_DIR}/capture.h)
    target_compile_definitions(test_firmware PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(example_runtime PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(example_runtime PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/capture.h)
    target_compile_options(example_runtime PRIVATE -Wall -Wextra -Werror)
endif()
