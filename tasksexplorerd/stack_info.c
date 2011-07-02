/*-
 * Copyright 2011, Mac OS X Internals. All rights reserved.
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

#include <sys/syscall.h>
#include <dispatch/dispatch.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include "sglib.h"

#include "stack_info.h"

#pragma mark -
#pragma mark internal data types

#define TRACEBUFF_LEN 101024

struct task_snapshot
{
    uint32_t snapshot_magic;
    int32_t pid;
    uint32_t nloadinfos;
    char ss_flags;
    char p_comm[17];
} __attribute__ ((packed));
typedef struct task_snapshot task_snapshot_t;

#define STACKSHOT_TASK_SNAPSHOT_MAGIC 0xdecafbad

struct thread_snapshot
{
    uint32_t snapshot_magic;
    uint32_t nkern_frames;
    uint32_t nuser_frames;
    uint64_t wait_event;
    uint64_t continuation;
    uint64_t thread_id;
    int32_t state;
    char ss_flags;
} __attribute__ ((packed));
typedef struct thread_snapshot thread_snapshot_t;

#define STACKSHOT_THREAD_SNAPSHOT_MAGIC 0xfeedface

typedef uint64_t addr64_t;

#define kUser64_p 1

typedef __uint64_t user64_addr_t __attribute__((aligned(8)));
typedef __uint32_t user32_addr_t;

struct dyld_uuid_info64
{
    user64_addr_t imageLoadAddress;	/* base address image is mapped into */
    uuid_t imageUUID;			/* UUID of image */
};

struct dyld_uuid_info
{
    user32_addr_t imageLoadAddress;	/* base address image is mapped into */
    uuid_t imageUUID;			/* UUID of image */
};

struct stack_record
{
    addr64_t return_addr;
    addr64_t frame_addr;
} __attribute__ ((packed));
typedef struct stack_record stack_record_t;

#pragma mark Helpers for adresses info updating

#define CMPARATOR(x,y) (x->key - y->key)

struct addr_list
{
    addr64_t key;
    struct addr_list *next_ptr;
};
typedef struct addr_list addr_list_t;

SGLIB_DEFINE_SORTED_LIST_PROTOTYPES(addr_list_t, CMPARATOR, next_ptr)
SGLIB_DEFINE_SORTED_LIST_FUNCTIONS(addr_list_t, CMPARATOR, next_ptr)

struct pid_update_info
{
	pid_t pid;
	addr_list_t *addresses;
};
typedef struct pid_update_info pid_update_info_t;

static pthread_rwlock_t proc_list_loc;
static int inited = 0;


struct call_stack_tree
{
	addr64_t key;
	char *descr;
	char color_field;
	struct call_stack_tree *left;
	struct call_stack_tree *right;
};

typedef struct call_stack_tree call_stack_tree_t;

SGLIB_DEFINE_RBTREE_PROTOTYPES(call_stack_tree_t, left, right, color_field, CMPARATOR);
SGLIB_DEFINE_RBTREE_FUNCTIONS(call_stack_tree_t, left, right, color_field, CMPARATOR);

struct pid_tree
{
	pid_t key;
	call_stack_tree_t *call_stacks;
	pthread_rwlock_t call_stack_lock;
	char color_field;
	struct pid_tree *left;
	struct pid_tree *right;
};

typedef struct pid_tree pid_tree_t;

SGLIB_DEFINE_RBTREE_PROTOTYPES(pid_tree_t, left, right, color_field, CMPARATOR);
SGLIB_DEFINE_RBTREE_FUNCTIONS(pid_tree_t, left, right, color_field, CMPARATOR);

static pid_tree_t *processes_tree = 0;

#pragma mark -
#pragma mark internal functions declaration

static int build_stacks_for_pid(pid_t pid, thread_record_t **threads, uint32_t count);
static thread_record_t* find_thread_record(uint64_t tid, thread_record_t **threads, uint32_t count);
static int update_names_for_pid(pid_t pid, thread_record_t **threads, uint32_t count);
static char* find_func_name(pid_t pid, unsigned long addr);
static void update_process_stacks(void *context);

#pragma mark -
#pragma mark exported functions

void stack_info_init()
{
	if (0 != inited)
		return;
	inited = 1;
	pthread_rwlock_init(&proc_list_loc, NULL);
}

void stack_info_free()
{
	inited = 0;
	pthread_rwlock_destroy(&proc_list_loc);
}

void stack_info_free_task_stack(task_record_t *task)
{
	pid_tree_t searched_process, *process;
	searched_process.key = task->pid;
	
	pthread_rwlock_wrlock(&proc_list_loc);
	if( NULL != (process = sglib_pid_tree_t_find_member(processes_tree, &searched_process)) )
	{
		struct sglib_call_stack_tree_t_iterator it;
		call_stack_tree_t *te;

		pthread_rwlock_wrlock(&process->call_stack_lock);
		for(te=sglib_call_stack_tree_t_it_init(&it,process->call_stacks); te!=NULL; te=sglib_call_stack_tree_t_it_next(&it)) 
		{
			free(te->descr);
			free(te);
		}
		sglib_pid_tree_t_delete(&processes_tree, process);
		free(process);
		pthread_rwlock_unlock(&process->call_stack_lock);
	}
	pthread_rwlock_unlock(&proc_list_loc);	
}

int stack_info_update_task_stack(task_record_t *task)
{
    int result;

    result = build_stacks_for_pid(task->pid, task->threads_arr, task->threads);

    if (result != -1)
        result = update_names_for_pid(task->pid, task->threads_arr, task->threads);
    
    return result;
}

#pragma mark -
#pragma mark internal functions

static thread_record_t* find_thread_record(uint64_t tid, thread_record_t **threads, uint32_t count)
{
    thread_record_t *thread = 0;
    uint32_t i;

    for (i=0; i < count; ++i)
    {
        if (threads[i]->thread_id == tid)
        {
            thread = threads[i];
            break;
        }
    }

    return thread;
}

static int update_names_for_pid(pid_t pid, thread_record_t **threads, uint32_t count)
{
    if (0 == pid)
        return 0;

    int cur_thrd, cur_frame;
    stack_info_t *stack;
    thread_record_t *thread;
	pid_update_info_t *update_info = calloc(1, sizeof(pid_update_info_t));
	addr_list_t *addr = 0;

	update_info->pid = pid;
    for (cur_thrd=0; cur_thrd < count; ++cur_thrd)
    {
        thread = threads[cur_thrd];
        for (cur_frame=0; cur_frame < thread->stack_records_count; ++cur_frame)
        {
            stack = thread->call_stack[cur_frame];

            if (0 != stack->func_name)
                continue;
            char *func_name = find_func_name(pid, stack->return_addr);
            if (func_name)
			{
                stack->func_name = func_name;
			}
			else
			{
				addr = calloc(1, sizeof(addr_list_t));
				addr->key = stack->return_addr;
				sglib_addr_list_t_add(&update_info->addresses, addr);
			}
        }
    }

	if (update_info->addresses)
	{
		dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
		dispatch_async_f(queue, (void*)update_info, update_process_stacks);
	}
	else
	{
		free(update_info);
	}

	
    return 0;
}

static int build_stacks_for_pid(pid_t pid, thread_record_t **threads, uint32_t count)
{
    char tracebuf[TRACEBUFF_LEN];
    char *tracepos = tracebuf;
    int result = syscall(SYS_stack_snapshot, pid, tracebuf, TRACEBUFF_LEN, 0);
    if( -1 == result )
        return -1;

    task_snapshot_t *snapshot = (task_snapshot_t*)tracepos;
    if (STACKSHOT_TASK_SNAPSHOT_MAGIC != snapshot->snapshot_magic || pid != snapshot->pid)
        return -1;

    tracepos += sizeof(task_snapshot_t);
    if (0 < snapshot->nloadinfos)
    {
        int uuid_info_size = snapshot->ss_flags & kUser64_p ?
        sizeof(struct dyld_uuid_info64) :
        sizeof(struct dyld_uuid_info);
        tracepos += snapshot->nloadinfos * uuid_info_size;
    }

	addr_list_t *adresses = 0;
    while (tracepos-tracebuf >= sizeof(task_snapshot_t))
    {
        thread_snapshot_t *tsnap = (thread_snapshot_t*)tracepos;
        if (STACKSHOT_THREAD_SNAPSHOT_MAGIC != tsnap->snapshot_magic)
            break;
        
        tracepos += sizeof(thread_snapshot_t);

        thread_record_t *thread = find_thread_record(tsnap->thread_id, threads, count);
        if (!thread)
            continue;

        thread->stack_records_count = tsnap->nuser_frames - 1;
        thread->call_stack = calloc(thread->stack_records_count, sizeof(stack_info_t*));

        int i, item_pos;
        for (i = 0; i < tsnap->nuser_frames; ++i)
        {
            stack_record_t *addr = (stack_record_t*)tracepos;

            if (i < thread->stack_records_count)
            {
                item_pos = thread->stack_records_count - i-1;
                thread->call_stack[item_pos] = calloc(1, sizeof(stack_info_t));
                thread->call_stack[item_pos]->return_addr = addr->return_addr;
                thread->call_stack[item_pos]->frame_addr = addr->frame_addr;
            }
            tracepos += sizeof(stack_record_t);
        }

        for (i = 0; i<tsnap->nkern_frames; ++i)
        {
            stack_record_t *addr = (stack_record_t*)tracepos;
            tracepos += sizeof(stack_record_t);
        }
    }

    return 0;
}

#pragma mark -
#pragma mark Stack info storage

static char* find_func_name(pid_t pid, unsigned long addr)
{
	pid_tree_t searched_process, *process;
	call_stack_tree_t searched_adress, *addr_info;
	searched_process.key = pid;
	searched_adress.key = addr;
	char *result = 0;

	pthread_rwlock_rdlock(&proc_list_loc);
	if( NULL != (process = sglib_pid_tree_t_find_member(processes_tree, &searched_process)) )
	{
		pthread_rwlock_rdlock(&process->call_stack_lock);
		if ( NULL != (addr_info = sglib_call_stack_tree_t_find_member(process->call_stacks, &searched_adress)))
		{
			result = addr_info->descr;
		}
		pthread_rwlock_unlock(&process->call_stack_lock);
	}
	pthread_rwlock_unlock(&proc_list_loc);

    return result;
}

static void update_process_stacks(void *context)
{
	if (NULL == context)
		return;

	pid_update_info_t *update_info = (pid_update_info_t*)context;

#define BUFF_LEN 2048
    char cmd_line[BUFF_LEN];
    char buff[BUFF_LEN];
    char *cmd_fmt = "atos -p %d ";
    sprintf( cmd_line, cmd_fmt, update_info->pid );

	SGLIB_LIST_MAP_ON_ELEMENTS(addr_list_t, update_info->addresses, ll, next_ptr, 
	{
		sprintf(buff, "0x%llx ", ll->key);
		strcat(cmd_line, buff);
	});

	pthread_rwlock_wrlock(&proc_list_loc);

	pid_tree_t searched_process, *process = 0;
	searched_process.key = update_info->pid;

	process = sglib_pid_tree_t_find_member(processes_tree, &searched_process);
	if (NULL == process)
	{
		process = calloc(1, sizeof(pid_tree_t));
		process->key = update_info->pid;
		pthread_rwlock_init(&process->call_stack_lock, NULL);
		pthread_rwlock_wrlock(&process->call_stack_lock);

		sglib_pid_tree_t_add(&processes_tree, process);
	}
	else 
	{
		pthread_rwlock_wrlock(&process->call_stack_lock);
	}

	pthread_rwlock_unlock(&proc_list_loc);

    FILE *out = popen(cmd_line, "r");

	call_stack_tree_t *addr_info = 0;
	addr_list_t *cur_addr = update_info->addresses;
	while (fgets(&buff[0], BUFF_LEN, out) && cur_addr)
    {
		addr_info = calloc(1, sizeof(call_stack_tree_t));
		addr_info->key = cur_addr->key;
		addr_info->descr = strdup(buff);
		sglib_call_stack_tree_t_add(&process->call_stacks, addr_info);
		cur_addr = cur_addr->next_ptr;
    }
	pthread_rwlock_unlock(&process->call_stack_lock);
    pclose(out);	

	SGLIB_LIST_MAP_ON_ELEMENTS(addr_list_t, update_info->addresses, ll, next_ptr, 
	{
		free(ll);
	});
	free(update_info);
	
#undef BUFF_LEN
}
