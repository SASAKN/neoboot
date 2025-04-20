#ifndef ELF64_HEADER_H
#define ELF64_HEADER_H

#define MAX_UINT64 18446744073709551615ULL
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))


typedef unsigned long long Elf64_Addr;
typedef unsigned long long Elf64_Off;
typedef unsigned short     Elf64_Half;
typedef unsigned int       Elf64_Word;
typedef int                Elf64_Sword;
typedef unsigned long long Elf64_Xword;
typedef long long          Elf64_Sxword;

#define EI_NIDENT 16

typedef struct {
  unsigned char e_ident[EI_NIDENT];
  Elf64_Half    e_type;
  Elf64_Half    e_machine;
  Elf64_Word    e_version;
  Elf64_Addr    e_entry;
  Elf64_Off     e_phoff;
  Elf64_Off     e_shoff;
  Elf64_Word    e_flags;
  Elf64_Half    e_ehsize;
  Elf64_Half    e_phentsize;
  Elf64_Half    e_phnum;
  Elf64_Half    e_shentsize;
  Elf64_Half    e_shnum;
  Elf64_Half    e_shstrndx;
} Elf64_Ehdr;

typedef struct {
  Elf64_Word  p_type;
  Elf64_Word  p_flags;
  Elf64_Off   p_offset;
  Elf64_Addr  p_vaddr;
  Elf64_Addr  p_paddr;
  Elf64_Xword p_filesz;
  Elf64_Xword p_memsz;
  Elf64_Xword p_align;
} Elf64_Phdr;

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_TLS     7

typedef struct {
  Elf64_Sxword d_tag;
  union {
    Elf64_Xword d_val;
    Elf64_Addr  d_ptr;
  } d_un;
} Elf64_Dyn;

#define DT_NULL     0
#define DT_RELA     7
#define DT_RELASZ   8
#define DT_RELAENT  9

typedef struct {
  Elf64_Addr   r_offset;
  Elf64_Xword  r_info;
  Elf64_Sxword r_addend;
} Elf64_Rela;

#define ELF64_R_SYM(i)    ((i) >> 32)
#define ELF64_R_TYPE(i)   ((i) & 0xffffffffUL)
#define ELF64_R_INFO(s,t) (((s) << 32) + ((t) & 0xffffffffUL))

#define R_X86_64_RELATIVE 8

#endif // ELF64_HEADER_H
