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

#import <Cocoa/Cocoa.h>

@class DaemonDataSource;

struct task_info_dynamic;

@interface ProcessInfo : NSObject
{
	NSString *name;
	NSString *commandLine;
	NSNumber *pid;
	NSNumber *ppid;

	NSNumber *realMemSize;
	NSNumber *virtualMemSize;

	NSNumber *threadsCount;
	NSNumber *portsCount;
	NSNumber *cpuUsageTotal;
	NSNumber *cpuUsageUser;
	NSNumber *cpuUsageKernel;

	NSNumber *cpuType;
	NSString *user;
	NSURL *path;
	NSString *pathName;

	NSMutableArray *children;

	NSImage *icon_small;
	NSImage *icon_large;

	NSString *copyright;
	NSString *version;

	NSInteger processState; // 0 - active, -1 - died, 1 - new.

	NSMutableArray *envList;
	NSMutableArray *fileList;
    
    NSMutableArray *threadList;
	NSMutableArray *callStacksList;
    
    NSString *description;

	DaemonDataSource *dataSource;

	struct task_info_dynamic *taskInfoDynCopy;
}

+(ProcessInfo*)createProcessInfo:(pid_t)procPid withDataSource:(DaemonDataSource*)data;

-(void)initWithPID:(pid_t) thePID;
-(void)addChild:(ProcessInfo*)child;
-(void)removeChild:(ProcessInfo*)child;
-(void)killSelf;
-(void)askForExtendedInfo;
-(void)updateDynInfo:(BOOL)init;
-(void)updateThreadsInfo;
-(NSArray*)generateStackTrace:(long)tid;

@property (assign) NSInteger processState;
@property (readonly, retain) NSString *pathName;
@property (readonly, retain) NSNumber *cpuUsageTotal;
@property (readonly, retain) NSNumber *cpuUsageUser;
@property (readonly, retain) NSNumber *cpuUsageKernel;
@property (readonly, retain) NSNumber *realMemSize;
@property (readonly, retain) NSNumber *virtualMemSize;
@property (readonly, retain) NSString *copyright;
@property (readonly, retain) NSString *version;
@property (readonly, retain) NSImage *icon_small;
@property (readonly, retain) NSImage *icon_large;
@property (readonly, retain) NSString *name;
@property (readonly, retain) NSString *commandLine;
@property (readonly, retain) NSURL *path;
@property (readonly, retain) NSString *user;
@property (readonly, retain) NSString *description;
@property (readonly, retain) NSNumber *pid;
@property (readonly, retain) NSNumber *ppid;
@property (readonly, retain) NSNumber *threadsCount;
@property (readonly, retain) NSNumber *portsCount;
@property (readonly, retain) NSNumber *cpuType;
@property (readonly, retain) NSMutableArray *envList;
@property (readonly, retain) NSMutableArray *fileList;
@property (readonly, retain) NSMutableArray *threadList;

@property (assign) NSMutableArray *children;

@end
