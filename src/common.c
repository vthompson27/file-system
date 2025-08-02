#include "common.h"

uint32_t strlen(const char *str) {
    uint32_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* strcpy(char *dest, const char *src) {
    char *orig_dest = dest;
    while ((*dest++ = *src++));
    return orig_dest;
}

void* memcpy(void *dest, const void *src, uint32_t n) {
    char *d = dest;
    const char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void* memset(void *s, int c, uint32_t n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

char* strcat(char *dest, const char *src) {
    // Encontra o final da string de destino
    char *p = dest + strlen(dest);
    // Copia a string de origem para o final
    strcpy(p, src);
    return dest;
}