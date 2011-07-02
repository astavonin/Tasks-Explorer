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

#import "SettingsController.h"

#import "Settings.h"

@implementation Settings

static Settings *instance_;

static void singleton_remover()
{
    [instance_ release];
}

+(Settings*)instance
{
    @synchronized(self) {
        if( instance_ == nil) {
			[[self alloc] init];
        }
    }
    return instance_;
}

-(void)initDefaults
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys: 
								 [NSNumber numberWithInt:1], @"currentView", 
								 [NSNumber numberWithInt:NSOnState], @"name",
								 [NSNumber numberWithInt:NSOnState], @"pid",
								 [NSNumber numberWithInt:NSOnState], @"cpu",
								 [NSNumber numberWithInt:NSOffState], @"cpu_user",
								 [NSNumber numberWithInt:NSOffState], @"cpu_kernel",
								 [NSNumber numberWithInt:NSOnState], @"real_mem",
								 [NSNumber numberWithInt:NSOffState], @"virtual_mem",
								 [NSNumber numberWithInt:NSOnState], @"user",
								 [NSNumber numberWithInt:NSOnState], @"threads",
								 [NSNumber numberWithInt:NSOffState], @"ports",
								 [NSNumber numberWithInt:NSOnState], @"cpu_type",
								 [NSNumber numberWithFloat:60], @"pid_len",
								 [NSNumber numberWithFloat:50], @"cpu_len",
								 [NSNumber numberWithFloat:50], @"cpu_user_len",
								 [NSNumber numberWithFloat:50], @"cpu_kernel_len",
								 [NSNumber numberWithFloat:60], @"real_mem_len",
								 [NSNumber numberWithFloat:60], @"virtual_mem_len",
								 [NSNumber numberWithFloat:110], @"user_len",
								 [NSNumber numberWithFloat:50], @"threads_len",
								 [NSNumber numberWithFloat:40], @"ports_len",
								 [NSNumber numberWithFloat:40], @"cpu_type_len",
								 [NSNumber numberWithInt:normalFrequency], @"update_frequency",
								 [NSNumber numberWithInt:YES], @"highlight_processes",
								 [NSNumber numberWithInt:YES], @"show_kernel_cpu_time",
								 [NSNumber numberWithInt:0], @"dock_icon",
								 [NSArray arrayWithObjects: [NSNumber numberWithInt:hightFrequency], 
								  [NSNumber numberWithInt:normalFrequency], 
								  [NSNumber numberWithInt:lowFrequency], nil], @"frequency_array",
								 nil];

    [defaults registerDefaults:appDefaults];
}

-(id)init
{
	if ((self = [super init])) {
		[self initDefaults];

		atexit(singleton_remover);
		instance_ = self;
	}
}

-(void)showSettingsDlg
{
	[[SettingsController sharedPrefsWindowController] showWindow:nil];
}

-(NSTimeInterval)updateInterval
{
	NSNumber *interval = [[NSUserDefaultsController sharedUserDefaultsController] valueForKeyPath:@"values.update_frequency"];
	NSTimeInterval result; 
	switch ([interval intValue]) {
		case hightFrequency:
			result = 0.5;
			break;
		case normalFrequency:
			result = 1;
			break;
		case lowFrequency:
			result = 2;
			break;
		default:
			result = 1;
			break;
	}
	return result;
}

-(DockIconType_t)docIconType
{
	NSNumber *iconId = [[NSUserDefaultsController sharedUserDefaultsController] 
									valueForKeyPath:@"values.dock_icon"];
	return [iconId intValue];
}

-(void)docIconTypeSet:(DockIconType_t)item
{
	NSNumber *iconId = [NSNumber numberWithInt:item];

	[[NSUserDefaultsController sharedUserDefaultsController] setValue:iconId 
															   forKeyPath:@"values.dock_icon"];
}

-(BOOL)highlightProcesses
{
	NSNumber *highlightProcesses = [[NSUserDefaultsController sharedUserDefaultsController] 
									valueForKeyPath:@"values.highlight_processes"];
	return [highlightProcesses intValue] == YES;
}

-(BOOL)showKernelCPUTime
{
	NSNumber *kernelCPUTime = [[NSUserDefaultsController sharedUserDefaultsController] 
							  valueForKeyPath:@"values.show_kernel_cpu_time"];
	return [kernelCPUTime intValue] == YES;
}

@end
