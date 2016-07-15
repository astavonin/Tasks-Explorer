cmake_minimum_required(VERSION 3.4)

set(PROJECT_NAME tasksexplorerd)
project(${PROJECT_NAME})

find_package(Protobuf REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS} ${GENERATED_PROTOBUF_PATH})

set(${PROJECT_NAME}_sources main.cpp info_manager.cpp te_service_impl.cpp)

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_sources})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(${PROJECT_NAME} ${BOOST_LIBRARIES} task utils rpc ${PROTOBUF_LIBRARIES})
