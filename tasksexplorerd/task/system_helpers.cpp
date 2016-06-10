#include "system_helpers.h"
#include <stdio.h>
#include <utils.h>
#include <prettyprint.hpp>
#include <string>
#include <unordered_map>

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

boost::optional<std::vector<char>> ReadProcArgs( pid_t pid, logger_ptr log )
{
    boost::optional<std::vector<char>> res;

    if( pid == 0 )  // we will not be able to extract data for kernel_task
        return res;

    static int argmax = [&log]() -> int {
        int    name[] = {CTL_KERN, KERN_ARGMAX, 0};
        int    argmax = 0;
        size_t size   = sizeof( argmax );

        int ret = sysctl( name, 2, &argmax, &size, nullptr, 0 );
        if( ret != 0 )
        {
            log->error( "{}: unable to get argmax, will use 1MB instead",
                        __func__ );
            argmax = 1024 * 1024;
        }
        return argmax;
    }();

    std::vector<char> procargv( argmax );
    int               name[] = {CTL_KERN, KERN_PROCARGS2, pid};
    size_t            size   = argmax;

    int err = sysctl( name, 3, &procargv[0], &size, nullptr, 0 );
    if( err != 0 )
    {
        log->warn( "{}: unable to get environment for PID {}", __func__, pid );
        procargv.resize( 0 );
        res = procargv;
    }

    return res;
}

ProcArgs ParseProcArgs( const std::vector<char> &procargv, logger_ptr /*log*/ )
{
    ProcArgs parsedArgs;

    if( procargv.size() < sizeof( int ) )
        return parsedArgs;

    const char *all_arguments = &procargv[0];
    int         argc          = *( (int *)all_arguments );
    parsedArgs.fullPathName.assign( all_arguments + sizeof( argc ) );

    static const char app[]    = ".app";
    auto              appBegin = parsedArgs.fullPathName.rfind( app );
    if( appBegin != std::string::npos )
    {
        auto nameBegin = parsedArgs.fullPathName.rfind( "/", appBegin ) + 1;
        parsedArgs.appName.assign( parsedArgs.fullPathName, nameBegin,
                                   appBegin - nameBegin + sizeof( app ) - 1 );
    }
    else
    {
        auto execBegin = parsedArgs.fullPathName.rfind( "/" ) + 1;
        parsedArgs.appName.assign( parsedArgs.fullPathName, execBegin,
                                   appBegin - execBegin );
    }

    all_arguments += sizeof( argc ) + parsedArgs.fullPathName.length();

    while( *( ++all_arguments ) != '\0' )
    {
    }
    while( *( ++all_arguments ) == '\0' )
    {
    }

    if( argc > 0 )
    {
        const char *ptr = all_arguments;
        for( int i = 0; i < argc; ++i )
        {
            std::string arg( ptr );
            ptr += arg.length() + 1;
            parsedArgs.argv.push_back( std::move( arg ) );
        }
        all_arguments = ptr;
    }

    all_arguments--;
    do
    {
        if( *all_arguments == '\0' )
        {
            all_arguments++;
            if( *all_arguments == '\0' )
                break;
            else
            {
                auto pos = strchr( all_arguments, '=' );
                parsedArgs.env.emplace(
                    std::string( all_arguments, pos - all_arguments ),
                    std::string( pos + 1 ) );
            }
        }
        all_arguments++;
    } while( true );

    return parsedArgs;
}

std::ostream &operator<<( std::ostream &os, const ProcArgs &p )
{
    os << "struct ProcArgs(" << std::hex << &p << std::dec << ") \n{\n"
       << "appName: " << p.appName << "\n"
       << "fullPathName: " << p.fullPathName << "\n"
       << "argv(" << p.argv.size() << "):" << p.argv << "\n"
       << "env(" << p.env.size() << "):" << p.env << "\n"
       << "}";

    return os;
}
}
