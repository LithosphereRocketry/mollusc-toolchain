#ifndef ELF_COMMON_H
#define ELF_COMMON_H

#include <stdint.h>

#define EI_NIDENT (16)

// structure definitions from https://gist.github.com/x0nu11byt3/bcb35c3de461e5fb66173071a2379779

typedef enum elf_type {
    ET_NONE = 0,  /* No file type */
    ET_REL = 1,  /* Relocatable file */
    ET_EXEC = 2,  /* Executable file */
    ET_DYN = 3,  /* Shared object file */
    ET_CORE = 4,  /* Core file */
    ET_NUM  /* Number of defined types */
} elf_type_t;

#define EM_MOLLUSC 0xE746 // large random number that hopefully doesn't collide
                          // with any other unofficial ELF types

typedef enum elf_version {
    EV_NONE = 0,  /* Invalid ELF version */
    EV_CURRENT = 1,  /* Current version */
    EV_NUM
} elf_version_t;
                          
typedef struct elf_header {
    unsigned char ident[EI_NIDENT]; /* Magic number and other info */
    uint16_t type;   /* Object file type */
    uint16_t machine;  /* Architecture */
    uint32_t version;  /* Object file version */
    uint32_t entry;  /* Entry point virtual address */
    uint32_t phoff;  /* Program header table file offset */
    uint32_t shoff;  /* Section header table file offset */
    uint32_t flags;  /* Processor-specific flags */
    uint16_t ehsize;  /* ELF header size in bytes */
    uint16_t phentsize;  /* Program header table entry size */
    uint16_t phnum;  /* Program header table entry count */
    uint16_t shentsize;  /* Section header table entry size */
    uint16_t shnum;  /* Section header table entry count */
    uint16_t shstrndx;  /* Section header string table index */
} elf_header_t;

typedef enum elf_shdr_type {
    SHT_NULL = 0, /* Section header table entry unused */
    SHT_PROGBITS = 1, /* Program data */
    SHT_SYMTAB = 2, /* Symbol table */
    SHT_STRTAB = 3, /* String table */
    SHT_RELA = 4, /* Relocation entries with addends */
    SHT_HASH = 5, /* Symbol hash table */
    SHT_DYNAMIC = 6, /* Dynamic linking information */
    SHT_NOTE = 7, /* Notes */
    SHT_NOBITS = 8, /* Program space with no data (bss) */
    SHT_REL = 9, /* Relocation entries, no addends */
    SHT_SHLIB = 10, /* Reserved */
    SHT_DYNSYM = 11, /* Dynamic linker symbol table */
    SHT_INIT_ARRAY = 14, /* Array of constructors */
    SHT_FINI_ARRAY = 15, /* Array of destructors */
    SHT_PREINIT_ARRAY = 16, /* Array of pre-constructors */
    SHT_GROUP = 17, /* Section group */
    SHT_SYMTAB_SHNDX = 18, /* Extended section indeces */
    SHT_NUM
} elf_shdr_type_t;

#define SHF_WRITE	            (1 << 0)	/* Writable */
#define SHF_ALLOC	            (1 << 1)	/* Occupies memory during execution */
#define SHF_EXECINSTR	        (1 << 2)	/* Executable */
#define SHF_MERGE	            (1 << 4)	/* Might be merged */
#define SHF_STRINGS             (1 << 5)	/* Contains nul-terminated strings */
#define SHF_INFO_LINK	        (1 << 6)	/* `sh_info' contains SHT index */
#define SHF_LINK_ORDER	        (1 << 7)	/* Preserve order after combining */
#define SHF_OS_NONCONFORMING    (1 << 8)	/* Non-standard OS specific handling required */
#define SHF_GROUP               (1 << 9)	/* Section is member of a group.  */
#define SHF_TLS                 (1 << 10)	/* Section hold thread-local data.  */
#define SHF_COMPRESSED	        (1 << 11)	/* Section with compressed data. */

typedef struct elf_section_header {
    uint32_t name;  /* Section name (string tbl index) */
    uint32_t type;  /* Section type */
    uint32_t flags;  /* Section flags */
    uint32_t addr;  /* Section virtual addr at execution */
    uint32_t offset;  /* Section file offset */
    uint32_t size;  /* Section size in bytes */
    uint32_t link;  /* Link to another section */
    uint32_t info;  /* Additional section information */
    uint32_t addralign;  /* Section alignment */
    uint32_t entsize;  /* Entry size if section holds table */
} elf_section_header_t;

typedef struct elf_symbol {
    uint32_t	st_name;		/* Symbol name (string tbl index) */
    uint32_t	st_value;		/* Symbol value */
    uint32_t	st_size;		/* Symbol size */
    unsigned char	st_info;		/* Symbol type and binding */
    unsigned char	st_other;		/* Symbol visibility */
    uint32_t	st_shndx;		/* Section index */
} elf_symbol_t;

#endif