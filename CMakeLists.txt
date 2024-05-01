cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(gpout_test)
pico_sdk_init()

include_directories(
        ./
)
add_library(pico_fsk_lib 
pico_fsk/pico_fsk.c
)

add_executable(clkout_test
        clk_gpout_test.c
        )
add_executable(
        fsk_test
        pico_fsk_test.c
)

# pull in common dependencies and additional pwm hardware support
target_link_libraries(clkout_test pico_stdlib hardware_pwm hardware_dma)
target_link_libraries(pico_fsk_lib pico_stdlib hardware_dma)
target_link_libraries(fsk_test pico_fsk_lib)


pico_enable_stdio_usb(clkout_test 1)
pico_enable_stdio_uart(clkout_test 0)
# create map/bin/hex file etc.
pico_add_extra_outputs(clkout_test)


pico_enable_stdio_usb(fsk_test 1)
pico_enable_stdio_uart(fsk_test 0)
# create map/bin/hex file etc.
pico_add_extra_outputs(fsk_test)





# add url via pico_set_program_url
# example_auto_set_url(dev_audio)
