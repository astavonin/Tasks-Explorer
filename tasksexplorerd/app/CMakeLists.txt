cmake_minimum_required(VERSION 3.4)

set(PROJECT_NAME tasksexpd)
project(${PROJECT_NAME})

set(tasksexpd_sources main.cpp info_manager.cpp)

add_executable(${PROJECT_NAME} ${tasksexpd_sources})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(tasksexpd ${BOOST_LIBRARIES} task)
