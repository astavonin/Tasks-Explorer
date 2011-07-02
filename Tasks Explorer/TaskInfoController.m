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

#import "ProcessInfo.h"
#import "Settings.h"
#import "MachOReader.h"
#import "MachOViewController.h"
#import "StackInfoController.h"
#import "ThreadInfo.h"

#import "TaskInfoController.h"

@interface InfoPage: NSObject{
@private
    NSView *view;
    NSString *name;
}
@property (retain) NSView *view;
@property (retain) NSString *name;

+(InfoPage*)infoPageForView:(NSView*)view withName:(NSString*)name;

@end

@implementation InfoPage

@synthesize view, name;

+(InfoPage*)infoPageForView:(NSView*)view withName:(NSString*)name
{
    InfoPage *info = [[InfoPage alloc] init];
    info.view = view;
    info.name = name;
    return info;
}

@end

@implementation TaskInfoController

@synthesize machoReader;
@synthesize processInfo;
@synthesize arch;
@synthesize infoPages;

#pragma mark -
#pragma mark Initialization/creation

-(NSArray*) getPages
{
    NSMutableArray *array = [[NSMutableArray alloc] init];
    
    [array addObject:[InfoPage infoPageForView:pageProcess withName:@"Process"]];
    [array addObject:[InfoPage infoPageForView:pageEnvironment withName:@"Environment"]];
    [array addObject:[InfoPage infoPageForView:pageFiles withName:@"Files"]];
    [array addObject:[InfoPage infoPageForView:pageDyLibs withName:@"DyLibs"]];
    [array addObject:[InfoPage infoPageForView:pageSymbols withName:@"Symbols"]];
    [array addObject:[InfoPage infoPageForView:pageThreads withName:@"Threads"]];
    
    return array;
}

+(TaskInfoController*)taskInfoController:(ProcessInfo*)pInfo
{
	TaskInfoController *controller = [[TaskInfoController alloc] init];
	controller.processInfo = pInfo;

	MachOReader *reader = [[MachOReader alloc] init];
	[reader openMachOFile:pInfo.pathName];
	controller.machoReader = reader;
	
	for (MachOArch *arch in reader.archs) {
		if ([arch.cputype isEqualToNumber:pInfo.cpuType]) {
			controller.arch = arch;
			break;
		}
	}
	return controller;
}

- (void)awakeFromNib
{
    [pathControl setTarget:self];
	[pathControl setDoubleAction:@selector(pathControlDoubleClick:)];
    [dyLibsView setTarget:self];
	[dyLibsView setDoubleAction:@selector(dyLibsDoubleClick:)];

    self.infoPages = [NSArray arrayWithArray:[self getPages]];
    
    [infoPagesController addObserver:self forKeyPath:@"selectionIndex" options:0 context:NULL];
    [infoPagesController setSelectionIndex:0];
    [splitView setDividerStyle:NSSplitViewDividerStyleThin];
    [threadsList setDoubleAction:@selector(onDoubleAction:)];
}

-(void)showInfo
{
	[processInfo askForExtendedInfo];
	[NSBundle loadNibNamed:@"TaskInfo" owner:self];
	[window makeKeyAndOrderFront:nil];
    
	NSTimeInterval curUpdateFrequency = [[Settings instance] updateInterval];
	updateTimer = [NSTimer scheduledTimerWithTimeInterval:curUpdateFrequency target:self 
												 selector:@selector(updateInfo) userInfo:nil repeats:YES];
	
	updateFrequency = curUpdateFrequency;
}

#pragma mark -
#pragma mark Data refreshing

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
	[self willChangeValueForKey:@"processInfo"];
	[self didChangeValueForKey:@"processInfo"];
	[self willChangeValueForKey:@"processInfo.threadList"];
    [processInfo updateThreadsInfo];
	[self didChangeValueForKey:@"processInfo.threadList"];

	[self chkTimerInterval];
}

#pragma mark -
#pragma mark Callbacks

-(void)showStackInfo:(id)sender
{
	NSArray *threadsArray = [threadsController selectedObjects];
	if ([threadsArray count] > 0) {
		ThreadInfo *curThread = [threadsArray objectAtIndex:0];

		StackInfoController *stackInfoController = [StackInfoController stackInfoController:
                                                    [processInfo generateStackTrace:[curThread.threadId longValue]]];
		[stackInfoController showInfo];
	}
}

-(void)showExtendedInfoDlg:(NSString*)path
{
	InfoViewController *controller;

	MachOReader *reader = [[MachOReader alloc] init];
	[reader openMachOFile:path];
	controller = [InfoViewController infoViewController:reader];	
	[NSBundle loadNibNamed:@"MachOView" owner:controller];
}

-(void)onShowExtendedInfo:(id)sender
{
	[self showExtendedInfoDlg:processInfo.pathName];
}

-(void)dyLibsDoubleClick:(id)sender
{
	NSArray *dyLibsArr = [dyLibsController selectedObjects];
	if ([dyLibsArr count] > 0) {
		MachODyLib *dyLib = [dyLibsArr objectAtIndex:0];
		NSString *pathName;
		if ([dyLib.pathNameString isAbsolutePath] == NO) {
			NSArray *arrEnd = [dyLib.pathNameString pathComponents],
					*arrBegin = [processInfo.pathName pathComponents];
			NSMutableArray *pathArr = [[NSMutableArray alloc] init];
			int i, count = [arrBegin count];
			for (i = 0; i < count-1; i++) {
				[pathArr addObject:[arrBegin objectAtIndex:i]];
			}
			count = [arrEnd count];
			for (i = 1; i < count; i++) {
				[pathArr addObject:[arrEnd objectAtIndex:i]];
			}
			pathName = [NSString pathWithComponents:pathArr];
		}
		else {
			pathName = dyLib.pathNameString;
		}

		[self showExtendedInfoDlg:pathName];
	}	
}

-(void)pathControlDoubleClick:(id)sender
{
    if ([pathControl clickedPathComponentCell] != nil)
        [[NSWorkspace sharedWorkspace] openURL:[[pathControl clickedPathComponentCell] URL]];
}

#pragma mark -
#pragma mark Informational pages manipulation

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object
                        change:(NSDictionary *)change context:(void *)context
{
	if ([keyPath isEqual:@"selectionIndex"])
	{
        NSArray *subviews = [dataPageView subviews];
        for(NSView *view in subviews)
        {
            [view retain];
            [view removeFromSuperview];
        }
        InfoPage *infoPage = [infoPages objectAtIndex: [infoPagesController selectionIndex]];
        [dataPageView addSubview:infoPage.view];
        [infoPage.view setFrame: [dataPageView bounds]];
		return;
	}
	
	[super observeValueForKeyPath:keyPath ofObject:object change:change
                          context:context];
}

#pragma mark NSSplitViewDelegate implementation

- (CGFloat)splitView:(NSSplitView *)sender constrainMinCoordinate:(CGFloat)proposedMax ofSubviewAt:(NSInteger)offset
{
    return 172;
}

#pragma mark -
#pragma mark Closing/deinitialization

-(void) windowWillClose:(NSNotification *)notification 
{
	[updateTimer invalidate];
	[self release];
}

-(void)dealloc
{
	[processInfo release];
	[machoReader release];
    [infoPages release];

	[super dealloc];
}

@end
