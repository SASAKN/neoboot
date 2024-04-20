#include <efi.h>
#include <efilib.h>
#include <efigpt.h>

void printHex(unsigned char *data, int len) {
    for (int i = 0; i < len; i++) {
        Print(L"%02X ", data[i]);
    }
    Print(L"\n");
}

// Function to read GPT partition entry
EFI_GUID gEfiPartTypeSystemPartitionGuid = { 0xC12A7328L, 0xF81F, 0x11D2, {0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B }};
EFI_GUID gEfiPartTypeBasicDataGuid = { 0xEBD0A0A2L, 0xB9E5, 0x4433, {0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 }};
EFI_STATUS ReadGptPartitionEntry(EFI_BLOCK_IO_PROTOCOL *BlockIo, UINT32 PartitionIndex, EFI_PARTITION_ENTRY **PartEntry) {
    EFI_STATUS Status;
    UINT32 BlockSize = BlockIo->Media->BlockSize;
    UINT64 Lba = 1 + PartitionIndex * sizeof(EFI_PARTITION_ENTRY) / BlockSize;

    *PartEntry = AllocatePool(sizeof(EFI_PARTITION_ENTRY));
    if (*PartEntry == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }

    Status = uefi_call_wrapper(BlockIo->ReadBlocks, 5, BlockIo, BlockIo->Media->MediaId, 1, 512, (VOID *)*PartEntry);
    if (EFI_ERROR(Status)) {
        printHex((unsigned char)**PartEntry, 512);
        FreePool(*PartEntry);
    }

    return Status;
}

CHAR16 *ConvertDevicePathToText(EFI_DEVICE_PATH_PROTOCOL *dev_path, BOOLEAN display_only, BOOLEAN allow_shortcuts) {
    EFI_STATUS Status;
    EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *dev_path_lib_to_str;

    // Get the handle of the DevicePathToStr protocol
    Status = uefi_call_wrapper(BS->LocateProtocol, 3, &gEfiDevicePathToTextProtocolGuid, NULL, (VOID **)&dev_path_lib_to_str);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to locate DevicePathToText protocol\n");
        return L"??";
    }

    // Convert the device path to text
    CHAR16 *dev_path_str = uefi_call_wrapper(dev_path_lib_to_str->ConvertDevicePathToText, 3, dev_path, display_only, allow_shortcuts);
    if (dev_path_str == NULL) {
        Print(L"Failed to convert device path to text\n");
        return L"??";
    }

    return dev_path_str;
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS Status;
    EFI_BLOCK_IO_PROTOCOL *BlockIo;
    EFI_HANDLE *HandleBuffer;
    UINTN NumHandles;
    EFI_GUID guid = EFI_BLOCK_IO_PROTOCOL_GUID;

    // Initialize the UEFI system table
    InitializeLib(ImageHandle, SystemTable);

    // Get the block I/O protocols
    Status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &guid, NULL, &NumHandles, &HandleBuffer);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to locate block I/O protocol\n");
        return Status;
    }

    // Process each handle
    for (UINTN i = 0; i < NumHandles; i++) {
        Status = uefi_call_wrapper(BS->HandleProtocol, 3, HandleBuffer[i], &guid, (VOID **)&BlockIo);
        if (!EFI_ERROR(Status)) {
            EFI_DEVICE_PATH_PROTOCOL *DevicePath;
            CHAR16 *DevicePathStr;

            // Get and display the device path
            Status = uefi_call_wrapper(BS->HandleProtocol, 3, HandleBuffer[i], &gEfiDevicePathProtocolGuid, (VOID **)&DevicePath);
            if (!EFI_ERROR(Status)) {
                DevicePathStr = ConvertDevicePathToText(DevicePath, TRUE, TRUE);
                Print(L"Device: %s\n", DevicePathStr);
                FreePool(DevicePathStr);
            }

            // Get media information from the block I/O protocol
            EFI_BLOCK_IO_MEDIA *Media = BlockIo->Media;

            // If media exists and block size is not 0
            if (Media && Media->BlockSize != 0) {
                // Identify partition type and display
                EFI_PARTITION_ENTRY *PartEntry;
                CHAR16 *PartitionTypeStr;

                // Get partition information from the EFI partition table
                for (UINT32 j = 0; j < 1; j++) {
                    Status = ReadGptPartitionEntry(BlockIo, j, &PartEntry);
                    if (!EFI_ERROR(Status)) {
                        // Identify filesystem type from partition GUID
                        if (CompareGuid(&PartEntry->PartitionTypeGUID, &gEfiPartTypeSystemPartitionGuid)) {
                            PartitionTypeStr = L"EFI System Partition (ESP)";
                        } else if (CompareGuid(&PartEntry->PartitionTypeGUID, &gEfiPartTypeBasicDataGuid)) {
                            PartitionTypeStr = L"Basic Data Partition";
                        } else {
                            PartitionTypeStr = L"Unknown Partition Type";
                        }

                        Print(L"  Partition %d :\nType : %s\nStart : %d\nEnd : %d\n", j, PartitionTypeStr, &PartEntry->StartingLBA, &PartEntry->EndingLBA);
                    }
                }
            }
        }
    }

    // Free memory
    FreePool(HandleBuffer);

    Print(L"All Done !\n");

    while(1) __asm__ ("hlt");


    return EFI_SUCCESS;
}
