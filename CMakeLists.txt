# This is the project CMakeLists.txt file for the test subproject
cmake_minimum_required(VERSION 3.16)

# Include the body of stuff in the directory
# above, main and the tests component from below
# Reduce the component list to a minimum
# execute_process (
#     COMMAND cd components/protobuf && bash -c "protoc --c_out=. lora.proto"
# )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(RF_Together)
