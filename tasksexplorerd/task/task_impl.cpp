#include "task_impl.h"
#include "errors.h"
#include "system_helpers.h"
#include "utils.h"
#include <assert.h>
#include <ctime>
#include <fmt/format.h>
#include <mach/mach.h>
#include <prettyprint.hpp>
#include <sstream>
#include <vector>

inline timeval to_timeval( time_value_t tv )
{
    return timeval{tv.seconds, tv.microseconds};
}

namespace tasks
{
task_impl::task_impl( std::uintmax_t stamp, timeval elapsed,
                      const kinfo_proc &proc, logger_ptr logger )
    : m_proc( proc )
    , m_log( logger )
    , m_stamp( stamp )
{
    assert( m_log.get() != nullptr );

    read_task_info( elapsed, true );
}

task_impl::~task_impl()
{
}

task_impl::usage_info task_impl::process_task_data()
{
    task_t task;
    auto   ret = task_for_pid( mach_task_self(), m_pid, &task );
    if( ret != KERN_SUCCESS )
    {
        BOOST_THROW_EXCEPTION( err::sys_api_error()
                               << err::description( fmt::format(
                                      "task_for_pid({}) failed", m_pid ) )
                               << err::mach_error( ret ) );
    }
    struct task_basic_info_64 ti;
    mach_msg_type_number_t    count;

    count = TASK_BASIC_INFO_64_COUNT;
    ret   = task_info( task, TASK_BASIC_INFO_64, (task_info_t)&ti, &count );
    if( ret != KERN_SUCCESS )
    {
        BOOST_THROW_EXCEPTION(
            err::sys_api_error()
            << err::description( fmt::format( "task_info({}) failed", m_pid ) )
            << err::mach_error( ret ) );
    }

    m_real_mem_size    = ti.resident_size;
    m_virtual_mem_size = ti.virtual_size;

    return std::make_tuple( to_timeval( ti.user_time ),
                            to_timeval( ti.system_time ) );
}

void task_impl::process_proc_args()
{
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

void task_impl::process_cpu_usage( bool new_task, timeval elapsed,
                                   usage_info usage )
{
    timeval time_user   = std::get<0>( usage ),
            time_kernel = std::get<1>( usage );

    if( new_task )
    {
        m_time_user   = std::get<0>( usage );
        m_time_kernel = std::get<1>( usage );
    }

    timeval used_user, used_kernel;
    timersub( &time_user, &m_time_user, &used_user );
    timersub( &time_kernel, &m_time_kernel, &used_kernel );
    uint64_t elapsed_us =
        (uint64_t)elapsed.tv_sec * 1000000ULL + (uint64_t)elapsed.tv_usec;

    uint64_t used_us_user =
        (uint64_t)used_user.tv_sec * 1000000ULL + (uint64_t)used_user.tv_usec;
    uint64_t used_us_kernerl = (uint64_t)used_kernel.tv_sec * 1000000ULL +
                               (uint64_t)used_kernel.tv_usec;

    int whole_user = 0, part_user = 0, whole_kernel = 0, part_kernel = 0;
    if( elapsed_us > 0 )
    {
        whole_user = ( used_us_user * 100ULL ) / elapsed_us;
        part_user =
            ( ( ( used_us_user * 100ULL ) - ( whole_user * elapsed_us ) ) *
              10ULL ) /
            elapsed_us;
        whole_kernel = ( used_us_kernerl * 100ULL ) / elapsed_us;
        part_kernel =
            ( ( ( used_us_kernerl * 100ULL ) - ( whole_kernel * elapsed_us ) ) *
              10ULL ) /
            elapsed_us;
    }

    m_cpu_usage_user = static_cast<float>( whole_user ) +
                       static_cast<float>( part_user ) / 10.;
    m_cpu_usage_kernel = static_cast<float>( part_kernel ) +
                         static_cast<float>( part_kernel ) / 10.;

    m_time_user   = std::get<0>( usage );
    m_time_kernel = std::get<1>( usage );
}

void task_impl::read_task_info( timeval elapsed, bool new_task )
{
    m_pid = m_proc.kp_proc.p_pid;

    try
    {
        process_proc_args();
        auto usage = process_task_data();
        process_cpu_usage( new_task, elapsed, usage );
    }
    catch( boost::exception &ex )
    {
        // TODO: log error
    }
}

void task_impl::refresh( std::uintmax_t stamp, timeval elapsed,
                         const kinfo_proc &proc )
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

void task_impl::dump( std::ostream &os ) const
{
    os << "class Task(0x" << std::hex << (long)this << std::dec << ") \n{\n"
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
