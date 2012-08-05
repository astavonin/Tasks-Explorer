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

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
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
#define STACKSHOT_TASK_SNAPSHOT_MAGIC 0xdecafbad
#define STACKSHOT_THREAD_SNAPSHOT_MAGIC 0xfeedface

struct task_snapshot_10_6
{
    uint32_t snapshot_magic;
    int32_t pid;
    uint32_t nloadinfos;
    char ss_flags;
    char p_comm[17];
} __attribute__ ((packed));
typedef struct task_snapshot_10_6 task_snapshot_10_6_t;

struct task_snapshot_10_7
{
	uint32_t		snapshot_magic;
	int32_t			pid;
	uint32_t		nloadinfos;
	uint64_t		user_time_in_terminated_threads;
	uint64_t		system_time_in_terminated_threads;
	int				suspend_count;
	int				task_size;    // pages
	int				faults;	 	// number of page faults
	int				pageins; 	// number of actual pageins
	int				cow_faults;	// number of copy-on-write faults
	char			ss_flags;
    char			p_comm[17];
} __attribute__ ((packed));
typedef struct task_snapshot_10_7 task_snapshot_10_7_t;

struct task_snapshot_10_8
{
	uint32_t		snapshot_magic;
	int32_t			pid;
	uint32_t		nloadinfos;
	uint64_t		user_time_in_terminated_threads;
	uint64_t		system_time_in_terminated_threads;
	int				suspend_count;
	int				task_size;    // pages
	int				faults;	 	// number of page faults
	int				pageins; 	// number of actual pageins
	int				cow_faults;	// number of copy-on-write faults
	char			ss_flags;
	char			p_comm[17];
} __attribute__ ((packed));
typedef struct task_snapshot_10_8 task_snapshot_10_8_t;


struct thread_snapshot_10_6
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
typedef struct thread_snapshot_10_6 thread_snapshot_10_6_t;

struct thread_snapshot_10_7
{
	uint32_t 		snapshot_magic;
	uint32_t 		nkern_frames;
	uint32_t 		nuser_frames;
	uint64_t 		wait_event;
	uint64_t 	 	continuation;
	uint64_t 		thread_id;
	uint64_t 		user_time;
	uint64_t 		system_time;
	int32_t  		state;
	char			ss_flags;
} __attribute__ ((packed));
typedef struct thread_snapshot_10_7 thread_snapshot_10_7_t;

struct thread_snapshot_10_8
{
	uint32_t 		snapshot_magic;
	uint32_t 		nkern_frames;
	uint32_t 		nuser_frames;
	uint64_t 		wait_event;
	uint64_t 	 	continuation;
	uint64_t 		thread_id;
	uint64_t 		user_time;
	uint64_t 		system_time;
	int32_t  		state;
	int32_t			sched_pri;   // scheduled (current) priority
	int32_t			sched_flags; // scheduler flags
	char			ss_flags;
} __attribute__ ((packed));
typedef struct thread_snapshot_10_8 thread_snapshot_10_8_t;

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

struct addr_list
{
    addr64_t key;
    struct addr_list *next_ptr;
};
typedef struct addr_list addr_list_t;

struct pid_update_info
{
	pid_t pid;
	addr_list_t *addresses;
};
typedef struct pid_update_info pid_update_info_t;

//static pthread_rwlock_t proc_list_loc;
static int inited = 0;

enum tree_node_type
{
    CallStackNode = 0,
    PidNode
};

typedef enum tree_node_type tree_node_type_t;

struct call_stack_tree
{
    tree_node_type_t type;
	addr64_t key;
	char *descr;
};
typedef struct call_stack_tree call_stack_tree_t;

struct pid_tree
{
    tree_node_type_t type;
	pid_t key;
	CFMutableDictionaryRef call_stacks;
};
typedef struct pid_tree pid_tree_t;

static CFMutableDictionaryRef processes_tree;

enum os_version {
    OS_10_6 = 6,
    OS_10_7 = 7,
    OS_10_8 = 8
};
typedef enum os_version os_version_t;

static os_version_t current_os;

#pragma mark -
#pragma mark internal functions declaration

static int build_stacks_for_pid(pid_t pid, thread_record_t **threads, uint32_t count);
static thread_record_t* find_thread_record(uint64_t tid, thread_record_t **threads, uint32_t count);
static int update_names_for_pid(pid_t pid, thread_record_t **threads, uint32_t count);
static char* find_func_name(pid_t pid, unsigned long addr);
static void update_process_stacks(pid_update_info_t *update_info);

static Boolean KeysCompareFunc(const void *left, const void *right);
static CFHashCode KeysHashFunc(const void *value);
static void KeyFreeFunc(CFAllocatorRef allocator, const void *value);
static os_version_t get_os_version();

#pragma mark -
#pragma mark exported functions

void stack_info_init()
{
	if (0 != inited)
		return;
	inited = 1;

    CFDictionaryKeyCallBacks tasksKeysCallBask = {0, NULL, NULL, NULL, KeysCompareFunc, KeysHashFunc};
    CFDictionaryValueCallBacks tasksValueCallBask = {0, NULL, KeyFreeFunc, NULL, KeysCompareFunc};

    processes_tree = CFDictionaryCreateMutable(NULL, 0, &tasksKeysCallBask, &tasksValueCallBask);
    current_os = get_os_version();
}

void stack_info_free()
{
	inited = 0;
}

void stack_info_free_task_stack(task_record_t *task)
{
	pid_tree_t searched_process, *process;
	searched_process.key = task->pid;

	if( NULL != (process = (pid_tree_t*)CFDictionaryGetValue(processes_tree, &searched_process)) )
	{
		call_stack_tree_t *te;

        CFDictionaryRemoveAllValues(process->call_stacks);
        CFDictionaryRemoveValue(processes_tree, process);
	}
}

int stack_info_update_task_stack(task_record_t *task)
{
    int result = 0;

    result = build_stacks_for_pid(task->pid, task->threads_arr, task->threads);

    if (result != -1)
        result = update_names_for_pid(task->pid, task->threads_arr, task->threads);

    return result;
}

#pragma mark -
#pragma mark internal functions

static os_version_t get_os_version()
{
    os_version_t ver = OS_10_8;
    
    SInt32 minorVersion;
    
    Gestalt(gestaltSystemVersionMinor, &minorVersion);
    ver = minorVersion;
    
    return ver;
}

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

static void load_names_for_pid(pid_t pid, thread_record_t **threads, uint32_t count)
{
    int cur_thrd, cur_frame;
    stack_info_t *stack;
    thread_record_t *thread;
	addr_list_t *addr = 0;

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
                stack->func_name = func_name;
        }
    }
}

static void prepare_names_for_pid(pid_t pid, thread_record_t **threads, pid_update_info_t *update_info, uint32_t count)
{
    int cur_thrd, cur_frame;
    stack_info_t *stack;
    thread_record_t *thread;
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
            if (!func_name)
			{
				addr = calloc(1, sizeof(addr_list_t));
				addr->key = stack->return_addr;
                addr->next_ptr = update_info->addresses;
                update_info->addresses = addr;
			}
        }
    }
}

static int update_names_for_pid(pid_t pid, thread_record_t **threads, uint32_t count)
{
    if (0 == pid)
        return 0;

	pid_update_info_t *update_info = calloc(1, sizeof(pid_update_info_t));
    prepare_names_for_pid(pid, threads, update_info, count); // create unknown stack frames list
    
	if (update_info->addresses)
        update_process_stacks(update_info);
	else
		free(update_info);
    load_names_for_pid(pid, threads, count);
	
    return 0;
}

static size_t get_task_snapshot_len()
{
    if (current_os == OS_10_6)
        return sizeof(task_snapshot_10_6_t);
    else if(current_os == OS_10_7)
        return sizeof(task_snapshot_10_7_t);
    else if (current_os == OS_10_8)
        return sizeof(task_snapshot_10_8_t);

    return 0;
}

static size_t get_thread_snapshot_len()
{
    if (current_os == OS_10_6)
        return sizeof(thread_snapshot_10_6_t);
    else if(current_os == OS_10_7)
        return sizeof(thread_snapshot_10_7_t);
    else if (current_os == OS_10_8)
        return sizeof(thread_snapshot_10_8_t);

    return 0;
}

static uint32_t get_snapshot_magic(void *snapshot)
{
    return *(uint32_t*)snapshot;
}

#define DECLARE_GET_TASK_SNAPSHOT_VAL(__field_name, __ret_type) \
__ret_type get_task_##__field_name##_value(void *snapshot) \
{\
    if (current_os == OS_10_6) \
        return ((task_snapshot_10_6_t*)snapshot)->__field_name; \
    else if(current_os == OS_10_7) \
        return ((task_snapshot_10_7_t*)snapshot)->__field_name; \
    else if (current_os == OS_10_8) \
        return ((task_snapshot_10_8_t*)snapshot)->__field_name; \
    return 0; \
}

DECLARE_GET_TASK_SNAPSHOT_VAL(nloadinfos, uint32_t)
DECLARE_GET_TASK_SNAPSHOT_VAL(pid, uint32_t)
DECLARE_GET_TASK_SNAPSHOT_VAL(ss_flags, char)


#define DECLARE_GET_THREAD_SNAPSHOT_VAL(__field_name, __ret_type) \
__ret_type get_thread_##__field_name##_value(void *snapshot) \
{\
    if (current_os == OS_10_6) \
        return ((thread_snapshot_10_6_t*)snapshot)->__field_name; \
    else if(current_os == OS_10_7) \
        return ((thread_snapshot_10_7_t*)snapshot)->__field_name; \
    else if (current_os == OS_10_8) \
        return ((thread_snapshot_10_8_t*)snapshot)->__field_name; \
    return 0; \
}

DECLARE_GET_THREAD_SNAPSHOT_VAL(thread_id, uint64_t)
DECLARE_GET_THREAD_SNAPSHOT_VAL(nuser_frames, uint32_t)
DECLARE_GET_THREAD_SNAPSHOT_VAL(nkern_frames, uint32_t)

static int build_stacks_for_pid(pid_t pid, thread_record_t **threads, uint32_t count)
{
    char tracebuf[TRACEBUFF_LEN];
    char *tracepos = tracebuf;
    int result = syscall(SYS_stack_snapshot, pid, tracebuf, TRACEBUFF_LEN, 0);
    if( -1 == result )
        return -1;

    void *task_stapshot = tracepos;

    if(STACKSHOT_TASK_SNAPSHOT_MAGIC != get_snapshot_magic(task_stapshot) || pid != get_task_pid_value(task_stapshot))
        return -1;
    
    tracepos += get_task_snapshot_len();

    if(0 < get_task_nloadinfos_value(task_stapshot))
    {
        int uuid_info_size = get_task_ss_flags_value(task_stapshot) & kUser64_p ?
                sizeof(struct dyld_uuid_info64) :
                sizeof(struct dyld_uuid_info);
        tracepos += get_task_nloadinfos_value(task_stapshot) * uuid_info_size;
    }

	addr_list_t *adresses = 0;
    while (tracepos-tracebuf >= get_task_snapshot_len())
    {
        void *tsnap = tracepos;
        if (STACKSHOT_THREAD_SNAPSHOT_MAGIC != get_snapshot_magic(tsnap))
            break;
        
        tracepos += get_thread_snapshot_len();

        thread_record_t *thread = find_thread_record(get_thread_thread_id_value(tsnap), threads, count);
        if (!thread)
            continue;

        thread->stack_records_count = get_thread_nuser_frames_value(tsnap) - 1;
        thread->call_stack = calloc(thread->stack_records_count, sizeof(stack_info_t*));

        int i, item_pos;
        for (i = 0; i < get_thread_nuser_frames_value(tsnap); ++i)
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

        for (i = 0; i < get_thread_nkern_frames_value(tsnap); ++i)
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
    searched_process.type = PidNode;
	searched_process.key = pid;
	searched_adress.key = addr;
	char *result = 0;

	if( NULL != (process = (pid_tree_t*)CFDictionaryGetValue(processes_tree, &searched_process)) )
	{
        searched_adress.type = CallStackNode;
        if( NULL != (addr_info = (call_stack_tree_t*)CFDictionaryGetValue(process->call_stacks, &searched_adress)))
			result = addr_info->descr;
	}

    return result;
}

static void update_process_stacks(pid_update_info_t *update_info)
{
	if (NULL == update_info)
		return;

#define BUFF_LEN 2048
    char cmd_line[BUFF_LEN];
    char buff[BUFF_LEN];
    char *cmd_fmt = "atos -p %d ";
    sprintf( cmd_line, cmd_fmt, update_info->pid );

    addr_list_t *addr_rec = update_info->addresses;
    while(addr_rec)
    {
		sprintf(buff, "0x%llx ", addr_rec->key);
		strcat(cmd_line, buff);
        
        addr_rec = addr_rec->next_ptr;
    }

	pid_tree_t searched_process, *process = 0;
	searched_process.key = update_info->pid;

	if( NULL == (process = (pid_tree_t*)CFDictionaryGetValue(processes_tree, &searched_process)) )
	{
		process = calloc(1, sizeof(pid_tree_t));

        CFDictionaryKeyCallBacks tasksKeysCallBask = {0, NULL, NULL, NULL, KeysCompareFunc, KeysHashFunc};
        CFDictionaryValueCallBacks tasksValueCallBask = {0, NULL, KeyFreeFunc, NULL, KeysCompareFunc};
        process->call_stacks = CFDictionaryCreateMutable(NULL, 0, &tasksKeysCallBask, &tasksValueCallBask);
		process->key = update_info->pid;
        process->type = PidNode;

        CFDictionaryAddValue(processes_tree, process, process);
	}

    FILE *out = popen(cmd_line, "r");

	call_stack_tree_t *addr_info = 0;
	addr_list_t *cur_addr = update_info->addresses;
	while (fgets(&buff[0], BUFF_LEN, out) && cur_addr)
    {
		addr_info = calloc(1, sizeof(call_stack_tree_t));
        addr_info->type = CallStackNode;
		addr_info->key = cur_addr->key;
		addr_info->descr = strdup(buff);
        CFDictionaryAddValue(process->call_stacks, addr_info, addr_info);
		cur_addr = cur_addr->next_ptr;
    }
    pclose(out);
    
    addr_list_t *tmp, *addr = update_info->addresses;
    while (addr) {
        tmp = addr;
        addr = addr->next_ptr;
        free(tmp);
    }
	free(update_info);
	
#undef BUFF_LEN
}

/****************************************************************************/
// Helpers for CFMutableDictionaryRef
/****************************************************************************/
static Boolean KeysCompareFunc(const void *left, const void *right)
{
    tree_node_type_t *node_type_left = (tree_node_type_t*)left;
    tree_node_type_t *node_type_right = (tree_node_type_t*)right;
    
    if(*node_type_left == CallStackNode && *node_type_right == CallStackNode)
    {
        call_stack_tree_t *left_node = (call_stack_tree_t*)left;
        call_stack_tree_t *right_node = (call_stack_tree_t*)right;
        return left_node->key == right_node->key;
    }
    else if (*node_type_left == PidNode && *node_type_right == PidNode)
    {
        pid_tree_t *left_node = (pid_tree_t*)left;
        pid_tree_t *right_node = (pid_tree_t*)right;
        return left_node->key == right_node->key;
    }
    
    return false;
}

static CFHashCode KeysHashFunc(const void *value)
{
    tree_node_type_t *node_type = (tree_node_type_t*)value;
    
    if(*node_type == CallStackNode)
        return ((call_stack_tree_t*)value)->key;
    else if (*node_type == PidNode)
        return ((pid_tree_t*)value)->key;

    return false;
}

static void KeyFreeFunc(CFAllocatorRef allocator, const void *value)
{
    tree_node_type_t *node_type = (tree_node_type_t*)value;
    if(*node_type == CallStackNode)
    {
        call_stack_tree_t *call_stack_node = (call_stack_tree_t*)value;
        free(call_stack_node->descr);
        free(call_stack_node);
    }
    else if (*node_type == PidNode)
    {
        pid_tree_t *pid_node = (pid_tree_t*)value;
        CFDictionaryRemoveAllValues(pid_node->call_stacks);
        free(pid_node);
    }
}

