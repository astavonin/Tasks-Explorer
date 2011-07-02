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

#import "Settings.h"

#import "Transformers.h"


@implementation MemSizeTransformer

+ (Class)transformedValueClass;
{
	return [NSString class];
}

+ (BOOL)allowsReverseTransformation;
{
	return NO;
}

-(id)transformedValue:(id)value;
{
	NSString *memSize;
	uint size = [value unsignedIntValue];
	NSString *result;
	static int GB = 1000000000;
	static int MB = 1000000;
	static int KB = 1000;
	if (size/GB > 0) {
		result = [[[NSString alloc] initWithFormat:@"%.1f GB", (float)size/GB] autorelease];
	}
	else if (size/MB > 0) {
		result = [[[NSString alloc] initWithFormat:@"%.1f MB", (float)size/MB] autorelease];
	}
	else {
		result = [[[NSString alloc] initWithFormat:@"%.1f KB", (float)size/KB] autorelease];
	}

	return result;	
}

@end

@implementation CPUArchTransformer

static NSString *kCPUx86_64 = @"64bit";
static NSString *kCPUx86_32 = @"32bit";
static NSString *kCPUUnknown = @"unknown";

+(Class)transformedValueClass;
{
	return [NSString class];
}

+ (BOOL)allowsReverseTransformation;
{
	return NO;
}

-(id)transformedValue:(id)value;
{
	NSString *result;
	int type = [value intValue];
	switch (type) {
		case CPU_TYPE_X86_64:
			result = kCPUx86_64;
			break;
		case CPU_TYPE_X86:
			result = kCPUx86_32;
			break;
		default:
			result = kCPUUnknown;
			break;
	}
	return result;
}

@end

@implementation UpdateFeqTransformer

+(Class)transformedValueClass;
{
	return [NSString class];
}

+ (BOOL)allowsReverseTransformation;
{
	return NO;
}

static NSString *kHightFrequency = @"0.5 sec.";
static NSString *kNormalFrequency = @"1 sec.";
static NSString *kLowFrequency = @"2 sec.";

-(id)transformedValue:(id)value;
{
	NSMutableArray *result = [[[NSMutableArray alloc] init] autorelease];
	int i, count = [value count];

	for (i = 0; i < count; i++) {
		switch ([[value objectAtIndex:i] intValue]) {
			case hightFrequency:
				[result addObject: kHightFrequency];
				break;
			case normalFrequency:
				[result addObject: kNormalFrequency ];
				break;
			case lowFrequency:
				[result addObject: kLowFrequency];
				break;
			default:
				[result addObject: kNormalFrequency];
				break;
		}
	}

	return result;
}	

@end

@implementation RunStateTransformer

static NSString *kStateRunning = @"Running";
static NSString *kStateStopped = @"Stopped";
static NSString *kStateWaiting = @"Waiting";
static NSString *kStateUninterruptible = @"Uninterruptible";
static NSString *kStateHalted = @"Halted";

+(Class)transformedValueClass;
{
	return [NSString class];
}

+ (BOOL)allowsReverseTransformation;
{
	return NO;
}

-(id)transformedValue:(id)value;
{
	NSString *result;
	int type = [value intValue];
	switch (type) {
		case TH_STATE_RUNNING:
			result = kStateRunning;
			break;
		case TH_STATE_STOPPED:
			result = kStateStopped;
			break;
		case TH_STATE_WAITING:
			result = kStateWaiting;
			break;
		case TH_STATE_UNINTERRUPTIBLE:
			result = kStateUninterruptible;
			break;
		case TH_STATE_HALTED:
			result = kStateHalted;
			break;
	}
	return result;
}

@end

@implementation RunFlagsTransformer

static NSString *kStateSwapped = @"Swapped";
static NSString *kStateIdle = @"Idle";

+(Class)transformedValueClass;
{
	return [NSString class];
}

+ (BOOL)allowsReverseTransformation;
{
	return NO;
}

-(id)transformedValue:(id)value;
{
	NSString *result;
	int type = [value intValue];
	switch (type) {
		case TH_FLAGS_SWAPPED:
			result = kStateSwapped;
			break;
		case TH_FLAGS_IDLE:
			result = kStateIdle;
			break;
	}
	return result;
}

@end


@implementation HexTransformer

+(Class)transformedValueClass;
{
	return [NSString class];
}

+ (BOOL)allowsReverseTransformation;
{
	return NO;
}

-(id)transformedValue:(id)value;
{
	return [[[NSString alloc] initWithFormat:@"0x%llx", [value longValue]] autorelease];
}

@end
