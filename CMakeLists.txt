# 可选 指明最低cmake版本
cmake_minimum_required(VERSION 3.10.1)

# 描述项目
project(PalmLocalDemo VERSION 1.0
                  DESCRIPTION "It is a local palm project demo"
                  LANGUAGES CXX)
                  
# 设置C++14编译，以及其他设置
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)  # 关闭编译器特有扩展
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pthread")  # 多线程
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-4") # 设置编译选项为DWARF 4
# 变量添加
set(EXECUTABLE_NAME test)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

# clangd确保有这行
set(CMAKE_EXPORT_COMPILE_COMMANDS True) 
# conan需要的命令
include(${CMAKE_SOURCE_DIR}/conan/conan_toolchain.cmake)

# 打印时间戳
string(TIMESTAMP COMPILE_TIME %Y%m%d-%H%M%S)

# 打包所有源文件 下面两种方式都可以
# aux_source_directory(${CMAKE_SOURCE_DIR}/src SOURCES)
file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADERS ${CMAKE_SOURCE_DIR}/include/PalmDemo/*.h)

# 设置外部库头文件变量
set(EIGEN3_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/include/Eigen)

find_package(cryptopp REQUIRED)
find_package(OpenCV 4.5.5 REQUIRED)

add_executable(${EXECUTABLE_NAME} ${CMAKE_SOURCE_DIR}/main/main.cpp ${SOURCES})

# 设置链接库和包含目录，使用一次性调用来减少重复代码
target_link_libraries(${EXECUTABLE_NAME}
    PUBLIC
    # opencv_dnn
    # opencv_highgui
    # opencv_imgproc
    # opencv_core
    ${OpenCV_LIBS}
    # opencv::opencv
    cryptopp::cryptopp
)

# 设置包含头文件的目录
target_include_directories(${EXECUTABLE_NAME}
    PUBLIC
    ${OpenCV_INCLUDE_DIRS}
    ${cryptopp_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include  # 自定义头文件目录
)

message(STATUS "OPencv Include Dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "Eigen include path: ${EIGEN3_INCLUDE_DIRS}")
