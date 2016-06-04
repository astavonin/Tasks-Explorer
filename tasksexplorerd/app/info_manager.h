#pragma once

#include <map>
#include <memory>
#include <sys/types.h>
#include <mach/mach.h>

#include <logger.hpp>

#include "tasks.h"



using TasksMapPtr = tasks::TasksMonitor::TasksMapPtr;

class InfoManager
{
public:

    InfoManager();
    ~InfoManager();

    TasksMapPtr GetTasksSnapshot();

private:
    using TasksMonitorPtr = std::unique_ptr<tasks::TasksMonitor>;

    mach_port_t m_hostPort;
    logger_ptr m_log;

    TasksMonitorPtr m_tasksMonitor;
};

