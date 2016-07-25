cmake_minimum_required(VERSION 3.4)

set(PROJECT_NAME test_client)
project(${PROJECT_NAME})

find_package(Protobuf REQUIRED)
find_package(Boost REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS}
    ${GENERATED_PROTOBUF_PATH}
    ${Boost_INCLUDE_DIRS})

set(te_sources main.cpp)

add_executable(${PROJECT_NAME} ${te_sources})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(${PROJECT_NAME} ${BOOST_LIBRARIES} rpc ${PROTOBUF_LIBRARIES})
