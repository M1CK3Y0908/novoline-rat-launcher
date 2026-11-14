#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdlib.h>
#include <wchar.h>
#include "win_extra.h"
#include "winhttp_flat.h"
#include "json.h"
#include "versions.h"

static void copy_w(WCHAR *dst, int cap, const WCHAR *src) {
    int i = 0;
    if (!dst || cap <= 0) return;
    if (!src) { dst[0] = 0; return; }
    while (src[i] && i < cap - 1) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

static void set_release_date(VersionEntry *entry, const WCHAR *date) {
    int i, len, yy, mm, dd;
    if (!entry) return;
    copy_w(entry->released, 64, L"Released ... days ago");
    if (!date) return;
    len = (int)wcslen(date);
    if (len < 6) return;
    for (i = 0; i < 6; i++) {
        if (date[i] < L'0' || date[i] > L'9') return;
    }
    mm = (date[0] - L'0') * 10 + (date[1] - L'0');
    dd = (date[2] - L'0') * 10 + (date[3] - L'0');
    yy = (date[4] - L'0') * 10 + (date[5] - L'0');
    if (mm < 1 || mm > 12 || dd < 1 || dd > 31) return;
    wsprintfW(entry->released, L"Released 20%02d-%02d-%02d", yy, mm, dd);
}

static void fill_entry(VersionEntry *entry, const Json *obj) {
    const WCHAR *type;
    const WCHAR *date;
    const WCHAR *description;
    const WCHAR *jar;
    const WCHAR *jsonUrl;
    if (!entry || !obj) return;
    ZeroMemory(entry, sizeof(*entry));
    type = json_get_string(obj, "type", L"Release");
    date = json_get_string(obj, "date", L"");
    description = json_get_string(obj, "description", L"");
    jar = json_get_string(obj, "jar", L"");
    jsonUrl = json_get_string(obj, "json", L"");
    if (date && date[0]) {
        wsprintfW(entry->title, L"Novoline %s #%s", type, date);
    } else {
        wsprintfW(entry->title, L"Novoline %s", type);
    }
    set_release_date(entry, date);
    copy_w(entry->description, VERSION_DESC_CAP, description);
    copy_w(entry->jarUrl, VERSION_URL_CAP, jar);
    copy_w(entry->jsonUrl, VERSION_URL_CAP, jsonUrl);
}

static int fetch_url(char **outData, DWORD *outLength, WCHAR *error, int errorCap) {
    static const WCHAR host[] = L"github.com";
    static const WCHAR path[] = L"/M1CK3Y0908/novoline-rat-versions/raw/refs/heads/main/versions.rat";
    HINTERNET session = 0, connect = 0, request = 0;
    DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    DWORD status = 0, statusSize = sizeof(status);
    DWORD total = 0, capacity = 8192;
    char *buf = 0;
    int ok = 0;
    *outData = 0;
    *outLength = 0;
    session = WinHttpOpen(L"NovolineLauncher/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                          WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) { copy_w(error, errorCap, L"Could not initialize WinHTTP."); return 0; }
    WinHttpSetTimeouts(session, 5000, 5000, 10000, 15000);
    connect = WinHttpConnect(session, host, 443, 0);
    if (!connect) {
        wsprintfW(error, L"Could not connect to GitHub (error %lu).", GetLastError());
        goto cleanup;
    }
    request = WinHttpOpenRequest(connect, L"GET", path, 0, WINHTTP_NO_REFERER,
                                 WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!request) {
        wsprintfW(error, L"Could not create HTTP request (error %lu).", GetLastError());
        goto cleanup;
    }
    WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy));
    if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        wsprintfW(error, L"Could not send HTTP request (error %lu).", GetLastError());
        goto cleanup;
    }
    if (!WinHttpReceiveResponse(request, 0)) {
        wsprintfW(error, L"Could not receive HTTP response (error %lu).", GetLastError());
        goto cleanup;
    }
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             0, &status, &statusSize, 0)) {
        wsprintfW(error, L"Could not read HTTP status (error %lu).", GetLastError());
        goto cleanup;
    }
    if (status != 200) {
        wsprintfW(error, L"GitHub returned HTTP %lu.", status);
        goto cleanup;
    }
    buf = (char *)malloc(capacity);
    if (!buf) { copy_w(error, errorCap, L"Out of memory."); goto cleanup; }
    for (;;) {
        DWORD available = 0, got = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) {
            wsprintfW(error, L"Could not read HTTP data (error %lu).", GetLastError());
            goto cleanup;
        }
        if (available == 0) break;
        if (total + available + 1 > 2 * 1024 * 1024) {
            copy_w(error, errorCap, L"versions.rat is too large.");
            goto cleanup;
        }
        if (total + available + 1 > capacity) {
            DWORD newCapacity = capacity;
            char *newBuf;
            while (newCapacity < total + available + 1) newCapacity *= 2;
            newBuf = (char *)realloc(buf, newCapacity);
            if (!newBuf) { copy_w(error, errorCap, L"Out of memory."); goto cleanup; }
            buf = newBuf;
            capacity = newCapacity;
        }
        if (!WinHttpReadData(request, buf + total, available, &got)) {
            wsprintfW(error, L"Could not read HTTP body (error %lu).", GetLastError());
            goto cleanup;
        }
        total += got;
        if (got == 0) break;
    }
    buf[total] = 0;
    *outData = buf;
    *outLength = total;
    buf = 0;
    ok = 1;

cleanup:
    if (buf) free(buf);
    if (request) WinHttpCloseHandle(request);
    if (connect) WinHttpCloseHandle(connect);
    if (session) WinHttpCloseHandle(session);
    return ok;
}

int versions_parse_utf8(const char *data, int length, VersionList *list) {
    Json *root;
    const Json *versions;
    int i, count;
    if (!list || !data) return 0;
    ZeroMemory(list, sizeof(*list));
    root = json_parse_utf8(data, length);
    if (!root) {
        copy_w(list->error, 256, L"Could not parse versions.rat.");
        return 0;
    }
    versions = json_get(root, "versions");
    count = json_count(versions);
    if (!json_is_array(versions) || count <= 0) {
        copy_w(list->error, 256, L"versions.rat contains no versions.");
        json_free(root);
        return 0;
    }
    if (count > VERSION_LIST_MAX) count = VERSION_LIST_MAX;
    for (i = 0; i < count; i++) {
        fill_entry(&list->items[i], json_at(versions, i));
    }
    list->count = count;
    json_free(root);
    return 1;
}

int versions_fetch(VersionList *list) {
    char *data = 0;
    DWORD length = 0;
    int ok;
    if (!list) return 0;
    ZeroMemory(list, sizeof(*list));
    if (!fetch_url(&data, &length, list->error, 256)) {
        if (!list->error[0]) copy_w(list->error, 256, L"Could not download versions.rat.");
        return 0;
    }
    ok = versions_parse_utf8(data, (int)length, list);
    free(data);
    return ok;
}
