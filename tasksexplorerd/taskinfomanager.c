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

#include "afx.h"

#include <mach/host_priv.h>
#include <sys/sysctl.h>
#include "data_types.h"
#include "tasks_explorer.h"
#include "man_info.h"
#include "stack_info.h"

#include "taskinfomanager.h"


#define   TIME_VALUE_TO_TIMEVAL(a, r) do {   \
     (r)->tv_sec = (a)->seconds;                  \
     (r)->tv_usec = (a)->microseconds;       \
} while (0)

/****************************************************************************/
// static variables and data types block
/****************************************************************************/

static mach_port_t task_manager_port;
static CFMutableDictionaryRef tasks_dict;
static host_record_t host_info_record={};
natural_t processor_count;

struct app_path_name {
     char*     app_name; 
     char**    argv;
     int       argc;
     char**    envv;
     int       envc;
     char*     path_to_executable;
     char*     path_to_app;
     char*     bundle_path_name;
};
typedef struct app_path_name app_path_name_t;

/****************************************************************************/
// internal functions declarations block
/****************************************************************************/

static int read_tasks_table(void);
static int read_task_info(task_t);
static int get_kinfo_for_pid(pid_t pid, struct kinfo_proc *kinfo);
static int update_task_from_kinfo(const struct kinfo_proc *kinfo, task_record_t *tinfo);
static Boolean KeysCompareFunc(const void *left, const void *right);
static CFHashCode KeysHashFunc(const void *value);
static void KeyFreeFunc(CFAllocatorRef allocator, const void *value);
static int get_task_name(const struct kinfo_proc *kinfo, task_record_t *tinfo);
static int update_threads_info(task_t task, task_record_t *tinfo);
static void free_task_record(task_record_t *task);
static int read_host_info();
static void extract_app_name(const char *all_arguments, app_path_name_t *path_name);
static char** strings_to_array(const char* strings, const int count);
static void build_tasks_array(const void *key, const void *data, void *context);
static void free_arr(void** arr, size_t len);
static int remove_killed_process();
static int cpu_type_for_pid(pid_t pid, cpu_type_t *cputype);
static int get_mach_ports(task_t task, task_record_t *tinfo);
static int update_cpu_info(task_record_t *task, int new_task);
static int update_task_from_task_info(task_t task, task_record_t *task_info);

/****************************************************************************/
// exported functions implementation block
/****************************************************************************/

host_record_t* task_manager_get_host_info()
{
	return &host_info_record;
}

/// Initialize Task Info Manager internal data.
/// @return Upon successful completion 0 is returned. 
int task_info_manager_init(void) 
{
    static int completed = FALSE;
    if (completed == TRUE) {
        return 0;
    }

    kern_return_t kr;
    host_priv_t host_priv;
    processor_port_array_t processor_list;

    task_manager_port = mach_host_self();
    kr = host_get_host_priv_port(task_manager_port, &host_priv);
    if (KERN_SUCCESS == kr) {
        host_processors(host_priv, &processor_list, &processor_count);
    }

    CFDictionaryKeyCallBacks tasksKeysCallBask = {0, NULL, NULL, NULL, KeysCompareFunc, KeysHashFunc};
    CFDictionaryValueCallBacks tasksValueCallBask = {0, NULL, KeyFreeFunc, NULL, KeysCompareFunc};
    tasks_dict = CFDictionaryCreateMutable(NULL, 0, &tasksKeysCallBask, &tasksValueCallBask);

    host_info_record.seq = 1;

    int mib[2] = {CTL_HW, HW_MEMSIZE};
    size_t size;

    size = sizeof(host_info_record.memsize);
    if (sysctl(mib, 2, &host_info_record.memsize, &size, NULL, 0) == -1) {
        syslog(LOG_WARNING, "%s, Error in sysctl(): %m", __FUNCTION__);
        return -1;
    }

    gettimeofday(&host_info_record.time, NULL);

    man_info_init();
	stack_info_init();

    completed = TRUE;

    return 0;
}

/// The call deallocate all allocated data, close all opened desckriptors.
void task_info_manager_free(void)
{
    CFDictionaryRemoveAllValues(tasks_dict);
    man_info_free();
}

/// The call update active tasks information.
int task_info_manager_update()
{
     int ret;

     host_info_record.seq++;
	host_info_record.cpu_user = 0;
	host_info_record.cpu_kernel = 0;

     ret = read_host_info();
     if (ret != 0) {
          return ret;
     }
     ret = read_tasks_table();
     if (ret != 0) {
          return ret;
     }
     ret = remove_killed_process();
     if (ret != 0) {
          return ret;
     }
     return 0;
}

/// active tasks count are returns.
size_t task_info_manager_get_tasks_count()
{
     return CFDictionaryGetCount(tasks_dict);
}

/// internal structure for collecting pid list.
struct pid_builder_struct {
     int pos;
     void *array;
};
typedef struct pid_builder_struct pid_builder_struct_t;

/// iterate all tasks and fill pidArr array with tasks' pid
/// @param pidArr [out] array for storing tasks' pid.
void task_info_manager_get_tasks_pid(pid_t *pidArr)
{
     struct pid_builder_struct pid_builder = {};
     pid_builder.array = pidArr;
     
     CFDictionaryApplyFunction(tasks_dict, build_tasks_array, (void*)&pid_builder);
}

/****************************************************************************/
// internal functions implementations block
/****************************************************************************/

/// the function used with CFDictionaryApplyFunction for building active tasks list
/// @param key [in] the key value associated with the current key-value pair
/// @param value [in] the value associated with the current key-value pair
/// @param array [out] pointer on pid_builder_struct structure
static void build_tasks_array(const void *key, const void *data, void *array)
{
     task_record_t *task = (task_record_t*)data;
     pid_builder_struct_t *arr_helper = (pid_builder_struct_t*)array;
     pid_t *pid_array = (pid_t*)arr_helper->array;

     pid_array[arr_helper->pos] = task->pid;
     arr_helper->pos++;
}

/// the call builds killed processes list
static void build_killed_array(const void *key, const void *data, void *array)
{
     pid_builder_struct_t *arr_helper = (pid_builder_struct_t*)array;
     task_record_t *task = (task_record_t*)data;
     task_record_t **tasks_array = (task_record_t**)arr_helper->array;

     if (task->seq != host_info_record.seq) {
		   tasks_array[arr_helper->pos] = task;
		   arr_helper->pos++;
     }
}

/// the call remove all kelled process from process list
static int remove_killed_process()
{
     int ret, i;
     struct pid_builder_struct pid_builder = {};
     task_record_t **killed_list[MAX_TASKS];
     pid_builder.array = &killed_list;

     CFDictionaryApplyFunction(tasks_dict, build_killed_array, (void*)&pid_builder);
     for (i = 0; i < pid_builder.pos; i++) {
          CFDictionaryRemoveValue(tasks_dict, killed_list[i]);
     }

     return ret;
}

/// update information for current host
/// @return Upon successful completion 0 is returned.
static int read_host_info()
{
     host_info_record.p_time = host_info_record.time;
     gettimeofday(&host_info_record.time, NULL);

     return 0;
}

/// Iterate all tasks and update/create their information.
/// @return The call returns -1 on error, 0 indicate successful completion.
static int read_tasks_table(void) 
{
     kern_return_t kr;
     processor_set_t     pset;
     task_array_t tasks;
     processor_set_name_array_t psets;
     mach_msg_type_number_t   i, j, pcnt, tcnt;

     kr = host_processor_sets(task_manager_port, &psets, &pcnt);
    if (kr != KERN_SUCCESS) {
          syslog(LOG_ERR, "error in host_processor_sets(): %s", mach_error_string(kr));
          return -1;
     }

     for (i = 0; i < pcnt; i++) {
          kr = host_processor_set_priv(task_manager_port, psets[i], &pset);
          if (kr != KERN_SUCCESS) {
               syslog(LOG_ERR, "error in host_processor_set_priv(): %s", mach_error_string(kr));
               return -1;
          }

          kr = processor_set_tasks(pset, &tasks, &tcnt);
          if (kr != KERN_SUCCESS) {
               syslog(LOG_ERR, "error in processor_set_tasks(): %s", mach_error_string(kr));
               return -1;
          }

          for (j = 0; j < tcnt; j++) {
               read_task_info(tasks[j]);

               kr = mach_port_deallocate(mach_task_self(), tasks[j]);
               if (kr != KERN_SUCCESS) {
                    syslog(LOG_WARNING, "%s, error in mach_port_deallocate(): %s", __FUNCTION__, mach_error_string(kr));
               }
          }
          kr = vm_deallocate(mach_task_self(), (vm_address_t)tasks, tcnt * sizeof(processor_set_t));
          kr = mach_port_deallocate(mach_task_self(), pset);
          kr = mach_port_deallocate(mach_task_self(), psets[i]);
     }
     kr = vm_deallocate(mach_task_self(), (vm_address_t)psets, pcnt * sizeof(processor_set_t));

     return 0;
}

/// Update/create task information for given port and insert it into the store.
/// @param [in] the port of the task for witch the information to be stored.
/// @return Upon successful completion 0 is returned.
static int read_task_info(task_t task)
{
    kern_return_t kr;
    int res;
    pid_t pid;
    struct kinfo_proc kinfo;
    task_record_t *task_info, *tmp_info;
    int new_task = FALSE;

    do {
        kr = pid_for_task(task, &pid);
        if (kr != KERN_SUCCESS) {
           syslog(LOG_WARNING, "error in pid_for_task(%i): %s", task, mach_error_string(kr));
           break;
        }

        res = get_kinfo_for_pid(pid, &kinfo);
        if (res != 0 ) {
           syslog(LOG_WARNING, "error in get_kinfo_for_pid(%i): %s", task, mach_error_string(kr));
           break;
        }

        task_info = task_info_manager_find_task(pid);
        if (task_info == NULL) {
            task_info = malloc(sizeof(task_record_t));
            memset(task_info, 0, sizeof(task_record_t));
            if (task_info == NULL) {
                syslog(LOG_ERR, "%s, error in malloc(): %m", __FUNCTION__);
                break;
           }
            task_info->pid = pid;
            new_task = TRUE;
        }

        res = update_task_from_kinfo(&kinfo, task_info);
        if (res != 0) {
           free_task_record(task_info);
           break;
        }
        res = update_task_from_task_info(task, task_info);
        if (res != 0) {
           free_task_record(task_info);
           break;
        }

        res = update_threads_info(task, task_info);
        if (res != 0) {
           free_task_record(task_info);
           break;
        }

        res = cpu_type_for_pid(task_info->pid, &task_info->cputype);
        if (res != 0) {
           free_task_record(task_info);
           break;
        }

        res = get_mach_ports(task, task_info);
        if (res != 0) {
           free_task_record(task_info);
           break;
        }

        res = update_cpu_info(task_info, new_task);
        if (res != 0) {
           free_task_record(task_info);
           break;
        }

        man_info_add_descr_by_name(task_info->app_name);
        stack_info_update_task_stack(task_info);

        tmp_info = (task_record_t*)CFDictionaryGetValue(tasks_dict, task_info);
        if (tmp_info == NULL) {
            CFDictionaryAddValue(tasks_dict, task_info, task_info);
        }

    } while (FALSE);

    return ((res == 0) ? 0 : -1);
}

static int update_task_from_task_info(task_t task, task_record_t *info)
{
     kern_return_t kr;
     struct task_basic_info_64 ti;
     mach_msg_type_number_t   count;

     count = TASK_BASIC_INFO_64_COUNT;
     kr = task_info(task, TASK_BASIC_INFO_64, (task_info_t)&ti, &count);
     if (kr != KERN_SUCCESS) {
          syslog(LOG_WARNING, "error in task_info(%i): %s", task, mach_error_string(kr));
          return -1;
     }

     info->p_time_user = info->time_user;
     info->p_time_kernel = info->time_kernel;

     struct timeval tv = {0,0};
     TIME_VALUE_TO_TIMEVAL(&ti.user_time, &info->time_user);
     TIME_VALUE_TO_TIMEVAL(&ti.system_time, &info->time_kernel);
     
     info->real_mem_size = ti.resident_size;
     info->virtual_mem_size = ti.virtual_size;

     return 0;
}

static int update_cpu_info(task_record_t *task, int new_task)
{
     int result;
     unsigned long long elapsed_us = 0, used_us_user = 0, used_us_kernerl = 0;
     struct timeval elapsed, used_user, used_kernel;
     int whole_user = 0, part_user = 0, whole_kernel = 0, part_kernel = 0;

     if (new_task) {
          task->p_time_user = task->time_user;
          task->p_time_kernel = task->time_kernel;
     }

     timersub(&host_info_record.time, &host_info_record.p_time, &elapsed);
     timersub(&task->time_user, &task->p_time_user, &used_user);
     timersub(&task->time_kernel, &task->p_time_kernel, &used_kernel);
    elapsed_us = (unsigned long long)elapsed.tv_sec * 1000000ULL
     + (unsigned long long)elapsed.tv_usec;

    used_us_user = (unsigned long long)used_user.tv_sec * 1000000ULL
     + (unsigned long long)used_user.tv_usec;
    used_us_kernerl = (unsigned long long)used_kernel.tv_sec * 1000000ULL
     + (unsigned long long)used_kernel.tv_usec;

    if(elapsed_us > 0) {
          whole_user = (used_us_user * 100ULL) / elapsed_us;
          part_user = (((used_us_user * 100ULL) - (whole_user * elapsed_us)) * 10ULL) / elapsed_us;
          whole_kernel = (used_us_kernerl * 100ULL) / elapsed_us;
          part_kernel = (((used_us_kernerl * 100ULL) - (whole_kernel * elapsed_us)) * 10ULL) / elapsed_us;
    }
     
     task->cpu_usage_whole_user = whole_user;
     task->cpu_usage_part_user = part_user;
     task->cpu_usage_whole_kernel = whole_kernel;
     task->cpu_usage_part_kernel = part_kernel;

	host_info_record.cpu_user += whole_user;
	host_info_record.cpu_kernel += whole_kernel;

     result = 0;

     return result;
}

/// Update task information from given kinfo_proc structure.
/// @param kinfo [in] OS structure with task information
/// @param tinfo [out] Information was extracted from kinfo_proc. 
/// @return Upon successful completion 0 is returned.
static int update_task_from_kinfo(const struct kinfo_proc *kinfo, task_record_t *tinfo)
{
     tinfo->seq = host_info_record.seq;
     tinfo->uid = kinfo->kp_eproc.e_ucred.cr_uid;
     tinfo->ppid = kinfo->kp_eproc.e_ppid;

     if (tinfo->app_name == NULL) { // first time call
          int ret;
          int argc;
          char *procargv;
          app_path_name_t path_name={};
          int mib[3] = {CTL_KERN, KERN_ARGMAX, 0};
          static int argmax = 0;
          size_t size = sizeof(argmax);

          if (argmax == 0) {
               ret = sysctl(mib, 2, &argmax, &size, NULL, 0);
               if (ret != 0) {
                    syslog(LOG_EMERG, "%s, error in sysctl(): %m", __FUNCTION__);
                    return -1;
               }
          }
          
          procargv = malloc(argmax);
          if (procargv == NULL) {
               syslog(LOG_ERR, "%s, error in malloc(), %m", __FUNCTION__);
               return -1;
          }
          mib[0] = CTL_KERN;
          mib[1] = KERN_PROCARGS2;
          mib[2] = kinfo->kp_proc.p_pid;
          size = (size_t)argmax;
          
          ret = sysctl(mib, 3, procargv, &size, NULL, 0);
          if (ret != 0) {     // Probably we didn't have a permissions for access information for this process.
               free(procargv);
               if (kinfo->kp_proc.p_pid == 0) {
                    tinfo->app_name = strdup("kernel_task");
               }
               return 0;
          }

          extract_app_name(procargv, &path_name);
          
          tinfo->argc = path_name.argc;
          tinfo->argv = path_name.argv;
          tinfo->envc = path_name.envc;
          tinfo->envv = path_name.envv;
          tinfo->app_name = path_name.app_name;
          tinfo->path_to_boundle = path_name.path_to_app;
          tinfo->path_to_executable = path_name.path_to_executable;
          tinfo->bundle_path_name = path_name.bundle_path_name;

          free(procargv);
     }
     return 0;
}

static void free_threads_array(thread_record_t **threads_arr, int count)
{
    int cur_thread, cur_rec;
    for (cur_thread = 0; cur_thread <count; ++cur_thread)
    {
        for (cur_rec=0; cur_rec < threads_arr[cur_thread]->stack_records_count; ++cur_rec)
        {
            free(threads_arr[cur_thread]->call_stack[cur_rec]);
        }
        free(threads_arr[cur_thread]->call_stack);
    }
    free_arr((void**)threads_arr, count);
}

/// Update information about threads count and CPU usage.
/// @param task [in] The port of task for with information is to be reterned.
/// @param tinfo [out] Information was updated with list of threads within given task. 
/// @return Upon successful completion 0 is returned.
static int update_threads_info(task_t task, task_record_t *tinfo)
{
     kern_return_t kr;
     thread_act_port_array_t threads_list;
     mach_msg_type_number_t threads_count, i;
     thread_record_t *thread;

     kr = task_threads(task, &threads_list, &threads_count);
     if (kr != KERN_SUCCESS) {
          syslog(LOG_WARNING, "error in task_threads(): %s", mach_error_string(kr));
          return -1;
     }
    
     free_threads_array(tinfo->threads_arr, tinfo->threads);
     free(tinfo->threads_arr);
     tinfo->threads = threads_count;
     tinfo->threads_arr = malloc(sizeof(thread_record_t*)*threads_count);

     for (i = 0; i < threads_count; i++) {
          thread_basic_info_data_t mach_thread_info;
          mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;

          thread = calloc(1, sizeof(thread_record_t));
          tinfo->threads_arr[i] = thread;
          
          kr = thread_info(threads_list[i], THREAD_BASIC_INFO, (thread_info_t)&mach_thread_info, &count);
          if (kr != KERN_SUCCESS) {
               syslog(LOG_INFO, "error in thread_info(basic_info): %s", mach_error_string(kr));
               continue;
          }

          thread->run_state = mach_thread_info.run_state;
          thread->sleep_time = mach_thread_info.sleep_time;
          thread->suspend_count = mach_thread_info.suspend_count;
          thread->user_time = mach_thread_info.user_time;
          thread->system_time = mach_thread_info.system_time;
          thread->flags = mach_thread_info.flags;

          if ((mach_thread_info.flags & TH_FLAGS_IDLE) == 0) {
               struct timeval tv;
               TIME_VALUE_TO_TIMEVAL(&mach_thread_info.user_time, &tv);
               timeradd(&tinfo->time_user, &tv, &tinfo->time_user);
               TIME_VALUE_TO_TIMEVAL(&mach_thread_info.system_time, &tv);
               timeradd(&tinfo->time_kernel, &tv, &tinfo->time_kernel);
          }

          thread_identifier_info_data_t mach_thread_id_info;
          count = THREAD_IDENTIFIER_INFO_COUNT;
          kr = thread_info(threads_list[i], THREAD_IDENTIFIER_INFO, (thread_info_t)&mach_thread_id_info, &count);
          if (kr != KERN_SUCCESS) {
               syslog(LOG_INFO, "error in thread_info(id_info): %s", mach_error_string(kr));
               continue;
          }
          thread->thread_id = mach_thread_id_info.thread_id;

          kr = mach_port_deallocate(mach_task_self(), threads_list[i]);
          if (kr != KERN_SUCCESS) {
               syslog(LOG_INFO, "%s, error in mach_port_deallocate(): ", __FUNCTION__, mach_error_string(kr));
          }
     }
     kr = vm_deallocate(mach_task_self(), (vm_address_t)threads_list, threads_count * sizeof(thread_act_t));

     return 0;
}

/// Get the command name for task associated with tinfo.
/// @param kinfo[in] 
/// @param tinfo [out] 
/// @return The call returns 0 when successful.
/// INFO: Depricated
static int get_task_name(const struct kinfo_proc *kinfo, task_record_t *tinfo)
{
     size_t len;
     
     if (tinfo->app_name) {
          free(tinfo->app_name);
          tinfo->app_name = NULL;
     }
     
     len = strlen(kinfo->kp_proc.p_comm);
     
     tinfo->app_name = strdup(kinfo->kp_proc.p_comm);
     if (!tinfo->app_name) {
          return -1;
     }

     return 0;
}

/// Return kinfo_proc information for given pid.
/// @param pid [in]
/// @parem kinfo [out]struct kinfo_proc for given pid
/// @return The call returns -1 on error, otherwise 0.
static int get_kinfo_for_pid(pid_t pid, struct kinfo_proc *kinfo)
{
     int res;
     int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, pid};
     int mib_len = sizeof(mib)/sizeof(int);
     size_t kinfo_len = sizeof(struct kinfo_proc);

     res = sysctl(mib, mib_len, kinfo, &kinfo_len, NULL, 0);
     if (res != 0) {
          syslog(LOG_INFO, "%s, error in sysctl(): %m", __FUNCTION__);
          return -1;
     }

     return 0;
}

static int get_mach_ports(task_t task, task_record_t *tinfo) 
{
     kern_return_t kr;
     mach_msg_type_number_t ncnt, tcnt;
     mach_port_name_array_t names;
     mach_port_type_array_t types;
     
     kr = mach_port_names(task, &names, &ncnt, &types, &tcnt);
     if (kr != KERN_SUCCESS) 
          return 0;

     tinfo->ports = ncnt;

     kr = vm_deallocate(mach_task_self(), (mach_vm_address_t)(uintptr_t)names, ncnt * sizeof(*names));
     kr = vm_deallocate(mach_task_self(), (mach_vm_address_t)(uintptr_t)types, tcnt * sizeof(*types));
     
     return kr;
}

/****************************************************************************/
// Helpers
/****************************************************************************/

/// Convert zero terminated string list to malloc()ed array
/// @param strings zero terminated string list
/// @param strings count in given list
/// @return pointer on malloc()ed zero terminated strings array when successful, otherwise NULL.
char** strings_to_array(const char* strings, const int count)
{
    int i, l;
    const char *ptr;
     char **array;
     if (count == 0) {
          return NULL;
     }

    array = malloc(count * sizeof(char*));
    if (array == NULL)
        return NULL;
    ptr = strings;
     
    for (i = 0; i < count; i++) {
        l = strlen(ptr) + 1;
        array[i] = malloc(l);
        memcpy(array[i], ptr, l);
        ptr += l;
    }
    return array;    
}

/// The call parse full_path_name string and store information into app_path_name structure.
/// @param full_path_name path to binary executable
/// @param path_name malloc()ed structure with path information. It should be managed by caller.
static void extract_app_name(const char *all_arguments, app_path_name_t *path_name)
{
     char *full_path, *app_end, *app_begin, *env_begin;
     size_t diff;
     int i;
     
     if (all_arguments == NULL || path_name == NULL) {
          return;
     }

     memcpy(&path_name->argc, all_arguments, sizeof(path_name->argc));
     full_path = (char*)all_arguments + sizeof(path_name->argc);

     { // App name
		app_end = strcasestr(full_path, ".app");
		if (app_end != NULL) {
		   diff = app_end+sizeof(".app") - full_path;
		   path_name->bundle_path_name = calloc(sizeof(char), diff);
		   strncpy(path_name->bundle_path_name, full_path, diff-1);
		}
	  
		app_begin = strrchr(full_path, '/');
		if (app_begin != NULL) {
			path_name->app_name = strdup(app_begin+1);
		}
		else {
			path_name->app_name = strdup(full_path);
		}
     }
     { // Path to App
          path_name->path_to_executable = strdup(full_path);

          diff = app_begin - full_path;
          path_name->path_to_app = calloc(sizeof(char), diff + 1);
          strncpy(path_name->path_to_app, full_path, diff);
     }
     { // arguments
          while (*(++full_path) != '\0') {}
          while (*(++full_path) == '\0') {}
          if ( path_name->argc > 0) {
               path_name->argv = strings_to_array(full_path, path_name->argc);
               if (path_name->argv == NULL) {
                    path_name->argc = 0;
               }
          }
          for (i = 0; i < path_name->argc; i++) {
               full_path += strlen(path_name->argv[i]) + 1;
          }
     }
     { // environment variables
          path_name->envc = 0;
          env_begin = full_path;
          env_begin--; 
          do {
               if (*env_begin == '\0') {
                    if ( *(env_begin+1) == '\0') {
                         break;
                    }
                    else {
                         path_name->envc++;
                    }

               }
               env_begin++;
          } while (TRUE);
          path_name->envv = strings_to_array(full_path, path_name->envc);
     }
}

/// search for task_info_t structure with given pid 
/// @param pid [in]
/// @return pointer on task_info_t if structure with .pid==pid present in store, otherwise NULL.
task_record_t* task_info_manager_find_task(pid_t pid)
{
     task_record_t key, *retval=NULL;

     key.pid = pid;
     retval = (task_record_t*)CFDictionaryGetValue(tasks_dict, &key);

     return retval;
}

static void free_arr(void** arr, size_t len)
{
     size_t i;
     void *data;
     for (i = 0; i < len; i++) {
          data = arr[i];
          free(data);
     }
};

/// Deallocate task_record_t structure.    
static void free_task_record(task_record_t *task)
{
    thread_record_t *thread;
    uint32_t i;
	
	stack_info_free_task_stack(task);

    free_arr((void**)task->threads_arr, task->threads);
    free(task->threads_arr);
    free_arr((void**)task->argv, task->argc);
    free(task->argv);
    free_arr((void**)task->envv, task->envc);
    free(task->envv);
    free(task->app_name);
    free(task->path_to_executable);
    free(task->path_to_boundle);
    free(task->bundle_path_name);

    free(task);    
}

/*
 * Return the CPU type of the process.
 */
static int cpu_type_for_pid(pid_t pid, cpu_type_t *cputype) 
{
     int res = -1;
     static int mib[CTL_MAXNAME];
     static size_t miblen = 0;
     
     *cputype = 0;
     
     if (miblen == 0) {
          miblen = CTL_MAXNAME;
          res = sysctlnametomib("sysctl.proc_cputype", mib, &miblen);
          if (res != 0) {
               miblen = 0;
          }
     }

     if (miblen > 0) {
          mib[miblen] = pid;
          size_t len = sizeof(*cputype);
          res = sysctl(mib, miblen + 1, cputype, &len, NULL, 0);
     }

     return res;
}

/****************************************************************************/
// Helpers for CFMutableDictionaryRef
/****************************************************************************/
static Boolean KeysCompareFunc(const void *left, const void *right)
{
     task_record_t *l = (task_record_t*)left;
     task_record_t *r = (task_record_t*)right;
     return l->pid == r->pid;
}

static CFHashCode KeysHashFunc(const void *value)
{
     return ((task_record_t*)value)->pid;
}

static void KeyFreeFunc(CFAllocatorRef allocator, const void *value)
{
     free_task_record((task_record_t*)value);
}
/****************************************************************************/

