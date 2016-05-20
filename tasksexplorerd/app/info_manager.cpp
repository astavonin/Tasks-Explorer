#include "info_manager.h"

InfoManager::InfoManager()
    : m_hostPort(mach_host_self())
    , m_tasksMonitor(new TasksMonitor(m_hostPort))
{
}

InfoManager::~InfoManager()
{
}

TasksMapPtr InfoManager::GetTasksSnapshot()
{
    return m_tasksMonitor->GetTasksSnapshot();
}

