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
    return m_app_name;
}

std::string task_impl::path_name() const
{
    return m_path_name;
}

pid_t task_impl::pid() const
{
    return m_pid;
}

std::vector<std::string> task_impl::argv() const
{
    return m_argv;
}

std::unordered_map<std::string, std::string> task_impl::envv() const
{
    return m_env;
}

void task_impl::dump( std::ostream& os ) const
{
    os << "class Task(" << std::hex /*<< (int)this*/ << std::dec << ") \n{\n"
       << "m_pid: " << m_pid << "\n"
       << "m_stamp: " << m_stamp << "\n"
       << "m_app_name: " << m_app_name << "\n"
       << "m_full_path_name: " << m_path_name << "\n"
       << "m_argv(" << m_argv.size() << "):" << m_argv << "\n"
       << "m_env(" << m_env.size() << "):" << m_env << "\n"
       << "m_log: " << std::hex << m_log.get() << std::dec << "\n"
       << "}";
}
}
