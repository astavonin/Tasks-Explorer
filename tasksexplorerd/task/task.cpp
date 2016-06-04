#include "task.h"
#include <assert.h>
#include <errors.hpp>

#include <vector>

#include <fstream>

namespace tasks
{
Task::Task( const kinfo_proc& proc, logger_ptr logger )
    : m_proc( proc ), m_log( logger )
{
    assert( m_log.get() != nullptr );

    ReadTaskInfo();
}

Task::~Task()
{
}

void Task::ReadTaskInfo()
{
    m_pid = m_proc.kp_proc.p_pid;

    int        name[] = {CTL_KERN, KERN_ARGMAX, 0};
    static int argmax = 0;
    size_t     size   = sizeof( argmax );

    int ret = sysctl( name, 2, &argmax, &size, nullptr, 0 );
    if( ret != 0 )
    {
        BOOST_THROW_EXCEPTION( err::sys_api_error()
                               << err::api_function( "sysctl" )
                               << err::errinfo_errno( errno ) );
    }

    std::vector<char> procargv( argmax );

    name[1] = KERN_PROCARGS2;
    name[2] = GetPID();
    size    = argmax;

    ret = sysctl( name, 3, &procargv[0], &size, nullptr, 0 );
    if( ret != 0 )
    {
        if( GetPID() == 0 )
        {
            m_name = "kernel_task";
        }
        else
        {
            m_log->warn( "{}: unable to get environment for PID {}", __func__,
                         GetPID() );
        }
        return;
    }
}

pid_t Task::GetPID() const
{
    return m_pid;
}

void Task::Refresh( const kinfo_proc& proc )
{
}
std::ostream& operator<<( std::ostream& os, const Task& t )
{
    return os << t.m_name << "(" << t.m_pid << ")";
}
}
