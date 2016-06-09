#pragma once

#include <mach/mach.h>
#include <logger.hpp>
#include <map>
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

    TasksMap GetTasks();

private:
    TasksMap m_tasks;
    mach_port_t m_hostPort;
    logger_ptr  m_log;
};
}
