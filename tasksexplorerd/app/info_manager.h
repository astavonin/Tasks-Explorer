#pragma once

#include <mach/mach.h>
#include <sys/types.h>
#include <map>
#include <memory>
#include "logger.h"
#include "tasks_monitor.h"

using tasks_map_ptr = tasks::tasks_monitor::tasks_map_ptr;

class InfoManager
{
public:
    InfoManager();
    ~InfoManager();

    tasks_map_ptr GetTasksSnapshot();

private:
    using tasks_monitor_ptr = std::unique_ptr<tasks::tasks_monitor>;

    mach_port_t m_hostPort;
    logger_ptr  m_log;

    tasks_monitor_ptr m_tasks_monitor;
};
