#include "info_manager.h"

InfoManager::InfoManager()
    : m_hostPort( mach_host_self() )
    , m_log( spdlog::stdout_logger_mt( "console", true ) )
    , m_tasksMonitor( new tasks::TasksMonitor( m_hostPort, m_log ) )
{
}

InfoManager::~InfoManager()
{
}

TasksMapPtr InfoManager::GetTasksSnapshot()
{
    return m_tasksMonitor->GetTasks();
}
