#include <efi.h>
#include <efilib.h>
#include <efigpt.h>

#include "memory.h"

// AsciiSPrint
UINTN EFIAPI AsciiSPrint(CHAR8 *buffer, UINTN buffer_size, CONST CHAR8 *str, ...) {
    va_list marker;
    UINTN num_printed;

    va_start(marker, str);
    num_printed = AsciiVSPrint(buffer, buffer_size, str, marker);
    va_end(marker);
    return num_printed;
}


// Get memory type
const CHAR16 *get_memtype(EFI_MEMORY_TYPE type) {

    switch (type) {
        case EfiReservedMemoryType: return L"EfiReservedMemoryType";
        case EfiLoaderCode: return L"EfiLoaderCode";
        case EfiLoaderData: return L"EfiLoaderData";
        case EfiBootServicesCode: return L"EfiBootServicesCode";
        case EfiBootServicesData: return L"EfiBootServicesData";
        case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
        case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
        case EfiConventionalMemory: return L"EfiConventionalMemory";
        case EfiUnusableMemory: return L"EfiUnusableMemory";
        case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
        case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
        case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
        case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
        case EfiPalCode: return L"EfiPalCode";
        case EfiPersistentMemory: return L"EfiPersistentMemory";
        case EfiMaxMemoryType: return L"EfiMaxMemoryType";
        default: return L"InvalidMemoryType";
    }

}

// Save memory map file
EFI_STATUS save_memmap(memmap *map, EFI_FILE_PROTOCOL *f, EFI_FILE_PROTOCOL *esp_root) {
    char buffer[4096];
    EFI_STATUS status;
    UINTN size;

    // Header
    CHAR8 *header = "Index, Buffer, Type, Type(name), PhysicalStart, VirtualStart, NumberOfPages, Size, Attribute\n"
                    "-----|------------------|----|----------------------|------------------|------------------|------------------|-----|----------------|\n";
    size = strlena(header);

    // Create a file
    status = uefi_call_wrapper(esp_root->Open, 5, esp_root, &f, L"\\memmap", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    ASSERT(!EFI_ERROR(status));

    // Write header
    status = uefi_call_wrapper(f->Write, 3, f, &size, header);
    ASSERT(!EFI_ERROR(status));

    // Write memory map
    for (UINTN i = 0; i < map->entry; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((char *)map->buffer + map->desc_size * i);
        size = AsciiSPrint(buffer, sizeof(buffer), "| %02u | %016x | %02x | %20ls | %016x | %016x | %016x | %3d | %2ls %5lx | \n", i, desc, desc->Type, get_memtype(desc->Type), desc->PhysicalStart, desc->VirtualStart, desc->NumberOfPages, desc->NumberOfPages, (desc->Attribute & EFI_MEMORY_RUNTIME) ? L"RT" : L"", desc->Attribute & 0xffffflu);

        status = uefi_call_wrapper(f->Write, 3, f, &size, buffer);
        ASSERT(!EFI_ERROR(status));
    }

    // Close file handle
    uefi_call_wrapper(f->Close, 1, f);

    return EFI_SUCCESS;
}

// Open protocol
EFI_STATUS open_protocol(EFI_HANDLE handle, EFI_GUID *guid, VOID **protocol, EFI_HANDLE ImageHandle, UINT32 attr) {
    EFI_STATUS status = uefi_call_wrapper(BS->OpenProtocol, 6, handle, guid, protocol, ImageHandle, NULL, attr);
    ASSERT(!EFI_ERROR(status));

    return EFI_SUCCESS;
}


EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Initalize
    InitializeLib(ImageHandle, SystemTable);
    ST = SystemTable;
    BS = SystemTable->BootServices;
    RT = SystemTable->RuntimeServices;
    Print(L"STEP0 \n");

    // Open LIP
    EFI_LOADED_IMAGE_PROTOCOL *lip = NULL;
    EFI_GUID lip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

    open_protocol(ImageHandle, &lip_guid, (void **)&lip, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    Print(L"STEP1 \n");

    // Open ESP Root
    EFI_FILE_HANDLE esp_root;
    esp_root = LibOpenRoot(lip->DeviceHandle);
    Print(L"STEP2 \n");

    // Get memory map
    memmap map;
    map.buffer = LibMemoryMap(&map.entry, &map.map_key, &map.desc_size, &map.desc_ver);
    ASSERT(map.buffer != NULL);
    Print(L"STEP3 \n");

    // Save memory map

    EFI_FILE_PROTOCOL *memmap_file = NULL;
    save_memmap(&map, memmap_file, esp_root);
    Print(L"STEP4 \n");
    
    // // Print Memory map
    // for (UINTN i = 0; i < map.entry; i++) {
    //     EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((char *)map.buffer + map.desc_size * i);
    //     Print(L"| %02u | %016x | %02x | %20ls | %016x | %016x | %016x | %3d | %2ls %5lx | \n", i, desc, desc->Type, get_memtype(desc->Type), desc->PhysicalStart, desc->VirtualStart, desc->NumberOfPages,desc->NumberOfPages , (desc->Attribute & EFI_MEMORY_RUNTIME) ? L"RT" : L"", desc->Attribute & 0xffffflu);
    // }

    // Free up of memory
    FreePool(map.buffer);
    Print(L"STEP5 \n");

    // Open Block IO Protocol
    EFI_BLOCK_IO *block_io;
    EFI_GUID block_io_guid = EFI_BLOCK_IO_PROTOCOL_GUID;
    open_protocol(lip->DeviceHandle, &block_io_guid, (VOID **)&block_io, ImageHandle, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    Print(L"STEP6 \n");

    // Get partitions info from block devices
    EFI_BLOCK_IO_MEDIA *block_io_media = block_io->Media;
    UINTN num_blocks = block_io_media->LastBlock + 1;
    Print(L"Block Info : Blocks : %x\n", num_blocks);
    Print(L"STEP7 \n");

    while(1) __asm__ ("hlt");
}