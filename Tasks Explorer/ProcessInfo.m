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

#import "EnvVariable.h"
#import "DaemonDataSource.h"
#import "ThreadInfo.h"

#import "ProcessInfo.h"

@implementation ProcessInfo

@synthesize name;
@synthesize user;
@synthesize pid;
@synthesize ppid;
@synthesize threadsCount;
@synthesize portsCount;
@synthesize cpuType;
@synthesize children;
@synthesize path;
@synthesize pathName;
@synthesize icon_small;
@synthesize icon_large;
@synthesize processState;
@synthesize commandLine;
@synthesize version;
@synthesize copyright;
@synthesize envList;
@synthesize fileList;
@synthesize cpuUsageTotal;
@synthesize cpuUsageKernel;
@synthesize cpuUsageUser;
@synthesize realMemSize;
@synthesize virtualMemSize;
@synthesize threadList;


+(ProcessInfo*)createProcessInfo:(pid_t)procPid withDataSource:(DaemonDataSource*)data
{
	ProcessInfo *pInfo = [[ProcessInfo alloc] init];
	pInfo->dataSource = data;
	pInfo->taskInfoDynCopy = NULL;
	[pInfo initWithPID:procPid];
	
	return pInfo;
}

-(void)loadIcons:(NSString*)pathToIcon
{
	NSImage *image = [[NSImage alloc] initWithContentsOfFile:pathToIcon];
	if (image == nil) {
		return;
	}
	NSArray *arr = [image representations];
	for (NSImageRep *rep in arr) {
		NSSize repSize = [rep size];
		if (repSize.width == 16.0 && repSize.height == 16.0 && !icon_small ) {
			icon_small = [[NSImage alloc] initWithSize:repSize];
			[icon_small addRepresentation:rep];
			continue;
		}
		if (repSize.width == 128.0 && repSize.height == 128.0 && !icon_large ) {
			icon_large = [[NSImage alloc] initWithSize:repSize];
			[icon_large addRepresentation:rep];
			continue;
		}
	}
	[image release];
}

-(void)initWithPID:(pid_t)thePID
{
	task_info_base baseInfo;
	BOOL res;

	res = [dataSource processBaseInfo:&baseInfo forPID:thePID];
	if (res == YES) {
		pid = [[NSNumber alloc] initWithUnsignedInt: baseInfo.pid];
		ppid = [[NSNumber alloc] initWithUnsignedInt: baseInfo.ppid];
		name = [[NSString alloc] initWithCString: baseInfo.name];
		user = [[NSString alloc] initWithCString: baseInfo.user];
		path = [[NSURL alloc] initFileURLWithPath:
				[NSString stringWithCString:baseInfo.path_to_boundle encoding:NSUTF8StringEncoding] 
									  isDirectory:YES];
		pathName = [[NSString alloc] initWithCString:baseInfo.path_to_executable];

		cpuType = [[NSNumber alloc] initWithInt:baseInfo.cputype];

		NSString *pathToIcon = [[NSString alloc] initWithCString:baseInfo.path_to_icon];
		[self loadIcons:pathToIcon];
		[pathToIcon release];

		copyright = [[NSString alloc] initWithCString: baseInfo.bundle_copyright];
		version = [[NSString alloc] initWithCString: baseInfo.bundle_ver];
		processState = 1;
		children = [[NSMutableArray alloc] init];
	}
	[self updateDynInfo:YES];
}

-(void)updateDynInfo:(BOOL)init
{
	task_info_dynamic dynInfo={};
	BOOL res;

	res = [dataSource processDynInfo:&dynInfo forPID:[pid intValue]];
	if (taskInfoDynCopy == NULL) {
		taskInfoDynCopy = calloc(1, sizeof(task_info_dynamic));
	}
	if (res == YES) {
		if (taskInfoDynCopy->cpu_usage_total != dynInfo.cpu_usage_total || !init) {
			[cpuUsageTotal release];
			cpuUsageTotal = [[NSNumber alloc] initWithFloat:dynInfo.cpu_usage_total];
			taskInfoDynCopy->cpu_usage_total = dynInfo.cpu_usage_total;
		}
		if (taskInfoDynCopy->cpu_usage_kernel != dynInfo.cpu_usage_kernel || !init) {
			[cpuUsageKernel release];
			cpuUsageKernel = [[NSNumber alloc] initWithFloat:dynInfo.cpu_usage_kernel];
			taskInfoDynCopy->cpu_usage_kernel = dynInfo.cpu_usage_kernel;
		}
		if (taskInfoDynCopy->cpu_usage_user != dynInfo.cpu_usage_total || !init) {
			[cpuUsageUser release];
			cpuUsageUser = [[NSNumber alloc] initWithFloat:dynInfo.cpu_usage_user];
			taskInfoDynCopy->cpu_usage_user = dynInfo.cpu_usage_total;
		}
		if (taskInfoDynCopy->threads != dynInfo.threads || !init) {
			[threadsCount release];
			threadsCount = [[NSNumber alloc] initWithUnsignedInt: dynInfo.threads];
			taskInfoDynCopy->threads = dynInfo.threads;
		}
		if (taskInfoDynCopy->ports != dynInfo.ports || !init) {
			[portsCount release];
			portsCount = [[NSNumber alloc] initWithUnsignedInt: dynInfo.ports];
			taskInfoDynCopy->ports = dynInfo.ports;
		}
		if (taskInfoDynCopy->real_mem_size != dynInfo.real_mem_size || !init) {
			[realMemSize release];
			realMemSize = [[NSNumber alloc] initWithUnsignedInt: dynInfo.real_mem_size];
			taskInfoDynCopy->real_mem_size = dynInfo.real_mem_size;
		}
		if (taskInfoDynCopy->virtual_mem_size != dynInfo.virtual_mem_size || !init) {
			[virtualMemSize release];
			virtualMemSize = [[NSNumber alloc] initWithUnsignedInt: dynInfo.virtual_mem_size];
			taskInfoDynCopy->virtual_mem_size = dynInfo.virtual_mem_size;
		}
	}
}

-(void)askForExtendedInfo
{
	commandLine = [dataSource commandLineForPID:[pid intValue]];
	envList = [dataSource createEnvList:[pid intValue]];
	fileList = [dataSource createFilesList:[pid intValue]];
    threadList = [dataSource createThreadsList:[pid intValue]];
}

-(void)updateThreadsInfo
{
    NSMutableArray *newThreadList = [dataSource createThreadsList:[pid intValue]];
    NSMutableArray *deletedThreads = [[NSMutableArray alloc] init];

    BOOL wasFound = NO;
    ThreadInfo *newInfo = nil;
    for (ThreadInfo *currentInfo in threadList) {
        for (newInfo in newThreadList) {
            if ([currentInfo.threadId isEqualToNumber:newInfo.threadId]) {
                currentInfo = newInfo;
                wasFound = YES;
                break;
            }
        }
        if (wasFound) {
            [newThreadList removeObject:newInfo];
        } else {
            [deletedThreads addObject:currentInfo];
        }
        wasFound = NO;
    }
    for (ThreadInfo *deletedThread in deletedThreads) {
        [threadList removeObject:deletedThread];
    }
    for (newInfo in newThreadList) {
        [threadList addObject:newInfo];
    }

    [deletedThreads release];
    [newThreadList release];
}

-(NSArray*)generateStackTrace:(long)tid
{
    NSMutableArray *stack = [dataSource createCallStack:[pid intValue] withTid:tid];
    
	[callStacksList addObject:stack];
	
    return stack;
}

-(void)removeChild:(ProcessInfo*)child
{
	[self willChangeValueForKey:@"children"];

	[children removeObject:child];

	[self didChangeValueForKey:@"children"];
}

-(void)addChild:(ProcessInfo*)child
{
	[self willChangeValueForKey:@"children"];

	[children addObject:child];
	
	[self didChangeValueForKey:@"children"];
}

-(void)killSelf
{
	[dataSource killProcess:[pid intValue]];
}

-(NSUInteger)childerenConut
{
	return [children count];
}

-(NSString*)description
{
    [description autorelease];

    description = [dataSource processDescription:[pid intValue]];

    return description;
}

-(void)dealloc
{
	[pid release];
	[ppid release];
	[name release];
	[user release];
	[path release];
	[children release];
	[threadsCount release];
	[portsCount release];
	[cpuType release];
	[icon_small release];
	[icon_large release];
	[commandLine release];
	[copyright release];
	[version release];
	[envList release];
	[cpuUsageTotal release];
	[cpuUsageUser release];
	[cpuUsageKernel release];
	[realMemSize release];
	[virtualMemSize release];
	[pathName release];
    [description release];
    [threadList release];
	[callStacksList release];
	free(taskInfoDynCopy);

	[super dealloc];
}

@end
