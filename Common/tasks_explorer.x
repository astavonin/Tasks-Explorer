const MAX_TASKS = 2048;
const MAX_PATH = 1024;
const MAX_PARAMS_LEN = 32767;
const MAX_ENV_RECORD = 32767;
const MAX_APP_DESCR = 1024;

typedef int tasks_list<MAX_TASKS>;
typedef string command_line<MAX_PARAMS_LEN>;
typedef string app_descr<MAX_APP_DESCR>;

typedef string env_type<MAX_ENV_RECORD>;
typedef struct env_node *env_list;
struct env_node {
	env_type name;
	env_list next;
};

typedef string file_name<MAX_PATH>;
typedef struct files_list_node *files_list;
struct files_list_node {
	file_name name;
	files_list next;
};

struct host_info_dynamic {
	unsigned int cpu_kernel;
	unsigned int cpu_user;
};

struct task_info_base {
	unsigned int pid;
	unsigned int ppid;
	char name[64];
	char user[64];
	unsigned int cputype;
	char path_to_boundle[MAX_PATH];
	char path_to_executable[MAX_PATH];
	char path_to_icon[MAX_PATH];
	char bundle_ver[64];
	char bundle_copyright[1024];
};
typedef struct task_info_base task_info_base_t;

struct task_info_dynamic {
	unsigned int threads;
	unsigned int ports;
	unsigned int real_mem_size;
	unsigned int virtual_mem_size;
	float cpu_usage_total;
	float cpu_usage_user;
	float cpu_usage_kernel;
};


typedef struct thread_info_node *thread_info_list;
struct thread_info_node {
    unsigned int run_state;
    unsigned int user_time;
    unsigned int system_time;
    unsigned int suspend_count;
    unsigned int sleep_time;
    unsigned int flags;
    string entry_point_name<1024>;
    unsigned long thread_id;

	thread_info_list next;
};

typedef struct stack_info_node *stack_info_list;
struct stack_info_node {
    unsigned long return_addr;
    unsigned long frame_addr;
    string func_name<1024>;
    
	stack_info_list next;
};

struct call_stack_request {
    unsigned int pid;
    unsigned long tid;
};

program TASK_EXPLORER_PROG {
     version TASK_EXPLORER { 
          int TASK_EXPLORER_INIT() = 1;
          int TASK_EXPLORER_FREE() = 2;
          host_info_dynamic TASK_EXPLORER_UPDATE() = 3;
          tasks_list TASK_EXPLORER_TASKS_LIST() = 5;
		task_info_base TASK_EXPLORER_BASE_INFO(int) = 6;
		command_line TASK_EXPLORER_PARAMS(int) = 7;
		int KILL_PROCESS(int) = 8;
		env_list TASK_EXPLORER_ENV_LIST(int) = 9;
		task_info_dynamic TASK_EXPLORER_DYNINFO(int)=10;
		files_list TASK_EXPLORER_FILES_LIST(int)=11;
		app_descr TASK_EXPLORER_APP_DESCR(int)=12;
        thread_info_list TASK_EXPLORER_THREADS_INFO(int)=13;
        stack_info_list TASK_EXPLORER_STACK_FOR_THREAD(struct call_stack_request)=14;
     } = 1; 
} = 1;

