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
    Task( const kinfo_proc& proc, logger_ptr logger );

    void Refresh( const kinfo_proc& proc );

    friend std::ostream& operator<<( std::ostream& os, const Task& t );

private:
    void ReadTaskInfo();

    inline pid_t GetPID() const;

private:
    const kinfo_proc m_proc;

    pid_t                     m_pid;
    std::unique_ptr<ProcArgs> m_args;
    logger_ptr                m_log;
};

std::ostream& operator<<( std::ostream& os, const Task& t );
}
