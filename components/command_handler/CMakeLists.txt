idf_component_register(SRCS "command_handler.c"
                       INCLUDE_DIRS "include"
                       REQUIRES data_protocol spi_slave_interface)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)