#include <assert.h>
#include <mach/host_priv.h>
#include <functional>
#include <iostream>

#include <errors.hpp>
#include "tasks_monitor.h"

namespace tasks
{
TasksMonitor::TasksMonitor( mach_port_t hostPort ) : m_hostPort( hostPort )
{
    assert( m_hostPort > 0 );
}

TasksMonitor::~TasksMonitor()
{
}

struct mach_port_deleter
{
    mach_port_name_t port;

    mach_port_deleter( const mach_port_deleter& ) = delete;
    mach_port_deleter& operator=( const mach_port_deleter& ) = delete;
    mach_port_deleter( mach_port_deleter&& )                 = delete;

    ~mach_port_deleter()
    {
        mach_port_deallocate( mach_task_self(), port );
    }
};

struct mach_array_deleter
{
    mach_port_name_t* arr;
    size_t            len;

    mach_array_deleter( const mach_array_deleter& ) = delete;
    mach_array_deleter& operator=( const mach_array_deleter& ) = delete;
    mach_array_deleter( mach_array_deleter&& )                 = delete;

    ~mach_array_deleter()
    {
        auto task_self = mach_task_self();

        for( auto i = 0; i < len; ++i )
            mach_port_deallocate( task_self, arr[i] );

        vm_deallocate( task_self, (vm_address_t)arr,
                       len * sizeof( mach_port_name_t ) );
    }
};

TasksMonitor::TasksMapPtr TasksMonitor::GetTasksSnapshot()
{
    processor_set_t            pset;
    processor_set_name_array_t psets;
    mach_msg_type_number_t     pcnt, tcnt;
    auto                       task_self = mach_task_self();

    kern_return_t kr = host_processor_sets( m_hostPort, &psets, &pcnt );
    if( kr != KERN_SUCCESS )
    {
        BOOST_THROW_EXCEPTION( err::sys_api_error()
                               << err::api_function( "host_processor_sets" )
                               << err::mach_error( kr ) );
    }
    mach_array_deleter psets_deleter{psets, pcnt};

    for( int i = 0; i < pcnt; i++ )
    {
        kr = host_processor_set_priv( m_hostPort, psets[i], &pset );
        if( kr != KERN_SUCCESS )
        {
            BOOST_THROW_EXCEPTION(
                err::sys_api_error()
                << err::api_function( "host_processor_set_priv" )
                << err::mach_error( kr ) );
        }
        mach_port_deleter pset_deleter{pset};

        task_array_t tasks;
        kr = processor_set_tasks( pset, &tasks, &tcnt );
        if( kr != KERN_SUCCESS )
        {
            BOOST_THROW_EXCEPTION( err::sys_api_error()
                                   << err::api_function( "processor_set_tasks" )
                                   << err::mach_error( kr ) );
        }
        mach_array_deleter tasks_deleter{tasks, tcnt};

        // for (j = 0; j < tcnt; j++)
        // read_task_info(tasks[j]);
        kr = mach_port_deallocate( mach_task_self(), pset );
    }

    return TasksMapPtr();
}
}
