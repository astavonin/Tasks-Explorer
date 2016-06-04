#include "system_helpers.h"

namespace tasks
{
proc_info_vec GetKinfoProcs()
{
    static size_t maxProcsCount = 500;

    bool             done = false;
    proc_info_vec    procs( maxProcsCount );
    static const int name[] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};

    do
    {
        size_t length = maxProcsCount * sizeof( kinfo_proc );
        sysctl( (int *)name, ( sizeof( name ) / sizeof( *name ) ) - 1,
                &procs[0], &length, nullptr, 0 );

        if( length != 0 )
        {
            done = true;
            procs.resize( length / sizeof( kinfo_proc ) );
        }
        else
        {
            sysctl( (int *)name, ( sizeof( name ) / sizeof( *name ) ) - 1,
                    nullptr, &length, nullptr, 0 );

            maxProcsCount = length / sizeof( kinfo_proc ) * 1.5;
            procs.resize( maxProcsCount );
        }
    } while( !done );

    return procs;
}
}
