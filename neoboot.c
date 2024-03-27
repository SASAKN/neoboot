#include <efi.h>
#include <efilib.h>
#include "inc/common.h"


// Main 
EFI_STATUS efi_main(EFI_HANDLE IH, EFI_SYSTEM_TABLE *SystemTable) {

    // Initalize EFI
    InitializeLib(IH, SystemTable);

    // Open Root Directory
    EFI_FILE_PROTOCOL *root_dir;
    open_root_dir(IH, root_dir);

    // Init Memory Map
    



}