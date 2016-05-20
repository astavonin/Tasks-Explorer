#pragma once

#include <map>
#include <memory>
#include <sys/types.h>
#include <mach/mach.h>
#include "tasks.h"



using TasksMapPtr = TasksMonitor::TasksMapPtr;

class InfoManager
{
public:

    InfoManager();
    ~InfoManager();

    TasksMapPtr GetTasksSnapshot();

private:
    using TasksMonitorPtr = std::unique_ptr<TasksMonitor>;

    mach_port_t m_hostPort;

    TasksMonitorPtr m_tasksMonitor;
};

