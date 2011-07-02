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

#import "ThreadInfo.h"

#import "DaemonDataSource.h"


@implementation CallStackRecord

@synthesize returnAddr;
@synthesize frameAddr;
@synthesize funcName;

-(void)initWithStackInfo:(struct stack_info_node*)stackInfo
{
    returnAddr = [[NSNumber alloc] initWithLong:stackInfo->return_addr];
    frameAddr = [[NSNumber alloc] initWithLong:stackInfo->frame_addr];
    funcName = [[NSString alloc] initWithCString:stackInfo->func_name encoding:NSUTF8StringEncoding];
}

+(CallStackRecord*)createStackInfo:(struct stack_info_node*)stackInfo
{
    CallStackRecord *stackRecord = [[CallStackRecord alloc] init];
    [stackRecord initWithStackInfo:stackInfo];

    return stackRecord;
}

-(void)dealloc
{
    [returnAddr release];
    [frameAddr release];
    [funcName release];
    
    [super dealloc];
}

@end

@implementation ThreadInfo

@synthesize runState;
@synthesize userTime;
@synthesize systemTime;
@synthesize suspendCount;
@synthesize sleepTime;
@synthesize flags;
@synthesize threadId;
@synthesize totalTime;
@synthesize entryPoint;

+(ThreadInfo*)createThreadInfo:(struct thread_info_node*)threadInfo
{
    ThreadInfo *tinfo = [[ThreadInfo alloc] init];
    [tinfo initWithThreadInfo:threadInfo];
    
    return tinfo;
}

-(void)initWithThreadInfo:(struct thread_info_node*)threadInfo
{
    runState = [[NSNumber alloc] initWithInt:threadInfo->run_state];
    userTime = [[NSNumber alloc] initWithInt:threadInfo->user_time];
    systemTime = [[NSNumber alloc] initWithInt:threadInfo->system_time];
    totalTime = [[NSNumber alloc] initWithInt:threadInfo->user_time+threadInfo->system_time];
    suspendCount = [[NSNumber alloc] initWithInt:threadInfo->suspend_count];
    sleepTime = [[NSNumber alloc] initWithInt:threadInfo->sleep_time];
    flags = [[NSNumber alloc] initWithInt:threadInfo->flags];
    threadId = [[NSNumber alloc] initWithInt:threadInfo->thread_id];
    entryPoint = [[NSString alloc] initWithCString:threadInfo->entry_point_name encoding:NSUTF8StringEncoding];
}

- (void)dealloc
{
    [runState release];
    [userTime release];
    [systemTime release];
    [suspendCount release];
    [sleepTime release];
    [flags release];
    [threadId release];
    [totalTime release];
    [entryPoint release];

    [super dealloc];
}

@end
