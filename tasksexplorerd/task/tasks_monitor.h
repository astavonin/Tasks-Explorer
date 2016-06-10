#pragma once

#include <mach/mach.h>
#include <logger.hpp>
#include <map>
#include "system_helpers.h"
#include "task.h"

namespace tasks
{
class TasksMonitor
{
public:
    using TaskPtr     = std::shared_ptr<Task>;
    using TasksMap    = std::map<pid_t, TaskPtr>;
    using TasksMapPtr = std::shared_ptr<TasksMap>;

    TasksMonitor( mach_port_t hostPort, logger_ptr logger );
    ~TasksMonitor();
    TasksMonitor( const TasksMonitor&& ) = delete;
    TasksMonitor&& operator=( const TasksMonitor&& ) = delete;
    TasksMonitor( const TasksMonitor& )              = delete;
    TasksMonitor& operator=( const TasksMonitor& ) = delete;

    TasksMapPtr GetTasks();

private:
    void RefreshTasksList( const proc_info_vec& procs );

private:
    TasksMapPtr m_tasks;
    mach_port_t m_hostPort;

    logger_ptr     m_log;
    std::uintmax_t m_stamp;
};
}
