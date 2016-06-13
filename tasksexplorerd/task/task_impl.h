#pragma once

#include <sys/sysctl.h>
#include <string>
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
    task_impl( std::uintmax_t stamp, const kinfo_proc& proc,
               logger_ptr logger );
    virtual ~task_impl();

    virtual std::string              name() const override;
    virtual std::string              path_name() const override;
    virtual pid_t                    pid() const override;
    virtual std::vector<std::string> argv() const override;
    virtual std::unordered_map<std::string, std::string> envv() const override;

    virtual void dump( std::ostream& os ) const override;

    // library internal functions
    void refresh( std::uintmax_t stamp, const kinfo_proc& proc );
    inline std::uintmax_t stamp() const
    {
        return m_stamp;
    }

private:
    void read_task_info();

private:
    pid_t                    m_pid;
    std::string              m_path_name;
    std::string              m_app_name;
    std::vector<std::string> m_argv;
    std::unordered_map<std::string, std::string> m_env;

    const kinfo_proc m_proc;
    std::uintmax_t   m_stamp;
    logger_ptr       m_log;
};
}
