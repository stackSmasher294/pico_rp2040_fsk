cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(gpout_test)
pico_sdk_init()

add_executable(clkout_test
        clk_gpout_test.c
        )

# pull in common dependencies and additional pwm hardware support
target_link_libraries(clkout_test pico_stdlib hardware_pwm hardware_dma)






pico_enable_stdio_usb(clkout_test 1)
pico_enable_stdio_uart(clkout_test 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(clkout_test)





# add url via pico_set_program_url
# example_auto_set_url(dev_audio)
