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

#include "helpers.h"
#include "taskinfomanager.h"
#include "tasks_explorer.h"
#include "external_utils.h"

#include "bundleinforeader.h"

static int is_gui_app(task_record_t *task, CFBundleRef bundle);
static int get_path_to_icon(task_record_t *task, CFBundleRef bundle, task_info_base *task_info);
static int update_task_name(task_record_t *task, CFBundleRef bundle, task_info_base *task_info);
static int get_bundle_copyright(task_record_t *task, CFBundleRef bundle, task_info_base *task_info);
static int get_bundle_ver(task_record_t *task, CFBundleRef bundle, task_info_base *task_info);


int extract_bundle_info(task_record_t *task, task_info_base *task_info)
{
     int result;

     if (task == NULL || task->bundle_path_name == NULL) {
          return -1;
     }

     CFURLRef url = CFURLCreateFromFileSystemRepresentation(NULL, (UInt8*)task->bundle_path_name, 
                                                                         strlen(task->bundle_path_name), TRUE);
     if (url == NULL) {
          return -1;
     }
     CFBundleRef bundle = CFBundleCreate(NULL, url);
     if (bundle == NULL) {
          CFRelease(url);
          return -1;
     }
     
     do {
		if (!is_gui_app(task, bundle)) {
			break;
		}
		update_task_name(task, bundle, task_info);

		result = get_path_to_icon(task, bundle, task_info);
		if (result != 0) {
			break;
		}
		result = get_bundle_ver(task, bundle, task_info);
		if (result != 0) {
			break;
		}
		result = get_bundle_copyright(task, bundle, task_info);
		if (result != 0) {
			break;
		}

		result = 0;
     } while (FALSE);    
     
     CFRelease(bundle);
     CFRelease(url);

     return result; 
}

static int update_task_name(task_record_t *task, CFBundleRef bundle, task_info_base *task_info)
{
     CFTypeRef value = CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("CFBundleName"));
     if (value == NULL) {
          return -1;
     }
     
     if (CFGetTypeID( value ) != CFStringGetTypeID()) {
          return -1;
     }
     const char *task_name = CFStringGetCStringPtr(value, 0);
     if (task_name == NULL) {
          return -1;
     }
	strcpy(task_info->name, task_name);
}

int is_gui_app(task_record_t *task, CFBundleRef bundle)
{
	int is_gui=0;

	CFTypeRef value = CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("CFBundleExecutable"));
	if (value == NULL) {
		return is_gui;
	}
	
	if (CFGetTypeID( value ) != CFStringGetTypeID()) {
		return is_gui;
	}

	const char *app_name = CFStringGetCStringPtr(value, 0);
	if (app_name == NULL) {
		return is_gui;
	}
	
	is_gui = (strcmp(app_name, task->app_name) == 0);

	return is_gui;
}

static int get_path_to_icon(task_record_t *task, CFBundleRef bundle, task_info_base *task_info)
{
     CFTypeRef value = CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("CFBundleIconFile"));
     if (value == NULL) {
          return -1;
     }
     
     if (CFGetTypeID( value ) != CFStringGetTypeID()) {
          return -1;
     }
     const char *icon_name = CFStringGetCStringPtr(value, 0);
     if (icon_name == NULL) {
          return -1;
     }
     
     static const char int_path[] = "/Contents/Resources/";
     static const char icns[] = ".icns";
     if (sizeof(task_info->path_to_icon) < (strlen(task->bundle_path_name) + sizeof(int_path) + strlen(icon_name) + sizeof(icns))) {
          return -1;
     }
     strcpy(task_info->path_to_icon, task->bundle_path_name);
     strcat(task_info->path_to_icon, int_path);
     strcat(task_info->path_to_icon, icon_name);
     if (strstr(icon_name, icns) == NULL) {
          strcat(task_info->path_to_icon, icns);
     }
     
     return 0;
}

static int get_bundle_ver(task_record_t *task, CFBundleRef bundle, task_info_base *task_info)
{
     CFTypeRef value = CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("CFBundleShortVersionString"));
     if (value == NULL) {
          return 0;
     }
     
     if (CFGetTypeID( value ) != CFStringGetTypeID()) {
          return 0;
     }
     CFStringGetCString(value, task_info->bundle_ver, sizeof(task_info->bundle_ver), kCFStringEncodingMacRoman);
     
     return 0;
}

static int get_bundle_copyright(task_record_t *task, CFBundleRef bundle, task_info_base *task_info)
{
     CFTypeRef value = CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("NSHumanReadableCopyright"));
     if (value == NULL) {
          value = CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("CFBundleGetInfoString"));
          if (value == NULL) {
               return 0;
          }
     }
     
     if (CFGetTypeID( value ) != CFStringGetTypeID()) {
          return 0;
     }
     CFStringGetCString(value, task_info->bundle_copyright, sizeof(task_info->bundle_copyright), kCFStringEncodingMacRoman);
     
     return 0;
}
