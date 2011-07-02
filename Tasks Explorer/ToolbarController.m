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

#import "TasksInfoManager.h"
#import "MainWindowController.h"
#import "GraphView.h"


static NSString *kToolbarIdentifier=@"Toolbar";
static NSString *kShowInfoIdentifier=@"Show info item";
static NSString *kKillProcessIdentifier=@"Kill process item";
static NSString *kViewPopUpIdentifier=@"View pop-up item";
static NSString *kSearchFieldIdentifier=@"Search field item";
static NSString *kCPUGraphIdentifier=@"CPU graph item";


@implementation MainWindowController(ToolbarController)

- (void)setupToolbarForWindow:(NSWindow *)theWindow
{
	NSToolbar *toolbar = [[[NSToolbar alloc] initWithIdentifier: kToolbarIdentifier] autorelease];

	[toolbar setAutosavesConfiguration: YES];
    [toolbar setAllowsUserCustomization: YES];
	[toolbar setDisplayMode: NSToolbarDisplayModeIconAndLabel];

	[toolbar setDelegate: self];

	[theWindow setToolbar:toolbar];
}

- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar itemForItemIdentifier:(NSString *)itemIdent 
				willBeInsertedIntoToolbar:(BOOL)willBeInserted 
{
	NSToolbarItem *toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier: itemIdent] autorelease];
	
	if ([itemIdent isEqual:kShowInfoIdentifier]) {
		[toolbarItem setLabel:@"Process info"];
		[toolbarItem setPaletteLabel:@"Process info"];
		
		[toolbarItem setToolTip:@"Show process info"];
		[toolbarItem setImage:[NSImage imageNamed: @"info"]];

		[toolbarItem setTarget:self];
		[toolbarItem setAction:@selector(onShowInfo:)];
		
	} else if ([itemIdent isEqual:kKillProcessIdentifier]) {
		[toolbarItem setLabel: @"Stop process"];
		[toolbarItem setPaletteLabel: @"Stop process"];
		
		[toolbarItem setToolTip: @"Stop process"];
		[toolbarItem setImage: [NSImage imageNamed: @"remove"]];
		
		[toolbarItem setTarget:self];
		[toolbarItem setAction:@selector(onKillProcess:)];

	} else if ([itemIdent isEqual:kViewPopUpIdentifier]) {		
		[toolbarItem setLabel: @"View"];
		[toolbarItem setPaletteLabel: @"View"];
		[toolbarItem setToolTip: @"Select view type"];
		
		[toolbarItem setView: viewPopUpButton];
		
	}
	else if( [itemIdent isEqual: kCPUGraphIdentifier]) {
		[toolbarItem setLabel: @"CPU Usage"];
		[toolbarItem setPaletteLabel: @"CPU Usage"];
		[toolbarItem setToolTip: @"CPU Usage graph"];

		cpuView = [[GraphView alloc] initWithFrame:NSMakeRect(0, 0, 120, 30)];

		[toolbarItem setView: cpuView];
		[self initCPUGraph];
		
	} else if([itemIdent isEqual: kSearchFieldIdentifier]) {
		[toolbarItem setLabel: @"Search"];
		[toolbarItem setPaletteLabel: @"Search"];
		[toolbarItem setToolTip: @"Search process by name"];
		
		[toolbarItem setView: searchField];

	} else {
		toolbarItem = nil;
	}
	return toolbarItem;
}

-(NSArray *)toolbarDefaultItemIdentifiers:(NSToolbar*)toolbar
{
	return [NSArray arrayWithObjects:
		   kShowInfoIdentifier, kKillProcessIdentifier,
		   kCPUGraphIdentifier,
		   NSToolbarFlexibleSpaceItemIdentifier, 
		   kSearchFieldIdentifier, kViewPopUpIdentifier,
		   nil];
}

-(NSArray *)toolbarAllowedItemIdentifiers:(NSToolbar*)toolbar 
{
	return [NSArray arrayWithObjects:
		   kShowInfoIdentifier, kKillProcessIdentifier,
		   kCPUGraphIdentifier,
		   kSearchFieldIdentifier, kViewPopUpIdentifier,
		   NSToolbarFlexibleSpaceItemIdentifier, NSToolbarSpaceItemIdentifier, 
		   NSToolbarSeparatorItemIdentifier, NSToolbarCustomizeToolbarItemIdentifier,
		   nil];
}

-(void)initCPUGraph
{
	cpuView.maxRange = 40;
	[cpuView addRecordType:[NSNumber numberWithInt:1] withColor:[NSColor redColor]];
	[cpuView addRecordType:[NSNumber numberWithInt:2] withColor:[NSColor greenColor]];
	[cpuView setDataSource:dataSource];
	[cpuView updateGraph];
}

@end
