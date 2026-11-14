#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include "win_extra.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdint.h>
#include "launcher.h"
#include "winhttp_flat.h"
#include "json.h"

#ifndef CSIDL_APPDATA
#define CSIDL_APPDATA 0x001a
#endif

#ifndef CP_ACP
#define CP_ACP 0
#endif

#define LIB_CAP 160
#define TOKEN_CAP 96
#define TOKEN_LEN 1024
#define CP_CAP 32768
#define CMD_CAP 65536
#define NATIVE_CAP 32
#define DEFAULT_GAME_DIR L"C:\\M1CK3Y\\novoline.rat"
#define DEFAULT_JAVA_DIR L"C:\\M1CK3Y\\novoline.rat\\java"
#define JAVA_ARCHIVE_URL L"https://github.com/M1CK3Y0908/novoline-rat-versions/raw/refs/heads/main/java.zip"

typedef struct LaunchPlan {
    WCHAR version[128];
    WCHAR jarId[128];
    WCHAR mainClass[256];
    WCHAR assets[64];
    WCHAR assetIndex[128];
    WCHAR tokens[TOKEN_CAP][TOKEN_LEN];
    int tokenCount;
    int argsSeen;
    WCHAR classpath[CP_CAP];
    WCHAR versionJar[PATH_CAP];
    WCHAR nativesDir[PATH_CAP];
    WCHAR nativeJars[NATIVE_CAP][PATH_CAP];
    int nativeCount;
    int javaMajor;
} LaunchPlan;

typedef struct JavaCandidate {
    WCHAR path[PATH_CAP];
    int major;
    int score;
    int found;
} JavaCandidate;

static WCHAR g_appDir[PATH_CAP];
static WCHAR g_configPath[PATH_CAP];
static int g_appDirReady = 0;

static void scopy(WCHAR *dst, const WCHAR *src, int cap) {
    int i = 0;
    if (!dst || cap <= 0) return;
    if (!src) { dst[0] = 0; return; }
    while (src[i] && i < cap - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static void sappend(WCHAR *dst, const WCHAR *src, int cap) {
    int n, i = 0;
    if (!dst || !src || cap <= 0) return;
    n = (int)wcslen(dst);
    if (n >= cap - 1) return;
    while (src[i] && n + i < cap - 1) {
        dst[n + i] = src[i];
        i++;
    }
    dst[n + i] = 0;
}

static void sappend_ch(WCHAR *dst, WCHAR ch, int cap) {
    int n = (int)wcslen(dst);
    if (n < cap - 1) {
        dst[n] = ch;
        dst[n + 1] = 0;
    }
}

static void sappend_int(WCHAR *dst, int value, int cap) {
    WCHAR tmp[32];
    wsprintfW(tmp, L"%d", value);
    sappend(dst, tmp, cap);
}

static int file_exists(const WCHAR *path) {
    DWORD a;
    if (!path || !path[0]) return 0;
    a = GetFileAttributesW(path);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static int dir_exists(const WCHAR *path) {
    DWORD a;
    if (!path || !path[0]) return 0;
    a = GetFileAttributesW(path);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

static void ensure_dir(const WCHAR *path) {
    if (path && path[0]) CreateDirectoryW(path, 0);
}

static void ensure_dir_tree(const WCHAR *path) {
    if (path && path[0]) SHCreateDirectoryExW(0, path, 0);
}

static void path_join(WCHAR *out, int cap, const WCHAR *a, const WCHAR *b) {
    int n;
    if (!out || cap <= 0) return;
    if (!a || !a[0]) { scopy(out, b, cap); return; }
    if (!b || !b[0]) { scopy(out, a, cap); return; }
    if ((b[0] && b[1] == L':') || (b[0] == L'\\' && b[1] == L'\\')) { scopy(out, b, cap); return; }
    scopy(out, a, cap);
    n = (int)wcslen(out);
    if (n > 0 && out[n - 1] != L'\\' && out[n - 1] != L'/') sappend_ch(out, L'\\', cap);
    sappend(out, b, cap);
}

static void path_dirname(WCHAR *path) {
    WCHAR *p, *last = 0;
    if (!path) return;
    for (p = path; *p; p++) if (*p == L'\\' || *p == L'/') last = p;
    if (last) *last = 0;
}

static void path_filename(const WCHAR *path, WCHAR *out, int cap) {
    const WCHAR *p, *last = path;
    if (!path) { scopy(out, L"", cap); return; }
    for (p = path; *p; p++) if (*p == L'\\' || *p == L'/') last = p + 1;
    scopy(out, last, cap);
}

static void get_env_path(const WCHAR *name, WCHAR *out, int cap, const WCHAR *fallback) {
    DWORD n = GetEnvironmentVariableW(name, out, (DWORD)cap);
    if (n == 0 || n >= (DWORD)cap) scopy(out, fallback, cap);
}

const WCHAR *settings_app_dir(void) {
    WCHAR base[PATH_CAP];
    if (g_appDirReady) return g_appDir;
    get_env_path(L"APPDATA", base, PATH_CAP, L".");
    path_join(g_appDir, PATH_CAP, base, L"NovolineLauncher");
    ensure_dir(g_appDir);
    path_join(g_configPath, PATH_CAP, g_appDir, L"settings.ini");
    g_appDirReady = 1;
    return g_appDir;
}

const WCHAR *settings_config_path(void) {
    settings_app_dir();
    return g_configPath;
}

static void sanitize_username(WCHAR *name, int cap) {
    WCHAR src[128];
    int i, j = 0;
    DWORD n = 128;
    src[0] = 0;
    if (!GetUserNameW(src, &n)) src[0] = 0;
    for (i = 0; src[i] && j < cap - 1 && j < 16; i++) {
        WCHAR c = src[i];
        if ((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') ||
            (c >= L'0' && c <= L'9') || c == L'_') {
            name[j++] = c;
        }
    }
    name[j] = 0;
    if (!name[0]) scopy(name, L"Player", cap);
}

const WCHAR *default_game_dir(void) { return DEFAULT_GAME_DIR; }
const WCHAR *default_java_dir(void) { return DEFAULT_JAVA_DIR; }

void settings_defaults(LauncherSettings *s) {
    if (!s) return;
    ZeroMemory(s, sizeof(*s));
    if (dir_exists(L"D:\\Minecraft\\.minecraft")) {
        scopy(s->gameDir, L"D:\\Minecraft\\.minecraft", PATH_CAP);
    } else {
        scopy(s->gameDir, DEFAULT_GAME_DIR, PATH_CAP);
    }
    if (file_exists(L"C:\\Program Files\\Java\\jdk-17\\bin\\java.exe")) {
        scopy(s->javaPath, L"C:\\Program Files\\Java\\jdk-17", PATH_CAP);
    } else {
        scopy(s->javaPath, DEFAULT_JAVA_DIR, PATH_CAP);
    }
    scopy(s->username, L"novoline_rat", 64);
    s->memoryMB = 4096;
}

static char *read_utf8_file(const WCHAR *path, int *outLen) {
    HANDLE h;
    LARGE_INTEGER size;
    DWORD got = 0;
    char *buf;
    if (outLen) *outLen = 0;
    h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (h == INVALID_HANDLE_VALUE) return 0;
    if (!GetFileSizeEx(h, &size) || size.QuadPart < 0 || size.QuadPart > 8 * 1024 * 1024) {
        CloseHandle(h);
        return 0;
    }
    buf = (char *)malloc((size_t)size.QuadPart + 1);
    if (!buf) { CloseHandle(h); return 0; }
    if (!ReadFile(h, buf, (DWORD)size.QuadPart, &got, 0)) {
        free(buf);
        CloseHandle(h);
        return 0;
    }
    CloseHandle(h);
    buf[got] = 0;
    if (outLen) *outLen = (int)got;
    return buf;
}

static WCHAR *utf8_to_wide_alloc(const char *s) {
    int n;
    WCHAR *w;
    if (!s) return 0;
    n = MultiByteToWideChar(CP_UTF8, 0, s, -1, 0, 0);
    if (n <= 0) return 0;
    w = (WCHAR *)calloc((size_t)n, sizeof(WCHAR));
    if (!w) return 0;
    MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n);
    return w;
}

static char *wide_to_utf8_alloc(const WCHAR *s) {
    int n;
    char *a;
    if (!s) return 0;
    n = WideCharToMultiByte(CP_UTF8, 0, s, -1, 0, 0, 0, 0);
    if (n <= 0) return 0;
    a = (char *)calloc((size_t)n, 1);
    if (!a) return 0;
    WideCharToMultiByte(CP_UTF8, 0, s, -1, a, n, 0, 0);
    return a;
}

void settings_load(LauncherSettings *s) {
    WCHAR *wide;
    char *buf;
    int len = 0;
    settings_defaults(s);
    buf = read_utf8_file(settings_config_path(), &len);
    if (!buf) return;
    if (len >= 3 && (unsigned char)buf[0] == 0xEF && (unsigned char)buf[1] == 0xBB &&
        (unsigned char)buf[2] == 0xBF) {
        memmove(buf, buf + 3, (size_t)(len - 2));
    }
    wide = utf8_to_wide_alloc(buf);
    free(buf);
    if (!wide) return;
    {
        WCHAR *p = wide;
        while (*p) {
            WCHAR *eq, *end, *k, *v;
            while (*p == L'\r' || *p == L'\n' || *p == L' ') p++;
            if (!*p) break;
            end = p;
            while (*end && *end != L'\n') end++;
            if (*end == L'\n') { *end = 0; end++; }
            {
                WCHAR *tail = p + wcslen(p);
                while (tail > p && (tail[-1] == L'\r' || tail[-1] == L' ')) *--tail = 0;
            }
            eq = wcschr(p, L'=');
            if (eq) {
                *eq = 0;
                k = p;
                v = eq + 1;
                if (!wcscmp(k, L"gameDir") && v[0]) scopy(s->gameDir, v, PATH_CAP);
                else if (!wcscmp(k, L"javaPath") && v[0]) scopy(s->javaPath, v, PATH_CAP);
                else if (!wcscmp(k, L"username") && v[0]) scopy(s->username, v, 64);
                else if (!wcscmp(k, L"memoryMB")) {
                    int m = _wtoi(v);
                    if (m >= 512 && m <= 32768) s->memoryMB = m;
                }
            }
            p = end;
        }
    }
    free(wide);
    /* Migrate the old AppData\.minecraft default to the game directory. */
    if (!s->gameDir[0] || wcsstr(s->gameDir, L"AppData\\Roaming\\.minecraft")) {
        if (dir_exists(L"D:\\Minecraft\\.minecraft")) scopy(s->gameDir, L"D:\\Minecraft\\.minecraft", PATH_CAP);
        else scopy(s->gameDir, DEFAULT_GAME_DIR, PATH_CAP);
    }
    if (!s->javaPath[0]) {
        if (file_exists(L"C:\\Program Files\\Java\\jdk-17\\bin\\java.exe")) scopy(s->javaPath, L"C:\\Program Files\\Java\\jdk-17", PATH_CAP);
        else scopy(s->javaPath, DEFAULT_JAVA_DIR, PATH_CAP);
    }
    if (s->memoryMB < 512) s->memoryMB = 4096;
    scopy(s->username, L"novoline_rat", 64);
}

void settings_save(const LauncherSettings *s) {
    WCHAR path[PATH_CAP];
    char *game, *java, *user;
    char *text;
    int cap, len;
    DWORD written = 0;
    HANDLE h;
    if (!s) return;
    settings_app_dir();
    path_join(path, PATH_CAP, settings_app_dir(), L"settings.ini");
    game = wide_to_utf8_alloc(s->gameDir);
    java = wide_to_utf8_alloc(s->javaPath);
    user = wide_to_utf8_alloc(s->username);
    if (!game || !java || !user) { free(game); free(java); free(user); return; }
    cap = (int)(strlen(game) + strlen(java) + strlen(user) + 128);
    text = (char *)calloc((size_t)cap, 1);
    if (!text) { free(game); free(java); free(user); return; }
    len = 0;
    len += sprintf(text + len, "gameDir=%s\r\n", game);
    len += sprintf(text + len, "javaPath=%s\r\n", java);
    len += sprintf(text + len, "username=%s\r\n", user);
    len += sprintf(text + len, "memoryMB=%d\r\n", s->memoryMB);
    h = CreateFileW(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (h != INVALID_HANDLE_VALUE) {
        WriteFile(h, text, (DWORD)len, &written, 0);
        CloseHandle(h);
    }
    free(text);
    free(game);
    free(java);
    free(user);
}

static int read_release_major(const WCHAR *dir) {
    WCHAR path[PATH_CAP];
    char *buf;
    int len = 0, i, major = 0;
    path_join(path, PATH_CAP, dir, L"release");
    buf = read_utf8_file(path, &len);
    if (buf) {
        char *p = strstr(buf, "JAVA_VERSION=\"");
        if (p) {
            p += 14;
            if (!strncmp(p, "1.", 2)) p += 2;
            while (*p >= '0' && *p <= '9') {
                major = major * 10 + (*p - '0');
                p++;
            }
        }
        free(buf);
    }
    if (major > 0) return major;
    {
        const WCHAR *p = dir;
        int last = 0;
        while (*p) {
            if (*p >= L'0' && *p <= L'9') {
                int value = 0;
                while (*p >= L'0' && *p <= L'9') { value = value * 10 + (*p - L'0'); p++; }
                if (value >= 5 && value <= 40) last = value;
            } else {
                p++;
            }
        }
        if (last) return last;
    }
    return 0;
}

static void java_candidate_consider(JavaCandidate *best, const WCHAR *dir, int required) {
    WCHAR path[PATH_CAP];
    int major, score;
    path_join(path, PATH_CAP, dir, L"bin\\javaw.exe");
    if (!file_exists(path)) return;
    major = read_release_major(dir);
    if (major <= 0) major = 8;
    if (required <= 0) required = 8;
    if (major == required) score = 1000;
    else if (required <= 8 && major == 8) score = 950;
    else if (major > required) score = 750 - (major - required) * 12;
    else score = 100 - (required - major) * 5;
    if (!best->found || score > best->score) {
        best->found = 1;
        best->major = major;
        best->score = score;
        scopy(best->path, path, PATH_CAP);
    }
}

static void scan_java_root(JavaCandidate *best, const WCHAR *root, int required) {
    WCHAR pattern[PATH_CAP];
    WIN32_FIND_DATAW fd;
    HANDLE h;
    if (!root || !root[0]) return;
    java_candidate_consider(best, root, required);
    path_join(pattern, PATH_CAP, root, L"*");
    h = FindFirstFileW(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L"..")) {
            WCHAR child[PATH_CAP];
            path_join(child, PATH_CAP, root, fd.cFileName);
            java_candidate_consider(best, child, required);
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

int detect_java(LauncherSettings *s, int requiredMajor) {
    JavaCandidate best;
    WCHAR base[PATH_CAP];
    WCHAR path[PATH_CAP];
    int i;
    static const WCHAR *roots[] = {
        L"ProgramFiles\\Eclipse Adoptium",
        L"ProgramFiles\\Java",
        L"ProgramFiles\\Microsoft",
        L"ProgramFiles\\Zulu",
        L"ProgramFiles\\BellSoft",
        L"ProgramFiles\\Amazon Corretto",
        L"ProgramFiles\\Semeru",
        L"ProgramFiles\\Eclipse Foundation",
        L"ProgramFiles(x86)\\Eclipse Adoptium",
        L"ProgramFiles(x86)\\Java",
        L"LocalAppData\\Programs\\Eclipse Adoptium",
        L"LocalAppData\\Programs\\Microsoft",
        0
    };
    if (!s) return 0;
    if (requiredMajor <= 0) requiredMajor = 8;
    ZeroMemory(&best, sizeof(best));
    if (s->javaPath[0] && dir_exists(s->javaPath)) {
        java_candidate_consider(&best, s->javaPath, requiredMajor);
    }
    if (s->javaPath[0] && file_exists(s->javaPath)) {
        WCHAR dir[PATH_CAP];
        scopy(dir, s->javaPath, PATH_CAP);
        path_dirname(dir);
        path_dirname(dir);
        java_candidate_consider(&best, dir, requiredMajor);
    }
    if (GetEnvironmentVariableW(L"JAVA_HOME", path, PATH_CAP) > 0) java_candidate_consider(&best, path, requiredMajor);
    for (i = 0; roots[i]; i++) {
        WCHAR envName[64];
        WCHAR *slash = wcschr(roots[i], L'\\');
        if (slash) {
            int n = (int)(slash - roots[i]);
            if (n > 63) n = 63;
            wcsncpy(envName, roots[i], (size_t)n);
            envName[n] = 0;
            if (GetEnvironmentVariableW(envName, base, PATH_CAP) > 0) {
                path_join(path, PATH_CAP, base, slash + 1);
                scan_java_root(&best, path, requiredMajor);
            }
        }
    }
    if (GetEnvironmentVariableW(L"USERPROFILE", base, PATH_CAP) > 0) {
        path_join(path, PATH_CAP, base, L".jdks");
        scan_java_root(&best, path, requiredMajor);
    }
    if (!best.found) {
        WCHAR found[PATH_CAP];
        DWORD n = SearchPathW(0, L"javaw.exe", 0, PATH_CAP, found, 0);
        if (n > 0 && n < PATH_CAP && file_exists(found)) {
            best.found = 1;
            best.major = 8;
            best.score = 1;
            scopy(best.path, found, PATH_CAP);
        }
    }
    if (best.found) {
        scopy(s->javaPath, best.path, PATH_CAP);
        return 1;
    }
    return 0;
}

static int newer_filetime(const FILETIME *a, const FILETIME *b) {
    return CompareFileTime(a, b) > 0;
}

int detect_version(const LauncherSettings *s, WCHAR *versionId, int versionCap,
                   WCHAR *jsonPath, int jsonCap) {
    WCHAR dir[PATH_CAP], pattern[PATH_CAP];
    WIN32_FIND_DATAW fd;
    HANDLE h;
    FILETIME bestTime;
    WCHAR bestName[128];
    int found = 0;
    if (!s || !s->gameDir[0]) return 0;
    path_join(dir, PATH_CAP, s->gameDir, L"versions");
    path_join(pattern, PATH_CAP, dir, L"*");
    h = FindFirstFileW(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    ZeroMemory(&bestTime, sizeof(bestTime));
    bestName[0] = 0;
    do {
        WCHAR jp[PATH_CAP], vp[PATH_CAP];
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
        path_join(vp, PATH_CAP, dir, fd.cFileName);
        path_join(jp, PATH_CAP, vp, fd.cFileName);
        sappend(jp, L".json", PATH_CAP);
        if (!file_exists(jp)) continue;
        if (!wcscmp(fd.cFileName, L"Novoline")) {
            found = 1;
            scopy(bestName, fd.cFileName, 128);
            bestTime = fd.ftLastWriteTime;
            break;
        }
        if (!found || newer_filetime(&fd.ftLastWriteTime, &bestTime)) {
            found = 1;
            scopy(bestName, fd.cFileName, 128);
            bestTime = fd.ftLastWriteTime;
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    if (!found) return 0;
    scopy(versionId, bestName, versionCap);
    path_join(jsonPath, jsonCap, dir, bestName);
    sappend(jsonPath, L"\\", jsonCap);
    sappend(jsonPath, bestName, jsonCap);
    sappend(jsonPath, L".json", jsonCap);
    return file_exists(jsonPath);
}

static void replace_all(WCHAR *buf, int cap, const WCHAR *key, const WCHAR *value) {
    WCHAR out[TOKEN_LEN * 2];
    WCHAR *p, *start;
    int len = 0, klen, vlen;
    if (!buf || !key || !value) return;
    klen = (int)wcslen(key);
    vlen = (int)wcslen(value);
    if (klen <= 0) return;
    start = buf;
    out[0] = 0;
    while ((p = wcsstr(start, key)) != 0) {
        int chunk = (int)(p - start);
        if (len + chunk + vlen >= (int)(sizeof(out) / sizeof(WCHAR)) - 1) break;
        wcsncpy(out + len, start, (size_t)chunk);
        len += chunk;
        wcsncpy(out + len, value, (size_t)vlen);
        len += vlen;
        out[len] = 0;
        start = p + klen;
    }
    if (len == 0) return;
    if (len + (int)wcslen(start) < (int)(sizeof(out) / sizeof(WCHAR))) {
        scopy(out + len, start, (int)(sizeof(out) / sizeof(WCHAR)) - len);
    }
    scopy(buf, out, cap);
}

/* ---------- MD5 for offline UUID ---------- */

typedef struct Md5Ctx {
    uint32_t state[4];
    uint64_t count;
    unsigned char buffer[64];
} Md5Ctx;

static uint32_t md5_rotl(uint32_t x, int c) { return (x << c) | (x >> (32 - c)); }

static void md5_transform(uint32_t state[4], const unsigned char block[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t x[16];
    int i;
    static const uint32_t K[64] = {
        0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
        0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
        0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
        0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
        0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
        0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
        0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
        0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
    };
    static const int S[64] = {
        7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
        5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
        4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
        6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
    };
    for (i = 0; i < 16; i++) {
        x[i] = (uint32_t)block[i * 4] | ((uint32_t)block[i * 4 + 1] << 8) |
               ((uint32_t)block[i * 4 + 2] << 16) | ((uint32_t)block[i * 4 + 3] << 24);
    }
    for (i = 0; i < 64; i++) {
        uint32_t f, g, temp;
        if (i < 16) { f = (b & c) | ((~b) & d); g = (uint32_t)i; }
        else if (i < 32) { f = (d & b) | ((~d) & c); g = (uint32_t)(5 * i + 1) % 16; }
        else if (i < 48) { f = b ^ c ^ d; g = (uint32_t)(3 * i + 5) % 16; }
        else { f = c ^ (b | (~d)); g = (uint32_t)(7 * i) % 16; }
        temp = d;
        d = c;
        c = b;
        b = b + md5_rotl(a + f + K[i] + x[g], S[i]);
        a = temp;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

static void md5_init(Md5Ctx *ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
    ctx->count = 0;
}

static void md5_update(Md5Ctx *ctx, const unsigned char *data, size_t len) {
    size_t i, index, part;
    index = (size_t)(ctx->count % 64);
    ctx->count += len;
    part = 64 - index;
    if (len >= part) {
        memcpy(ctx->buffer + index, data, part);
        md5_transform(ctx->state, ctx->buffer);
        for (i = part; i + 63 < len; i += 64) md5_transform(ctx->state, data + i);
        index = 0;
    } else {
        i = 0;
    }
    memcpy(ctx->buffer + index, data + i, len - i);
}

static void md5_final(Md5Ctx *ctx, unsigned char digest[16]) {
    unsigned char bits[8];
    size_t index, padLen;
    uint64_t bitCount = ctx->count * 8;
    int i;
    for (i = 0; i < 8; i++) bits[i] = (unsigned char)(bitCount >> (8 * i));
    index = (size_t)(ctx->count % 64);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    {
        unsigned char pad[64];
        ZeroMemory(pad, sizeof(pad));
        pad[0] = 0x80;
        md5_update(ctx, pad, padLen);
    }
    md5_update(ctx, bits, 8);
    for (i = 0; i < 4; i++) {
        digest[i * 4] = (unsigned char)(ctx->state[i] & 0xff);
        digest[i * 4 + 1] = (unsigned char)((ctx->state[i] >> 8) & 0xff);
        digest[i * 4 + 2] = (unsigned char)((ctx->state[i] >> 16) & 0xff);
        digest[i * 4 + 3] = (unsigned char)((ctx->state[i] >> 24) & 0xff);
    }
}

static void offline_uuid(const WCHAR *username, WCHAR *out, int cap) {
    char *user;
    char text[256];
    unsigned char digest[16];
    Md5Ctx ctx;
    user = wide_to_utf8_alloc(username && username[0] ? username : L"novoline_rat");
    if (!user) user = (char *)"novoline_rat";
    sprintf(text, "OfflinePlayer:%s", user);
    md5_init(&ctx);
    md5_update(&ctx, (const unsigned char *)text, strlen(text));
    md5_final(&ctx, digest);
    digest[6] = (unsigned char)((digest[6] & 0x0F) | 0x30);
    digest[8] = (unsigned char)((digest[8] & 0x3F) | 0x80);
    wsprintfW(out,
        L"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        digest[0], digest[1], digest[2], digest[3], digest[4], digest[5],
        digest[6], digest[7], digest[8], digest[9], digest[10], digest[11],
        digest[12], digest[13], digest[14], digest[15]);
    if (user != (char *)"novoline_rat") free(user);
}

/* ---------- version plan ---------- */

static int rules_allow(const Json *rules) {
    int i, allowed = 0;
    if (!rules) return 1;
    if (!json_is_array(rules)) return 1;
    for (i = 0; i < json_count(rules); i++) {
        const Json *rule = json_at(rules, i);
        const WCHAR *action;
        int applies = 1;
        if (!json_is_object(rule)) continue;
        action = json_get_string(rule, "action", 0);
        {
            const Json *os = json_get(rule, "os");
            if (json_is_object(os)) {
                const WCHAR *name = json_get_string(os, "name", 0);
                const WCHAR *arch = json_get_string(os, "arch", 0);
                if (name && wcscmp(name, L"windows")) applies = 0;
                if (arch && wcscmp(arch, L"x86_64") && wcscmp(arch, L"amd64")) applies = 0;
            }
            {
                const Json *features = json_get(rule, "features");
                if (json_is_object(features) && json_count(features) > 0) applies = 0;
            }
        }
        if (applies) {
            if (action && !wcscmp(action, L"allow")) allowed = 1;
            else if (action && !wcscmp(action, L"disallow")) allowed = 0;
        }
    }
    return allowed;
}

static void maven_to_path(const WCHAR *name, const WCHAR *classifierOverride, WCHAR *out, int cap) {
    WCHAR work[512];
    WCHAR *parts[5];
    int count = 0, i;
    WCHAR group[256], artifact[128], version[128], classifier[128];
    WCHAR rel[512];
    scopy(work, name, 512);
    parts[count++] = work;
    for (i = 0; work[i] && count < 5; i++) {
        if (work[i] == L':') {
            work[i] = 0;
            parts[count++] = work + i + 1;
        }
    }
    group[0] = artifact[0] = version[0] = classifier[0] = 0;
    if (count > 0) scopy(group, parts[0], 256);
    if (count > 1) scopy(artifact, parts[1], 128);
    if (count > 2) scopy(version, parts[2], 128);
    if (count > 3) scopy(classifier, parts[3], 128);
    if (classifierOverride && classifierOverride[0]) scopy(classifier, classifierOverride, 128);
    rel[0] = 0;
    {
        int j;
        for (j = 0; group[j]; j++) {
            sappend_ch(rel, group[j] == L'.' ? L'\\' : group[j], 512);
        }
    }
    sappend_ch(rel, L'\\', 512);
    sappend(rel, artifact, 512);
    sappend_ch(rel, L'\\', 512);
    sappend(rel, version, 512);
    sappend_ch(rel, L'\\', 512);
    sappend(rel, artifact, 512);
    sappend_ch(rel, L'-', 512);
    sappend(rel, version, 512);
    if (classifier[0]) {
        sappend_ch(rel, L'-', 512);
        sappend(rel, classifier, 512);
    }
    sappend(rel, L".jar", 512);
    scopy(out, rel, cap);
}

static void plan_add_classpath(LaunchPlan *p, const WCHAR *path) {
    if (!path || !path[0]) return;
    if (p->classpath[0]) sappend(p->classpath, L";", CP_CAP);
    sappend(p->classpath, path, CP_CAP);
}

static void plan_add_native(LaunchPlan *p, const WCHAR *path) {
    if (p->nativeCount < NATIVE_CAP) scopy(p->nativeJars[p->nativeCount++], path, PATH_CAP);
}

static void plan_add_library(LaunchPlan *p, const WCHAR *gameDir, const Json *lib) {
    const WCHAR *name;
    const Json *natives;
    WCHAR classifier[128];
    WCHAR rel[512];
    WCHAR full[PATH_CAP];
    const Json *downloads;
    if (!json_is_object(lib)) return;
    if (!rules_allow(json_get(lib, "rules"))) return;
    name = json_get_string(lib, "name", 0);
    if (!name) return;
    classifier[0] = 0;
    natives = json_get(lib, "natives");
    if (json_is_object(natives)) {
        const WCHAR *win = json_get_string(natives, "windows", 0);
        if (win) {
            scopy(classifier, win, 128);
            {
                WCHAR *a;
                while ((a = wcsstr(classifier, L"${arch}")) != 0) {
                    WCHAR temp[128];
                    scopy(temp, classifier, 128);
                    {
                        WCHAR *pos = wcsstr(temp, L"${arch}");
                        *pos = 0;
                        scopy(classifier, temp, 128);
                        sappend(classifier, L"64", 128);
                        sappend(classifier, pos + 7, 128);
                    }
                }
            }
        }
    }
    downloads = json_get(lib, "downloads");
    if (json_is_object(downloads)) {
        if (classifier[0]) {
            char *key = wide_to_utf8_alloc(classifier);
            const Json *cls = json_get(downloads, "classifiers");
            const Json *nativeEntry = key ? json_get(cls, key) : 0;
            const WCHAR *path = nativeEntry ? json_get_string(nativeEntry, "path", 0) : 0;
            if (path) {
                path_join(full, PATH_CAP, gameDir, L"libraries");
                path_join(full, PATH_CAP, full, path);
                plan_add_native(p, full);
                free(key);
                return;
            }
            free(key);
        } else {
            const Json *artifact = json_get(downloads, "artifact");
            const WCHAR *path = artifact ? json_get_string(artifact, "path", 0) : 0;
            if (path) {
                path_join(full, PATH_CAP, gameDir, L"libraries");
                path_join(full, PATH_CAP, full, path);
                if (file_exists(full)) plan_add_classpath(p, full);
                return;
            }
        }
    }
    maven_to_path(name, classifier[0] ? classifier : 0, rel, 512);
    path_join(full, PATH_CAP, gameDir, L"libraries");
    path_join(full, PATH_CAP, full, rel);
    if (classifier[0]) plan_add_native(p, full);
    else if (file_exists(full)) plan_add_classpath(p, full);
}

static void plan_add_token(LaunchPlan *p, const WCHAR *token) {
    if (!token || !token[0]) return;
    if (p->tokenCount >= TOKEN_CAP) return;
    scopy(p->tokens[p->tokenCount], token, TOKEN_LEN);
    p->tokenCount++;
}

static void plan_args_from_legacy(LaunchPlan *p, const WCHAR *text) {
    WCHAR token[TOKEN_LEN];
    int ti = 0, quote = 0;
    const WCHAR *s = text;
    while (*s) {
        while (*s == L' ' || *s == L'\t' || *s == L'\r' || *s == L'\n') s++;
        if (!*s) break;
        ti = 0;
        quote = 0;
        while (*s) {
            WCHAR c = *s;
            if (quote) {
                if (c == L'"') { quote = 0; s++; continue; }
                if (c == L'\\' && s[1] == L'"') { token[ti++] = L'"'; s += 2; continue; }
                if (ti < TOKEN_LEN - 1) token[ti++] = c;
                s++;
            } else {
                if (c == L'"') { quote = 1; s++; continue; }
                if (c == L' ' || c == L'\t' || c == L'\r' || c == L'\n') break;
                if (ti < TOKEN_LEN - 1) token[ti++] = c;
                s++;
            }
        }
        token[ti] = 0;
        plan_add_token(p, token);
    }
}

static void plan_args_from_array(LaunchPlan *p, const Json *arr) {
    int i;
    if (!json_is_array(arr)) return;
    for (i = 0; i < json_count(arr); i++) {
        const Json *v = json_at(arr, i);
        if (json_is_string(v)) {
            plan_add_token(p, json_string(v));
        } else if (json_is_object(v)) {
            const Json *value;
            if (!rules_allow(json_get(v, "rules"))) continue;
            value = json_get(v, "value");
            if (json_is_string(value)) plan_add_token(p, json_string(value));
            else if (json_is_array(value)) {
                int j;
                for (j = 0; j < json_count(value); j++) {
                    if (json_is_string(json_at(value, j))) plan_add_token(p, json_string(json_at(value, j)));
                }
            }
        }
    }
}

static void plan_process_arguments(LaunchPlan *p, const Json *v) {
    const WCHAR *legacy = json_get_string(v, "minecraftArguments", 0);
    const Json *args = json_get(v, "arguments");
    if (legacy) {
        p->tokenCount = 0;
        p->argsSeen = 1;
        plan_args_from_legacy(p, legacy);
        return;
    }
    if (json_is_object(args)) {
        const Json *game = json_get(args, "game");
        if (json_is_array(game)) {
            p->tokenCount = 0;
            p->argsSeen = 1;
            plan_args_from_array(p, game);
        }
    }
}

static int plan_load_version(LaunchPlan *p, const WCHAR *gameDir, const WCHAR *id, int depth) {
    WCHAR path[PATH_CAP];
    Json *v;
    const WCHAR *parent;
    const Json *libs;
    int i;
    if (depth > 8) return 0;
    path_join(path, PATH_CAP, gameDir, L"versions");
    path_join(path, PATH_CAP, path, id);
    path_join(path, PATH_CAP, path, id);
    sappend(path, L".json", PATH_CAP);
    v = json_parse_file(path);
    if (!v) return 0;
    parent = json_get_string(v, "inheritsFrom", 0);
    if (parent && parent[0]) {
        if (!plan_load_version(p, gameDir, parent, depth + 1)) {
            json_free(v);
            return 0;
        }
    }
    if (depth == 0) {
        scopy(p->version, id, 128);
        {
            const WCHAR *jar = json_get_string(v, "jar", 0);
            if (jar && jar[0]) scopy(p->jarId, jar, 128);
            else scopy(p->jarId, id, 128);
        }
    }
    {
        const WCHAR *mc = json_get_string(v, "mainClass", 0);
        if (mc && mc[0]) scopy(p->mainClass, mc, 256);
    }
    {
        const WCHAR *assets = json_get_string(v, "assets", 0);
        if (assets && assets[0]) scopy(p->assets, assets, 64);
    }
    {
        const Json *ai = json_get(v, "assetIndex");
        if (json_is_object(ai)) {
            const WCHAR *a = json_get_string(ai, "id", 0);
            if (a && a[0]) scopy(p->assetIndex, a, 128);
        }
    }
    {
        const Json *jv = json_get(v, "javaVersion");
        if (json_is_object(jv)) {
            int major = json_get_int(jv, "majorVersion", 0);
            if (major > 0) p->javaMajor = major;
        }
    }
    plan_process_arguments(p, v);
    libs = json_get(v, "libraries");
    if (json_is_array(libs)) {
        for (i = 0; i < json_count(libs); i++) plan_add_library(p, gameDir, json_at(libs, i));
    }
    json_free(v);
    return 1;
}

static void quote_arg(WCHAR *out, int cap, const WCHAR *arg) {
    int i = 0, n = 0, slashes = 0;
    if (!out || cap < 3) return;
    out[n++] = L'"';
    while (arg && arg[i]) {
        if (arg[i] == L'\\') { slashes++; i++; continue; }
        if (arg[i] == L'"') {
            int j;
            for (j = 0; j < slashes * 2 + 1 && n < cap - 1; j++) out[n++] = L'\\';
            if (n < cap - 1) out[n++] = L'"';
            slashes = 0;
            i++;
            continue;
        }
        while (slashes > 0 && n < cap - 1) { out[n++] = L'\\'; slashes--; }
        if (n < cap - 1) out[n++] = arg[i];
        i++;
    }
    while (slashes > 0 && n < cap - 1) { out[n++] = L'\\'; slashes--; }
    if (n < cap - 1) out[n++] = L'"';
    out[n] = 0;
}

static void append_arg(WCHAR *cmd, int cap, const WCHAR *arg) {
    WCHAR q[TOKEN_LEN * 2];
    quote_arg(q, TOKEN_LEN * 2, arg);
    sappend(cmd, L" ", cap);
    sappend(cmd, q, cap);
}

static int run_tar_extract(const WCHAR *jar, const WCHAR *dest) {
    WCHAR sysDir[MAX_PATH];
    WCHAR tarPath[PATH_CAP];
    WCHAR cmd[PATH_CAP * 3];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    DWORD waitResult;
    int ok = 0;
    if (!file_exists(jar)) return 0;
    if (!GetSystemDirectoryW(sysDir, MAX_PATH)) return 0;
    path_join(tarPath, PATH_CAP, sysDir, L"tar.exe");
    if (!file_exists(tarPath)) return 0;
    cmd[0] = 0;
    scopy(cmd, L"\"", PATH_CAP * 3);
    sappend(cmd, tarPath, PATH_CAP * 3);
    sappend(cmd, L"\" -xf \"", PATH_CAP * 3);
    sappend(cmd, jar, PATH_CAP * 3);
    sappend(cmd, L"\" -C \"", PATH_CAP * 3);
    sappend(cmd, dest, PATH_CAP * 3);
    sappend(cmd, L"\"", PATH_CAP * 3);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcessW(tarPath, cmd, 0, 0, FALSE, CREATE_NO_WINDOW, 0, dest, &si, &pi)) return 0;
    waitResult = WaitForSingleObject(pi.hProcess, 30000);
    if (waitResult == WAIT_OBJECT_0) {
        DWORD code = 1;
        if (GetExitCodeProcess(pi.hProcess, &code) && code == 0) ok = 1;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return ok;
}

static int prepare_natives(LaunchPlan *p, const WCHAR *gameDir) {
    WCHAR base[PATH_CAP], dir[PATH_CAP];
    int i, ok = 0;
    path_join(dir, PATH_CAP, gameDir, L"versions");
    path_join(dir, PATH_CAP, dir, p->version);
    path_join(dir, PATH_CAP, dir, p->version);
    sappend(dir, L"-natives", PATH_CAP);
    if (dir_exists(dir)) {
        scopy(p->nativesDir, dir, PATH_CAP);
        return 1;
    }
    path_join(base, PATH_CAP, gameDir, L"natives");
    ensure_dir_tree(base);
    path_join(dir, PATH_CAP, base, p->version);
    ensure_dir_tree(dir);
    scopy(p->nativesDir, dir, PATH_CAP);
    for (i = 0; i < p->nativeCount; i++) {
        if (run_tar_extract(p->nativeJars[i], dir)) ok++;
    }
    return ok;
}

static void write_launch_log(const WCHAR *gameDir, const WCHAR *text) {
    WCHAR path[PATH_CAP];
    char *utf8;
    DWORD written = 0;
    HANDLE h;
    if (!text) return;
    path_join(path, PATH_CAP, gameDir, L"logs");
    ensure_dir_tree(path);
    path_join(path, PATH_CAP, path, L"launcher.log");
    utf8 = wide_to_utf8_alloc(text);
    if (!utf8) return;
    h = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (h != INVALID_HANDLE_VALUE) {
        WriteFile(h, utf8, (DWORD)strlen(utf8), &written, 0);
        CloseHandle(h);
    }
    free(utf8);
}

static void set_download_error(WCHAR *error, int cap, const WCHAR *prefix, DWORD code) {
    if (!error || cap <= 0) return;
    wsprintfW(error, L"%s (error %lu)", prefix, code);
    error[cap - 1] = 0;
}

static int split_https(const WCHAR *url, WCHAR *host, int hostCap, WCHAR *path, int pathCap) {
    const WCHAR *p;
    int n = 0;
    if (!url || wcsncmp(url, L"https://", 8)) return 0;
    p = url + 8;
    while (*p && *p != L'/' && n < hostCap - 1) host[n++] = *p++;
    host[n] = 0;
    if (!host[0] || *p != L'/') return 0;
    scopy(path, p, pathCap);
    return 1;
}

static int download_file(const WCHAR *url, const WCHAR *dest, const WCHAR *label,
                         LaunchStatusFn status, WCHAR *error, int errorCap) {
    HINTERNET session = 0, connect = 0, request = 0;
    HANDLE file = INVALID_HANDLE_VALUE;
    WCHAR host[256], path[2048], part[PATH_CAP], dir[PATH_CAP], msg[320];
    DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    DWORD httpStatus = 0, statusSize = sizeof(httpStatus);
    DWORD contentLength = 0, lengthSize = sizeof(contentLength);
    DWORD total = 0, lastPercent = (DWORD)-1;
    BYTE buffer[65536];
    int ok = 0;
    if (!split_https(url, host, 256, path, 2048)) {
        scopy(error, L"Invalid HTTPS download URL.", errorCap);
        return 0;
    }
    scopy(part, dest, PATH_CAP);
    sappend(part, L".part", PATH_CAP);
    scopy(dir, dest, PATH_CAP);
    path_dirname(dir);
    ensure_dir_tree(dir);
    if (status) status(label);
    session = WinHttpOpen(L"NovolineLauncher/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                          WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        set_download_error(error, errorCap, L"Could not initialize WinHTTP", GetLastError());
        goto cleanup;
    }
    WinHttpSetTimeouts(session, 10000, 10000, 30000, 60000);
    connect = WinHttpConnect(session, host, 443, 0);
    if (!connect) {
        set_download_error(error, errorCap, L"Could not connect to GitHub", GetLastError());
        goto cleanup;
    }
    request = WinHttpOpenRequest(connect, L"GET", path, 0, WINHTTP_NO_REFERER,
                                 WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!request) {
        set_download_error(error, errorCap, L"Could not create download request", GetLastError());
        goto cleanup;
    }
    WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy));
    if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        set_download_error(error, errorCap, L"Could not send download request", GetLastError());
        goto cleanup;
    }
    if (!WinHttpReceiveResponse(request, 0)) {
        set_download_error(error, errorCap, L"Could not receive download response", GetLastError());
        goto cleanup;
    }
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             0, &httpStatus, &statusSize, 0)) {
        set_download_error(error, errorCap, L"Could not read download status", GetLastError());
        goto cleanup;
    }
    if (httpStatus != 200) {
        wsprintfW(error, L"HTTP download returned status %lu.", httpStatus);
        error[errorCap - 1] = 0;
        goto cleanup;
    }
    WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                        0, &contentLength, &lengthSize, 0);
    file = CreateFileW(part, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) {
        set_download_error(error, errorCap, L"Could not create download file", GetLastError());
        goto cleanup;
    }
    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) {
            set_download_error(error, errorCap, L"Could not read download data", GetLastError());
            goto cleanup;
        }
        if (available == 0) break;
        while (available > 0) {
            DWORD chunk = available > sizeof(buffer) ? (DWORD)sizeof(buffer) : available;
            DWORD got = 0, written = 0;
            if (!WinHttpReadData(request, buffer, chunk, &got)) {
                set_download_error(error, errorCap, L"Could not read download body", GetLastError());
                goto cleanup;
            }
            if (got == 0) break;
            if (!WriteFile(file, buffer, got, &written, 0) || written != got) {
                set_download_error(error, errorCap, L"Could not write download file", GetLastError());
                goto cleanup;
            }
            total += got;
            available -= got;
            if (status && contentLength > 0) {
                DWORD percent = (DWORD)(((uint64_t)total * 100) / contentLength);
                if (percent != lastPercent && (percent % 2 == 0 || percent >= 99)) {
                    wsprintfW(msg, L"%s %lu%%", label, percent);
                    status(msg);
                    lastPercent = percent;
                }
            }
        }
    }
    if (file != INVALID_HANDLE_VALUE) {
        CloseHandle(file);
        file = INVALID_HANDLE_VALUE;
    }
    if (!MoveFileExW(part, dest, MOVEFILE_REPLACE_EXISTING)) {
        set_download_error(error, errorCap, L"Could not finish download", GetLastError());
        goto cleanup;
    }
    ok = 1;

cleanup:
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    if (request) WinHttpCloseHandle(request);
    if (connect) WinHttpCloseHandle(connect);
    if (session) WinHttpCloseHandle(session);
    if (!ok) DeleteFileW(part);
    return ok;
}

static int run_java_extract(const WCHAR *archive, const WCHAR *dest, int strip) {
    WCHAR sysDir[MAX_PATH];
    WCHAR tarPath[PATH_CAP];
    WCHAR cmd[PATH_CAP * 4];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    DWORD waitResult;
    int ok = 0;
    if (!file_exists(archive) || !dir_exists(dest)) return 0;
    if (!GetSystemDirectoryW(sysDir, MAX_PATH)) return 0;
    path_join(tarPath, PATH_CAP, sysDir, L"tar.exe");
    if (!file_exists(tarPath)) return 0;
    cmd[0] = 0;
    scopy(cmd, L"\"", PATH_CAP * 4);
    sappend(cmd, tarPath, PATH_CAP * 4);
    sappend(cmd, L"\" -xf \"", PATH_CAP * 4);
    sappend(cmd, archive, PATH_CAP * 4);
    sappend(cmd, L"\" -C \"", PATH_CAP * 4);
    sappend(cmd, dest, PATH_CAP * 4);
    sappend(cmd, L"\"", PATH_CAP * 4);
    if (strip > 0) sappend(cmd, L" --strip-components=1", PATH_CAP * 4);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcessW(tarPath, cmd, 0, 0, FALSE, CREATE_NO_WINDOW, 0, dest, &si, &pi))
        return 0;
    waitResult = WaitForSingleObject(pi.hProcess, 300000);
    if (waitResult == WAIT_OBJECT_0) {
        DWORD code = 1;
        if (GetExitCodeProcess(pi.hProcess, &code) && code == 0) ok = 1;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return ok;
}

static void java_root_path(const LauncherSettings *s, WCHAR *root, int cap) {
    WCHAR name[64];
    if (!s || !root) return;
    if (dir_exists(s->javaPath)) {
        scopy(root, s->javaPath, cap);
        return;
    }
    if (file_exists(s->javaPath)) {
        scopy(root, s->javaPath, cap);
        path_dirname(root);
        path_filename(root, name, 64);
        if (!_wcsicmp(name, L"bin")) path_dirname(root);
        return;
    }
    scopy(root, s->javaPath[0] ? s->javaPath : DEFAULT_JAVA_DIR, cap);
}

static int install_java_archive(const LaunchRequest *request, const WCHAR *url, const WCHAR *name,
                                const WCHAR *dest, int strip, WCHAR *error, int errorCap,
                                LaunchStatusFn status) {
    const LauncherSettings *s = &request->settings;
    WCHAR downloads[PATH_CAP], archive[PATH_CAP], exe[PATH_CAP];
    ensure_dir_tree(s->gameDir);
    path_join(downloads, PATH_CAP, s->gameDir, L"downloads");
    ensure_dir_tree(downloads);
    path_join(archive, PATH_CAP, downloads, name);
    if (!file_exists(archive)) {
        if (!download_file(url, archive, L"Downloading Java runtime", status, error, errorCap))
            return 0;
    }
    if (status) status(L"Extracting Java runtime...");
    ensure_dir_tree(dest);
    path_join(exe, PATH_CAP, dest, L"bin\\java.exe");
    if (!run_java_extract(archive, dest, strip) || !file_exists(exe)) {
        DeleteFileW(archive);
        scopy(error, L"Could not extract the Java runtime archive.", errorCap);
        return 0;
    }
    DeleteFileW(archive);
    return 1;
}

static int ensure_java_runtime(const LaunchRequest *request,
                               WCHAR *error, int errorCap, LaunchStatusFn status) {
    LauncherSettings *s = (LauncherSettings *)&request->settings;
    WCHAR root[PATH_CAP], exe[PATH_CAP];
    if (s->javaPath[0] && file_exists(s->javaPath)) {
        /* The user picked java.exe directly. */
        return 1;
    }
    if (s->javaPath[0] && dir_exists(s->javaPath)) {
        scopy(root, s->javaPath, PATH_CAP);
    } else {
        scopy(root, DEFAULT_JAVA_DIR, PATH_CAP);
    }
    path_join(exe, PATH_CAP, root, L"bin\\java.exe");
    if (file_exists(exe)) {
        scopy(s->javaPath, root, PATH_CAP);
        return 1;
    }
    if (s->javaPath[0] && wcscmp(s->javaPath, DEFAULT_JAVA_DIR)) {
        scopy(error, L"java.exe was not found at the selected Java path.", errorCap);
        return 0;
    }
    if (!install_java_archive(request, JAVA_ARCHIVE_URL, L"java.zip", root, 0,
                              error, errorCap, status))
        return 0;
    scopy(s->javaPath, root, PATH_CAP);
    return 1;
}

static int ensure_version_files(const LaunchRequest *request, WCHAR *versionId, int idCap,
                                WCHAR *error, int errorCap, LaunchStatusFn status) {
    const LauncherSettings *s = &request->settings;
    WCHAR dir[PATH_CAP], jar[PATH_CAP], json[PATH_CAP], label[256];
    int i, n;
    if (!request->versionId[0] || !request->jarUrl[0] || !request->jsonUrl[0]) return 0;
    n = 0;
    for (i = 0; request->versionId[i] && n < idCap - 1; i++) {
        WCHAR c = request->versionId[i];
        if ((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') ||
            (c >= L'0' && c <= L'9') || c == L'-' || c == L'_' || c == L'.') {
            versionId[n++] = c;
        } else {
            versionId[n++] = L'_';
        }
    }
    versionId[n] = 0;
    if (!versionId[0]) scopy(versionId, L"Novoline", idCap);
    ensure_dir_tree(s->gameDir);
    path_join(dir, PATH_CAP, s->gameDir, L"versions");
    ensure_dir(dir);
    path_join(dir, PATH_CAP, dir, versionId);
    ensure_dir(dir);
    path_join(jar, PATH_CAP, dir, versionId);
    sappend(jar, L".jar", PATH_CAP);
    path_join(json, PATH_CAP, dir, versionId);
    sappend(json, L".json", PATH_CAP);
    if (!file_exists(jar)) {
        scopy(label, L"Downloading version ", 256);
        sappend(label, versionId, 256);
        if (!download_file(request->jarUrl, jar, label, status, error, errorCap)) return 0;
    }
    if (!file_exists(json)) {
        if (!download_file(request->jsonUrl, json, L"Downloading version metadata", status,
                           error, errorCap))
            return 0;
    }
    return 1;
}

static int ensure_libraries(const WCHAR *gameDir, const WCHAR *versionJson,
                            LaunchStatusFn status, WCHAR *error, int errorCap) {
    Json *root;
    const Json *libs;
    int i, count;
    root = json_parse_file(versionJson);
    if (!root) {
        scopy(error, L"Could not read version metadata for libraries.", errorCap);
        return 0;
    }
    libs = json_get(root, "libraries");
    count = json_count(libs);
    for (i = 0; i < count; i++) {
        const Json *lib = json_at(libs, i);
        const WCHAR *name;
        const Json *natives;
        WCHAR classifier[128], rel[512], dest[PATH_CAP], url[2048];
        int j, k;
        if (!json_is_object(lib) || !rules_allow(json_get(lib, "rules"))) continue;
        name = json_get_string(lib, "name", 0);
        if (!name) continue;
        classifier[0] = 0;
        natives = json_get(lib, "natives");
        if (json_is_object(natives)) {
            const WCHAR *win = json_get_string(natives, "windows", 0);
            if (win) {
                scopy(classifier, win, 128);
                for (j = 0; classifier[j]; j++) {
                    if (!wcsncmp(classifier + j, L"${arch}", 7)) {
                        WCHAR tmp[128];
                        scopy(tmp, classifier, 128);
                        tmp[j] = 0;
                        scopy(classifier, tmp, 128);
                        sappend(classifier, L"64", 128);
                        sappend(classifier, tmp + j + 7, 128);
                        break;
                    }
                }
            }
        }
        maven_to_path(name, classifier[0] ? classifier : 0, rel, 512);
        path_join(dest, PATH_CAP, gameDir, L"libraries");
        path_join(dest, PATH_CAP, dest, rel);
        if (file_exists(dest)) continue;
        scopy(url, L"https://libraries.minecraft.net/", 2048);
        k = (int)wcslen(url);
        for (j = 0; rel[j] && k < 2047; j++) {
            url[k++] = rel[j] == L'\\' ? L'/' : rel[j];
        }
        url[k] = 0;
        if (!download_file(url, dest, L"Downloading libraries", status, error, errorCap)) {
            json_free(root);
            return 0;
        }
    }
    json_free(root);
    return 1;
}

static void set_message(WCHAR *message, int cap, const WCHAR *text) {
    if (message && cap > 0) scopy(message, text, cap);
}

int launch_minecraft(const LaunchRequest *request, WCHAR *message, int messageCap,
                     LaunchStatusFn status, HANDLE *processOut, DWORD *pidOut) {
    LaunchPlan *plan;
    LauncherSettings *s;
    WCHAR jsonPath[PATH_CAP];
    WCHAR versionId[128];
    WCHAR javaRoot[PATH_CAP], javaExe[PATH_CAP];
    WCHAR fullCp[CP_CAP];
    WCHAR assetsRoot[PATH_CAP], gameAssets[PATH_CAP];
    WCHAR uuid[64];
    WCHAR versionDir[PATH_CAP];
    WCHAR token[TOKEN_LEN * 2];
    WCHAR *cmdHeap;
    int i, javaMajor;
    int result = 0;
    if (processOut) *processOut = 0;
    if (pidOut) *pidOut = 0;
    if (!request) return 0;
    s = (LauncherSettings *)&request->settings;
    set_message(message, messageCap, L"Preparing Minecraft...");
    if (!s->gameDir[0]) {
        set_message(message, messageCap, L"No game directory is configured.");
        return 0;
    }
    ensure_dir_tree(s->gameDir);
    jsonPath[0] = 0;
    if (request->jarUrl[0] && request->jsonUrl[0]) {
        if (!ensure_version_files(request, versionId, 128, message, messageCap, status)) return 0;
    } else if (!detect_version(s, versionId, 128, jsonPath, PATH_CAP)) {
        WCHAR text[PATH_CAP + 128];
        scopy(text, L"No Minecraft versions found in ", PATH_CAP + 128);
        sappend(text, s->gameDir, PATH_CAP + 128);
        set_message(message, messageCap, text);
        return 0;
    }
    {
        WCHAR versionJson[PATH_CAP];
        path_join(versionJson, PATH_CAP, s->gameDir, L"versions");
        path_join(versionJson, PATH_CAP, versionJson, versionId);
        path_join(versionJson, PATH_CAP, versionJson, versionId);
        sappend(versionJson, L".json", PATH_CAP);
        if (!ensure_libraries(s->gameDir, versionJson, status, message, messageCap)) return 0;
    }
    plan = (LaunchPlan *)calloc(1, sizeof(LaunchPlan));
    if (!plan) { set_message(message, messageCap, L"Out of memory."); return 0; }
    plan->javaMajor = -1;
    if (!plan_load_version(plan, s->gameDir, versionId, 0)) {
        free(plan);
        set_message(message, messageCap, L"Could not read the Minecraft version metadata.");
        return 0;
    }
    if (!plan->mainClass[0]) scopy(plan->mainClass, L"net.minecraft.client.main.Main", 256);
    if (!plan->assetIndex[0]) {
        if (plan->assets[0]) scopy(plan->assetIndex, plan->assets, 128);
        else scopy(plan->assetIndex, L"legacy", 128);
    }
    if (!ensure_java_runtime(request, message, messageCap, status)) {
        free(plan);
        return 0;
    }
    java_root_path(s, javaRoot, PATH_CAP);
    path_join(javaExe, PATH_CAP, javaRoot, L"bin\\java.exe");
    if (!file_exists(javaExe)) {
        free(plan);
        set_message(message, messageCap, L"Java runtime is missing after selection.");
        return 0;
    }
    javaMajor = read_release_major(javaRoot);
    if (javaMajor <= 0) javaMajor = 17;
    if (status) {
        WCHAR jmsg[128];
        wsprintfW(jmsg, L"Using Java %d...", javaMajor);
        status(jmsg);
    }
    if (status) status(L"Starting Minecraft...");
    path_join(plan->versionJar, PATH_CAP, s->gameDir, L"versions");
    path_join(plan->versionJar, PATH_CAP, plan->versionJar, plan->jarId);
    path_join(plan->versionJar, PATH_CAP, plan->versionJar, plan->jarId);
    sappend(plan->versionJar, L".jar", PATH_CAP);
    if (!file_exists(plan->versionJar)) {
        WCHAR text[PATH_CAP + 128];
        scopy(text, L"Version jar not found: ", PATH_CAP + 128);
        sappend(text, plan->versionJar, PATH_CAP + 128);
        set_message(message, messageCap, text);
        free(plan);
        return 0;
    }
    fullCp[0] = 0;
    if (plan->classpath[0]) {
        scopy(fullCp, plan->classpath, CP_CAP);
        sappend(fullCp, L";", CP_CAP);
        sappend(fullCp, plan->versionJar, CP_CAP);
    } else {
        scopy(fullCp, plan->versionJar, CP_CAP);
    }
    prepare_natives(plan, s->gameDir);
    path_join(versionDir, PATH_CAP, s->gameDir, L"versions");
    path_join(versionDir, PATH_CAP, versionDir, versionId);
    path_join(assetsRoot, PATH_CAP, s->gameDir, L"assets");
    if (!dir_exists(assetsRoot) && dir_exists(L"D:\\Minecraft\\.minecraft\\assets")) {
        scopy(assetsRoot, L"D:\\Minecraft\\.minecraft\\assets", PATH_CAP);
    }
    path_join(gameAssets, PATH_CAP, assetsRoot, L"virtual\\legacy");
    offline_uuid(s->username, uuid, 64);
    cmdHeap = (WCHAR *)calloc(CMD_CAP, sizeof(WCHAR));
    if (!cmdHeap) { free(plan); set_message(message, messageCap, L"Out of memory."); return 0; }
    scopy(cmdHeap, L"\"", CMD_CAP);
    sappend(cmdHeap, javaExe, CMD_CAP);
    sappend(cmdHeap, L"\" -XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_minecraft.exe.heapdump", CMD_CAP);
    sappend(cmdHeap, L" -Djava.library.path=\"", CMD_CAP);
    sappend(cmdHeap, plan->nativesDir, CMD_CAP);
    sappend(cmdHeap, L"\" -Dorg.lwjgl.librarypath=\"", CMD_CAP);
    sappend(cmdHeap, plan->nativesDir, CMD_CAP);
    sappend(cmdHeap, L"\" -cp \"", CMD_CAP);
    sappend(cmdHeap, fullCp, CMD_CAP);
    sappend(cmdHeap, L"\" -XX:-OmitStackTraceInFastThrow", CMD_CAP);
    sappend(cmdHeap, L" -Djdk.lang.Process.allowAmbiguousCommands=True", CMD_CAP);
    sappend(cmdHeap, L" -Dfml.ignoreInvalidMinecraftCertificates=True", CMD_CAP);
    sappend(cmdHeap, L" -Dfml.ignorePatchDiscrepancies=True", CMD_CAP);
    sappend(cmdHeap, L" -Xmx", CMD_CAP);
    sappend_int(cmdHeap, s->memoryMB > 0 ? s->memoryMB : 4096, CMD_CAP);
    sappend(cmdHeap, L"m -XX:+UnlockExperimentalVMOptions -XX:+UseG1GC -XX:G1NewSizePercent=20 -XX:G1ReservePercent=20 -XX:G1HeapRegionSize=32M -XX:MaxGCPauseMillis=50 -XX:+PerfDisableSharedMem -XX:MinHeapFreeRatio=25 -XX:MaxHeapFreeRatio=40", CMD_CAP);
    sappend(cmdHeap, L" -Dlog4j2.formatMsgNoLookups=true", CMD_CAP);
    sappend(cmdHeap, L" -Dsun.stdout.encoding=GBK -Dsun.stderr.encoding=GBK ", CMD_CAP);
    sappend(cmdHeap, plan->mainClass, CMD_CAP);
    for (i = 0; i < plan->tokenCount; i++) {
        scopy(token, plan->tokens[i], TOKEN_LEN * 2);
        replace_all(token, TOKEN_LEN * 2, L"${auth_player_name}", s->username);
        replace_all(token, TOKEN_LEN * 2, L"${version_name}", plan->version);
        replace_all(token, TOKEN_LEN * 2, L"${game_directory}", versionDir);
        replace_all(token, TOKEN_LEN * 2, L"${assets_root}", assetsRoot);
        replace_all(token, TOKEN_LEN * 2, L"${assets_index_name}", plan->assetIndex);
        replace_all(token, TOKEN_LEN * 2, L"${auth_uuid}", uuid);
        replace_all(token, TOKEN_LEN * 2, L"${auth_access_token}", L"0");
        replace_all(token, TOKEN_LEN * 2, L"${auth_session}", L"0");
        replace_all(token, TOKEN_LEN * 2, L"${user_properties}", L"{}");
        replace_all(token, TOKEN_LEN * 2, L"${user_type}", L"msa");
        replace_all(token, TOKEN_LEN * 2, L"${game_assets}", gameAssets);
        replace_all(token, TOKEN_LEN * 2, L"${natives_directory}", plan->nativesDir);
        replace_all(token, TOKEN_LEN * 2, L"${classpath}", fullCp);
        replace_all(token, TOKEN_LEN * 2, L"${launcher_name}", L"NovolineLauncher");
        replace_all(token, TOKEN_LEN * 2, L"${launcher_version}", L"1.0.0");
        replace_all(token, TOKEN_LEN * 2, L"${resolution_width}", L"854");
        replace_all(token, TOKEN_LEN * 2, L"${resolution_height}", L"480");
        if (wcslen(cmdHeap) + wcslen(token) + 8 >= CMD_CAP) {
            set_message(message, messageCap, L"The launch command is too long.");
            free(cmdHeap);
            free(plan);
            return 0;
        }
        append_arg(cmdHeap, CMD_CAP, token);
    }
    if (!wcsstr(cmdHeap, L"--width")) {
        sappend(cmdHeap, L" --height 480 --width 854", CMD_CAP);
    }
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;

        /* Write dynamically generated launch command to logs\launcher.log */
        {
            WCHAR *header = (WCHAR *)malloc((CMD_CAP + 1024) * sizeof(WCHAR));
            if (header) {
                scopy(header, L"Novoline Launcher Dynamic Command Launch\r\nJava: ", CMD_CAP + 1024);
                sappend(header, javaExe, CMD_CAP + 1024);
                sappend(header, L"\r\nVersion: ", CMD_CAP + 1024);
                sappend(header, plan->version, CMD_CAP + 1024);
                sappend(header, L"\r\nGame directory: ", CMD_CAP + 1024);
                sappend(header, versionDir, CMD_CAP + 1024);
                sappend(header, L"\r\n\r\nCommand:\r\n", CMD_CAP + 1024);
                sappend(header, cmdHeap, CMD_CAP + 1024);
                write_launch_log(s->gameDir, header);
                free(header);
            }
        }

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        /* Launch java directly without creating any bat file, with CREATE_NO_WINDOW
           so no console/command prompt shows, and DETACHED_PROCESS detached. */
        if (CreateProcessW(NULL, cmdHeap, NULL, NULL, FALSE,
                           CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS | CREATE_UNICODE_ENVIRONMENT,
                           NULL, versionDir, &si, &pi)) {
            CloseHandle(pi.hThread);
            if (processOut) {
                *processOut = pi.hProcess;
            } else {
                CloseHandle(pi.hProcess);
            }
            if (pidOut) *pidOut = pi.dwProcessId;
            result = 1;
            set_message(message, messageCap, L"Minecraft is running.");
        } else {
            WCHAR text[256];
            wsprintfW(text, L"Could not start Minecraft (error %lu).", GetLastError());
            set_message(message, messageCap, text);
        }
    }
    free(cmdHeap);
    free(plan);
    return result;
}
