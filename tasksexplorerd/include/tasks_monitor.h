#pragma once

#include <sys/sysctl.h>
#include <map>
#include "dumpable.h"

namespace tasks
{
class task_impl;

class tasks_monitor : public common::dumpable
{
public:
    using task_ptr      = std::shared_ptr<task_impl>;
    using tasks_map     = std::map<pid_t, task_ptr>;
    using tasks_map_ptr = std::shared_ptr<tasks_map>;

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
}
