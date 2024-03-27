#include <efi.h>
#include <efilib.h>
#include "inc/common.h"
#include "inc/memory.h"




// Main 
EFI_STATUS efi_main(EFI_HANDLE IH, EFI_SYSTEM_TABLE *SystemTable) {

    // Initalize EFI
    InitializeLib(IH, SystemTable);

    // Open Root Directory
    EFI_FILE_PROTOCOL *root_dir;
    open_root_dir(IH, root_dir);

    // Init Memory Map
    memmap map;
    init_memmap(&map);

    // Get Memory Map
    get_memmap(&map);

    // Save Memory Map
    EFI_FILE_PROTOCOL *memmap_f;
    memmap_f = NULL;
    save_memmap(&map, memmap_f, root_dir);

    // Free up of memory
    FreePool(map.buffer);

    // Get Partition Table
    


}