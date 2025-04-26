#include <efi.h>
#include <efilib.h>

#define ErrorCheck(actual, expected) if(actual != expected) return actual

EFI_STATUS EFIAPI efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable)
{
    EFI_STATUS result;
    InitializeLib(imageHandle, systemTable);


   // TODO: Load anything that would change the memory map... (ex: OS kernal executable)


    UINTN mapSize = 0, mapKey, descriptorSize;
    EFI_MEMORY_DESCRIPTOR *memoryMap = NULL;
    UINT32 descriptorVersion;
    // Get the required memory pool size for the memory map...
    result = uefi_call_wrapper((void *)systemTable->BootServices->GetMemoryMap, 5, &mapSize, &memoryMap, NULL, &descriptorSize, NULL);
    ErrorCheck(result, EFI_BUFFER_TOO_SMALL);
    // Allocating the pool creates at least one new descriptor... for the chunk of memory changed to EfiLoaderData
    // Not sure that UEFI firmware must allocate on a memory type boundry... if not, then two descriptors might be created
    mapSize += 2 * descriptorSize;
    // Get a pool of memory to hold the map...
    result = uefi_call_wrapper((void *)systemTable->BootServices->AllocatePool, 3, EfiLoaderData, mapSize, (void **)&memoryMap);
    ErrorCheck(result, EFI_SUCCESS);
    // Get the actual memory map...
    result = uefi_call_wrapper((void *)systemTable->BootServices->GetMemoryMap, 5, &mapSize, &memoryMap, &mapKey, &descriptorSize, &descriptorVersion);
    ErrorCheck(result, EFI_SUCCESS);

    result = uefi_call_wrapper((void *)systemTable->BootServices->ExitBootServices, 2, imageHandle, mapKey);
    ErrorCheck(result, EFI_SUCCESS);


    // TODO: Boot Services no longer available. Do whatever with Runtime Services... (ex: start OS kernal executable)


    return EFI_SUCCESS;
}