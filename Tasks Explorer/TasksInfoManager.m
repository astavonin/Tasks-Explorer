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

#import "ProcessInfo.h"
#import "Settings.h"
#import "DaemonDataSource.h"

#import "TasksInfoManager.h"

#define GLOBAL_CHG_VAL

@implementation TasksInfoManager

@synthesize processes;
@synthesize rootProcess;
@synthesize cpuLoadUser;
@synthesize cpuLoadKernel;

static TasksInfoManager *instance_;
static const int historyObjCnt = 40;

static void singleton_remover()
{
    [instance_ release];
}

-(void)addProcessInfo:(ProcessInfo*)newInfo
{
	if ([newInfo.pid intValue] == 0) {
		rootProcess = [[NSArray alloc] initWithObjects:newInfo, nil];
	}

	for (ProcessInfo *pInfo in processes) {
		// looking for parent of newInfo process 
		if ([pInfo.pid isEqualToNumber:newInfo.ppid]) {
				[pInfo addChild:newInfo];
				break;
			}
	}
	for (ProcessInfo *pInfo in processes) {
		// looking for childeren of newInfo process
		if ([pInfo.ppid isEqualToNumber:newInfo.pid]) {
			[newInfo addChild:pInfo];
		}
	}
#ifdef LOCAL_CHG_VAL
	[self willChangeValueForKey:@"processes"];
#endif
	[processes addObject:newInfo];
#ifdef LOCAL_CHG_VAL
	[self didChangeValueForKey:@"processes"];
#endif
}

-(void)removeProcessInfo:(ProcessInfo*)removedInfo
{
	for (ProcessInfo *pInfo in processes) {
		if ([pInfo.pid isEqualToNumber:removedInfo.ppid]) {
			[pInfo removeChild:removedInfo];
			break;
		}
	}
#ifdef LOCAL_CHG_VAL
	[self willChangeValueForKey:@"processes"];
#endif
	[processes removeObject:removedInfo];
#ifdef LOCAL_CHG_VAL
	[self didChangeValueForKey:@"processes"];
#endif
	[removedInfo release];
}

+(TasksInfoManager*)instance 
{
    @synchronized(self) {
        if( instance_ == nil) {
			[[self alloc] init];
        }
    }
    return instance_;
}

-(id)init
{
	if ((self = [super init])) {

		dataSource = [[DaemonDataSource alloc] init];
		if (dataSource == nil) {
			[self release];
			return nil;
		}
		atexit(singleton_remover);
		instance_ = self;

		processes = [[NSMutableArray alloc] init];
		[self updateInfo];
		[self updateInfo]; // updating dynamic info
		NSTimeInterval curUpdateFrequency = [[Settings instance] updateInterval];
		updateTimer = [NSTimer scheduledTimerWithTimeInterval:curUpdateFrequency target:self 
													 selector:@selector(updateInfo) userInfo:nil repeats:YES];

		cpuLoadUser = [[NSMutableArray alloc] initWithCapacity:historyObjCnt];
		cpuLoadKernel = [[NSMutableArray alloc] initWithCapacity:historyObjCnt];
		int i;
		for (i=0; i<historyObjCnt; ++i) {
			[cpuLoadUser addObject:[NSDecimalNumber numberWithInt:0]];
			[cpuLoadKernel addObject:[NSDecimalNumber numberWithInt:0]];
		}

		updateFrequency = curUpdateFrequency;
	}
}

-(void)chkTimerInterval
{
	if (updateFrequency == 0) {
		return;
	}
	NSTimeInterval curUpdateFrequency = [[Settings instance] updateInterval];
	if (curUpdateFrequency != updateFrequency) {
		[updateTimer invalidate];
		updateTimer = [NSTimer scheduledTimerWithTimeInterval:curUpdateFrequency target:self
													 selector:@selector(updateInfo) userInfo:nil repeats:YES];
		updateFrequency = curUpdateFrequency;
	}
}

-(void)updateInfo
{
	[dataSource updateProcessInfo];
	int i, j, cntCurrent = [processes count]
			, cntNew = [dataSource processesCount];
	int *tasksList = [dataSource processesPIDCArray];
	ProcessInfo *procInfo;

#ifdef GLOBAL_CHG_VAL
	[self willChangeValueForKey:@"rootProcess"];
	[self willChangeValueForKey:@"processes"];
#endif

	if (cntCurrent == 0) {
		// startup
		for (i = 0; i < cntNew; i++) {
			procInfo = [ProcessInfo createProcessInfo:tasksList[i] withDataSource:dataSource];
			procInfo.processState = 0;
			[self addProcessInfo:procInfo];
		}
	}
	else {
		BOOL *oldThread = calloc(cntNew, sizeof(BOOL));
		BOOL killed;
		NSMutableArray *toDelete;

		toDelete = [[NSMutableArray alloc] init];
		for (ProcessInfo *pInfo in processes) {
			if (pInfo.processState == -1) {
				[toDelete addObject:pInfo];
				continue;
			}
			if (pInfo.processState != 0) {
#ifdef LOCAL_CHG_VAL
				[self willChangeValueForKey:@"rootProcess"];
				[self willChangeValueForKey:@"processes"];
#endif
				pInfo.processState = 0;
#ifdef LOCAL_CHG_VAL
				[self didChangeValueForKey:@"processes"];
				[self didChangeValueForKey:@"rootProcess"];
#endif
			}
		}
		for (ProcessInfo *delInfo in toDelete) {
			[self removeProcessInfo:delInfo];
			cntCurrent--;
		}
		[toDelete release];

		for (i = 0; i < cntCurrent; i++) {
			procInfo = [processes objectAtIndex:i];
			killed = YES;
			for (j = 0; j < cntNew; j++) {
				if ([procInfo.pid intValue] == tasksList[j] ) {
					killed = NO;
					oldThread[j] = YES;
					break;
				}
			}
			if (killed == YES) {
#ifdef LOCAL_CHG_VAL
				[self willChangeValueForKey:@"rootProcess"];
				[self willChangeValueForKey:@"processes"];
#endif
				procInfo.processState = -1;
#ifdef LOCAL_CHG_VAL
				[self didChangeValueForKey:@"processes"];
				[self didChangeValueForKey:@"rootProcess"];
#endif
			}
			else {
				[procInfo updateDynInfo:NO];
			}

		}
#ifdef LOCAL_CHG_VAL
		[self willChangeValueForKey:@"rootProcess"];
		[self willChangeValueForKey:@"processes"];
#endif
		for (i = 0; i < cntNew; i++) {
			if (oldThread[i] == NO) {
				procInfo = [ProcessInfo createProcessInfo:tasksList[i] withDataSource:dataSource];
				[self addProcessInfo:procInfo];
			}
		}
#ifdef LOCAL_CHG_VAL
		[self didChangeValueForKey:@"processes"];
		[self didChangeValueForKey:@"rootProcess"];
#endif
		
		free(oldThread);
	}
#ifdef GLOBAL_CHG_VAL
	[self didChangeValueForKey:@"processes"];
	[self didChangeValueForKey:@"rootProcess"];
#endif
	
	host_info_dynamic *host_info = [dataSource hostInfoDynamic];

	[cpuLoadUser insertObject:[NSDecimalNumber numberWithInt:host_info->cpu_user] atIndex:0];
	[cpuLoadUser removeLastObject];
	[cpuLoadKernel insertObject:[NSDecimalNumber numberWithInt:host_info->cpu_kernel] atIndex:0];
	[cpuLoadKernel removeLastObject];

	[updateObject performSelector:updateSel];

	[self chkTimerInterval];
}

-(BOOL)alive
{
	return (dataSource != nil);
}

-(void)updateEventForObject:(id)object withSelector:(SEL)selector;
{
	updateObject = object;
	updateSel = selector;
}

-(void)dealloc
{
	[updateTimer invalidate];

	[processes release];
	[dataSource release];
	[rootProcess release];
	[super dealloc];
}

@end
