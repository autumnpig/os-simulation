cmake_minimum_required(VERSION 3.20)
project(OS_Simulation)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 头文件搜索路径
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/scheduler
    ${CMAKE_SOURCE_DIR}/memory_manager
    # ${CMAKE_SOURCE_DIR}/sync
)

# 定义所有源文件
set(SOURCES
    main.cpp
    scheduler/scheduler.cpp
    memory_manager/memory_manager.cpp
    # sync/mutex.cpp
)

# 生成可执行文件
add_executable(os_sim ${SOURCES})
