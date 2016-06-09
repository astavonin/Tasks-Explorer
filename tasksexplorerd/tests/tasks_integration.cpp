#include <spdlog/spdlog.h>
#include <boost/exception/all.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>

#include <tasks.h>

#include <iostream>

BOOST_AUTO_TEST_SUITE( TasksIntegrationTests )

BOOST_AUTO_TEST_CASE( GetTasksList )
{
    try
    {
        auto logger = spdlog::stdout_logger_mt( "GetTasksList", true /*use color*/ );
        logger->set_level(spdlog::level::debug);

        auto tm     = tasks::TasksMonitor( mach_host_self(), logger );
        auto tasks  = tm.GetTasks();
        BOOST_TEST( tasks.size() > 0 );
        // we always have tasks with PID 0 and 1
        BOOST_REQUIRE( tasks.find(0) != tasks.end() );
        BOOST_REQUIRE( tasks.find(1) != tasks.end() );
    }
    catch( boost::exception &err )
    {
        BOOST_FAIL( boost::diagnostic_information( err ) );
    }
}

BOOST_AUTO_TEST_SUITE_END()
