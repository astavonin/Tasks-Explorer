#include "info_manager.h"

info_manager::info_manager()
    : m_hostPort( mach_host_self() )
    , m_log( spdlog::stdout_logger_mt( "console", true ) )
    , m_tasks_monitor( tasks::create_tasks_monitor( m_hostPort, m_log ) )
{
}

info_manager::~info_manager()
{
}

tasks_map_ptr info_manager::active_tasks()
{
    return m_tasks_monitor->active_tasks();
}
