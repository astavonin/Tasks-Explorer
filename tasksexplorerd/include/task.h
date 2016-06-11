#pragma once

#include <string>
#include "dumpable.h"

namespace tasks
{
class task : public common::dumpable
{
public:
    task() { }
    virtual ~task() { }
    task( const task&& ) = delete;
    task&& operator=( const task&& ) = delete;
    task( const task& )              = delete;
    task& operator=( const task& ) = delete;

    virtual std::string name() const = 0;
};
}
