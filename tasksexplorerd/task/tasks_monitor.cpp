#include "tasks_monitor.h"


TasksMonitor::TasksMonitor(mach_port_t hostPort)
    : m_hostPort(hostPort)
{
}

TasksMonitor::~TasksMonitor()
{
}

TasksMonitor::TasksMapPtr TasksMonitor::GetTasksSnapshot()
{
    return TasksMapPtr();
}

