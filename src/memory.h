#include <efi.h>
#include <efilib.h>

typedef struct memory_map {
    EFI_MEMORY_DESCRIPTOR *buffer;
    UINTN buffer_size;
    UINTN map_size;
    UINTN map_key;
    UINTN desc_size;
    UINT32 desc_ver;
    UINTN entry;
} memmap;