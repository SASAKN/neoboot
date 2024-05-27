// GNU_EFI
#include <efi.h>
#include <efilib.h>
#include <efigpt.h>

// Disk Info
typedef struct _NEO_BOOT_DISK_INFO {
    // Media Info
    EFI_BLOCK_IO_MEDIA Media;

    // GPT Header
    EFI_PARTITION_TABLE_HEADER *GPT_Header;

    // GPT Partition Entries
    EFI_PARTITION_ENTRY *Partition_Entry;
} NEO_BOOT_DISK_INFO;