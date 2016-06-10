#pragma once

#include <sys/sysctl.h>
#include <iostream>
#include <logger.hpp>
#include <string>
#include <vector>

namespace tasks
{
class ProcArgs;

class Task
{
public:
    Task( std::uintmax_t stamp, const kinfo_proc& proc, logger_ptr logger );
    ~Task();
    Task( const Task&& ) = delete;
    Task&& operator=( const Task&& ) = delete;
    Task( const Task& )              = delete;
    Task& operator=( const Task& ) = delete;

    void Refresh( std::uintmax_t stamp, const kinfo_proc& proc );
    inline std::uintmax_t GetStamp() const
    {
        return m_stamp;
    }

    friend std::ostream& operator<<( std::ostream& os, const Task& t );

private:
    void ReadTaskInfo();

    inline pid_t GetPID() const
    {
        return m_pid;
    }

private:
    const kinfo_proc m_proc;

    pid_t                    m_pid;
    std::string              m_fullPathName;
    std::string              m_appName;
    std::vector<std::string> m_argv;
    std::unordered_map<std::string, std::string> m_env;

    std::uintmax_t m_stamp;
    logger_ptr     m_log;
};

std::ostream& operator<<( std::ostream& os, const Task& t );
}
