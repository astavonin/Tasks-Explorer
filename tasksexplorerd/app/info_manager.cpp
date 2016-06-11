#include "info_manager.h"

InfoManager::InfoManager()
    : m_hostPort( mach_host_self() )
    , m_log( spdlog::stdout_logger_mt( "console", true ) )
    //, m_tasksMonitor( new tasks::TasksMonitorImpl( m_hostPort, m_log ) )
{
}

InfoManager::~InfoManager()
{
}

tasks_map_ptr InfoManager::GetTasksSnapshot()
{
    return tasks_map_ptr();
    //return m_tasksMonitor->GetTasks();
}
