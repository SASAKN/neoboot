#include <efi.h>

// メニューエントリー
typedef struct _OS_MENU_ENTRY {
    CHAR16 *os_name;
    BOOLEAN is_selected;
} MENU_ENTRY;