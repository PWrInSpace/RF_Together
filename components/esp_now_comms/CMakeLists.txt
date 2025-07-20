idf_component_register(SRCS "esp_now_comms.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_wifi esp_now)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)