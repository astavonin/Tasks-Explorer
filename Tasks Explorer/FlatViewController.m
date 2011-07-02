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

#import "Settings.h"
#import "ColumnsBinder.h"
#import "ProcessInfo.h"
#import "ImageAndTextCell.h"
#import "TasksInfoManager.h"
#import "TaskInfoController.h"

#import "FlatViewController.h"

@implementation FlatViewController 


-(void)awakeFromNib
{
	[tableView setTarget:self];
	[tableView setDoubleAction:@selector(onDoubleAction:)];

	ColumnsBinder *binder = [[ColumnsBinder alloc] init];
	NSArray *columns = [tableView tableColumns];
	[binder bindColumnsToSettings:columns];
	[binder release];
	
	predicateTemplate = [[NSPredicate predicateWithFormat:@"name contains[cd] $searchString"] retain];
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

-(void)onDoubleAction:(NSEvent*)theEvent
{
	[self showProcessInfo];
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

-(NSArray*)sortDescriptors
{
	return [NSArray arrayWithObject:[NSSortDescriptor sortDescriptorWithKey:@"name" ascending:YES]];
}

-(void)tableView:(NSTableView *)aTableView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)rowIndex
{
    ProcessInfo *pInfo = [[arrayController arrangedObjects] objectAtIndex:rowIndex];
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

- (NSString *)tableView:(NSTableView *)tableView toolTipForCell:(NSCell *)cell 
				   rect:(NSRectPointer)rect tableColumn:(NSTableColumn *)tableColumn 
					row:(NSInteger)row mouseLocation:(NSPoint)mouseLocation;
{
    ProcessInfo *pInfo = [[arrayController arrangedObjects] objectAtIndex:row];
	NSNumber *id_table;
	NSString *id_column;
	id       *cell_data;
    
    NSString *descr = pInfo.description;

	return (descr);
}

-(void)setFilterString:(NSString *)newFilter
{
	NSPredicate *predicate;

	if ([newFilter isEqualToString:@""]) {
		predicate = nil;
	} else {
		NSMutableDictionary *bindVariables = [[NSMutableDictionary alloc] init];
		[bindVariables setObject:newFilter forKey:@"searchString"];

		predicate = [predicateTemplate predicateWithSubstitutionVariables:bindVariables];
	}

	[arrayController setFilterPredicate:predicate];
	
}

@end
