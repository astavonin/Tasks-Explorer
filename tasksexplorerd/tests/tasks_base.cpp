#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Calculator
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <tasks.h>

BOOST_AUTO_TEST_SUITE(BaseTask)

BOOST_AUTO_TEST_CASE(GetTasksList)
{
    auto tm = TasksMonitor(0);
}

BOOST_AUTO_TEST_SUITE_END()
