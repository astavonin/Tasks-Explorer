#pragma once

#include <sys/types.h>
#include <memory>

class Task
{
private:
    pid_t m_pid;

public:
    Task();
    virtual ~Task();
};

