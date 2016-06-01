#pragma once

#include <mach/mach.h>
#include <spdlog/spdlog.h>
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

    TasksMonitor(mach_port_t hostPort);
    ~TasksMonitor();

    TasksMapPtr GetTasksSnapshot();

private:
    mach_port_t                     m_hostPort;
    std::shared_ptr<spdlog::logger> m_log;
};
}
