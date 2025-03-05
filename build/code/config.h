#ifndef _CONFIG_H

#define _CONFIG_H
#include <efi.h>

// Config Fileの構造体
typedef struct _MENU_CONFIG_FILE {

    // コンフィグファイルのキー
    char **keys;

    // その値
    char **values;

    // キーの数
    int num_keys;

} Config;

// エントリー
typedef struct _OS_MENU_ENTRY {

    // OSの名前
    CHAR16 *os_name;

    // OSのConfig
    Config *config;

    // 選択されているか
    BOOLEAN is_selected;

} entry;

// エントリーリスト
typedef struct _OS_MENU_ENTRY_LIST {

    // エントリーの個数
    unsigned int no_of_entries;

    // 選択中のエントリー番号
    unsigned int selected_entry_number;

    // エントリーの配列
    entry *entries;

} entries_list;

#endif