// GNU_EFI
#include <efi.h>
#include <efilib.h>
#include <efigpt.h>

// NEOBOOT
#include "memory.h"
#include "disk.h"
#include "config.h"
#include "proto.h"

// Strlen
unsigned int my_strlen(const char *str) {

    unsigned int str_size = 0;

    while(*str != '\0') {
        str++;
        str_size++;
    }

    return str_size;
}

// Strcpy
char *my_strcpy(char *dest, const char *src) {
    char *original_dest = dest;

    while (*src) {
        *dest = *src;
        dest++;
        src++;
    }

    *dest = '\0';

    return original_dest;
}

// Strchr
char *my_strchr(const char *str, int c) {

    // 一文字づつ探す
    while(*str != '\0') {
        if (*str == (char)c) {
            return (char *)str;
        }
        str++;
    }

    // 終端文字を探している場合
    if (c == '\0') {
        return (char *)str;
    }

    return NULL;

}

void split_key_value(char *str, char **key, char **value) {
    char *eq = my_strchr(str, '=');
    if (eq) {
        *eq = '\0';
        *key = str;
        *value = eq + 1;
    } else {
        *key = NULL;
        *value = NULL;
    }
}


// Strdup
char *my_strdup(const char *s) {

    // Caluclate size of string
    int len = 0;
    while (s[len] != '\0') {
        len++;
    }

    // Reserve memory
    char *dup = (char *)AllocatePool(len + 1);
    if (dup == NULL) {
        return NULL;
    }

    // Copy string
    for (int i = 0; i < len; i++) {
        dup[i] = s[i];
    }
    dup[len] = '\0'; // NULL終端

    return dup;
}

// Strtok
char *my_strtok(char *str, const char *delim) {

    static char *next_token = NULL; // トークンを保存
    
    if (str == NULL) {
        str = next_token;
    }

    if (str == NULL) {
        return NULL;
    }

    while (*str && my_strchr(delim, *str)) {
        str++;
    }

    if (*str == '\0') {
        return NULL;
    }

    char *start = str; // トークンの開始位置を保存

    // トークンの終端を探す
    while( *str && !my_strchr(delim, *str)) {
        str++;
    }

    if (*str) {
        *str = '\0';
        next_token = str + 1; // 次の文字へ
        
        // This does NOT have in original C library
        if (delim == ",") {
            next_token = str + 2; // Ignore "\n" after ","
        }

    } else {
        next_token = NULL; // 次のトークンなし
    }

    return start;

}

// Split
char **split(char *txt, const char *delimiter, int *count) {

    // Array of tokens
    char **tokens = AllocatePool(100 * sizeof(char));
    if (tokens == NULL) {
        return NULL;
    }

    UINT32 i = 0;

    // Find a first token by strtok
    char *token = my_strtok(txt, ",");

    // Copy tokens from text to array
    while (token != NULL && i < 100) {
        tokens[i] = token;
        i++;
        token = my_strtok(NULL, ",");
    }

    // NULL terminate the array of tokens
    tokens[i] = NULL;

    // Set count
    *count = i;

    // Return
    return tokens;
}

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

// Config file Parser
Config *config_file_parser(char *config_txt) {
    char **lines;
    int count;
    Config *config = AllocatePool(sizeof(config));

    // Split by ","
    lines = split(config_txt, ",", &count);

    config->keys = AllocatePool(sizeof(char *) * count);
    config->values = AllocatePool(sizeof(char *) * count);

    for (int i = 0; i < count; i++) {

        char *key, *value = NULL;
        unsigned int key_size, value_size = 0;

        // Split by "="
        split_key_value(lines[i], &key, &value);

        // Strlen
        key_size = my_strlen(key) + 1;
        value_size = my_strlen(value) + 1;

        // Allocate each elements
        config->keys[i] = AllocatePool(20);
        config->values[i] = AllocatePool(20);

        // Copy to arrays
        my_strcpy(config->keys[i], key);
        my_strcpy(config->values[i], value);
    }

    // Free
    FreePool(lines);

    // Final
    config->num_keys = count;

    // Return
    return config;

};

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

// List Bootable Disk
void list_bootable_disk(struct bootable_disk_info **disk_info, UINTN *no_of_disks) {
    EFI_STATUS status;

    // Handle
    EFI_HANDLE *handle_buffer = NULL;
    UINTN handle_count = 0;

    // FAT
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    EFI_HANDLE *device_handle;
    EFI_FILE_PROTOCOL *root;

    // Get FS Protocols
    status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &handle_count, &handle_buffer);
    if (EFI_ERROR(status)) {
        Print(L"Failed to locate handles\n");
        return;
    }

    // Allocate the srtuct
    *disk_info = AllocatePool(sizeof(struct bootable_disk_info));

    *no_of_disks = handle_count;

    // Iterate over each handle
    for (UINTN i = 0; i < handle_count; i++) {

        // Select Device Handle
        device_handle = handle_buffer[i];

        // Get a Simple FS Protocol
        status = uefi_call_wrapper(BS->HandleProtocol, 3, device_handle, &gEfiSimpleFileSystemProtocolGuid, (VOID **) &fs);
        ASSERT(!EFI_ERROR(status));

        // Open Root Directory
        status = uefi_call_wrapper(fs->OpenVolume, 2, fs, &root);
        ASSERT(!EFI_ERROR(status));

        // Put root struct in struct
        (*disk_info)->root = root;
    }

    // Count devices
    (*disk_info)->no_of_partition = handle_count;

    // Free
    FreePool(handle_buffer);

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
    BOOLEAN is_selected;
    UINT32 index = (*entries)->no_of_entries - 1;
    index == 0 ? (is_selected = TRUE) : (is_selected = FALSE); // デフォルトで0が選択される
    (*entries)->entries[index].os_name = os_name; // OSの名前
    (*entries)->entries[index].is_selected = is_selected; // 選択状態

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
    UINTN default_font = EFI_LIGHTGRAY | EFI_BLACK;

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

    // Back to the default
    uefi_call_wrapper(ST->ConOut->SetAttribute, 2, ST->ConOut, default_font);

    // Return
    return;

}


// Print entries
void print_entries(entries_list *entries, UINTN *pos_x, UINTN *pos_y, UINTN c) {

    EFI_STATUS status;

    // if entries is NULL, return
    if (entries == NULL) {
        return;
    }

    // Print entries
    for (UINTN i = 0; i < entries->no_of_entries; i++) {
        print_a_entry(entries->entries[i].os_name, i, pos_x, pos_y, c, entries->entries[i].is_selected);
    }

}

// エントリー番号の変更
void modify_an_entry_order(entries_list *list_entries, UINT32 new_entry_order) {

    // Change the order
    list_entries->selected_entry_number = new_entry_order;

    // Change the state
    for (UINT32 i = 0; i < list_entries->no_of_entries; i++) {
        if (i == new_entry_order) {
            list_entries->entries[i].is_selected = TRUE;
        } else {
            list_entries->entries[i].is_selected = FALSE;
        }
    }

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

// コマンドの判別
void determine_command(CHAR16 *buffer) {

    // コマンドを実行
    if ( StrCmp(buffer, L"help") == 0) {

        // Shows help
        Print(L"\nNEOBOOT Console\nCommands\n  1.help - shows help\n  2.menu - back to menu\n  3.start [number] - start any entry\n  4.version - shows version of neoboot\n  5.memmap - shows memory map\n  6.pcinfo - shows info of your pc\n");

    } else if (StrCmp(buffer, L"menu") == 0 ) {
        // Back to the menu
        open_menu(NULL);
    } else if (StrCmp(buffer, L"") == 0) {
        Print(L"\nneoboot >");
        return;
    } else {
        Print(L"\nUnknown Command : %s", buffer);
    }

    // コンソールの表示
    Print(L"\nneoboot >");
    return;

}

// Open the console
void open_console() {

    EFI_STATUS status;

    // Clear the screen
    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

    // Print the title
    Print(L"Welcome to NEOBOOT Console !\n");

    // Print the screen
    Print(L"neoboot > ");

    // Buffer
    CHAR16 buffer[100]; // コマンドは100文字以内
    UINT32 buffer_index = 0;

    // Main Loop
    EFI_INPUT_KEY key;
    while (TRUE) {

        // Reauest keytype
        status = uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &key);

        // Request Commands
        if (!EFI_ERROR(status)) {
            if (key.UnicodeChar != CHAR_CARRIAGE_RETURN) {

                // Save texts and Print
                Print(L"%c", key.UnicodeChar);
                buffer[buffer_index] = key.UnicodeChar;
                buffer_index++;
                
            } else if (key.ScanCode == SCAN_ESC) {
                open_menu(NULL);
            } else {
                
                buffer[buffer_index] = '\0'; // コマンドの終端
                buffer_index = 0; // バッファーも初めに戻る

                determine_command(buffer); // コマンドの判別

            }
        }

    }
}

// Read config file
VOID *read_config_file(EFI_FILE_PROTOCOL *root) {
    
    EFI_FILE_PROTOCOL *config_file;
    EFI_STATUS status;
    CHAR16 *file_name = L"\\config.cfg";
    UINTN buffer_size = 0;
    VOID *buffer = NULL;

    // Open the config file
    status = uefi_call_wrapper(root->Open, 5, root, &config_file, file_name, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        Print(L"Cannot open the config file\n");
    }

    // Get the config file size
    status = uefi_call_wrapper(config_file->GetInfo, 4, config_file, &gEfiFileInfoGuid, &buffer_size, NULL);
    if (status == EFI_BUFFER_TOO_SMALL) {
        buffer = AllocatePool(buffer_size);
        status = uefi_call_wrapper(config_file->GetInfo, 4, config_file, &gEfiFileInfoGuid, &buffer_size, buffer);
        if (EFI_ERROR(status)) {
            Print(L"Cannot get the config file size\n");
        }
    } else {
        Print(L"Cannot determine file size\n");
    }

    // Read the file content
    buffer_size = ((EFI_FILE_INFO *)buffer)->FileSize;
    FreePool(buffer);
    buffer = AllocatePool(buffer_size);
    status = uefi_call_wrapper(config_file->Read, 3, config_file, &buffer_size, buffer);
    if (EFI_ERROR(status)) {
        Print(L"Cannot read the config file\n");
    }

    // Add the NULL end
    CHAR8 *char8_buffer = (CHAR8 *)buffer;
    char8_buffer[buffer_size] = '\0';

    // Close the file
    uefi_call_wrapper(config_file->Close, 1, config_file);

    return buffer;
}

// Open the menu
void open_menu(Config *con) {

    EFI_STATUS status;
    UINTN c, r;
    UINTN pos_x, pos_y;
    UINTN length;
    UINT32 selected_index = 0; // デフォルトで0が選択される
    static int count_opened = 0;
    static Config *config = NULL;

    // ユーザーがメニューを開いた回数を記録
    count_opened += 1;
    
    // メニューを開いた回数によって動作を変える
    if (count_opened = 1) {

        // 1回目にNULLであれば
        if (con = NULL) {
            Print(L"[FATAL ERROR] Could not open the menu");
            return;
        }

        // NULLでなければ
        config = con;

    }

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
    add_a_entry(L"OS 3", &list_entries);
    add_a_entry(L"OS 4", &list_entries);

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
                        break;
                    case 'c':
                    case 'C':
                        open_console();
                    default:
                        break;
                }
            } else {
                switch (key.ScanCode) {
                    case SCAN_UP:

                        // 0は上に行けないし、再描画する必要もない
                        if (selected_index  == 0) {
                            break;
                        }

                        // Indexの変更
                        selected_index = selected_index - 1;

                        // 表示順を変更
                        modify_an_entry_order(list_entries, selected_index);

                        // 再描画
                        redraw_menu(title, c, r, list_entries);
                        break;
                    case SCAN_DOWN:

                        // 合計数より下には行けないし、再描画する必要もない
                        if (selected_index  == list_entries->no_of_entries - 1) {
                            break;
                        }

                        // Indexの変更
                        selected_index = selected_index + 1;

                        // 表示順を変更
                        modify_an_entry_order(list_entries, selected_index);

                        // 再描画
                        redraw_menu(title, c, r, list_entries);
                        break;
                    case SCAN_ESC:
                        return; // BIOSに戻る
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

    // Open config file
    VOID *config_txt = read_config_file(esp_root);
    Config *config = config_file_parser(config_txt);

    // Get memory map
    memmap map;
    map.buffer = LibMemoryMap(&map.entry, &map.map_key, &map.desc_size, &map.desc_ver);
    ASSERT(map.buffer != NULL);

    // Save memory map
    EFI_FILE_PROTOCOL *memmap_file = NULL;
    save_memmap(&map, memmap_file, esp_root);

    // Stall
    uefi_call_wrapper(BS->Stall, 1, 10000000);

    // Open a menu
    open_menu(config);

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

    // Wait for a minute
    uefi_call_wrapper(BS->Stall, 1, 5000000);

    return EFI_SUCCESS;
}