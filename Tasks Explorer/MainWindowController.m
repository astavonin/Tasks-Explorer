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

#import "TreeViewController.h"
#import "FlatViewController.h"
#import "TasksInfoManager.h"
#import "Settings.h"
#import "MainWindowController.h"

struct ViewTypeRecord {
	NSString *name;
	NSString *nibName;
};

const static struct ViewTypeRecord viewList[] = {
	{@"Tree", @"TreeView"},
	{@"Flat", @"FlatView"}
};

@implementation MainWindowController

@synthesize currentViewController;

-(void)changeViewController:(NSInteger)whichViewTag
{
	[self willChangeValueForKey:@"viewController"];

	if ([currentViewController view] != nil)
		[[currentViewController view] removeFromSuperview];

	if (currentViewController != nil)
		[currentViewController release];

	switch (whichViewTag)
	{
		case 0:
		{
			TreeViewController* treeViewController =
			[[TreeViewController alloc] initWithNibName:viewList[0].nibName bundle:nil];
			if (treeViewController != nil)
			{
				
				currentViewController = treeViewController;
				[currentViewController setTitle:viewList[0].nibName];
			}
			break;
		}

		case 1:
		{
			FlatViewController* flatViewController =
			[[FlatViewController alloc] initWithNibName:viewList[1].nibName bundle:nil];
			if (flatViewController != nil)
			{
				currentViewController = flatViewController;
				[currentViewController setTitle:viewList[1].nibName];
			}
			break;
		}
	}
	
	[searchField setEnabled:whichViewTag];

	[targetView addSubview: [currentViewController view]];
	[[currentViewController view] setFrame: [targetView bounds]];
	[currentViewController setRepresentedObject: 
					[NSNumber numberWithUnsignedInt: [[[currentViewController view] subviews] count]]];

	[self didChangeValueForKey:@"viewController"];
}

- (IBAction)openPreferences:(id)sender
{
	[[Settings instance] showSettingsDlg];
}

-(IBAction)viewChoicePopupAction:(id)sender
{
	[self changeViewController: [[sender selectedCell] tag]];
}

-(IBAction)onShowInfo:(id)sender
{
	[(id)currentViewController showProcessInfo];
}

-(void)setActiveMenuDocItem:(int)itemId
{
	NSMenuItem *rootItem = [dockMenu itemWithTag: 0]; // Dock view root-item
	NSMenu *dockSubMenu = [rootItem submenu];
	NSArray *menuItems = [dockSubMenu itemArray];

	NSUInteger i, count = [menuItems count];
	for (i = 0; i < count; i++) {
		NSMenuItem * item = [menuItems objectAtIndex:i];
		if (itemId == [item tag]) {
			[item setState:NSOnState];
		} else {
			[item setState:NSOffState];
		}
	}

	NSRect frame = NSMakeRect(0, 0, dockTile.size.width, dockTile.size.height);
	NSView *dockView = nil;

	switch (itemId) {
		case cpuHistoryType:
			dockGraph = [[GraphView alloc] initWithFrame: frame];

			dockGraph.maxRange = 10;
			[dockGraph addRecordType:[NSNumber numberWithInt:1] withColor:[NSColor redColor]];
			[dockGraph addRecordType:[NSNumber numberWithInt:2] withColor:[NSColor greenColor]];
			[dockGraph setDataSource:dataSource];
			dockView = dockGraph;
			break;
		case cpuUsageType:
		case iconType:
		default:
			dockView = [[NSImageView alloc] initWithFrame: frame];
			[(NSImageView*)dockView setImage: [NSImage imageNamed:@"Matrix.icns"]];
			break;
	}
	[dockTile setContentView: dockView];
}

-(IBAction)onSetDockIcon:(id)sender
{
	int tag = [sender tag];
	
	[self setActiveMenuDocItem:tag];
	[[Settings instance] docIconTypeSet:tag];
}

-(IBAction)onKillProcess:(id)sender
{
	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	[alert setIcon:[NSImage imageNamed:@"process_remove.icns"]];
	[alert addButtonWithTitle:@"OK"];
	[alert addButtonWithTitle:@"Cancel"];
	[alert setMessageText:@"Kill the process."];
	[alert setInformativeText:[NSString stringWithFormat:@"Do you really want to kill \"%@\"?", 
							   [(id)currentViewController selectedAppName]]];
	[alert setAlertStyle:NSWarningAlertStyle];
	[alert beginSheetModalForWindow:[targetView window] modalDelegate:self 
					 didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:) contextInfo:nil];
}

-(void)alertDidEnd:(NSAlert *)alert returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo {
    if (returnCode == NSAlertFirstButtonReturn) {
		[(id)currentViewController killProcess];
    }
}

-(void)daemonError
{
	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	[alert setIcon:[NSImage imageNamed:@"warning.icns"]];
	[alert addButtonWithTitle:@"Exit"];
	[alert setMessageText:@"Internal error."];
	[alert setInformativeText:[NSString stringWithFormat:@"Connection with tasksexplorerd daemon cann't be estableshed."]];
	[alert setAlertStyle:NSWarningAlertStyle];
	[alert beginSheetModalForWindow:[targetView window] modalDelegate:self 
					 didEndSelector:@selector(alertDaemonError:returnCode:contextInfo:) contextInfo:nil];	
}

-(void)alertDaemonError:(NSAlert *)alert returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo {
	exit(0);
}

-(void)updateGraphics
{
	[cpuView updateGraph];
	[dockGraph updateGraph];
	[dockTile display];
}

-(id)init 
{
	if( (self = [super init]) ) {
		viewNames = [NSMutableArray array];
		int i, cnt = sizeof(viewList)/sizeof(struct ViewTypeRecord);
		for(i = 0; i < cnt; i++) {
			[viewNames addObject: viewList[i].name];
		}
	}

	return self;
}

-(void)chkForDaemon
{
	BOOL alive = [[TasksInfoManager instance] alive];
	if (alive == NO) {
		[self daemonError];
	}
}

-(void)awakeFromNib
{
	dataSource = [[DataSource alloc] init];
	dockTile = [NSApp dockTile];

	NSNumber *currentView = [[NSUserDefaultsController sharedUserDefaultsController] valueForKeyPath:@"values.currentView"];
	[self changeViewController:[currentView integerValue]];
	[NSTimer scheduledTimerWithTimeInterval:1 target:self selector:@selector(chkForDaemon)
												 userInfo:nil repeats:NO];	
	[searchField retain];
	[self setupToolbarForWindow:[targetView window]];

	[self setActiveMenuDocItem:[[Settings instance] docIconType]];

	[[TasksInfoManager instance] updateEventForObject:self withSelector:@selector(updateGraphics)];	
}

-(void)windowControllerDidLoadNib:(NSWindowController *)windowController {
	[searchField retain];
	[self setupToolbarForWindow:[self window]];
}

-(IBAction)updateFilterAction:(id)sender
{
	NSString *searchString = [searchField stringValue];
	[(id)currentViewController setFilterString:searchString];
}

@end
