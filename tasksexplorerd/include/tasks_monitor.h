#pragma once

#include <sys/sysctl.h>
#include <map>
#include "dumpable.h"
#include "logger.h"

namespace tasks
{
class task;
using task_ptr      = std::shared_ptr<task>;
using tasks_map     = std::map<pid_t, task_ptr>;
using tasks_map_ptr = std::shared_ptr<tasks_map>;

class tasks_monitor : public common::dumpable
{
public:
    tasks_monitor()
    {
    }
    virtual ~tasks_monitor()
    {
    }
    tasks_monitor( const tasks_monitor&& ) = delete;
    tasks_monitor&& operator=( const tasks_monitor&& ) = delete;
    tasks_monitor( const tasks_monitor& )              = delete;
    tasks_monitor& operator=( const tasks_monitor& ) = delete;

    virtual tasks_map_ptr active_tasks() = 0;
};

using tasks_monitor_ptr = std::unique_ptr<tasks_monitor>;
tasks_monitor_ptr create_tasks_monitor( mach_port_t hostPort,
                                        logger_ptr  logger );
}
