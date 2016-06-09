cmake_minimum_required(VERSION 3.4)

set(PROJECT_NAME taskexpd_tests)
project(${PROJECT_NAME})

find_package (Boost COMPONENTS unit_test_framework system filesystem REQUIRED)

set(test_tasks_source
    tasks_base.cpp
    helpers.cpp
    tasks_integration.cpp
    main.cpp
    )
file(COPY procargv.bin DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

set(bench_tasks_source
    helpers.cpp
    bench_task.cpp
    ../../externals/hayai/src/hayai_posix_main.cpp
    )

include_directories(../../externals/hayai/src)


add_executable(test_tasks ${test_tasks_source})
set_property(TARGET test_tasks PROPERTY CXX_STANDARD 14)
set_property(TARGET test_tasks PROPERTY CXX_STANDARD_REQUIRED ON)

add_executable(bench_tasks ${bench_tasks_source})
set_property(TARGET bench_tasks PROPERTY CXX_STANDARD 14)
set_property(TARGET bench_tasks PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(test_tasks ${Boost_LIBRARIES} task utils)
target_link_libraries(bench_tasks ${Boost_LIBRARIES} task utils)

add_test(NAME TaskTests COMMAND test_tasks)

set_property(TEST TaskTests PROPERTY LABELS "unit")
