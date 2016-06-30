#pragma once

#include <string>
#include <future>
#include <dispatch/dispatch.h>

namespace utils
{
std::string exec_dir();
void hex_dump( const char* desc, const void* addr, int len );

template <class Function, class... Args>
std::future<std::result_of_t<std::decay_t<Function>( std::decay_t<Args>... )>>
async( Function&& f, Args&&... args )
{
    using result_t   = typename std::result_of<Function( Args... )>::type;
    using packaged_t = typename std::packaged_task<result_t()>;

    auto task = new packaged_t( std::bind( std::forward<Function>( f ),
                                           std::forward<Args>( args )... ) );
    auto res = task->get_future();
    dispatch_async_f(
        dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0 ), task,
        []( void* task_ptr ) {
            auto smart_task_ptr = std::unique_ptr<packaged_t>(
                static_cast<packaged_t*>( task_ptr ) );

            ( *smart_task_ptr )();
        } );
    return res;
}
}
