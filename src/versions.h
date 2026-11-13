#ifndef NOVOLINE_VERSIONS_H
#define NOVOLINE_VERSIONS_H

#include <windows.h>

#define VERSION_LIST_MAX 64
#define VERSION_TITLE_CAP 160
#define VERSION_DESC_CAP 4096
#define VERSION_URL_CAP 1024

typedef struct VersionEntry {
    WCHAR title[VERSION_TITLE_CAP];
    WCHAR released[64];
    WCHAR description[VERSION_DESC_CAP];
    WCHAR jarUrl[VERSION_URL_CAP];
    WCHAR jsonUrl[VERSION_URL_CAP];
} VersionEntry;

typedef struct VersionList {
    int count;
    WCHAR error[256];
    VersionEntry items[VERSION_LIST_MAX];
} VersionList;

void versions_fallback(VersionList *list);
int versions_parse_utf8(const char *data, int length, VersionList *list);
int versions_fetch(VersionList *list);

#endif
