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

#define MAX_ARCH_COUNT 2


#define LIST_OF_TYPE(type,name) \
struct name { \
	type *data; \
	struct name *next_ptr; \
}; \
typedef struct name name##_t;

#define ILIST_COMPARATOR(e1, e2) (e1->i - e2->i)

struct macho_section_64 {
	char sectname[16];
	char segname[16];
	uint64_t addr;
	uint64_t size;
	uint32_t offset;
	uint32_t align;
	uint32_t reloff;
	uint32_t nreloc;
	uint32_t flags;
};
typedef struct macho_section_64 macho_section_64_t;

LIST_OF_TYPE(macho_section_64_t, section_64_list);

struct macho_segment_64 {
	char segname[16];
	uint64_t vmaddr;
	uint64_t vmsize;
	uint64_t fileoff;
	uint64_t filesize;
	uint32_t nsects; 
	uint32_t flags;
	section_64_list_t *sections;
};
typedef struct macho_segment_64 macho_segment_64_t;

LIST_OF_TYPE(macho_segment_64_t, segment_64_list);

struct macho_section {
	char sectname[16];
	char segname[16];
	uint32_t addr;
	uint32_t size;
	uint32_t offset;
	uint32_t align;
	uint32_t reloff;
	uint32_t nreloc;
	uint32_t flags;
};
typedef struct macho_section macho_section_t;

LIST_OF_TYPE(macho_section_t, section_list);

struct macho_segment {
	char segname[16];
	uint32_t vmaddr;
	uint32_t vmsize;
	uint32_t fileoff;
	uint32_t filesize;
	uint32_t flags;
	uint32_t nsects; 
	section_list_t *sections;
};
typedef struct macho_segment macho_segment_t;

LIST_OF_TYPE(macho_segment_t, segment_list);

struct macho_dylib {
	char *name; 
	uint32_t timestamp; 
	uint32_t current_version; 
	uint32_t compatibility_version;
};
typedef struct macho_dylib macho_dylib_t;

LIST_OF_TYPE(macho_dylib_t, dylib_list);

struct macho_symbol {
	char *name;
	uint8_t type;
	uint8_t sect;
	uint16_t desc;
};
typedef struct macho_symbol macho_symbol_t;

LIST_OF_TYPE(macho_symbol_t, symbol_list);

struct macho_arch {
	uint32_t magic;
	cpu_type_t cputype;
	cpu_subtype_t cpusubtype;
	uint32_t offset;
	uint32_t filetype;
	uint32_t ncmds;
	uint32_t flags;
	char *dylinker;
	segment_list_t *segments;
	segment_64_list_t *segments64;
	dylib_list_t *dylibs;
	symbol_list_t *symbols;
};
typedef struct macho_arch macho_arch_t;

LIST_OF_TYPE(macho_arch_t, arch_list);

struct macho_info {
	uint32_t magic;
	int arch_count;
	arch_list_t *arch;
};
typedef struct macho_info macho_info_t;

struct macho_file {
	size_t size;
	char *begin;
	char *read_ptr;
};
typedef struct macho_file macho_file_t;

macho_info_t* open_macho_file(const char *path);
void close_macho_file(macho_info_t* macho_file);
