#include "task_impl.h"
#include <assert.h>
#include <iostream>
#include <prettyprint.hpp>
#include <vector>
#include "errors.h"
#include "system_helpers.h"
#include "utils.h"

namespace tasks
{
task_impl::task_impl( std::uintmax_t stamp, const kinfo_proc& proc,
                      logger_ptr logger )
    : m_proc( proc ), m_log( logger ), m_stamp( stamp )
{
    assert( m_log.get() != nullptr );

    read_task_info();
}

task_impl::~task_impl()
{
}

void task_impl::read_task_info()
{
    m_pid = m_proc.kp_proc.p_pid;

    if( m_pid == 0 )
    {
        m_app_name = "kernel_task";
    }
    else
    {
        auto rawArgs = read_proc_args( m_pid, m_log );
        if( rawArgs )
        {
            auto parsedArgs = parse_proc_args( *rawArgs, m_log );

            m_app_name  = std::move( parsedArgs.app_name );
            m_path_name = std::move( parsedArgs.path_name );
            m_argv      = std::move( parsedArgs.argv );
            m_env       = std::move( parsedArgs.env );
        }
        else
        {
            m_app_name = m_proc.kp_proc.p_comm;
        }
    }
}

void task_impl::refresh( std::uintmax_t stamp, const kinfo_proc& proc )
{
    m_stamp = stamp;
}

std::string task_impl::name() const
{
    return "";
}

void task_impl::dump( std::ostream& os ) const
{
}

std::ostream& operator<<( std::ostream& os, const task& t )
{
    t.dump( os );
    return os;
    // return os << "class Task(" << std::hex << &t << std::dec << ") \n{\n"
    //<< "m_pid: " << t.m_pid << "\n"
    //<< "m_stamp: " << t.m_stamp << "\n"
    //<< "m_app_name: " << t.m_app_name << "\n"
    //<< "m_full_path_name: " << t.m_path_name << "\n"
    //<< "m_argv(" << t.m_argv.size() << "):" << t.m_argv << "\n"
    //<< "m_env(" << t.m_env.size() << "):" << t.m_env << "\n"
    //<< "m_log: " << std::hex << t.m_log.get() << std::dec << "\n"
    //<< "}";
}
}
