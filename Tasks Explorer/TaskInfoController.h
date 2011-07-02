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

#import <Cocoa/Cocoa.h>

@class ProcessInfo;
@class MachOReader;
@class MachOArch;

@interface TaskInfoController : NSObject <NSSplitViewDelegate>
{   
    // Windows management
	IBOutlet id window;
    IBOutlet NSSplitView *splitView;

    NSArray *infoPages;
    IBOutlet NSArrayController *infoPagesController;
    
    IBOutlet NSView *dataPageView;
    IBOutlet NSView *pageProcess;
    IBOutlet NSView *pageEnvironment;
    IBOutlet NSView *pageFiles;
    IBOutlet NSView *pageDyLibs;
    IBOutlet NSView *pageSymbols;
    IBOutlet NSView *pageThreads;
    IBOutlet NSTableView *threadsList;
	IBOutlet NSArrayController *threadsController;

    // Data management
	MachOReader *machoReader;
	MachOArch *arch;
	ProcessInfo *processInfo;

	IBOutlet NSPathControl *pathControl;
	IBOutlet NSArrayController *dyLibsController;
	IBOutlet NSTableView *dyLibsView;

    // Data updates
	NSTimeInterval updateFrequency;
	NSTimer *updateTimer;
}

@property (nonatomic, retain) NSArray *infoPages;

@property (assign) MachOArch *arch;
@property (retain) MachOReader *machoReader;
@property (retain) ProcessInfo *processInfo;

+(TaskInfoController*)taskInfoController:(ProcessInfo*)pInfo;

-(void)showInfo;
-(void)pathControlDoubleClick:(id)sender;
-(void)dyLibsDoubleClick:(id)sender;
-(IBAction)onShowExtendedInfo:(id)sender;
-(IBAction)showStackInfo:(id)sender;

-(void)showExtendedInfoDlg:(NSString*)path;


@end
