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

#include "sglib.h"
#import <mach-o/loader.h>
#import <mach-o/nlist.h>
#import "MachOReader.h"

////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////  Data representation  //////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

@implementation MachODyLib

@synthesize pathNameString;
@synthesize name;
@synthesize pathName;
@synthesize timestamp;
@synthesize current_version;
@synthesize compatibility_version;

-(id)initWithDylib:(macho_dylib_t*)data
{
	if ((self = [super init])) {
		pathNameString = [[NSString alloc] initWithCString:data->name];
		NSArray *pathComp = [pathNameString componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"/"]];
		name = [pathComp objectAtIndex:([pathComp count] - 1)];
		[name retain];
		pathName = [[NSURL alloc] initFileURLWithPath:[NSString stringWithCString:data->name encoding:NSUTF8StringEncoding] isDirectory:NO];
		timestamp = [[NSNumber alloc] initWithUnsignedInt:data->timestamp];
		current_version = [[NSNumber alloc] initWithUnsignedInt:data->current_version];
		compatibility_version = [[NSNumber alloc] initWithUnsignedInt:data->compatibility_version];
	}
	return self;
}

-(void)dealloc
{
	[name release];
	[pathName release];
	[timestamp release];
	[current_version release];
	[compatibility_version release];
	[pathNameString release];
	[super dealloc];
}

@end


@implementation MachOSymbol

@synthesize name;
@synthesize type;
@synthesize sect;
@synthesize desc;

-(id)initWithSymbol:(macho_symbol_t*)data;
{
	if ((self = [super init])) {
		name = [[NSString alloc] initWithCString:data->name];
		type = [[NSNumber alloc] initWithUnsignedChar:data->type];
		sect = data->sect;
		desc = data->desc;
	}
	return self;
}

-(void)dealloc
{
	[name release];
	[type release];
	[super dealloc];
}

@end

@implementation MachOSection

@synthesize sectname;
@synthesize segname;
@synthesize addr;
@synthesize size;
@synthesize offset;
@synthesize align;
@synthesize reloff;
@synthesize nreloc;
@synthesize flags;	

@synthesize sections;
@synthesize asString;

-(id)initWithSection:(macho_section_t*)data
{
	if ((self = [super init])) {
		sectname = [[NSString alloc] initWithCString:data->sectname];
		segname = [[NSString alloc] initWithCString:data->segname];
		addr = data->addr;
		size = data->size;
		offset = data->offset;
		align = data->align;
		reloff = data->reloff;
		flags = data->flags;
		sections = [[NSArray alloc] init];
		[self buildInfoString];
	}
	return self;
}

-(id)initWithSection64:(macho_section_64_t*)data
{
	if ((self = [super init])) {
		sectname = [[NSString alloc] initWithCString:data->sectname];
		segname = [[NSString alloc] initWithCString:data->segname];
		addr = data->addr;
		size = data->size;
		offset = data->offset;
		align = data->align;
		reloff = data->reloff;
		flags = data->flags;
		sections = [[NSArray alloc] init];
		[self buildInfoString];
	}
	return self;
}

-(void)buildInfoString
{
	StringFormatter *formatter = [[StringFormatter alloc] init];

	[formatter addString:sectname withName:@"Section name"];
	[formatter addUInt:addr withName:@"Memory address"];
	[formatter addUInt:size withName:@"Section size"];
	[formatter addUInt:offset withName:@"File offset"];
	[formatter addUInt:align withName:@"Section alignment"];
	[formatter addUInt:reloff withName:@"Relocation entries offset"];
	[formatter addUInt:flags withName:@"Flags"];

	NSAttributedString *info = [formatter resultString];

	asString = [[NSAttributedString alloc] initWithAttributedString:info];

	[formatter release];
}

-(void)dealloc
{
	[sectname release];
	[segname release];
	[sections release];
	[super dealloc];
}

@end

@implementation MachOSegment

@synthesize segname;
@synthesize vmaddr;
@synthesize vmsize;
@synthesize fileoff;
@synthesize filesize;
@synthesize flags;
@synthesize nsects; 
@synthesize sections;
@synthesize asString;

-(id)initWithSegment:(macho_segment_t*)data
{
	if ((self = [super init])) {
		segname = [[NSString alloc] initWithCString:data->segname];
		vmaddr = data->vmaddr;
		vmsize = data->vmsize;
		fileoff = data->fileoff;
		filesize = data->filesize;
		flags = data->flags;
		sections = [[NSMutableArray alloc] init];
		
		SGLIB_LIST_MAP_ON_ELEMENTS(section_list_t, data->sections, ll, next_ptr, {
			MachOSection *section = [[MachOSection alloc] initWithSection:ll->data];
			[sections addObject:section];
		});
		nsects = [sections count];
		[self buildInfoString];
	}
	return self;
}

-(id)initWithSegment64:(macho_segment_64_t*)data
{
	if ((self = [super init])) {
		segname = [[NSString alloc] initWithCString:data->segname];
		vmaddr = data->vmaddr;
		vmsize = data->vmsize;
		fileoff = data->fileoff;
		filesize = data->filesize;
		flags = data->flags;
		sections = [[NSMutableArray alloc] init];

		SGLIB_LIST_MAP_ON_ELEMENTS(section_64_list_t, data->sections, ll, next_ptr, {
			MachOSection *section = [[MachOSection alloc] initWithSection64:ll->data];
			[sections addObject:section];
		});
		nsects = [sections count];
		[self buildInfoString];
	}
	return self;
}

-(void)buildInfoString
{
	StringFormatter *formatter = [[StringFormatter alloc] init];

	[formatter addString:segname withName:@"Segment name"];
	[formatter addUInt:vmaddr withName:@"Memory address"];
	[formatter addUInt:vmsize withName:@"Memory size"];
	[formatter addUInt:fileoff withName:@"File offset"];
	[formatter addUInt:filesize withName:@"File size"];
	[formatter addUInt:nsects withName:@"Number of sections"];
	[formatter addUInt:flags withName:@"Flags"];

	NSAttributedString *info = [formatter resultString];

	asString = [[NSAttributedString alloc] initWithAttributedString:info];

	[formatter release];
}

-(void)dealloc
{
	[segname release];
	[sections release];
	[super dealloc];
}

@end

@implementation MachOArch

@synthesize magic;
@synthesize cputype;
@synthesize cpusubtype;
@synthesize offset;
@synthesize filetype;
@synthesize ncmds;
@synthesize flags;
@synthesize dylinker;
@synthesize segments;
@synthesize dylibs;
@synthesize symbols;
@synthesize nseg;

-(id)initWithArch:(macho_arch_t*)data
{
	if ((self = [super init])) {
		magic = [[NSNumber alloc] initWithUnsignedInt:data->magic];
		cputype = [[NSNumber alloc] initWithInt:data->cputype];
		cpusubtype = [[NSNumber alloc] initWithInt:data->cpusubtype];
		offset = [[NSNumber alloc] initWithUnsignedInt:data->offset];
		filetype = [[NSNumber alloc] initWithUnsignedInt:data->filetype];
		ncmds = [[NSNumber alloc] initWithUnsignedInt:data->ncmds];
		flags = [[NSNumber alloc] initWithUnsignedInt:data->flags];
		if (data->dylinker) {
			dylinker = [[NSURL alloc] initFileURLWithPath:
						[NSString stringWithCString:data->dylinker encoding:NSUTF8StringEncoding] 
											  isDirectory:NO];
		}
		segments = [[NSMutableArray alloc] init];
		dylibs = [[NSMutableArray alloc] init];
		symbols = [[NSMutableArray alloc] init];

		SGLIB_LIST_MAP_ON_ELEMENTS(segment_list_t, data->segments, lseg, next_ptr, {
			MachOSegment *segment = [[MachOSegment alloc] initWithSegment:lseg->data];
			[segments addObject:segment];
		});
		SGLIB_LIST_MAP_ON_ELEMENTS(segment_64_list_t, data->segments64, lseg, next_ptr, {
			MachOSegment *segment = [[MachOSegment alloc] initWithSegment64:lseg->data];
			[segments addObject:segment];
		});		
		SGLIB_LIST_MAP_ON_ELEMENTS(dylib_list_t, data->dylibs, ldlib, next_ptr, {
			MachODyLib *dylib = [[MachODyLib alloc] initWithDylib:ldlib->data];
			[dylibs addObject:dylib];
		});
		SGLIB_LIST_MAP_ON_ELEMENTS(symbol_list_t, data->symbols, lsym, next_ptr, {
			MachOSymbol *symbol = [[MachOSymbol alloc] initWithSymbol:lsym->data];
			[symbols addObject:symbol];
		});
		nseg = [[NSNumber alloc] initWithUnsignedInt:[segments count]];
	}
	return self;
}

-(void)dealloc
{
	[magic release];
	[cputype release];
	[cpusubtype release];
	[offset release];
	[filetype release];
	[ncmds release];
	[flags release];
	[dylinker release];
	[segments release];
	[dylibs release];
	[symbols release];
	[nseg release];
	[super dealloc];
}

@end


////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Reader   /////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

@implementation MachOReader

@synthesize archs;
@synthesize binaryPath;
@synthesize binaryName;

-(id)init
{
	if ((self = [super init])) {
		archs = [[NSMutableArray alloc] init];
	}
	return self;
}

-(BOOL)openMachOFile:(NSString*)path
{
	machoInfo = open_macho_file([path UTF8String]);
	if (machoInfo == NULL) {
		return NO;
	}
	NSArray *pathComp;

	[self willChangeValueForKey:@"archs"];
	binaryPath = [[NSURL alloc] initFileURLWithPath:path isDirectory:NO];
	pathComp = [path componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"/"]];
	binaryName = [pathComp objectAtIndex:([pathComp count] - 1)];

	[archs removeAllObjects];
	SGLIB_LIST_MAP_ON_ELEMENTS(arch_list_t, machoInfo->arch, larch, next_ptr, {
		MachOArch *arch = [[MachOArch alloc] initWithArch:larch->data];
		[archs addObject:arch];
	});
	[self didChangeValueForKey:@"archs"];

	return YES;
}

-(void)closeMachOFile
{
	if (machoInfo != NULL) {
		close_macho_file(machoInfo);
		machoInfo = NULL;
	}
}

-(void)dealloc
{
	[archs release];
	[super dealloc];
}

@end

////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////  Transformers  /////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

static NSString *kArchI386 = @"Intel-based";
static NSString *kArchPPC = @"PowerPC-based";
static NSString *kArchX86_64 = @"64-bit Intel-based";
static NSString *kArchPPC_64 = @"64-bit PowerPC–based";
static NSString *kArchUnknown = @"Unknown";


static NSString* arch_to_string(cpu_type_t cpu_type)
{
	NSString *result;
	switch (cpu_type) {
		case CPU_TYPE_I386:
			result = kArchI386;
			break;
		case CPU_TYPE_POWERPC:
			result = kArchPPC;
			break;
		case CPU_TYPE_X86_64:
			result = kArchX86_64;
			break;
		case CPU_TYPE_POWERPC64:
			result = kArchPPC_64;
			break;
		default:
			result = kArchUnknown;
			break;
	}
	return result;
}


@implementation ArchListToNamesTransformer

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
	MachOArch *arch;
	cpu_type_t cpu_type;
	NSMutableArray *result = [[[NSMutableArray alloc] init] autorelease];
	int i, count = [value count];

	for (i = 0; i < count; i++) {
		arch = [value objectAtIndex:i];
		cpu_type = [arch.cputype unsignedIntValue];
		[result addObject:arch_to_string(cpu_type)];
	}

	return result;	
}

@end

@implementation ArchToNameTransformer

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
	cpu_type_t cpu_type;
	NSString *result;

	cpu_type = [value unsignedIntValue];
	result = arch_to_string(cpu_type);
	
	return result;	
}

@end

@implementation DecToHexTransformer

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
	NSString *result;
	
	result = [[[NSString alloc] initWithFormat:@"0x%X (%u)", [value unsignedIntValue], [value unsignedIntValue]] autorelease];
	
	return result;	
}

@end

@implementation FileTypeTransformer

static NSString *kFTObject = @"MH_OBJECT";
static NSString *kFTExecute = @"MH_EXECUTE";
static NSString *kFTBundle = @"MH_BUNDLE";
static NSString *kFTDylib = @"MH_DYLIB";
static NSString *kFTPreload = @"MH_PRELOAD";
static NSString *kFTCore = @"MH_CORE";
static NSString *kFTDylinker = @"MH_DYLINKER";
static NSString *kFTDsym = @"MH_DSYM";
static NSString *kFTInvalid = @"(Invalid)";

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
	NSString *result;
	
	switch ([value unsignedIntValue]) {
		case MH_OBJECT:
			result = kFTObject;
			break;
		case MH_EXECUTE:
			result = kFTExecute;
			break;
		case MH_BUNDLE:
			result = kFTBundle;
			break;
		case MH_DYLIB:
			result = kFTDylib;
			break;
		case MH_PRELOAD:
			result = kFTPreload;
			break;
		case MH_CORE:
			result = kFTCore;
			break;
		case MH_DYLINKER:
			result = kFTDylinker;
			break;
		case MH_DSYM:
			result = kFTDsym;
			break;
		default:
			result = kFTInvalid;
			break;
	}
	
	return result;	
}

@end

@implementation ArchFlagsTransformer

struct arch_flag_strings {
	uint32 flag;
	NSString *name;
};
typedef struct arch_flag_strings arch_flag_strings_t;
static arch_flag_strings_t arch_flags[] = {
	{MH_NOUNDEFS, @"MH_NOUNDEFS"},
	{MH_INCRLINK, @"MH_INCRLINK"},
	{MH_DYLDLINK, @"MH_DYLDLINK"},
	{MH_TWOLEVEL, @"MH_TWOLEVEL"},
	{MH_BINDATLOAD, @"MH_BINDATLOAD"},
	{MH_PREBOUND, @"MH_PREBOUND"},
	{MH_PREBINDABLE, @"MH_PREBINDABLE"},
	{MH_NOFIXPREBINDING, @"MH_NOFIXPREBINDING"},
	{MH_ALLMODSBOUND, @"MH_ALLMODSBOUND"},
	{MH_CANONICAL, @"MH_CANONICAL"},
	{MH_SPLIT_SEGS, @"MH_SPLIT_SEGS"},
	{MH_FORCE_FLAT, @"MH_FORCE_FLAT"},
	{MH_SUBSECTIONS_VIA_SYMBOLS, @"MH_SUBSECTIONS_VIA_SYMBOLS"},
	{MH_NOMULTIDEFS, @"MH_NOMULTIDEFS"}
};

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
	static NSString *emptyFormatString = @"%@";
	static NSString *nonEmptyFormatString = @" | %@";
	NSString *formatter;
	NSMutableString *result;
	int i, count = sizeof(arch_flags) / sizeof(arch_flag_strings_t);
	uint32 flags = [value unsignedIntValue];

	result = [[[NSMutableString alloc] init] autorelease];

	for (i = 0; i < count; i++) {
		if ((flags & arch_flags[i].flag) != 0 ) {
			if ([result isEqualToString:@""] == YES) {
				formatter = emptyFormatString;
			}
			else {
				formatter = nonEmptyFormatString;
			}
			[result appendString:[NSString stringWithFormat:formatter, arch_flags[i].name]];
		}
	}
	
	return result;	
}

@end

@implementation SymbFlagsTransformer

struct symb_flag_strings {
	uint8_t flag;
	NSString *name;
};
typedef struct symb_flag_strings symb_flag_strings_t;
static symb_flag_strings_t symb_flags_type[] = {
	{N_STAB, @"S"},
	{N_PEXT, @"P"},
	{N_TYPE, @"T"},
	{N_EXT, @"E"}
};

static symb_flag_strings_t symb_flags_ext[] = {
	{N_UNDF, @"u"},
	{N_ABS, @"a"},
	{N_SECT, @"s"},
	{N_PBUD, @"p"},
	{N_INDR, @"i"}
};

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
	NSMutableString *result;
//	NSString *asNumber;
	int i, count = sizeof(symb_flags_type) / sizeof(symb_flag_strings_t),
		j, count_ext = sizeof(symb_flags_ext) / sizeof(symb_flag_strings_t);
	uint8 flags = [value unsignedCharValue];

	result = [[[NSMutableString alloc] init] autorelease];

	for (i = 0; i < count; i++) {
		if ((flags & symb_flags_type[i].flag) != 0 ) {
			[result appendString:symb_flags_type[i].name];
			if (flags & (symb_flags_type[i].flag == N_TYPE)) {
				for (j = 0; j < count_ext; j++) {
					if ((flags & symb_flags_ext[j].flag) != 0 ) {
						[result appendString:symb_flags_ext[j].name];
					}
				}
			}			
		}
	}

//	asNumber = [[NSString alloc] initWithFormat:@" (0x%X)", flags];
//	[result appendString:asNumber];
//	[asNumber release];
	
	return result;	
}

@end

@implementation StringFormatter

@synthesize resultString;

-(id)init
{
	if ((self = [super init])) {
		boldFont = [NSDictionary dictionaryWithObject:[NSFont boldSystemFontOfSize:10.0] forKey:NSFontAttributeName];
		normalFont = [NSDictionary dictionaryWithObject:[NSFont systemFontOfSize:10.0] forKey:NSFontAttributeName];
		resultString = [[NSMutableAttributedString alloc] init];
	}
	return self;
}

-(void)dealloc
{
	[resultString release];
	[super dealloc];
}

-(void)addUInt:(uint)data withName:(NSString*)name
{
	NSString *formatter = [NSString stringWithFormat:@"0x%X (%u)", data, data];
	[self addString:formatter withName:name];
}

-(void)addString:(NSString*)data withName:(NSString*)name
{
	[resultString appendAttributedString:[[NSAttributedString alloc] initWithString:name attributes:boldFont]];
	[resultString appendAttributedString:[[NSAttributedString alloc] initWithString:@": " attributes:normalFont]];
	[resultString appendAttributedString:[[NSAttributedString alloc] initWithString:data attributes:normalFont]];
	[resultString appendAttributedString:[[NSAttributedString alloc] initWithString:@"\n"]];
}

@end

