#include "tasks_monitor.h"
#include <assert.h>
#include <mach/host_priv.h>
#include "errors.h"
#include "system_helpers.h"
#include "task.h"

namespace tasks
{
TasksMonitor::TasksMonitor( mach_port_t hostPort, logger_ptr logger )
    : m_hostPort( hostPort )
    , m_log( logger )
    , m_tasks( new TasksMap() )
    , m_stamp( 0 )
{
    assert( m_hostPort > 0 );
    assert( m_log.get() != nullptr );
}

TasksMonitor::~TasksMonitor()
{
}

TasksMonitor::TasksMapPtr TasksMonitor::GetTasks()
{
    auto procs = GetKinfoProcs();

    if( procs.size() <= 0 )
    {
        BOOST_THROW_EXCEPTION( err::internal_error() << err::description(
                                   "Tasks array could not be empty" ) );
    }
    m_log->debug( "{}: tasks count {}", __func__, procs.size() );

    m_stamp++;

    for( auto &proc : procs )
    {
        auto pid  = proc.kp_proc.p_pid;
        auto task = m_tasks->find( pid );
        if( task == m_tasks->end() )
        {
            m_tasks->emplace( std::make_pair(
                pid, std::make_shared<Task>( m_stamp, proc, m_log ) ) );
        }
        else
        {
            task->second->Refresh( m_stamp, proc );
        }
    }

    for( auto it = m_tasks->begin(); it != m_tasks->end(); )
    {
        if( it->second->GetStamp() != m_stamp )
            m_tasks->erase( it++ );
        else
            ++it;
    }

    return m_tasks;
}
}
