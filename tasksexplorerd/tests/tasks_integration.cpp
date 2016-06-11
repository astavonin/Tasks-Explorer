#include <spdlog/spdlog.h>
#include <boost/exception/all.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>
#include "../task/tasks_monitor_impl.h"

BOOST_AUTO_TEST_SUITE( TasksIntegrationTests )

BOOST_AUTO_TEST_CASE( active_tasks )
{
    try
    {
        auto logger =
            spdlog::stdout_logger_mt( "active_tasks", true /*use color*/ );

        tasks::tasks_monitor_impl tm( mach_host_self(), logger );
        auto                tasks = tm.active_tasks();
        BOOST_TEST( tasks->size() > 0 );
        // we always have tasks with PID 0 and 1
        BOOST_REQUIRE( tasks->find( 0 ) != tasks->end() );
        BOOST_REQUIRE( tasks->find( 1 ) != tasks->end() );
    }
    catch( boost::exception &err )
    {
        BOOST_FAIL( boost::diagnostic_information( err ) );
    }
    catch( std::exception &err )
    {
        BOOST_FAIL( err.what() );
    }
}

BOOST_AUTO_TEST_SUITE_END()
