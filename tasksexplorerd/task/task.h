#pragma once

#include <sys/sysctl.h>
#include <iostream>
#include <logger.hpp>
#include <string>
#include <vector>

namespace tasks
{
class Task
{
public:
    Task( const kinfo_proc& proc, logger_ptr logger );
    virtual ~Task();

    void Refresh( const kinfo_proc& proc );

    friend std::ostream& operator<<( std::ostream& os, const Task& t );

private:
    void ReadTaskInfo();

    inline pid_t GetPID() const;

private:
    const kinfo_proc m_proc;

    pid_t                    m_pid;
    std::string              m_name;
    std::vector<std::string> m_argv;
    std::vector<std::string> m_envv;
    std::string              m_path2exec;
    std::string              m_path2app;
    std::string              m_bundlePathName;

    logger_ptr m_log;
};

std::ostream& operator<<( std::ostream& os, const Task& t );
}
