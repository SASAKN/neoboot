// GNU_EFI
#include <efi.h>
#include <efilib.h>
#include <efigpt.h>

// NEOBOOT
#include "memory.h"
#include "disk.h"
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

// Add spaces around text
CHAR16 *add_spaces_around_text(const CHAR16 *text, UINTN num_spaces) {
    UINTN text_length = StrLen(text);
    UINTN new_length = text_length + 2 * num_spaces;

    // AllocatePoolでメモリーを確保
    CHAR16 *new_text = AllocatePool((new_length + 1) * sizeof(CHAR16));
    if (new_text == NULL) {
        return NULL;
    }

    // 先頭にスペースを挿入
    for (UINTN i = 0; i < num_spaces; i++) {
        new_text[i] = ' ';
    }

    // 元の文字列を挿入
    for (UINTN i = 0; i < text_length; i++) {
        new_text[num_spaces + i] = text[i];
    }

    // 後尾にスペースを挿入
    for (UINTN i = 0; i < num_spaces; i++) {
        new_text[num_spaces + text_length + i] = ' ';
    }

    // 終端の設定
    new_text[new_length] = '\0';

    return new_text;
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

// List disks
void list_disks(EFI_HANDLE ImageHandle, struct disk_info **disk_info, UINTN *no_of_disks) {
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

    // Allocate the disk_info struct in the memory
    *disk_info = AllocatePool(handleCount * sizeof(struct disk_info));
    if (*disk_info == NULL) {
        Print(L"Failed to allocate memory\n");
        FreePool(handleBuffer);
        return;
    }

    // Put handle Count into no_of_disks
    *no_of_disks = handleCount;

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

        // Check the media
        if (!BlockIo->Media->MediaPresent) {
            Print(L"  No media present.\n");
            (*disk_info)[i].gpt_found = 0;
            continue;
        }

        

        // Put Media into disk_info
        (*disk_info)[i].Media = *(BlockIo->Media);

        // Read GPT header
        CHAR8 headerBuffer[512];
        EFI_PARTITION_TABLE_HEADER *GptHeader;
        status = uefi_call_wrapper(DiskIo->ReadDisk, 5, DiskIo, BlockIo->Media->MediaId, 1 * BlockIo->Media->BlockSize, sizeof(headerBuffer), headerBuffer);
        if (EFI_ERROR(status)) {
            Print(L"  Failed to read GPT header: %r\n", status);
            continue;
        }

        // Validate GPT header
        if (!strncmpa(headerBuffer, EFI_PTAB_HEADER_ID, 8) == 0) {
            Print(L"GPT Header is Not Found \n");
            (*disk_info)[i].gpt_found = 0;
            continue;
        }

        // Put struct into gpt_header
        GptHeader = (EFI_PARTITION_TABLE_HEADER *)headerBuffer;

        // Put GptHeader into disk_info
        (*disk_info)[i].gpt_found = 1;
        (*disk_info)[i].gpt_header = *GptHeader;

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
        
        // Allocate partition_entries structure
        (*disk_info)[i].partition_entries = AllocatePool(GptHeader->NumberOfPartitionEntries * sizeof(EFI_PARTITION_ENTRY));

        // Get the number of partition
        (*disk_info)[i].no_of_partition = GptHeader->NumberOfPartitionEntries;


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

            (*disk_info)[i].partition_entries[j] = *PartitionEntry;
        }
    }

    // Free the handle buffer
    FreePool(handleBuffer);
}

// Init a struct for the menu
entries_list *init_entries_list() {

    // Allocate the struct
    entries_list *entries = AllocatePool(sizeof(entries_list));

    // Initallize
    if (entries != NULL) {
        entries->entries = NULL;
        entries->no_of_entries = 0;
        entries->selected_entry_number = 0;
    }

    return entries;

}

// Add a entry to the struct
void add_a_entry(CHAR16 *os_name, entries_list **entries) {

    // Allocate a entry
    if ((*entries)->no_of_entries == 0) {
        (*entries)->entries = AllocatePool(sizeof(entry));
        if ((*entries)->entries == NULL) {
            return;
        }
        (*entries)->no_of_entries = 1;
    } else {
        // Reallocate a entry
        entry *new_entries = ReallocatePool((*entries)->entries, ((*entries)->no_of_entries * sizeof(entry)), ( ((*entries)->no_of_entries + 1) * sizeof(entry)));
        if (new_entries == NULL) {
            return;
        }
        (*entries)->entries = new_entries;
        (*entries)->no_of_entries += 1;
    }

    // Create a entry
    is_selected == 0 ? (font = not_selected) : (font = selected);
    UINT32 index = (*entries)->no_of_entries - 1;
    (*entries)->entries[index].os_name = os_name; // OSの名前
    (*entries)->entries[index].is_selected = 0; // 選択していない

    // Return
    return;

}

// Print a entry to the menu
void print_a_entry(CHAR16 *name, UINTN no_of_entries, UINTN *pos_x, UINTN *pos_y, UINTN c, BOOLEAN is_selected) {

    EFI_STATUS status;
    UINTN length;

    // Print Attribute Modes
    UINTN not_selected = EFI_WHITE | EFI_BACKGROUND_BLACK;
    UINTN selected = EFI_BLACK | EFI_BACKGROUND_LIGHTGRAY;

    // Decide text color and background color
    UINTN font;
    is_selected == 0 ? (font = not_selected) : (font = selected);

    // Get length name 
    length = StrLen(name);

    // Calculate the entry text position
    *pos_x = (c - length) / 2;
    *pos_y += 3;
    if (no_of_entries == 0) {
        *pos_y += 2;
    }

    // Set the cursor
    status = uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, 0, *pos_y);
    ASSERT(!EFI_ERROR(status));

    // Add spaces around the text
    name = add_spaces_around_text(name, *pos_x);

    // Set the background color and font color
    uefi_call_wrapper(ST->ConOut->SetAttribute, 2, ST->ConOut, font);
    
    // Print the entry
    uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, name);

}


// Print entries
void print_entries(entries_list *entries, UINTN *pos_x, UINTN *pos_y, UINTN c) {

    EFI_STATUS status;

    // if entries is NULL, return
    if (entries == NULL) {
        return;
    }

    // Change to selected state
    entries->entries[0].is_selected = 1;

    // Print entries
    for (UINTN i = 0; i < entries->no_of_entries; i++) {
        print_a_entry(entries->entries[i].os_name, i, pos_x, pos_y, c, entries->entries[i].is_selected);
    }

}

// エントリー番号の変更
void modify_an_entry_order(entries_list *list_entries, UINT32 new_entry_order) {

    // Change the order
    list_entries->selected_entry_number = new_entry_order;

    // Change the status
    list_entries->entries[new_entry_order]->is_selected = 1;

    // Return
    return;

}

// Redraw the menu
void redraw_menu(CHAR16 *title, UINTN c, UINTN r, entries_list *list_entries) {

    EFI_STATUS status;
    UINTN pos_x, pos_y;
    UINTN length;

    // Clear the screen
    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

    // Get the title
    length = StrLen(title);

    // Calculate the title position
    pos_x = (c - length) / 2;
    pos_y = r / 8;

    // Set the cursor
    status = uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, pos_x, pos_y);
    ASSERT(!EFI_ERROR(status));

    // Print the title
    uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, title);

    // Print entries
    print_entries(list_entries, &pos_x, &pos_y, c);

    // Return
    return;

}

// Open the menu
// 再描画 1h 50m
void open_menu() {

    EFI_STATUS status;
    UINTN c, r;
    UINTN pos_x, pos_y;
    UINTN length;
    UINTN selected_index;

    // Set the title
    CHAR16 *title = L"NEOBOOT Version 0.01";
    length = StrLen(title);

    // Clear the screen
    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

    // Get the conosole size
    status = uefi_call_wrapper(ST->ConOut->QueryMode, 4, ST->ConOut, ST->ConOut->Mode->Mode, &c, &r);
    ASSERT(!EFI_ERROR(status));

    // Calculate the title text position
    pos_x = (c - length) / 2;
    pos_y = r / 8;

    // Set the cursor
    status = uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, pos_x, pos_y);
    ASSERT(!EFI_ERROR(status));
    
    // Print the title
    uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, title);

    // Create entries list
    entries_list *list_entries;

    // Init entries list
    list_entries = init_entries_list();

    // Add entries
    add_a_entry(L"OS 1", &list_entries);
    add_a_entry(L"OS 2", &list_entries);

    // Print entries
    print_entries(list_entries, &pos_x, &pos_y, c);

    // Main Loop 
    EFI_INPUT_KEY key;
    while (TRUE) {
        status = uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &key);
        if (!EFI_ERROR(status)) {
            if (key.UnicodeChar != 0) {
                switch (key.UnicodeChar) {
                    case CHAR_CARRIAGE_RETURN: // Enterキー
                        redraw_menu(title, c, r, list_entries);
                        Print(L"Redrawed\n");
                        break;
                    default:
                        break;
                }
            } else {
                switch (key.ScanCode) {
                    case SCAN_UP:
                        Print(L"Up");
                        break;
                    case SCAN_DOWN:
                        Print(L"Down");
                        break;
                    default:
                        break;
            }
        }
    }
}


}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Initialize
    EFI_STATUS status;
    InitializeLib(ImageHandle, SystemTable);

    // Unlock the watch dog timer
    uefi_call_wrapper(BS->SetWatchdogTimer, 4, 0, 0, 0, NULL);

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
    struct disk_info *disk_info;
    UINTN no_of_disks;
    list_disks(ImageHandle, &disk_info, &no_of_disks);

    // Open a menu
    open_menu();

    // Free up memory
    FreePool(map.buffer);

    // End timer
    uefi_call_wrapper(RT->GetTime, 2, &end_time, NULL);
    UINTN end_time_second = 0;

    // 分が違う場合
    end_time_second += (end_time.Minute - start_time.Minute) * 60;

    // 秒を追加
    end_time_second += end_time.Second - start_time.Second;

    // Print
    Print(L"\nBoot Time: %us \n", end_time_second);

    // All Done
    Print(L"All Done!\n");

    while (1) __asm__("hlt");

    return EFI_SUCCESS;
}