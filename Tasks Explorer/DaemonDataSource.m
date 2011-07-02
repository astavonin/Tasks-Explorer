/*-
 * Copyright 2010, Mac OS X Internals. All rights reserved.
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

#import "EnvVariable.h"
#import "ThreadInfo.h"
#import "StackRecord.h"

#import "DaemonDataSource.h"

static char watch_path[]="/Library/Application Support/TasksExplorer/watch_dir/XXXXXXXX";

@implementation DaemonDataSource

-(BOOL)initDaemon
{
	BOOL result = YES;

	client = clnt_create("localhost", TASK_EXPLORER_PROG, TASK_EXPLORER, "tcp");
	if (client == nil) {
		[self restart_daemon];
		sleep(1);
		client = clnt_create("localhost", TASK_EXPLORER_PROG, TASK_EXPLORER, "tcp");
		if (client == nil) {
			inited = NO;
			result = NO;
		}
	}
	if (result == YES) {
		task_explorer_init_1(nil, client);
		inited = YES;
	}
			
	return result;
}

-(id)init
{
	if ((self = [super init])) {
		if ([self initDaemon] == NO) {
			[self release];
			return nil;
		}
	}
    return self;
}

-(void)restart_daemon
{
	char *path = mktemp(watch_path);
	FILE *handle = fopen(path, "w");
	unlink(path);
	if (handle) {
		fclose(handle);
	}
}

-(void)dealloc 
{	
	free(tasksPidList);
	if (inited == YES) {
		task_explorer_free_1(nil, client);
		clnt_destroy(client);
	}

	[super dealloc];
}

-(void)updateProcessInfo
{
	host_info_dynamic *host_info =task_explorer_update_1(nil, client);
	if (host_info == 0) {
		if ([self initDaemon] == NO) {
			exit(0);
		}
		host_info =task_explorer_update_1(nil, client);
	}

	hostInfo.cpu_user = host_info->cpu_user;
	hostInfo.cpu_kernel = host_info->cpu_kernel;

	tasks_list *tasks = task_explorer_tasks_list_1(nil, client);

	tasksCount = tasks->tasks_list_len;
	size_t memLen = sizeof(int)*tasksCount;
	free(tasksPidList);
	tasksPidList = malloc(memLen);
	memcpy(tasksPidList, tasks->tasks_list_val, memLen);
	free(tasks->tasks_list_val);
}

-(int)processesCount
{
	return tasksCount;
}

-(int*)processesPIDCArray
{
	return tasksPidList;
}

-(BOOL)processBaseInfo:(task_info_base*)taskInfo forPID:(int)pid
{
	task_info_base *info = task_explorer_base_info_1(&pid, client);
	if (info != nil) {
		memcpy(taskInfo, info, sizeof(task_info_base));
		return YES;
	}
	return NO;
}

-(BOOL)processDynInfo:(task_info_dynamic*)dynInfo forPID:(int)pid
{
	task_info_dynamic *info = task_explorer_dyninfo_1(&pid, client);
	if (info != nil) {
		memcpy(dynInfo, info, sizeof(task_info_dynamic));
		return YES;
	}
	return NO;
}

-(NSString*)commandLineForPID:(int)pid
{
	command_line cmdLine = *task_explorer_params_1(&pid, client);
	NSString *result = [[NSString alloc] initWithCString:cmdLine];
	free(cmdLine);
	return result;
}

-(NSMutableArray*)createEnvList:(int)pid
{
	NSMutableArray *arr = [[NSMutableArray alloc] init];
	env_list tmp, *envList = task_explorer_env_list_1(&pid, client);

	while (*envList) {
		EnvVariable *env = [[EnvVariable alloc] init];
		[env setEnvRec:(*envList)->name];
		[arr addObject:env];
		tmp = *envList;
		 *envList = (*envList)->next;
		free(tmp->name);
		free(tmp);
	}
	return arr;
}

-(NSMutableArray*)createFilesList:(int)pid
{
	NSMutableArray *arr = [[NSMutableArray alloc] init];
	files_list tmp, *filesList = task_explorer_files_list_1(&pid, client);

	while (*filesList) {
		NSString *name = [[NSString alloc] initWithCString:(*filesList)->name];
		[arr addObject:name];
		tmp = *filesList;
		*filesList = (*filesList)->next;
		free(tmp->name);
		free(tmp);
	}
	return arr;
}

-(int)killProcess:(int)pid
{
	return *kill_process_1(&pid, client);
}

-(host_info_dynamic*)hostInfoDynamic
{
	return &hostInfo;
}

-(NSString*)processDescription:(int)pid
{
    NSString *result = nil;
	app_descr *appDescr = task_explorer_app_descr_1(&pid, client);
    if (NULL != appDescr ) {
        if (0 != **appDescr) {
            result = [[NSString alloc] initWithCString:*appDescr];
        }
        
        free(*appDescr);
    }
	return result;
}

-(NSMutableArray*)createThreadsList:(int)pid
{
    NSMutableArray *arr = [[NSMutableArray alloc] init];
    thread_info_list tmp, *tinfoList = task_explorer_threads_info_1(&pid, client);
    ThreadInfo *threadInfo;

    while (*tinfoList) {
        threadInfo = [ThreadInfo createThreadInfo:*tinfoList];
        [arr addObject:threadInfo];

        tmp = *tinfoList;
        *tinfoList = (*tinfoList)->next;
        free(tmp->entry_point_name);
        free(tmp);
    }

    return arr;
}

-(NSMutableArray*)createCallStack:(int)pid withTid:(u_long)tid
{
    NSMutableArray *arr = [[NSMutableArray alloc] init];
	struct call_stack_request request = {pid, tid};
	stack_info_list tmp, *callStack = task_explorer_stack_for_thread_1(&request, client);
	StackRecord *stackRecord = 0;
	
	while (*callStack) {
		stackRecord = [StackRecord createStackRecord:*callStack];
		[arr addObject:stackRecord];
		
		tmp = *callStack;
		*callStack = (*callStack)->next;
		free(tmp->func_name);
		free(tmp);
	}
	
	return arr;
}

@end
