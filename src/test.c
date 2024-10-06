#include <stdio.h>
#include <stdlib.h>

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

// Split key and values
void split_key_value(char *str, char **key, char **value) {
    char *eq = my_strchr(str, '=');
    if (eq) {
        *eq = '\0'; // = を NULL で置き換え
        *key = str; // キー
        *value = eq + 1; // バリュー
    }
}

// 自分で実装する strtok
char *my_strtok(char *str, const char *delim) {
    static char *next_token = NULL;

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

    char *start = str;

    while (*str && !my_strchr(delim, *str)) {
        str++;
    }

    if (*str) {
        *str = '\0';
        next_token = str + 1;
    } else {
        next_token = NULL;
    }

    return start;
}

// Split
char **split(char *txt, const char *delimiter, int *count) {
    char **tokens = malloc(100 * sizeof(char *));
    if (tokens == NULL) {
        return NULL;
    }

    int i = 0;
    char *token = my_strtok(txt, delimiter);

    while (token != NULL && i < 100) {
        tokens[i] = token;
        i++;
        token = my_strtok(NULL, delimiter);
    }

    tokens[i] = NULL; // NULL で終わる配列
    *count = i;
    return tokens;
}

int main() {
    char input[] = "kernel=kernel.elf\nname=nextos\nimage=zipped.image\nflags=quiet";
    char **lines;
    int count;

    // 改行で分割
    lines = split(input, "\n", &count);
    
    char *keys[count];
    char *values[count];
    for (int i = 0; i < count; i++) {
        // Split
        char *key = NULL;
        char *value = NULL;
        split_key_value(lines[i], &key, &value);

        // Add to arrays
        keys[i] = key;
        values[i] = value;
    }

    printf("Key, Value\n");
    for (int i = 0; i < count; i++) {
        printf("%s, %s\n", keys[i], values[i]);
    }

    free(lines); // メモリの解放

    return 0;
}
