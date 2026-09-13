cmake_minimum_required(VERSION 3.16)
project(picopal_host_tests C)

enable_testing()

add_executable(
    timer_model_test
    timer_model_test.c
    ../main/picopal_timer_model.c
)
target_include_directories(timer_model_test PRIVATE ../main)
target_compile_options(timer_model_test PRIVATE -Wall -Wextra -Werror)

add_test(NAME timer_model COMMAND timer_model_test)

add_executable(
    page_model_test
    page_model_test.c
    ../main/picopal_page_model.c
)
target_include_directories(page_model_test PRIVATE ../main)
target_compile_options(page_model_test PRIVATE -Wall -Wextra -Werror)

add_test(NAME page_model COMMAND page_model_test)

add_executable(
    pico_state_model_test
    pico_state_model_test.c
    ../main/picopal_pico_state_model.c
)
target_include_directories(pico_state_model_test PRIVATE ../main)
target_compile_options(pico_state_model_test PRIVATE -Wall -Wextra -Werror)

add_test(NAME pico_state_model COMMAND pico_state_model_test)

add_executable(
    command_parser_test
    command_parser_test.c
    ../main/picopal_command_parser.c
)
target_include_directories(command_parser_test PRIVATE ../main)
target_compile_options(command_parser_test PRIVATE -Wall -Wextra -Werror)

add_test(NAME command_parser COMMAND command_parser_test)
