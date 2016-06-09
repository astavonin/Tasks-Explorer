#include <utils.h>
#include <algorithm>
#include <boost/exception/all.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/test/unit_test.hpp>
#include <fstream>
#include "helpers.h"
#include "../task/system_helpers.h"

namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( TasksBaseTests )

BOOST_AUTO_TEST_CASE( Helpers_GetKinfoProcs )
{
    auto kinfos = tasks::GetKinfoProcs();

    // We have severral processes for sure
    BOOST_REQUIRE( kinfos.size() );

    // we always have process with PID == 1
    BOOST_REQUIRE(
        std::find_if( kinfos.begin(), kinfos.end(), []( auto kinfo ) {
            return kinfo.kp_proc.p_pid == 1;
        } ) != kinfos.end() );
}

BOOST_AUTO_TEST_CASE( Helpers_ParseProcArgs )
{
    auto buff = tests::helpers::read_file( "procargv.bin" );
    tasks::ParseProcArgs( buff );
}

BOOST_AUTO_TEST_SUITE_END()
