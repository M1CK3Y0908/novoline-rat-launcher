#ifndef NOVOLINE_LAUNCHER_H
#define NOVOLINE_LAUNCHER_H

#include <windows.h>

#define PATH_CAP 1024

typedef struct LauncherSettings {
    WCHAR gameDir[PATH_CAP];
    WCHAR javaPath[PATH_CAP];
    WCHAR username[64];
    WCHAR uuid[64];
    WCHAR accessToken[4096];
    WCHAR refreshToken[4096];
    long long tokenExpires;
    int accountType; /* 0 = offline, 1 = Microsoft */
    int memoryMB;
} LauncherSettings;

typedef struct LaunchRequest {
    LauncherSettings settings;
    WCHAR versionId[128];
    WCHAR jarUrl[PATH_CAP];
    WCHAR jsonUrl[PATH_CAP];
} LaunchRequest;

typedef void (*LaunchStatusFn)(const WCHAR *text);

void settings_defaults(LauncherSettings *s);
const WCHAR *default_game_dir(void);
const WCHAR *default_java_dir(void);
void settings_load(LauncherSettings *s);
void settings_save(const LauncherSettings *s);
int detect_java(LauncherSettings *s, int requiredMajor);
int detect_version(const LauncherSettings *s, WCHAR *versionId, int versionCap,
                   WCHAR *jsonPath, int jsonCap);
int launch_minecraft(const LaunchRequest *request, WCHAR *message, int messageCap,
                     LaunchStatusFn status, HANDLE *processOut, DWORD *pidOut);
const WCHAR *msa_authorization_url(void);
int msa_login_with_code(const WCHAR *code, LauncherSettings *s, WCHAR *error, int errorCap);
int msa_refresh_session(LauncherSettings *s, WCHAR *error, int errorCap);

const WCHAR *settings_config_path(void);
const WCHAR *settings_app_dir(void);

#endif
