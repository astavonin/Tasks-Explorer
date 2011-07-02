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

#include "data_types.h"
#include "taskinfomanager.h"
#include "tasks_explorer.h"
#include "bundleinforeader.h"
#include "helpers.h"
#include <pwd.h>
#include "external_utils.h"
#include "man_info.h"
#include "stack_info.h"

#include "server.h"

extern natural_t processor_count;


int* task_explorer_init_1_svc(argp, rqstp)
     void *argp;
     struct svc_req *rqstp;
{
     static int  result = 0;

     task_info_manager_init();

     return(&result);
}

int* task_explorer_free_1_svc(argp, rqstp)
     void *argp;
     struct svc_req *rqstp;
{
     static int  result = 0;
     
     task_info_manager_free();

     return(&result);
}

host_info_dynamic* task_explorer_update_1_svc(argp, rqstp)
     void *argp;
     struct svc_req *rqstp;
{
     static struct host_info_dynamic  result = {};
	host_record_t* host_record=0;

     task_info_manager_update();
	
	host_record = task_manager_get_host_info();

	result.cpu_user = host_record->cpu_user / processor_count;
	result.cpu_kernel = host_record->cpu_kernel / processor_count;
     
     return(&result);
}

tasks_list* task_explorer_tasks_list_1_svc(argp, rqstp)
     void *argp;
     struct svc_req *rqstp;
{
     static tasks_list  result={};

     free(result.tasks_list_val);

     result.tasks_list_len = task_info_manager_get_tasks_count();
     result.tasks_list_val = malloc(sizeof(int) * result.tasks_list_len);
     task_info_manager_get_tasks_pid(result.tasks_list_val);

     return(&result);
}

task_info_base* task_explorer_base_info_1_svc(argp, rqstp)
     int *argp;
     struct svc_req *rqstp;
{
     static task_info_base  result={};
     memset(&result, 0, sizeof(result));

     task_record_t *task = task_info_manager_find_task(*argp);
     if (task != NULL) {
          result.pid = task->pid;
          result.ppid = task->ppid;
          result.cputype = task->cputype;
          strcpy_if(result.name, task->app_name, sizeof(result.name));
          strcpy_if(result.path_to_executable, task->path_to_executable, sizeof(result.path_to_executable));

          struct passwd psw={};
          struct passwd* pwdptr=&psw;
          struct passwd* tempPwdPtr;
          char pwdbuffer[200];
          int  pwdlinelen = sizeof(pwdbuffer);

          if (0 == getpwuid_r(task->uid, pwdptr,pwdbuffer,pwdlinelen,&tempPwdPtr)) {
               strcpy_if(result.user, psw.pw_name, sizeof(result.user));
          }
          strcpy_if(result.path_to_boundle, task->path_to_boundle, sizeof(result.path_to_boundle));
          extract_bundle_info(task, &result);
     }

     return(&result);
}

command_line* task_explorer_params_1_svc(argp, rqstp)
     int *argp;
     struct svc_req *rqstp;
{
     static command_line  result = 0;
     task_record_t *task;
     size_t len = 0;
     int i;

     task = task_info_manager_find_task(*argp);
     if (task != NULL) {
          if (task->argc > 0) {
               for (i = 0; i < task->argc; i++) {
                    len += strlen(task->argv[i]) + 1;
               }
               
               if (len <= MAX_PARAMS_LEN) {
                    free(result);
                    result = malloc(len);
                    strcpy(result, task->argv[0]);
                    for (i = 1; i < task->argc; i++) {
                         strcat(result, " ");
                         strcat(result, task->argv[i]);
                    }
               }
          }
          else {
               result = malloc(sizeof(""));
               result[0] = 0;
          }
     }

     return(&result);
}

int* kill_process_1_svc(argp, rqstp)
     int *argp;
     struct svc_req *rqstp;
{
     static int  result;

     result = kill(*argp, 9);
     
     return(&result);
}

env_list* task_explorer_env_list_1_svc(argp, rqstp)
     int *argp;
     struct svc_req *rqstp;
{
     static env_list result;

     {
          env_list tmp, envList = result;    
          while (envList) {
               free(envList->name);
               tmp = envList;
               envList = envList->next;
               free(tmp);
          }
     }

     task_record_t *task = task_info_manager_find_task(*argp);
     if (task != NULL) {
          char **envv = task->envv;
          int i, envc = task->envc;
          env_list node, *nodep = &result;

          for (i = 0; i < envc; i++) {
               node = *nodep = (env_list)malloc(sizeof(env_node));
               node->name = strdup(envv[i]);
               nodep = &node->next;
          }
          *nodep = (env_list)NULL;
     }
     
     return(&result);
}

task_info_dynamic *
task_explorer_dyninfo_1_svc(argp, rqstp)
     int *argp;
     struct svc_req *rqstp;
{
     static task_info_dynamic  result;
     memset(&result, 0, sizeof(result));

     task_record_t *task = task_info_manager_find_task(*argp);
     if (task != NULL) {      
          result.threads = task->threads;
          result.ports = task->ports;
          result.real_mem_size = task->real_mem_size;
          result.virtual_mem_size = task->virtual_mem_size;
          
          result.cpu_usage_user = ((float)task->cpu_usage_whole_user + (float)task->cpu_usage_part_user/(float)10);
          result.cpu_usage_kernel = (float)task->cpu_usage_whole_kernel + (float)task->cpu_usage_part_kernel/(float)10;
          result.cpu_usage_total = (float)task->cpu_usage_whole_user + (float)task->cpu_usage_part_user/(float)10 +
                                             (float)task->cpu_usage_whole_kernel + (float)task->cpu_usage_part_kernel/(float)10;
         
         result.cpu_usage_user = result.cpu_usage_user / processor_count;
         result.cpu_usage_kernel = result.cpu_usage_kernel / processor_count;
         result.cpu_usage_total = result.cpu_usage_total / processor_count;
     }

     return(&result);
}

files_list* task_explorer_files_list_1_svc(argp, rqstp)
int *argp;
struct svc_req *rqstp;
{
	static files_list  result;

	{
		files_list tmp, filesList = result;
		while (filesList) {
			free(filesList->name);
			tmp = filesList;
			filesList = filesList->next;
			free(tmp);
		}
	}

	string_list_t list = files_list_for_task(*argp);
	files_list node, *nodep = &result;
	int i;

	for (i = 0; i < list.count; ++i) {
		node = *nodep = (files_list)malloc(sizeof(files_list));
		char *name = list.strings[i];
		int len = strlen(name);
		node->name = calloc(len, sizeof(char));
		strncpy(node->name, name, len-2);
		nodep = &node->next;
	}
	
	*nodep = (files_list)NULL;

	return(&result);
}


app_descr * task_explorer_app_descr_1_svc(argp, rqstp)
int *argp;
struct svc_req *rqstp;
{
	static app_descr result = 0;
	task_record_t *task;
	size_t len = 0;

	task = task_info_manager_find_task(*argp);
	if (task != NULL) {
        const char *descr = man_info_get_descr_by_name(task->app_name);
        if (NULL != descr) {
            int len = strlen(descr) + 1;
            free(result);
            result = malloc(len < MAX_APP_DESCR ? len : MAX_APP_DESCR);
            strcpy(result, descr);
        }
        else {
            result = malloc(sizeof(""));
            result[0] = 0;
        }        
	}

	return(&result);
}


thread_info_list *
task_explorer_threads_info_1_svc(argp, rqstp)
int *argp;
struct svc_req *rqstp;
{
    
	static thread_info_list result;
	task_record_t *task;
    thread_record_t **threads;
    int i;

    {
        thread_info_list tmp, tinfoList = result;    
        while (tinfoList) {
            free(tinfoList->entry_point_name);
            tmp = tinfoList;
            tinfoList = tinfoList->next;
            free(tmp);
        }
        result = 0;
    }

	task = task_info_manager_find_task(*argp);
	if (task != NULL) {
        threads = task->threads_arr;

        thread_info_list node, *nodep = &result;

        for (i = 0; i < task->threads; ++i ) {
            node = *nodep = calloc(1, sizeof(thread_info_node));
            node->thread_id = threads[i]->thread_id;
            node->user_time = threads[i]->user_time.seconds;
            node->system_time = threads[i]->system_time.seconds;
            node->flags = threads[i]->flags;
            node->run_state = threads[i]->run_state;
            node->sleep_time = threads[i]->sleep_time;
            node->suspend_count = threads[i]->suspend_count;
            if (threads[i]->stack_records_count > 0 && threads[i]->call_stack[0]->func_name)
                node->entry_point_name = strdup(threads[i]->call_stack[0]->func_name);
            else
                node->entry_point_name = strdup("Invalid stack");
            nodep = &node->next;
        }
        *nodep = NULL;
    }
	return(&result);
}


stack_info_list *
task_explorer_stack_for_thread_1_svc(argp, rqstp)
struct call_stack_request *argp;
struct svc_req *rqstp;
{
	static stack_info_list result;

    {
        stack_info_list tmp, sinfoList = result;    
        while (sinfoList) {
            free(sinfoList->func_name);
            tmp = sinfoList;
            sinfoList = sinfoList->next;
            free(tmp);
        }
        result = 0;
    }
    
    int i;
    task_record_t *task;
    thread_record_t *thread = 0;

    task = task_info_manager_find_task(argp->pid);
    for (i = 0; i < task->threads; ++i) {
        if (task->threads_arr[i]->thread_id == argp->tid) {
            thread = task->threads_arr[i];
            break;
        }
    }
    if (thread) {
        stack_info_list node, *nodep = &result;
        stack_info_t *sinfo;

        for (i = 0; i < thread->stack_records_count; ++i ) {
            node = *nodep = calloc(1, sizeof(stack_info_node));

            sinfo = thread->call_stack[i];
            node->func_name = strdup(sinfo->func_name);
            node->frame_addr = sinfo->frame_addr;
            node->return_addr = sinfo->return_addr;

            nodep = &node->next;
        }
        *nodep = NULL;
    }

	return(&result);
}
