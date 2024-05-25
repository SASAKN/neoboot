// GNU_EFI
#include <efi.h>
#include <efilib.h>

// AsciiSPrint
UINTN EFIAPI AsciiSPrint(CHAR8 *buffer, UINTN buffer_size, CONST CHAR8 *str, ...) {
    va_list marker;
    UINTN num_printed;

    va_start(marker, str);
    num_printed = AsciiVSPrint(buffer, buffer_size, str, marker);
    va_end(marker);
    return num_printed;
}
