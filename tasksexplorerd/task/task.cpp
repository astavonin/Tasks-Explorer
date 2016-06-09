#include "task.h"
#include <assert.h>
#include <errors.hpp>
#include <iostream>
#include <vector>
#include "system_helpers.h"

namespace tasks
{
Task::Task( const kinfo_proc& proc, logger_ptr logger )
    : m_proc( proc ), m_log( logger )
{
    assert( m_log.get() != nullptr );

    ReadTaskInfo();
}

void Task::ReadTaskInfo()
{
    m_pid = m_proc.kp_proc.p_pid;

    static int argmax = [this]() -> int {
        int    name[] = {CTL_KERN, KERN_ARGMAX, 0};
        int    argmax = 0;
        size_t size   = sizeof( argmax );

        int ret = sysctl( name, 2, &argmax, &size, nullptr, 0 );
        if( ret != 0 )
        {
            m_log->error( "{}: unable to get argmax", __func__ );
        }
        return argmax;
    }();

    std::vector<char> procargv( argmax );
    int               name[] = {CTL_KERN, KERN_PROCARGS2, GetPID()};
    size_t            size   = argmax;

    int ret = sysctl( name, 3, &procargv[0], &size, nullptr, 0 );
    if( ret != 0 )
    {
        if( GetPID() == 0 )
        {
            m_args          = std::make_unique<ProcArgs>();
            m_args->appName = "kernel_task";
        }
        else
        {
            m_log->warn( "{}: unable to get environment for PID {}", __func__,
                         GetPID() );
        }
        return;
    }
    procargv.resize( size );

    m_args = ParseProcArgs( procargv );
}

pid_t Task::GetPID() const { return m_pid; }
void Task::Refresh( const kinfo_proc& proc ) {}
std::ostream& operator<<( std::ostream& os, const Task& t )
{
    return os << "class Task(" << std::hex << &t << std::dec << ") \n{\n"
              << "m_pid: " << t.m_pid << "\n"
              << "m_args: " << *t.m_args << "\n"
              << "}";
}
}
