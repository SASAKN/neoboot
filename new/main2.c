// GNU_EFI
#include <efi.h>
#include <efilib.h>
#include <efigpt.h>

// NEOBOOT
#include "memory.h"
#include "config.h"

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

// new 
void ListDisks(EFI_HANDLE ImageHandle) {
    EFI_STATUS status;
    EFI_HANDLE *handleBuffer;
    UINTN handleCount;
    EFI_GUID BlockIoProtocol = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_BLOCK_IO_PROTOCOL *BlockIo;
    EFI_DISK_IO_PROTOCOL *DiskIo;
    EFI_GUID DiskIoProtocol = EFI_DISK_IO_PROTOCOL_GUID;

    // Locate all handles that support the Block I/O protocol
    status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &BlockIoProtocol, NULL, &handleCount, &handleBuffer);
    if (EFI_ERROR(status)) {
        Print(L"Failed to locate handles: %r\n", status);
        return;
    }

    // Iterate over each handle
    for (UINTN i = 0; i < handleCount; i++) {
        // Open Block I/O protocol
        status = open_protocol(handleBuffer[i], &BlockIoProtocol, (void **)&BlockIo, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(status)) {
            Print(L"Failed to open Block I/O protocol: %r\n", status);
            continue;
        }

        // Open Disk I/O protocol
        status = open_protocol(handleBuffer[i], &DiskIoProtocol, (void **)&DiskIo, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(status)) {
            Print(L"Failed to open Disk I/O protocol: %r\n", status);
            continue;
        }

        // Print disk information
        Print(L"Disk %u:\n", i);
        Print(L"  MediaId: %u\n", BlockIo->Media->MediaId);
        Print(L"  RemovableMedia: %u\n", BlockIo->Media->RemovableMedia);
        Print(L"  MediaPresent: %u\n", BlockIo->Media->MediaPresent);
        Print(L"  LastBlock: %lu\n", BlockIo->Media->LastBlock);
        Print(L"  BlockSize: %u\n", BlockIo->Media->BlockSize);
        Print(L"  LogicalPartition: %u\n", BlockIo->Media->LogicalPartition);
        Print(L"  ReadOnly: %u\n", BlockIo->Media->ReadOnly);
        Print(L"  WriteCaching: %u\n", BlockIo->Media->WriteCaching);

        if (!BlockIo->Media->MediaPresent) {
            Print(L"  No media present.\n");
            continue;
        }

        // Read GPT header
        CHAR8 headerBuffer[512];
        EFI_PARTITION_TABLE_HEADER *GptHeader;
        status = uefi_call_wrapper(DiskIo->ReadDisk, 5, DiskIo, BlockIo->Media->MediaId, 1 * BlockIo->Media->BlockSize, sizeof(headerBuffer), headerBuffer);
        if (EFI_ERROR(status)) {
            Print(L"  Failed to read GPT header: %r\n", status);
            continue;
        }

        GptHeader = (EFI_PARTITION_TABLE_HEADER *)headerBuffer;

        // Validate GPT header
        if (!strncmpa(headerBuffer, EFI_PTAB_HEADER_ID, 8) == 0) {
            Print(L"GPT Header is Not Found \n");
            continue;
        }
        
        Print(L"  GPT Header found:\n");
        Print(L"    Signature :%u", GptHeader->Header.Signature);
        Print(L"    Revision: %u.%u\n", GptHeader->Header.Revision >> 16, GptHeader->Header.Revision & 0xFFFF);
        Print(L"    HeaderSize: %u\n", GptHeader->Header.HeaderSize);
        Print(L"    MyLBA: %lu\n", GptHeader->MyLBA);
        Print(L"    AlternateLBA: %lu\n", GptHeader->AlternateLBA);
        Print(L"    FirstUsableLBA: %lu\n", GptHeader->FirstUsableLBA);
        Print(L"    LastUsableLBA: %lu\n", GptHeader->LastUsableLBA);
        Print(L"    NumberOfPartitionEntries: %u\n", GptHeader->NumberOfPartitionEntries);

        // Read partition entries
        UINT8 partitionBuffer[BlockIo->Media->BlockSize];
        for (UINTN j = 0; j < GptHeader->NumberOfPartitionEntries; j++) {
            status = uefi_call_wrapper(DiskIo->ReadDisk, 5, DiskIo, BlockIo->Media->MediaId, GptHeader->PartitionEntryLBA * BlockIo->Media->BlockSize + j * sizeof(EFI_PARTITION_ENTRY), sizeof(partitionBuffer), partitionBuffer);
            if (EFI_ERROR(status)) {
                Print(L"    Failed to read partition entry: %r\n", status);
                continue;
            }

            EFI_PARTITION_ENTRY *PartitionEntry = (EFI_PARTITION_ENTRY *)partitionBuffer;
            if (PartitionEntry->PartitionTypeGUID.Data1 != 0 || PartitionEntry->PartitionTypeGUID.Data2 != 0 || PartitionEntry->PartitionTypeGUID.Data3 != 0 || PartitionEntry->PartitionTypeGUID.Data4[0] != 0) {
                Print(L"    Partition %u:\n", j);
                Print(L"      StartingLBA: %lu\n", PartitionEntry->StartingLBA);
                Print(L"      EndingLBA: %lu\n", PartitionEntry->EndingLBA);
                Print(L"      PartitionName: %s\n", PartitionEntry->PartitionName);
            }
        }
    }

    // Free the handle buffer
    FreePool(handleBuffer);
}

void ListDisks(EFI_HANDLE ImageHandle) {
    EFI_STATUS status;
    EFI_HANDLE *handleBuffer;
    UINTN handleCount;
    EFI_GUID BlockIoProtocol = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_BLOCK_IO_PROTOCOL *BlockIo;
    EFI_DISK_IO_PROTOCOL *DiskIo;
    EFI_GUID DiskIoProtocol = EFI_DISK_IO_PROTOCOL_GUID;

    // Locate all handles that support the Block I/O protocol
    status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &BlockIoProtocol, NULL, &handleCount, &handleBuffer);
    if (EFI_ERROR(status)) {
        Print(L"Failed to locate handles: %r\n", status);
        return;
    }

    // Iterate over each handle
    for (UINTN i = 0; i < handleCount; i++) {
        // Open Block I/O protocol
        status = open_protocol(handleBuffer[i], &BlockIoProtocol, (void **)&BlockIo, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(status)) {
            Print(L"Failed to open Block I/O protocol: %r\n", status);
            continue;
        }

        // Open Disk I/O protocol
        status = open_protocol(handleBuffer[i], &DiskIoProtocol, (void **)&DiskIo, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(status)) {
            Print(L"Failed to open Disk I/O protocol: %r\n", status);
            continue;
        }

        // Print disk information
        Print(L"Disk %u:\n", i);
        Print(L"  MediaId: %u\n", BlockIo->Media->MediaId);
        Print(L"  RemovableMedia: %u\n", BlockIo->Media->RemovableMedia);
        Print(L"  MediaPresent: %u\n", BlockIo->Media->MediaPresent);
        Print(L"  LastBlock: %lu\n", BlockIo->Media->LastBlock);
        Print(L"  BlockSize: %u\n", BlockIo->Media->BlockSize);
        Print(L"  LogicalPartition: %u\n", BlockIo->Media->LogicalPartition);
        Print(L"  ReadOnly: %u\n", BlockIo->Media->ReadOnly);
        Print(L"  WriteCaching: %u\n", BlockIo->Media->WriteCaching);

        if (!BlockIo->Media->MediaPresent) {
            Print(L"  No media present.\n");
            continue;
        }

        // Read GPT header
        CHAR8 headerBuffer[512];
        EFI_PARTITION_TABLE_HEADER *GptHeader;
        status = uefi_call_wrapper(DiskIo->ReadDisk, 5, DiskIo, BlockIo->Media->MediaId, 1 * BlockIo->Media->BlockSize, sizeof(headerBuffer), headerBuffer);
        if (EFI_ERROR(status)) {
            Print(L"  Failed to read GPT header: %r\n", status);
            continue;
        }

        GptHeader = (EFI_PARTITION_TABLE_HEADER *)headerBuffer;

        // Validate GPT header
        if (!strncmpa(headerBuffer, EFI_PTAB_HEADER_ID, 8) == 0) {
            Print(L"GPT Header is Not Found \n");
            continue;
        }
        
        Print(L"  GPT Header found:\n");
        Print(L"    Signature :%u", GptHeader->Header.Signature);
        Print(L"    Revision: %u.%u\n", GptHeader->Header.Revision >> 16, GptHeader->Header.Revision & 0xFFFF);
        Print(L"    HeaderSize: %u\n", GptHeader->Header.HeaderSize);
        Print(L"    MyLBA: %lu\n", GptHeader->MyLBA);
        Print(L"    AlternateLBA: %lu\n", GptHeader->AlternateLBA);
        Print(L"    FirstUsableLBA: %lu\n", GptHeader->FirstUsableLBA);
        Print(L"    LastUsableLBA: %lu\n", GptHeader->LastUsableLBA);
        Print(L"    NumberOfPartitionEntries: %u\n", GptHeader->NumberOfPartitionEntries);

        // Read partition entries
        UINT8 partitionBuffer[BlockIo->Media->BlockSize];
        for (UINTN j = 0; j < GptHeader->NumberOfPartitionEntries; j++) {
            status = uefi_call_wrapper(DiskIo->ReadDisk, 5, DiskIo, BlockIo->Media->MediaId, GptHeader->PartitionEntryLBA * BlockIo->Media->BlockSize + j * sizeof(EFI_PARTITION_ENTRY), sizeof(partitionBuffer), partitionBuffer);
            if (EFI_ERROR(status)) {
                Print(L"    Failed to read partition entry: %r\n", status);
                continue;
            }

            EFI_PARTITION_ENTRY *PartitionEntry = (EFI_PARTITION_ENTRY *)partitionBuffer;
            if (PartitionEntry->PartitionTypeGUID.Data1 != 0 || PartitionEntry->PartitionTypeGUID.Data2 != 0 || PartitionEntry->PartitionTypeGUID.Data3 != 0 || PartitionEntry->PartitionTypeGUID.Data4[0] != 0) {
                Print(L"    Partition %u:\n", j);
                Print(L"      StartingLBA: %lu\n", PartitionEntry->StartingLBA);
                Print(L"      EndingLBA: %lu\n", PartitionEntry->EndingLBA);
                Print(L"      PartitionName: %s\n", PartitionEntry->PartitionName);
            }
        }
    }

    // Free the handle buffer
    FreePool(handleBuffer);
}


EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Initialize
    EFI_STATUS status;
    InitializeLib(ImageHandle, SystemTable);

    // Start timer
    EFI_TIME start_time;
    EFI_TIME end_time;
    uefi_call_wrapper(RT->GetTime, 2, &start_time, NULL);
    Print(L"Start Time: %d:%d:%d\n", start_time.Hour, start_time.Minute, start_time.Second);

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

    // List GPT partitions
    ListDisks(ImageHandle);

    // Free up memory
    FreePool(map.buffer);

    // End timer
    uefi_call_wrapper(RT->GetTime, 2, &end_time, NULL);
    UINTN end_time_second = 0;

    // 分が違う場合
    end_time_second += (end_time.Minute - start_time.Minute) * 60;

    // 秒を追加
    end_time_second += (end_time.Second - start_time.Second);

    // Print
    Print(L"Boot Time: %us \n", end_time_second);

    // All Done
    Print(L"All Done!\n");

    while (1) __asm__("hlt");

    return EFI_SUCCESS;
}