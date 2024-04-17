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

// UEFI Device Path Lib Locate Protocol
VOID *uefi_dev_path_lib_locate_protocol (EFI_GUID *p_guid) {
    EFI_STATUS status;
    VOID *p;

    status = uefi_call_wrapper(BS->LocateProtocol, 3, p_guid, NULL, (VOID **)&p);
    if (EFI_ERROR(status)) {
        return NULL;
    } else {
        return p;
    }
}

// Convert Device Path To Text
EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *dev_path_lib_to_str = NULL;
CHAR16* EFIAPI ConvertDevicePathToText(CONST EFI_DEVICE_PATH_PROTOCOL *dev_path, BOOLEAN display_only, BOOLEAN allow_shortcuts) {
    if (dev_path_lib_to_str == NULL) {
        dev_path_lib_to_str = uefi_dev_path_lib_locate_protocol(&gEfiDevicePathToTextProtocolGuid);
    }

    if (dev_path_lib_to_str != NULL) {
        return uefi_call_wrapper(dev_path_lib_to_str->ConvertDevicePathToText, 3, dev_path, display_only, allow_shortcuts);
    } else {
        return NULL;
    }
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
    EFI_STATUS status;
    InitializeLib(ImageHandle, SystemTable);
    ST = SystemTable;
    BS = SystemTable->BootServices;
    RT = SystemTable->RuntimeServices;

    // Open LIP
    EFI_LOADED_IMAGE_PROTOCOL *lip = NULL;
    EFI_GUID lip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

    open_protocol(ImageHandle, &lip_guid, (void **)&lip, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

    // Open ESP Root
    EFI_FILE_HANDLE esp_root;
    esp_root = LibOpenRoot(lip->DeviceHandle);

    // Get memory map
    memmap map;
    map.buffer = LibMemoryMap(&map.entry, &map.map_key, &map.desc_size, &map.desc_ver);
    ASSERT(map.buffer != NULL);

    // Save memory map
    EFI_FILE_PROTOCOL *memmap_file = NULL;
    save_memmap(&map, memmap_file, esp_root);

    // Free up of memory
    FreePool(map.buffer);

    // Get Block IO Protocol
    EFI_HANDLE block_io_handle;
    EFI_BLOCK_IO *block_io;
    EFI_GUID block_io_guid = EFI_BLOCK_IO_PROTOCOL_GUID;
    UINTN num_handles = 0;
    status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &block_io_guid, NULL, &num_handles, &block_io_handle);
    ASSERT(!EFI_ERROR(status));

    // Process Block IO Handle
    for (UINTN i = 0; i < num_handles; i++) {
        status = uefi_call_wrapper(BS->HandleProtocol, 3, block_io_handle[i], &block_io_guid, (VOID **)&block_io);
        if (!EFI_ERROR(status)) {
            EFI_DEVICE_PATH_PROTOCOL *dev_path;
            CHAR16 *dev_path_str;

            status = uefi_call_wrapper(BS->HandleProtocol, 3, block_io_handle[i], &gEfiDevicePathProtocolGuid, (VOID **)&dev_path);
            if (!EFI_ERROR(status)) {
                dev_path_str = 
            }
        }

    }

    // Open Block IO Protocol
    open_protocol(lip->DeviceHandle, &block_io_guid, (VOID **)&block_io, ImageHandle, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    // Get the device info
    EFI_BLOCK_IO_MEDIA *block_io_media = block_io->Media;
    UINTN num_blocks = block_io_media->LastBlock + 1;
    Print(L"Block Info : Blocks : %x\n", num_blocks);

    // Allocate a buffer
    UINT8 *mbr_buffer = AllocatePool(block_io_media->BlockSize);
    ASSERT(mbr_buffer != NULL);

    // Read blocks
    for (UINTN i = 0; i < num_blocks; i++) {
        status = uefi_call_wrapper(block_io->ReadBlocks, 5, block_io, block_io_media->MediaId, i, block_io_media->BlockSize, mbr_buffer);
        ASSERT(!EFI_ERROR(status));

        // Search EFI PART
        for (UINTN j = 0; j < block_io_media->BlockSize - 6; j++) {
            if (CompareMem(&mbr_buffer[j], EFI_PTAB_HEADER_ID, strlena(EFI_PTAB_HEADER_ID)) == 0) {
                Print(L"Found EFI PART ! \n at block %d, offset %d \n", i, j);
                UINT8 *gpt_buffer = AllocatePool(block_io_media->BlockSize);
                status = uefi_call_wrapper(block_io->ReadBlocks, 5, block_io, block_io_media->MediaId, 0, block_io_media->BlockSize, gpt_buffer);
                ASSERT(!EFI_ERROR(status));
                for (UINTN k = 0; k < block_io_media->BlockSize; k++) {
                    Print(L"%02x ", gpt_buffer[k]);
                    if ((k + 1) % 16 == 0) Print(L"\n");
                }
            }
        }
    }


    while(1) __asm__ ("hlt");
}