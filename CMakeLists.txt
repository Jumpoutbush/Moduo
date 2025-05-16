cmake_minimum_required(VERSION 3.0)
project(Moduo)

#生成debug版本，可以进行gdb调试
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

#设置动态库输出
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#源文件
aux_source_directory(./src SRC_LIST)
include_directories(${PROJECT_SOURCE_DIR}/src/include)
#编译动态库
add_library(Moduo SHARED ${SRC_LIST})

# add_subdirectory(src)
# src下新建rpc