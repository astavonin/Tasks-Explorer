#pragma once

#include <sys/sysctl.h>
#include <ctime>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "logger.h"
#include "task.h"

namespace tasks
{
class proc_args;

class task_impl : public task
{
public:
    task_impl( std::uintmax_t stamp, timeval elapsed, const kinfo_proc &proc,
               logger_ptr logger );
    virtual ~task_impl();

    virtual std::string              name() const override;
    virtual std::string              path_name() const override;
    virtual pid_t                    pid() const override;
    virtual std::vector<std::string> argv() const override;
    virtual std::unordered_map<std::string, std::string> envv() const override;
    virtual float cpu_usage_user() const override;
    virtual float cpu_usage_kernel() const override;
    virtual int   real_mem_size() const override;
    virtual int   virtual_mem_size() const override;

    virtual void dump( std::ostream &os ) const override;

    // library internal functions
    void refresh( std::uintmax_t stamp, timeval elapsed,
                  const kinfo_proc &proc );
    inline std::uintmax_t stamp() const
    {
        return m_stamp;
    }

private:
    void read_task_info( timeval elapsed, bool new_task = false );
    using usage_info = std::tuple<timeval, timeval>;
    usage_info process_task_data();
    void       process_proc_args();
    void process_cpu_usage( bool new_task, timeval elapsed, usage_info usage );

private:
    pid_t                    m_pid;
    std::string              m_path_name;
    std::string              m_app_name;
    std::vector<std::string> m_argv;
    std::unordered_map<std::string, std::string> m_env;
    timeval m_time_user;
    timeval m_time_kernel;
    float   m_cpu_usage_user;
    float   m_cpu_usage_kernel;
    int     m_real_mem_size;
    int     m_virtual_mem_size;

    const kinfo_proc m_proc;
    std::uintmax_t   m_stamp;
    logger_ptr       m_log;
};
}
