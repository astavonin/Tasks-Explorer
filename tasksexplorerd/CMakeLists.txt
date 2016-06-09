cmake_minimum_required(VERSION 3.4)

set(PROJECT_NAME tasksexplorerd)
project(${PROJECT_NAME})

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS}
    include
    ../externals/prettyprint
    ../externals/spdlog/include)

add_subdirectory(utils)
add_subdirectory(task)
add_subdirectory(app)
add_subdirectory(tests)
