/*-
 * Copyright 2009, Mac OS X Internals. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY Mac OS X Internals ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Mac OS X Internals OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Mac OS X Internals.
 */

#ifndef __COMMON_DATA_TYPES__
#define __COMMON_DATA_TYPES__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>

struct stack_info {
    unsigned long return_addr;
    unsigned long frame_addr;
    char *func_name;
};
typedef struct stack_info stack_info_t;

struct thread_record
{
    enum run_state_t 
    {
        TH_RUNNING = TH_STATE_RUNNING,
        TH_STOPPED = TH_STATE_STOPPED,
        TH_WAITING = TH_STATE_WAITING,
        TH_UNINTERRUPTIBLE = TH_STATE_UNINTERRUPTIBLE,
        TH_HALTED = TH_STATE_HALTED
    } run_state;
     
    time_value_t   user_time;
    time_value_t   system_time;
    int            suspend_count;
    int            sleep_time;
    int            flags;
    uint64_t       thread_id;
    int            stack_records_count;
    stack_info_t   **call_stack;
};
typedef struct thread_record thread_record_t;

struct task_record 
{
     uid_t               uid;
     pid_t               pid;
     pid_t               ppid;
     
     int                 real_mem_size;
     int                 virtual_mem_size;

     /* Number of threads. */
     uint32_t       threads;
     uint32_t       running_threads;
     thread_record_t **threads_arr;

     /* Number of ports. */
     uint32_t       ports;

     /* executable name, process arguments, environment variables. */
     char           *app_name;
     char           **argv;
     int            argc;
     char           **envv;
     int            envc;
     char           *path_to_executable;
     char           *path_to_boundle;
     char           *bundle_path_name;

     /* flags for managing killed processes*/
     int                 seq;
     
     int                 cpu_usage_whole_kernel;
     int                 cpu_usage_part_kernel;
     int                 cpu_usage_whole_user;
     int                 cpu_usage_part_user;

     cpu_type_t     cputype;
     
     struct timeval      time_kernel;
     struct timeval      p_time_kernel;
     struct timeval      time_user;
     struct timeval      p_time_user;
};
typedef struct task_record task_record_t;

struct host_record 
{
     /*
      * Sample sequence number, incremented for every sample.  The first
      * sample has a sequence number of 1.
      */
     uint32_t       seq;

     /* Number of processes. */
     uint32_t       nprocs;
     
     /* CPU loads. */
     host_cpu_load_info_data_t cpu;
     host_cpu_load_info_data_t p_cpu;

	u_int cpu_user;
	u_int cpu_kernel;

     /* Previous sample time, and current sample time. */
     struct timeval      time;
     struct timeval      p_time;
     
     /* Total number of threads. */
     uint32_t       threads;
     
     /* Physical memory size. */
     uint64_t       memsize;

     /* Memory statistics. */
     /* Network statistics. */
     /* Disk statistics. */
};
typedef struct host_record host_record_t;

#endif //__COMMON_DATA_TYPES__
