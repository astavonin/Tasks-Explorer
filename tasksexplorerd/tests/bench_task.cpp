#include "../task/system_helpers.h"
#include "../task/task_impl.h"
#include "../task/tasks_monitor_impl.h"
#include "helpers.h"
#include "logger.h"
#include "utils.h"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <ctime>
#include <fstream>
#include <hayai.hpp>

namespace fs = boost::filesystem;

BENCHMARK( ProcsGoup, build_tasks_list, 10, 10 )
{
    auto procs = tasks::build_tasks_list();
}

class CreateTaskFixture : public ::hayai::Fixture
{
public:
    virtual void SetUp()
    {
        tasks  = tasks::build_tasks_list();
        logger = spdlog::get( "CreateTask" );
        if( !logger.get() )
            logger = spdlog::stdout_logger_mt( "CreateTask", true );
    }
    tasks::proc_info_vec tasks;
    logger_ptr           logger;
};

BENCHMARK_F( CreateTaskFixture, CreateTask, 10, 10 )
{
    auto task = std::make_shared<tasks::task_impl>(
        0, timeval { 0, 0 }, tasks[2], logger );
}

class ParseProcArgsFixture : public ::hayai::Fixture
{
public:
    virtual void SetUp()
    {
        buff = tests::helpers::read_file( "procargv.bin" );
    }
    std::vector<char> buff;
};

BENCHMARK_F( ParseProcArgsFixture, parse_proc_args, 10, 10 )
{
    auto args = tasks::parse_proc_args( buff, nullptr );
}

class GetTasksFixture : public ::hayai::Fixture
{
public:
    virtual void SetUp()
    {
        logger = spdlog::get( "active_tasks" );
        if( !logger.get() )
            logger = spdlog::stdout_logger_mt( "active_tasks", true );
        tm = std::make_unique<tasks::tasks_monitor_impl>( mach_host_self(),
                                                          logger );
    }
    logger_ptr                            logger;
    std::unique_ptr<tasks::tasks_monitor> tm;
};

BENCHMARK_F( GetTasksFixture, active_tasks, 5, 1 )
{
    auto tasks = tm->active_tasks();
}
