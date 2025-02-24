// Prototype Functions

#ifndef _PROTO_H
#define _PROTO_H

// EFI LIBRARY FILES
#include <efi.h>
#include <efilib.h>

// NEOBOOT FILES
#include "memory.h"
#include "disk.h"
#include "config.h"

// Functions

// String
CHAR16 *atou(CHAR8 *str);
unsigned int my_strlen(const char *str);
char *my_strcpy(char *dest, const char *src);
UINTN EFIAPI AsciiSPrint(CHAR8 *buffer, UINTN buffer_size, CONST CHAR8 *str, ...);
CHAR16 *add_spaces_around_text(const CHAR16 *text, UINTN num_spaces);
char *my_strtok(char *str, const char *delim);
char *my_strchr(const char *str, int c);
char *my_strdup(const char *s);
void split_key_value(char *str, char **key, char **value);
char **split(char *txt, const char *delimiter, int *count);
Config *config_file_parser(char *config_txt);

// Memorymap
const CHAR16 *get_memtype(EFI_MEMORY_TYPE type);
EFI_STATUS save_memmap(memmap *map, EFI_FILE_PROTOCOL *f, EFI_FILE_PROTOCOL *esp_root);

// Protocol
EFI_STATUS open_protocol(EFI_HANDLE handle, EFI_GUID *guid, VOID **protocol, EFI_HANDLE ImageHandle, UINT32 attr);

// Disk
void list_disks(EFI_HANDLE ImageHandle, struct disk_info **disk_info, UINTN *no_of_disks);
void list_bootable_disk(struct bootable_disk_info **disk_info, UINTN *no_of_disks);

// Menu
entries_list *init_entries_list();
void add_a_entry(CHAR16 *os_name, entries_list **entries);
void print_a_entry(CHAR16 *name, UINTN no_of_entries, UINTN *pos_x, UINTN *pos_y, UINTN c, BOOLEAN is_selected);
void print_entries(entries_list *entries, UINTN *pos_x, UINTN *pos_y, UINTN c);
void modify_an_entry_order(entries_list *list_entries, UINT32 new_entry_order);
void redraw_menu(CHAR16 *title, UINTN c, UINTN r, entries_list *list_entries);
void open_menu(Config *con);

// Console
void determine_command(CHAR16 *buffer);
void open_console();

// Config file
VOID *read_config_file(EFI_FILE_PROTOCOL *root);

// Main
EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);



#endif