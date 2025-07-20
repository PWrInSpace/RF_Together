idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "")

idf_build_set_property(COMPILE_OPTIONS "-Wall" "-Wextra" "-Wno-unused-function" "-Werror" APPEND)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format -Wall -W -Werror)
