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

#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include <mach-o/nlist.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "sglib.h"

#include "macho_reader.h"

static int read_fat_binary(macho_file_t *file, macho_info_t *info);
static int read_binary(macho_file_t *file, macho_arch_t *arch);
static int read_binary_x64(macho_file_t *file, macho_arch_t *arch);
static int read_header(macho_file_t *file, uint32_t magic, cpu_type_t cpu_type, macho_info_t *info);
static int read_arch(macho_file_t *file, cpu_type_t cpu_type, macho_arch_t *arch);
static int read_load_commands(macho_file_t *file, macho_arch_t *arch, macho_info_t *info);
static int read_segment_command(macho_file_t *file, macho_arch_t *arch);
static int read_segment_64_command(macho_file_t *file, macho_arch_t *arch);
static int read_dylib_command(macho_file_t *file, macho_arch_t *arch);
static int read_dylinker_command(macho_file_t *file, macho_arch_t *arch);
static int read_symtab_command(macho_file_t *file, macho_arch_t *arch);
static void free_macho_info(macho_info_t *info);

static uint16 endian_change_16(uint16 x)
{
	uint16 result;
    result = (x>>8) | 
	(x<<8);
	return result;
}

static uint32 endian_change_32(uint32 x)
{
	uint32 result;
    result = (x>>24) | 
	((x<<8) & 0x00FF0000) |
	((x>>8) & 0x0000FF00) |
	(x<<24);
	return result;
}

static uint64 endian_change_64(uint64 x)
{
	uint64 result;
    result = (x>>56) | 
	((x<<40) & 0x00FF000000000000) |
	((x<<24) & 0x0000FF0000000000) |
	((x<<8)  & 0x000000FF00000000) |
	((x>>8)  & 0x00000000FF000000) |
	((x>>24) & 0x0000000000FF0000) |
	((x>>40) & 0x000000000000FF00) |
	(x<<56);
	return result;
}

static uint16 endian_change_16_if(uint16 x, cpu_type_t cpu_type)
{
	if (cpu_type == CPU_TYPE_POWERPC || cpu_type == CPU_TYPE_POWERPC64) {
		return endian_change_16(x);
	}
	return x;
}

static uint32 endian_change_32_if(uint32 x, cpu_type_t cpu_type)
{
	if (cpu_type == CPU_TYPE_POWERPC || cpu_type == CPU_TYPE_POWERPC64) {
		return endian_change_32(x);
	}
	return x;
}

static uint64 endian_change_64_if(uint64 x, cpu_type_t cpu_type)
{
	if (cpu_type == CPU_TYPE_POWERPC || cpu_type == CPU_TYPE_POWERPC64) {
		return endian_change_64(x);
	}
	return x;
}

struct macho_header_info {
	uint32_t magic;
	cpu_type_t cputype;
};
typedef struct macho_header_info macho_header_info_t;

macho_info_t* open_macho_file(const char *path)
{
	macho_info_t *info;
    int fd;
	int result;
	struct stat stat_buf;
	macho_file_t file={};

	if ((fd = open(path, O_RDONLY)) == -1){
		return NULL;
	}
	if (fstat(fd, &stat_buf) == -1){
	    close(fd);
		return NULL;
	}
	file.size = stat_buf.st_size;
	if (file.size != 0){
	    file.begin = mmap(0, file.size, PROT_READ|PROT_WRITE, MAP_FILE|MAP_PRIVATE, fd,
					0);
		file.read_ptr = file.begin;
	    if((intptr_t)file.begin == -1){
			close(fd);
			return NULL;
	    }
	}
	close(fd);

	macho_header_info_t *header = (macho_header_info_t*)file.begin;

	info = calloc(sizeof(macho_info_t), 1);

	result = read_header(&file, header->magic, header->cputype, info);
	if (result == -1) {
		free_macho_info(info);
		return NULL;
	}

	SGLIB_LIST_MAP_ON_ELEMENTS(arch_list_t, info->arch, larchs, next_ptr, {
		result = read_load_commands(&file, larchs->data, info);
		if (result == -1) {
			free_macho_info(info);
			return NULL;
		}
	});
	munmap(file.begin, file.size);

	return info;
}

static int read_header(macho_file_t *file, uint32_t magic, cpu_type_t cpu_type, macho_info_t *info)
{
	int result;

	if (magic == FAT_MAGIC || magic == FAT_CIGAM) {
		result = read_fat_binary(file, info);
	}
	else if (magic == MH_MAGIC || magic == MH_CIGAM ||
			 magic == MH_MAGIC_64 || magic == MH_CIGAM_64) {
		arch_list_t *l;
		macho_arch_t *arch = calloc(sizeof(macho_arch_t), 1);

		result = read_arch(file, cpu_type, arch);
		
		l = calloc(sizeof(arch_list_t), 1);
		l->data = arch;
		SGLIB_LIST_ADD(arch_list_t, info->arch, l, next_ptr);					
	}
	else {
		result = -1;
	}

	return result;
}

static void free_macho_info(macho_info_t *info)
{
	SGLIB_LIST_MAP_ON_ELEMENTS(arch_list_t, info->arch, larchs, next_ptr, {
		SGLIB_LIST_MAP_ON_ELEMENTS(segment_list_t, larchs->data->segments, lseg, next_ptr, {
			SGLIB_LIST_MAP_ON_ELEMENTS(section_list_t, lseg->data->sections, ll, next_ptr, {
				free(ll->data);
				free(ll);
			});
		});
		SGLIB_LIST_MAP_ON_ELEMENTS(segment_64_list_t, larchs->data->segments64, lseg, next_ptr, {
			SGLIB_LIST_MAP_ON_ELEMENTS(section_64_list_t, lseg->data->sections, ll, next_ptr, {
				free(ll->data);
				free(ll);
			});
		});
		SGLIB_LIST_MAP_ON_ELEMENTS(dylib_list_t, larchs->data->dylibs, ldlib, next_ptr, {
			free(ldlib->data->name);
		});
		SGLIB_LIST_MAP_ON_ELEMENTS(symbol_list_t, larchs->data->symbols, lsym, next_ptr, {
			free(lsym->data->name);
		});
		free(larchs->data->dylinker);
	});
	free(info->arch);
	free(info);
}

static int read_arch(macho_file_t *file, cpu_type_t cpu_type, macho_arch_t *arch)
{
	int result = 0;

	switch (cpu_type) {
		case CPU_TYPE_I386:
			arch->cputype = CPU_TYPE_I386;
			read_binary(file, arch);
			break;
		case CPU_TYPE_POWERPC:
			arch->cputype = CPU_TYPE_POWERPC;
			read_binary(file, arch);
			break;
		case CPU_TYPE_X86_64:
			arch->cputype = CPU_TYPE_X86_64;
			read_binary_x64(file, arch);
			break;
		case CPU_TYPE_POWERPC64:
			arch->cputype = CPU_TYPE_POWERPC64;
			read_binary_x64(file, arch);
			break;
		default:
			result = -1;
			break;
	}
	
	return result;
}

void close_macho_file(macho_info_t* macho_file)
{
	free_macho_info(macho_file);
}

#define MAP_STRUCT(struct_type, struct_ptr, file) \
{ \
	struct_ptr = (struct_type*)file->read_ptr; \
	file->read_ptr = file->read_ptr + sizeof(struct_type); \
}

static int read_fat_binary(macho_file_t *file, macho_info_t *info)
{
	struct fat_header *header;
	struct fat_arch *arch;
	int i;

	MAP_STRUCT(struct fat_header, header, file);
	info->magic = header->magic;
	info->arch_count = endian_change_32(header->nfat_arch);

	for (i = 0; i < info->arch_count; i++) {
		arch_list_t *l;
		macho_arch_t *march = calloc(sizeof(macho_arch_t), 1);

		MAP_STRUCT(struct fat_arch, arch, file);
		march->cputype = endian_change_32(arch->cputype);
		march->offset = endian_change_32(arch->offset);		

		l = calloc(sizeof(arch_list_t), 1);
		l->data = march;
		SGLIB_LIST_ADD(arch_list_t, info->arch, l, next_ptr);			
	}

	SGLIB_LIST_MAP_ON_ELEMENTS(arch_list_t, info->arch, larchs, next_ptr, {
		file->read_ptr = file->begin + larchs->data->offset;
		read_arch(file, larchs->data->cputype, larchs->data);
	});

	return 0;
}

static int read_binary(macho_file_t *file, macho_arch_t *arch)
{
	struct mach_header *header;

	MAP_STRUCT(struct mach_header, header, file);

	arch->magic = header->magic;
	arch->cpusubtype = endian_change_32_if(header->cpusubtype, arch->cputype);
	arch->filetype = endian_change_32_if(header->filetype, arch->cputype);
	arch->flags = endian_change_32_if(header->flags, arch->cputype);
	arch->ncmds = endian_change_32_if(header->ncmds, arch->cputype);

	return 0;
}

static int read_binary_x64(macho_file_t *file, macho_arch_t *arch)
{
	struct mach_header_64 *header;
	
	MAP_STRUCT(struct mach_header_64, header, file);

	arch->magic = header->magic;
	arch->cpusubtype = endian_change_32_if(header->cpusubtype, arch->cputype);
	arch->filetype = endian_change_32_if(header->filetype, arch->cputype);
	arch->flags = endian_change_32_if(header->flags, arch->cputype);
	arch->ncmds = endian_change_32_if(header->ncmds, arch->cputype);

	return 0;
}

static int read_load_commands(macho_file_t *file, macho_arch_t *arch, macho_info_t *info)
{
	int i;
	uint32_t cmd_header[2];

	if (arch->cputype == CPU_TYPE_X86_64 || arch->cputype == CPU_TYPE_POWERPC64) {
		file->read_ptr = file->begin + arch->offset + sizeof(struct mach_header_64);
	}
	else if (arch->cputype == CPU_TYPE_I386 || arch->cputype == CPU_TYPE_POWERPC){
		file->read_ptr = file->begin + arch->offset + sizeof(struct mach_header);
	}
	else {
		return -1;
	}

	for (i = 0; i < arch->ncmds; i++) {
		cmd_header[0] = endian_change_32_if(((uint32_t*)file->read_ptr)[0], arch->cputype);
		cmd_header[1] = endian_change_32_if(((uint32_t*)file->read_ptr)[1], arch->cputype);
		switch (cmd_header[0]) {
			case LC_SEGMENT:
				read_segment_command(file, arch);
				break;
			case LC_SEGMENT_64:
				read_segment_64_command(file, arch);
				break;
			case LC_LOAD_DYLIB:
			case LC_ID_DYLIB:
				read_dylib_command(file, arch);
				break;
			case LC_LOAD_DYLINKER:
			case LC_ID_DYLINKER:
				read_dylinker_command(file, arch);
				break;
			case LC_SYMTAB:
				read_symtab_command(file, arch);
				break;
			default:
				file->read_ptr = file->read_ptr + cmd_header[1];
				break;
		}
	}
	SGLIB_LIST_REVERSE(segment_list_t, arch->segments, next_ptr);
	SGLIB_LIST_REVERSE(segment_64_list_t, arch->segments64, next_ptr);

	return 0;
}

static int read_segment_command(macho_file_t *file, macho_arch_t *arch)
{
	struct segment_command *command;
	macho_segment_t *segment;
	
	segment = calloc(sizeof(macho_segment_t), 1);
	MAP_STRUCT(struct segment_command, command, file);
	segment->fileoff = endian_change_32_if(command->fileoff, arch->cputype);
	segment->filesize = endian_change_32_if(command->filesize, arch->cputype);
	segment->flags = endian_change_32_if(command->flags, arch->cputype);
	segment->vmaddr = endian_change_32_if(command->vmaddr, arch->cputype);
	segment->vmsize = endian_change_32_if(command->vmsize, arch->cputype);
	segment->nsects = endian_change_32_if(command->nsects, arch->cputype);
	memcpy(segment->segname, command->segname, sizeof(segment->segname));

	if (segment->nsects > 0) {
		int i;
		for (i = 0; i < segment->nsects; i++) {
			struct section *section;
			macho_section_t *msection;
			
			msection = calloc(sizeof(macho_section_t), 1);

			MAP_STRUCT(struct section, section, file);

			memcpy(msection->sectname, section->sectname, sizeof(msection->sectname));
			memcpy(msection->segname, section->segname, sizeof(msection->segname));
			msection->addr = endian_change_32_if(section->addr, arch->cputype);
			msection->size = endian_change_32_if(section->size, arch->cputype);
			msection->offset = endian_change_32_if(section->offset, arch->cputype);
			msection->align = endian_change_32_if(section->align, arch->cputype);
			msection->reloff = endian_change_32_if(section->reloff, arch->cputype);
			msection->nreloc = endian_change_32_if(section->nreloc, arch->cputype);
			msection->flags = endian_change_32_if(section->flags, arch->cputype);

			section_list_t *l;
			l = calloc(sizeof(section_list_t), 1);
			l->data = msection;
			SGLIB_LIST_ADD(section_list_t, segment->sections, l, next_ptr);			
		}
	}
	SGLIB_LIST_REVERSE(section_list_t, segment->sections, next_ptr);

	segment_list_t *lseg;
	lseg = calloc(sizeof(segment_list_t), 1);
	lseg->data = segment;
	SGLIB_LIST_ADD(segment_list_t, arch->segments, lseg, next_ptr);			

	return 0;
}

static int read_segment_64_command(macho_file_t *file, macho_arch_t *arch)
{
	macho_segment_64_t *segment;
	
	segment = calloc(sizeof(macho_segment_64_t), 1);
	struct segment_command_64 *command;

	MAP_STRUCT(struct segment_command_64, command, file);
	segment->fileoff = endian_change_64_if(command->fileoff, arch->cputype);
	segment->filesize = endian_change_64_if(command->filesize, arch->cputype);
	segment->flags = endian_change_32_if(command->flags, arch->cputype);
	segment->vmaddr = endian_change_64_if(command->vmaddr, arch->cputype);
	segment->vmsize = endian_change_64_if(command->vmsize, arch->cputype);
	segment->nsects = endian_change_32_if(command->nsects, arch->cputype);
	memcpy(segment->segname, command->segname, sizeof(segment->segname));

	if (segment->nsects > 0) {
		int i;
		for (i = 0; i < segment->nsects; i++) {
			struct section_64 *section;
			macho_section_64_t *msection;

			msection = calloc(sizeof(macho_section_64_t), 1);

			MAP_STRUCT(struct section_64, section, file);
			memcpy(msection->sectname, section->sectname, sizeof(msection->sectname));
			memcpy(msection->segname, section->segname, sizeof(msection->segname));
			msection->addr = endian_change_32_if(section->addr, arch->cputype);
			msection->size = endian_change_32_if(section->size, arch->cputype);
			msection->offset = endian_change_32_if(section->offset, arch->cputype);
			msection->align = endian_change_32_if(section->align, arch->cputype);
			msection->reloff = endian_change_32_if(section->reloff, arch->cputype);
			msection->nreloc = endian_change_32_if(section->nreloc, arch->cputype);
			msection->flags = endian_change_32_if(section->flags, arch->cputype);
			
			section_64_list_t *l;
			l = calloc(sizeof(section_64_list_t), 1);
			l->data = msection;
			SGLIB_LIST_ADD(section_64_list_t, segment->sections, l, next_ptr);
		}
	}
	segment_64_list_t *lseg;
	lseg = calloc(sizeof(segment_64_list_t), 1);
	lseg->data = segment;
	SGLIB_LIST_ADD(segment_64_list_t, arch->segments64, lseg, next_ptr);	

	return 0;
}

static int read_dylib_command(macho_file_t *file, macho_arch_t *arch)
{
	macho_dylib_t *mdlib;
	struct dylib_command *dlib;
	long name_size;
	char *dlib_name;
	char *pos;
	
	pos = file->read_ptr;
	mdlib = calloc(sizeof(macho_dylib_t), 1);
	MAP_STRUCT(struct dylib_command, dlib, file);
	mdlib->timestamp = endian_change_32_if(dlib->dylib.timestamp, arch->cputype);
	mdlib->compatibility_version = endian_change_32_if(dlib->dylib.compatibility_version, arch->cputype);
	mdlib->current_version = endian_change_32_if(dlib->dylib.current_version, arch->cputype);
	name_size = endian_change_32_if(dlib->cmdsize, arch->cputype) - sizeof(struct dylib_command);

	dlib_name = pos + endian_change_32_if(dlib->dylib.name.offset, arch->cputype);
	mdlib->name = strdup(dlib_name);
	file->read_ptr = pos + endian_change_32_if(dlib->cmdsize, arch->cputype);

	dylib_list_t *ldlib;
	ldlib = calloc(sizeof(dylib_list_t), 1);
	ldlib->data = mdlib;
	SGLIB_LIST_ADD(dylib_list_t, arch->dylibs, ldlib, next_ptr);

	return 0;
}

static int read_dylinker_command(macho_file_t *file, macho_arch_t *arch)
{
	struct dylinker_command *dylinker;
	long name_size;
	char *pos;
	char *dylinker_name;

	pos = file->read_ptr;
	MAP_STRUCT(struct dylinker_command, dylinker, file);
	name_size = endian_change_32_if(dylinker->cmdsize, arch->cputype) - sizeof(struct dylinker_command);
	dylinker_name = pos + endian_change_32_if(dylinker->name.offset, arch->cputype);
	arch->dylinker = strdup(dylinker_name);
	file->read_ptr = pos + endian_change_32_if(dylinker->cmdsize, arch->cputype);

	return 0;
}

static int read_symtab_command(macho_file_t *file, macho_arch_t *arch)
{
	struct symtab_command *symtab;
	long symoff, nsyms, stroff, stlen;
	int i;
	char *pos;

	pos = file->read_ptr;
	MAP_STRUCT(struct symtab_command, symtab, file);
	symoff = endian_change_32_if(symtab->symoff, arch->cputype);
	nsyms = endian_change_32_if(symtab->nsyms, arch->cputype);
	stroff = endian_change_32_if(symtab->stroff, arch->cputype);
	stlen = endian_change_32_if(symtab->strsize, arch->cputype);
	symoff = symoff + arch->offset;
	stroff = stroff + arch->offset;

	size_t nlist_len;
	if (arch->cputype == CPU_TYPE_X86_64 || arch->cputype == CPU_TYPE_POWERPC64) {
		nlist_len = sizeof(struct nlist_64);
	}
	else if (arch->cputype == CPU_TYPE_I386 || arch->cputype == CPU_TYPE_POWERPC){
		nlist_len = sizeof(struct nlist);
	}
	else {
		return -1;
	}

	struct nlist_64 *sym;
	file->read_ptr = file->begin + symoff;
	uint32 offset;
	uint16 desc;
	macho_symbol_t *symb;
	for (i = 0; i < nsyms; i++) {
		sym = (struct nlist_64*)file->read_ptr;
		file->read_ptr = file->read_ptr + nlist_len;
		symb = NULL;
		desc = endian_change_16_if(sym->n_desc, arch->cputype);
		offset = endian_change_32_if(sym->n_un.n_strx, arch->cputype) + stroff;
		if ((sym->n_un.n_strx > 0) &&
			(offset < stroff + stlen)) {
			if (strlen(file->begin + offset) > 0) {
				symb = calloc(sizeof(macho_symbol_t), 1);
				symb->name = strdup(file->begin + offset);
			}
		}
		if (symb != NULL) {
			symb->type = sym->n_type;
			symb->desc = desc;
			symb->sect = sym->n_sect;

			symbol_list_t *lsymb;
			lsymb = calloc(sizeof(symbol_list_t), 1);
			lsymb->data = symb;
			SGLIB_LIST_ADD(symbol_list_t, arch->symbols, lsymb, next_ptr);
		}
	}

	file->read_ptr = pos + sizeof(struct symtab_command);

	return 0;
}
