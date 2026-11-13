#ifndef NOVOLINE_LAUNCHER_H
#define NOVOLINE_LAUNCHER_H

#include <windows.h>

#define PATH_CAP 1024

typedef struct LauncherSettings {
    WCHAR gameDir[PATH_CAP];
    WCHAR javaPath[PATH_CAP];
    WCHAR username[64];
    int memoryMB;
} LauncherSettings;

void settings_defaults(LauncherSettings *s);
void settings_load(LauncherSettings *s);
void settings_save(const LauncherSettings *s);
int detect_java(LauncherSettings *s, int requiredMajor);
int detect_version(const LauncherSettings *s, WCHAR *versionId, int versionCap,
                   WCHAR *jsonPath, int jsonCap);
int launch_minecraft(LauncherSettings *s, WCHAR *message, int messageCap);

const WCHAR *settings_config_path(void);
const WCHAR *settings_app_dir(void);

#endif
