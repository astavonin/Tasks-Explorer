#pragma once

#include "logger.h"
#include "system_helpers.h"
#include "tasks_monitor.h"
#include <future>
#include <list>
#include <mach/mach.h>
#include <map>

namespace tasks
{
class task_impl;

class tasks_monitor_impl : public tasks_monitor
{
public:
    tasks_monitor_impl( mach_port_t hostPort, logger_ptr logger );
    virtual ~tasks_monitor_impl();

    virtual void dump( std::ostream &os ) const override;

    virtual tasks_map_ptr active_tasks() override;

private:
    using tasks_refresh_list =
        std::list<std::future<std::shared_ptr<task_impl>>>;

    void refresh_tasks( tasks_refresh_list &tl );
    void update_tasks_map( tasks_refresh_list &tl );

    tasks_map_ptr m_tasks;
    mach_port_t   m_hostPort;

    logger_ptr     m_log;
    std::uintmax_t m_stamp;
    timeval        m_refresh_time;
};
}
