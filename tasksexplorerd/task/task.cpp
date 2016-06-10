#include "task.h"
#include <assert.h>
#include <iostream>
#include <prettyprint.hpp>
#include <vector>
#include "errors.h"
#include "system_helpers.h"
#include "utils.h"

namespace tasks
{
Task::Task( std::uintmax_t stamp, const kinfo_proc& proc, logger_ptr logger )
    : m_proc( proc ), m_log( logger ), m_stamp( stamp )
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

    if( GetPID() == 0 )
    {
        m_appName = "kernel_task";
    }
    else
    {
        auto rawArgs = ReadProcArgs( m_pid, m_log );
        if( rawArgs )
        {
            auto parsedArgs = ParseProcArgs( *rawArgs, m_log );

            m_appName      = std::move( parsedArgs.appName );
            m_fullPathName = std::move( parsedArgs.fullPathName );
            m_argv         = std::move( parsedArgs.argv );
            m_env          = std::move( parsedArgs.env );
        }
        else
        {
            m_appName = m_proc.kp_proc.p_comm;
        }
    }
}

void Task::Refresh( std::uintmax_t stamp, const kinfo_proc& proc )
{
    m_stamp = stamp;
}

std::ostream& operator<<( std::ostream& os, const Task& t )
{
    return os << "class Task(" << std::hex << &t << std::dec << ") \n{\n"
              << "m_pid: " << t.m_pid << "\n"
              << "m_stamp: " << t.m_stamp << "\n"
              << "m_appName: " << t.m_appName << "\n"
              << "m_fullPathName: " << t.m_fullPathName << "\n"
              << "m_argv(" << t.m_argv.size() << "):" << t.m_argv << "\n"
              << "m_env(" << t.m_env.size() << "):" << t.m_env << "\n"
              << "m_log: " << std::hex << t.m_log.get() << std::dec << "\n"
              << "}";
}
}
