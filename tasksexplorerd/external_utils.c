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
#include "external_utils.h"

//const char cmd_template[] = "lsof -p %d | awk '/REG|IPv/{print substr($5,"",1) \" \" $9\" \"$10}'";
// 
const char cmd_template[] = "lsof -p %d | awk '/REG/{for (i=9; i <= NF; i++){printf(\"%%s \", $(i))}printf(\"\\n\")}'";


string_list_t files_list_for_task(int pid)
{
	string_list_t result = {};
	char cmd[256]={};
	FILE *flsof ;

	sprintf(cmd, cmd_template, pid);
	if (NULL == (flsof = popen(cmd, "r"))) {
		return result;
	}

	char path[1024];
	char **files = 0;
	int item_cout;
	int i, iter, buf_len = 256;
	int more_data = 1;

	for (iter = 0; more_data != 0;) {
		files = realloc(files, buf_len*(iter+1)*sizeof(char*));
		for (i=0; i<buf_len; ++i) {
			if (fgets(path, 1024, flsof)) {
				files[iter*buf_len+i] = strdup(path);
			} else {
				item_cout = iter*buf_len+i;
				more_data = 0;
				break;
			}
		}
	}
	result.count = item_cout;
	result.strings = files;

	return result;
}
