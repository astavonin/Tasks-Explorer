cmake_minimum_required(VERSION 3.4)

set(PROJECT_NAME utils)
project(${PROJECT_NAME})

set(${PROJECT_NAME}_sources utils.cpp)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_sources})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(${PROJECT_NAME} ${BOOST_LIBRARIES})
