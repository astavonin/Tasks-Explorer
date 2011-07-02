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
#import "ColumnsBinder.h"
#import "TasksInfoManager.h"
#import "ImageAndTextCell.h"
#import "TaskInfoController.h"

#import "TreeViewController.h"

@implementation TreeViewController

@synthesize arrayController;

-(void)awakeFromNib
{
	[outlineView expandItem:nil expandChildren:YES];

	[outlineView setTarget:self];
	[outlineView setDoubleAction:@selector(onDoubleAction:)];

	ColumnsBinder *binder = [[ColumnsBinder alloc] init];
	NSArray *columns = [outlineView tableColumns];
	[binder bindColumnsToSettings:columns];
	[binder release];
}

-(void)showProcessInfo
{
	NSArray *processes = [arrayController selectedObjects];
	if ([processes count] > 0) {
		ProcessInfo *pInfo = [processes objectAtIndex:0];
		if (pInfo.processState != -1 ) {
			TaskInfoController *taskInfoController = [TaskInfoController taskInfoController:pInfo];
			[taskInfoController showInfo];
		}
	}
}

-(void)killProcess
{
	NSArray *processes = [arrayController selectedObjects];
	if ([processes count] > 0) {
		ProcessInfo *pInfo = [processes objectAtIndex:0];
		[pInfo killSelf];
	}	
}

-(void)onDoubleAction:(NSEvent*)theEvent;
{
	[self showProcessInfo];
}

-(NSArray*)sortDescriptors
{
	return [NSArray arrayWithObject:[NSSortDescriptor sortDescriptorWithKey:@"name" ascending:YES]];
}

-(IBAction)menuItemAction:(id)sender
{
	NSInteger clickedRow = [sender tag];
}

-(TasksInfoManager*)tasksInfoManager
{
	return [TasksInfoManager instance];
}

-(NSString*)selectedAppName
{
	NSString *result;
	NSArray *processes = [arrayController selectedObjects];

	if ([processes count] > 0) {
		ProcessInfo *pInfo = [processes objectAtIndex:0];
		result = pInfo.name;
	}

	return result;
}

-(void)outlineView:(NSOutlineView *)outlineView willDisplayCell:(NSCell *)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item
{
    ProcessInfo *pInfo = [item representedObject];
    if ((tableColumn == nil) || [[tableColumn identifier] isEqualToString:@"name"]) {
        ImageAndTextCell *imageAndTextCell = (ImageAndTextCell *)cell;
		[imageAndTextCell setImage:pInfo.icon_small];
    }
	if ([[Settings instance] highlightProcesses] == YES) {
		if (pInfo.processState == 0) { // process
			[(NSTextFieldCell*)cell setTextColor:[NSColor blackColor]];
		}
		else if (pInfo.processState == 1) { // new process
			[(NSTextFieldCell*)cell setTextColor:[NSColor greenColor]];
		}
		else if (pInfo.processState == -1) { // ended process
			[(NSTextFieldCell*)cell setTextColor:[NSColor redColor]];
		}
	}
}

- (NSString *)outlineView:(NSOutlineView *)outlineView toolTipForCell:(NSCell *)cell rect:(NSRectPointer)rect tableColumn:(NSTableColumn *)tc item:(id)item mouseLocation:(NSPoint)mouseLocation
{
    ProcessInfo *pInfo = [item representedObject];
	NSNumber *id_table;
	NSString *id_column;
	id       *cell_data;
    
    NSString *descr = pInfo.description;
    
	return (descr);
}

@end
