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

#import <Foundation/Foundation.h>

struct stack_info_node;

@interface CallStackRecord : NSObject
{
    NSNumber *returnAddr;
    NSNumber *frameAddr;
    NSString *funcName;
}

+(CallStackRecord*)createStackInfo:(struct stack_info_node*)stackInfo;

@property (retain, readonly) NSNumber *returnAddr;
@property (retain, readonly) NSNumber *frameAddr;
@property (retain, readonly) NSString *funcName;

@end

struct thread_info_node;
@class DaemonDataSource;

@interface ThreadInfo : NSObject 
{
    NSNumber* runState;
	NSNumber* totalTime;
	NSNumber* userTime;
	NSNumber* systemTime;
	NSNumber* suspendCount;
	NSNumber* sleepTime;
	NSNumber* flags;
	NSNumber* threadId;
    NSString* entryPoint;
}

+(ThreadInfo*)createThreadInfo:(struct thread_info_node*)threadInfo;

-(void)initWithThreadInfo:(struct thread_info_node*)threadInfo;

@property (retain, readonly) NSNumber *runState;
@property (retain, readonly) NSNumber *userTime;
@property (retain, readonly) NSNumber *systemTime;
@property (retain, readonly) NSNumber *suspendCount;
@property (retain, readonly) NSNumber *sleepTime;
@property (retain, readonly) NSNumber *flags;
@property (retain, readonly) NSNumber *threadId;
@property (retain, readonly) NSNumber* totalTime;
@property (retain, readonly) NSString* entryPoint;

@end
