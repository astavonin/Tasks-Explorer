#include <assert.h>
#include <mach/host_priv.h>

#include <errors.hpp>

#include "mach_helpers.h"
#include "system_helpers.h"
#include "tasks_monitor.h"

namespace tasks
{
TasksMonitor::TasksMonitor( mach_port_t hostPort, logger_ptr logger )
    : m_hostPort( hostPort ), m_log( logger ), m_maxProcsCount( 500 )
{
    assert( m_hostPort > 0 );
    assert( m_log.get() != nullptr );
}

TasksMonitor::~TasksMonitor()
{
}

TasksMonitor::TasksMapPtr TasksMonitor::GetTasks()
{
    auto tasks = std::make_shared<TasksMap>();
    auto procs = GetKinfoProcs();

    if( procs.size() <= 0 )
    {
        BOOST_THROW_EXCEPTION( err::internal_error() << err::description(
                                   "Tasks array could not be empty" ) );
    }
    m_log->debug( "{}: tasks count {}", __func__, procs.size() );

    for( auto &proc : procs )
    {
        auto pid = proc.kp_proc.p_pid;
        if( tasks->find( pid ) == tasks->end() )
        {
            tasks->emplace(
                std::make_pair( pid, std::make_shared<Task>( proc, m_log ) ) );
        }
    }

    return tasks;
}
}
