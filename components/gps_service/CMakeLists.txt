idf_component_register(SRCS "gps_service.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES driver__i2c)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror -Wno-format)