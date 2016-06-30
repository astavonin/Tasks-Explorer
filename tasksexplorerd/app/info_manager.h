#pragma once

#include <mach/mach.h>
#include <sys/types.h>
#include <map>
#include <memory>
#include "logger.h"
#include "tasks_monitor.h"

using tasks_map_ptr = tasks::tasks_map_ptr;

class info_manager
{
public:
    info_manager();
    ~info_manager();

    tasks_map_ptr active_tasks();

private:
    using tasks_monitor_ptr = tasks::tasks_monitor_ptr;

    mach_port_t m_hostPort;
    logger_ptr  m_log;

    tasks_monitor_ptr m_tasks_monitor;
};
