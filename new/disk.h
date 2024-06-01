#include <efi.h>
#include <efilib.h>
#include <efigpt.h>

// DISK_INFO
struct disk_info{
    EFI_BLOCK_IO_MEDIA Media;
    EFI_PARTITION_TABLE_HEADER gpt_header;
    UINTN no_of_partition;
    EFI_PARTITION_ENTRY *partition_entries;
};