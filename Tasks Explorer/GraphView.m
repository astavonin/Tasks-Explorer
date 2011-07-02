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

#import "GraphView.h"

@interface NSAffineTransform (RectMapping)

- (NSAffineTransform*)mapFrom:(NSRect) srcBounds to: (NSRect) dstBounds;
- (NSAffineTransform*)scaleBounds:(NSRect) bounds toHeight: (float) height 
						 andWidth:(float) width;
- (NSAffineTransform*)flipVertical:(NSRect) bounds;

@end

@implementation NSAffineTransform (RectMapping)

- (NSAffineTransform*)mapFrom:(NSRect) src to: (NSRect) dst 
{
	NSAffineTransformStruct at;
	at.m11 = (dst.size.width/src.size.width);
	at.m12 = 0.0;
	at.tX = dst.origin.x - at.m11*src.origin.x;
	at.m21 = 0.0;
	at.m22 = (dst.size.height/src.size.height);
	at.tY = dst.origin.y - at.m22*src.origin.y;
	[self setTransformStruct: at];

	return self;
}

- (NSAffineTransform*)scaleBounds:(NSRect) bounds toHeight: (float) height 
						 andWidth:(float) width
{
	NSRect dst = bounds;
	dst.size.width *= (width / dst.size.width);
	dst.size.height *= (height / dst.size.height);

	return [self mapFrom:bounds to:dst];
}

- (NSAffineTransform*)flipVertical:(NSRect) bounds 
{
	NSAffineTransformStruct at;
	at.m11 = 1.0;
	at.m12 = 0.0;
	at.tX = 0;
	at.m21 = 0.0;
	at.m22 = -1.0;
	at.tY = bounds.size.height;
	[self setTransformStruct: at];

	return self;
}

@end



@implementation GraphView

@synthesize itemsInColumn;
@synthesize maxRange;

- (id)initWithFrame:(NSRect)frame 
{
    self = [super initWithFrame:frame];
    if (self) {
		self.itemsInColumn = 10;
		self.maxRange = 60;
		recordTypes = [[NSMutableArray alloc]init];
    }
    return self;
}

-(NSBezierPath*)graphItem
{
	NSRect rect = {{0,0},10,10};
	NSBezierPath* thePath = [NSBezierPath bezierPath];

	[thePath appendBezierPathWithRoundedRect:rect xRadius:1 yRadius:1];
	[thePath setLineWidth: 0.3];

	[thePath setLineJoinStyle:NSRoundLineJoinStyle];
	[thePath setLineCapStyle:NSRoundLineCapStyle];

	return thePath;
}

-(void)drawElement:(NSBezierPath*)item withColor:(NSColor*)color
{
	[color set];
	[item fill];
}

-(void)updateGraph
{
	[self setNeedsDisplay:YES];
}

-(void)drawRect:(NSRect)rect 
{
	float itemWidthWithBorders = rect.size.width / maxRange;
	float itemWidth = itemWidthWithBorders - itemWidthWithBorders/5.0; //(20% from element width)
	float itemHeightWithBorders = rect.size.height * itemsInColumn/100.0;
	float itemHeight = itemHeightWithBorders - itemHeightWithBorders/5.0; //(20% from element height)
	float labelHeight = rect.size.height / 10.0;
	float labelWidth = rect.size.width;

	NSBezierPath* theBorder = [NSBezierPath bezierPath];
	
	[theBorder appendBezierPathWithRect:rect];
	[theBorder setLineWidth: 0.3];

	[theBorder setLineJoinStyle:NSRoundLineJoinStyle];
	[theBorder setLineCapStyle:NSRoundLineCapStyle];
	[theBorder stroke];

	NSBezierPath *item = [self graphItem];

	[item transformUsingAffineTransform: [[NSAffineTransform transform]
										  scaleBounds: [item bounds] 
										  toHeight: itemHeight
										  andWidth: itemWidth]];

	NSAffineTransform* transform = [NSAffineTransform transform];
	NSAffineTransform* identity = [NSAffineTransform transform];

	int typesCout = [recordTypes count] / 2;
	NSColor *color = 0;
	NSObject *type = 0;

	float offsetX = 0.0;
	int record, typeNr, i;
	for (record = 0; record < maxRange; ++record) {
		float offsetY = 0.0;
		for (typeNr=0; typeNr<typesCout; ++typeNr) {
			type = [recordTypes objectAtIndex:typeNr*2];
			color = [recordTypes objectAtIndex:typeNr*2+1];
			int elemCount = ([dataSource numberForRecord:record withType:type]+5)*itemsInColumn/100;
            
            if (itemsInColumn < elemCount)
                elemCount = itemsInColumn;

			for (i = 0; i < elemCount; ++i) {
				[transform initWithTransform: identity];

				[transform translateXBy:offsetX yBy:offsetY];
				[transform concat];		

				[self drawElement:item withColor:color];

				offsetY += itemHeightWithBorders;

				[transform invert];
				[transform concat];
			}
		}
		offsetX += itemWidthWithBorders;
	}
}

-(void)addRecordType:(id)newType withColor:(NSColor*)color
{
	[recordTypes addObject:newType];
	[recordTypes addObject:color];
}

-(void)setDataSource:(id<GraphDataSource>)source
{
	dataSource = source;
}

-(void)dealloc
{
	[recordTypes release];
	[super dealloc];
}

@end
