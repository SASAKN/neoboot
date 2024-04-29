#include <efi.h>
#include <efilib.h>

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    EFI_STATUS Status;
    EFI_HANDLE *HandleBuffer;
    UINTN HandleCount;

    // UEFIブートサービスを使用してブロックデバイスのハンドルを取得
    Status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &HandleCount, &HandleBuffer);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to locate block device handles: %r\n", Status);
        return Status;
    }

    Print(L"Connected block devices:\n");
    for (UINTN i = 0; i < HandleCount; i++) {
        EFI_DEVICE_PATH_PROTOCOL *DevicePath;
        Status = uefi_call_wrapper(BS->HandleProtocol, 3, HandleBuffer[i], &gEfiDevicePathProtocolGuid, (VOID **)&DevicePath);
        if (EFI_ERROR(Status)) {
            Print(L"Failed to get device path for handle: %r\n", Status);
            continue;
        }

        // DevicePathを表示
        CHAR16 *DevicePathStr = ConvertDevicePathToText(DevicePath, TRUE, TRUE);
        if (DevicePathStr != NULL) {
            Print(L"%s\n", DevicePathStr);
            FreePool(DevicePathStr);
        }
    }

    // メモリを解放
    FreePool(HandleBuffer);

    return EFI_SUCCESS;
}
