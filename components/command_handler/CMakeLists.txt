idf_component_register(  SRC_DIRS "."
                         INCLUDE_DIRS "."
                         REQUIRES  data_protocol spi_slave)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format" "-Wall" "-Werror")