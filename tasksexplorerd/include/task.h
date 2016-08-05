#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "dumpable.h"

namespace tasks
{
class task : public common::dumpable
{
public:
    task()
    {
    }
    virtual ~task()
    {
    }
    task( const task&& ) = delete;
    task&& operator=( const task&& ) = delete;
    task( const task& )              = delete;
    task& operator=( const task& ) = delete;

    virtual std::string              name() const      = 0;
    virtual std::string              path_name() const = 0;
    virtual pid_t                    pid() const       = 0;
    virtual std::vector<std::string> argv() const      = 0;
    virtual std::unordered_map<std::string, std::string> envv() const = 0;
    virtual float cpu_usage_user() const     = 0;
    virtual float cpu_usage_kernel() const = 0;
    virtual int   real_mem_size() const    = 0;
    virtual int   virtual_mem_size() const = 0;
};
}
