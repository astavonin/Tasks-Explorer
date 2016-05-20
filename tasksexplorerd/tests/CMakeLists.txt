cmake_minimum_required(VERSION 3.4)

set(PROJECT_NAME taskexpd_tests)
project(${PROJECT_NAME})

find_package (Boost COMPONENTS unit_test_framework REQUIRED)

set(test_tasks_base_source tasks_base.cpp)
#set(test_main_source main.cpp)

add_executable(test_tasks_base ${test_tasks_base_source})
set_property(TARGET test_tasks_base PROPERTY CXX_STANDARD 14)
set_property(TARGET test_tasks_base PROPERTY CXX_STANDARD_REQUIRED ON)

#add_executable(test_main ${test_main_source})

target_link_libraries(test_tasks_base ${Boost_LIBRARIES} task)
#target_link_libraries(test_tasks_base ${Boost_LIBRARIES} task)

add_test(NAME TaskBaseTests COMMAND test_tasks_base)
