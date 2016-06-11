cmake_minimum_required(VERSION 3.4)

set(PROJECT_NAME task)
project(${PROJECT_NAME})

set(task_sources
    task_impl.cpp
    tasks_monitor_impl.cpp
    system_helpers.cpp
    )

add_library(${PROJECT_NAME} STATIC ${task_sources})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(${PROJECT_NAME} ${BOOST_LIBRARIES} utils)
