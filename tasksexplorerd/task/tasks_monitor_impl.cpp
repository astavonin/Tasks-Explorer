#include "tasks_monitor_impl.h"
#include <assert.h>
#include <mach/host_priv.h>
#include <ctime>
#include <list>
#include "errors.h"
#include "system_helpers.h"
#include "task_impl.h"
#include "utils.h"

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

void tasks_monitor_impl::refresh_tasks( tasks_refresh_list &tl )
{
    timeval now;
    gettimeofday( &now, NULL );

    auto procs = build_tasks_list();

    if( procs.size() <= 0 )
    {
        BOOST_THROW_EXCEPTION( err::internal_error() << err::description(
                                   "Tasks array could not be empty" ) );
    }
    m_log->debug( "{}: tasks count {}", __func__, procs.size() );

    timeval elapsed;
    timersub( &now, &m_refresh_time, &elapsed );
    for( auto &proc : procs )
    {
        auto pid  = proc.kp_proc.p_pid;
        auto task = m_tasks->find( pid );
        if( task == m_tasks->end() )
        {
            auto f = std::async( [&proc, &elapsed, this ]() -> auto {
                return std::make_shared<task_impl>( m_stamp, elapsed, proc,
                                                    m_log );
            } );
            tl.emplace_back( std::move( f ) );
        }
        else
        {
            auto task_i = std::static_pointer_cast<task_impl>( task->second );

            task_i->refresh( m_stamp, elapsed, proc );
        }
    }
    m_refresh_time = now;
}

void tasks_monitor_impl::update_tasks_map( tasks_refresh_list &tl )
{
    for( auto &tf : tl )
    {
        auto t = tf.get();
        m_tasks->insert( std::make_pair( t->pid(), t ) );
    }

    for( auto it = m_tasks->begin(); it != m_tasks->end(); )
    {
        auto task_i = std::static_pointer_cast<task_impl>( it->second );
        if( task_i->stamp() != m_stamp )
            m_tasks->erase( it++ );
        else
            ++it;
    }
}

tasks_map_ptr tasks_monitor_impl::active_tasks()
{
    m_stamp++;

    tasks_refresh_list tl;
    refresh_tasks( tl );
    update_tasks_map( tl );

    return m_tasks;
}

void tasks_monitor_impl::dump( std::ostream &os ) const
{
    os << "class tasks_monitor_impl(0x" << std::hex << (long)this << std::dec
       << ") \n{\n"
       << "tasks count: " << m_tasks->size() << "\n"
       << "m_refresh_time: " << m_refresh_time.tv_sec << " sec, "
       << m_refresh_time.tv_usec << " usec" << std::dec << "\n"
       << "m_stamp: " << m_stamp << std::dec << "\n"
       << "m_hostPort: " << std::hex << m_hostPort << std::dec << "\n"
       << "m_log: " << std::hex << m_log.get() << std::dec << "\n"
       << "}";
}

tasks_monitor_ptr create_tasks_monitor( mach_port_t hostPort,
                                        logger_ptr  logger )
{
    return std::make_unique<tasks_monitor_impl>( hostPort, logger );
}
}
