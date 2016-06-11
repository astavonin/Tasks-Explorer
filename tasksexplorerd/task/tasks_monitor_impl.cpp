#include "tasks_monitor_impl.h"
#include <assert.h>
#include <mach/host_priv.h>
#include "../task/task_impl.h"
#include "errors.h"
#include "system_helpers.h"

namespace tasks
{
tasks_monitor_impl::tasks_monitor_impl( mach_port_t hostPort,
                                        logger_ptr  logger )
    : m_hostPort( hostPort )
    , m_log( logger )
    , m_tasks( new tasks_map() )
    , m_stamp( 0 )
{
    assert( m_hostPort > 0 );
    assert( m_log.get() != nullptr );
}

tasks_monitor_impl::~tasks_monitor_impl()
{
}

tasks_monitor_impl::tasks_map_ptr tasks_monitor_impl::active_tasks()
{
    auto procs = build_tasks_list();

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
                pid, std::make_shared<task_impl>( m_stamp, proc, m_log ) ) );
        }
        else
        {
            task->second->refresh( m_stamp, proc );
        }
    }

    for( auto it = m_tasks->begin(); it != m_tasks->end(); )
    {
        if( it->second->stamp() != m_stamp )
            m_tasks->erase( it++ );
        else
            ++it;
    }

    return m_tasks;
}

void tasks_monitor_impl::dump( std::ostream &os ) const
{
}
}
