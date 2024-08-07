#ifndef _DISK_H
#define _DISK_H

#include <efi.h>
#include <efilib.h>
#include <efigpt.h>

// DISK_INFO
struct disk_info{
    EFI_BLOCK_IO_MEDIA Media;
    BOOLEAN gpt_found; // GPTヘッダーが存在するか
    EFI_PARTITION_TABLE_HEADER gpt_header;
    UINT32 no_of_partition;
    EFI_PARTITION_ENTRY *partition_entries;
};

// BOOTABLE DISK INFO
struct bootable_disk_info {
    EFI_FILE_PROTOCOL *root;
    UINTN no_of_partition;
};

#endif