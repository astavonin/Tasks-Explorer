#include <tasks.h>
#include <utils.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <fstream>
#include <hayai.hpp>
#include <logger.hpp>
#include "../task/system_helpers.h"
#include "helpers.h"

namespace fs = boost::filesystem;

BENCHMARK( ProcsGoup, GetKinfoProcs, 10, 10 )
{
    auto procs = tasks::GetKinfoProcs();
}

class CreateTaskFixture : public ::hayai::Fixture
{
public:
    virtual void SetUp()
    {
        tasks  = tasks::GetKinfoProcs();
        logger = spdlog::get( "CreateTask" );
        if( !logger.get() )
            logger = spdlog::stdout_logger_mt( "CreateTask", true );
    }
    tasks::proc_info_vec tasks;
    logger_ptr           logger;
};

BENCHMARK_F( CreateTaskFixture, CreateTask, 10, 10 )
{
    auto task = std::make_shared<tasks::Task>( 0, tasks[2], logger );
}

class ParseProcArgsFixture : public ::hayai::Fixture
{
public:
    virtual void SetUp() { buff = tests::helpers::read_file( "procargv.bin" ); }
    std::vector<char> buff;
};

BENCHMARK_F( ParseProcArgsFixture, ParseProcArgs, 10, 10 )
{
    auto args = tasks::ParseProcArgs( buff, nullptr );
}

class GetTasksFixture : public ::hayai::Fixture
{
public:
    virtual void SetUp()
    {
        logger = spdlog::get( "GetTasks" );
        if( !logger.get() )
            logger = spdlog::stdout_logger_mt( "GetTasks", true );
        tm = std::make_unique<tasks::TasksMonitor>( mach_host_self(), logger );
    }
    logger_ptr                           logger;
    std::unique_ptr<tasks::TasksMonitor> tm;
};

BENCHMARK_F( GetTasksFixture, GetTasks, 5, 1 ) { auto tasks = tm->GetTasks(); }
