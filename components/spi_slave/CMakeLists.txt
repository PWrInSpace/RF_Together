idf_component_register(SRCS "spi_slave_interface.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES driver__spi_slave)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)