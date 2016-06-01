#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE BaseTask
#include <boost/test/unit_test.hpp>

#include <boost/exception/all.hpp>

#include <iostream>
#include <tasks.h>

BOOST_AUTO_TEST_SUITE(BaseTask)

BOOST_AUTO_TEST_CASE(GetTasksList)
{
    try
    {
        auto tm = tasks::TasksMonitor(mach_host_self());
        auto tasks = tm.GetTasksSnapshot();
    } catch(boost::exception &err)
    {
        BOOST_FAIL(boost::diagnostic_information(err));
    }
}

BOOST_AUTO_TEST_SUITE_END()
