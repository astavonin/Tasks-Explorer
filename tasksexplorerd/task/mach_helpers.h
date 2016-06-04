#pragma once

#include <mach/mach.h>

struct mach_port_deleter
{
    mach_port_name_t port = 0;

    mach_port_deleter( const mach_port_deleter& ) = delete;
    mach_port_deleter& operator=( const mach_port_deleter& ) = delete;
    mach_port_deleter( mach_port_deleter&& )                 = delete;

    ~mach_port_deleter();
};

struct mach_array_deleter
{
    mach_port_name_t* arr = nullptr;
    size_t            len = 0;

    mach_array_deleter( const mach_array_deleter& ) = delete;
    mach_array_deleter& operator=( const mach_array_deleter& ) = delete;
    mach_array_deleter( mach_array_deleter&& )                 = delete;

    ~mach_array_deleter();
};
