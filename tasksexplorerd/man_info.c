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

#include "afx.h"

#include "sglib.h"
#include "tasks_explorer.h"
#include <pthread.h>
#include <semaphore.h>

#include "man_info.h"


#define CMPARATOR(x,y) (strcmp(x->key,y->key))

//////////////////////////////////////////////////
// Process description map
//////////////////////////////////////////////////
typedef struct descr_tree {
	char *key;
	char *descr;
	char color_field;
	struct descr_tree *left;
	struct descr_tree *right;
} descr_tree;

SGLIB_DEFINE_RBTREE_PROTOTYPES(descr_tree, left, right, color_field, CMPARATOR);
SGLIB_DEFINE_RBTREE_FUNCTIONS(descr_tree, left, right, color_field, CMPARATOR);

static struct descr_tree *man_info_tree = 0;

//////////////////////////////////////////////////
// Static variables
//////////////////////////////////////////////////

//static pthread_t update_thread;
static int is_active = 1;
static pthread_mutex_t descr_tree_lock;

//////////////////////////////////////////////////
// Internal function prototypes
//////////////////////////////////////////////////

static char* get_man_by_name( const char* name );
static void update_info(void *context);

//////////////////////////////////////////////////
// External functions
//////////////////////////////////////////////////


int man_info_init()
{
    if (man_info_tree) {
        return 0;
    }

    pthread_mutex_init(&descr_tree_lock, NULL);

    return 0;
}

void man_info_free()
{
	struct descr_tree *te;
    void *status;
	struct sglib_descr_tree_iterator  it;

	for(te=sglib_descr_tree_it_init(&it,man_info_tree); te!=NULL; te=sglib_descr_tree_it_next(&it)) {
		free( te->descr );
		free( te->key );
		free( te );
	}

    is_active = 0;
    pthread_mutex_destroy(&descr_tree_lock);

    pthread_exit(NULL);
}

const char* man_info_get_descr_by_name( const char *name )
{
	const char *result = 0;
    
	struct descr_tree e, *t;
	e.key = (char*)name;
    pthread_mutex_lock(&descr_tree_lock);
	if( NULL != (t = sglib_descr_tree_find_member(man_info_tree, &e)) ){
		result = t->descr;
	}
    pthread_mutex_unlock(&descr_tree_lock);
	
	return result;
}

void man_info_add_descr_by_name( const char *name )
{
	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	dispatch_async_f(queue, (void*)strdup(name), update_info);
}

////////////////////////////////////////////////////////
// Internal functions
////////////////////////////////////////////////////////


static void update_info(void *context)
{
	char *process_name = (char*)context;
	struct proc_list *l, *the_list;
	struct descr_tree e, *t;
	e.key = process_name;

	pthread_mutex_lock(&descr_tree_lock);
	if( NULL == sglib_descr_tree_find_member(man_info_tree, &e) ){
		t = malloc(sizeof(struct descr_tree));
		t->key = process_name;
		t->descr = get_man_by_name( process_name );
		sglib_descr_tree_add(&man_info_tree, t);
	}
	else {
		free(process_name);
	}

	pthread_mutex_unlock(&descr_tree_lock);
}

static char* get_man_by_name( const char* name )
{
#define buff_len 1024
	char *result = 0;
	const char *cmd_fmt = "man %s | col -b";
	char cmd_line[buff_len] = {};
    
	if (buff_len <= (strlen( name ) + strlen( cmd_fmt ) ) ) {
		return 0;
	}
	
	snprintf( cmd_line, buff_len, cmd_fmt, name);
    
	FILE *out = popen( cmd_line, "r" );
	char buff[buff_len] = {};
	char *position = 0;
	int len = 0;

	while (!fgetc(out)) {}
	while (fgets(buff, buff_len-1, out)) {
		if ( 0 == strncmp("NAME", buff, 4) ) {
			if( fgets(buff, buff_len-1, out) ) {
				position = buff;
				while (' ' == *position) {
					position++;
				}
				len = strchr(position, '\n') - position;
				result = calloc(len+1, sizeof(char));
				strncpy(result, position, len);
				break;
			}
		}
	}
    pclose(out);
    
	return result;
#undef buff_len
}
