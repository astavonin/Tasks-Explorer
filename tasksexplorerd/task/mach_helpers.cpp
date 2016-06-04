#include <assert.h>
#include "mach_helpers.h"

mach_port_deleter::~mach_port_deleter()
{
    assert( port != 0 );

    mach_port_deallocate( mach_task_self(), port );
}

mach_array_deleter::~mach_array_deleter()
{
    assert( arr != nullptr );
    assert( len > 0 );

    auto task_self = mach_task_self();

    for( auto i = 0; i < len; ++i ) mach_port_deallocate( task_self, arr[i] );

    vm_deallocate( task_self, (vm_address_t)arr,
                   len * sizeof( mach_port_name_t ) );
}
