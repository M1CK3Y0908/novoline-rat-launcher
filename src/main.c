#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include "win_extra.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <math.h>
#include "gdiplus_flat.h"
#include "launcher.h"
#include "versions.h"

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
#ifndef WM_APP
#define WM_APP 0x8000
#endif
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

#define APP_CLASS L"NovolineLauncherWindow"
#define APP_TITLE L"Novoline Launcher"
#define IDC_LIST       1001
#define IDC_SETTINGS   1002
#define IDC_LOGOUT     1003
#define IDC_LAUNCH     1004
#define IDC_OPEN       1005
#define IDC_DETECT     1006
#define IDC_MEM_EDIT   1007
#define DESIGN_W 584.0f
#define DESIGN_H 361.0f
#define WM_APP_LAUNCH_DONE (WM_APP + 17)
#define WM_APP_VERSIONS_DONE (WM_APP + 18)
#define WM_APP_LAUNCH_STATUS (WM_APP + 19)

#define RGB_BG        RGB(33, 36, 41)
#define RGB_SEP       RGB(43, 46, 53)
#define RGB_PANEL     RGB(37, 41, 48)
#define RGB_BORDER    RGB(48, 51, 59)
#define RGB_TEXT      RGB(198, 200, 205)
#define RGB_HEADLINE  RGB(201, 203, 208)
#define RGB_DIM       RGB(158, 161, 168)
#define RGB_MUTED     RGB(132, 136, 144)
#define RGB_BUTTON    RGB(52, 56, 67)
#define RGB_BUTTON_H  RGB(59, 64, 76)
#define RGB_BUTTON_P  RGB(45, 49, 59)
#define RGB_BTN_BORD  RGB(76, 81, 95)
#define RGB_BTN_BORD_H RGB(88, 94, 110)
#define RGB_GREEN     RGB(86, 200, 120)

typedef struct TRect {
    float x, y, w, h;
} TRect;

typedef struct Layout {
    float W, H;
    TRect items[4];
    TRect settingsBtn;
    TRect logoutBtn;
    TRect launchBtn;
    TRect card;
    TRect openBtn, detectBtn;
    TRect memEdit;
    float mainX, mainW;
} Layout;

static HWND g_hwnd;
static ULONG_PTR g_gdiplusToken;
static HWND g_list;
static HWND g_settingsBtn;
static HWND g_logoutBtn;
static HWND g_launchBtn;
static HWND g_openBtn;
static HWND g_detectBtn;
static HWND g_memEdit;
static HBRUSH g_editBrush;
static HWND g_hoverButton;
static int g_listHover = -1;
static HBRUSH g_listBrush;
static int g_dpi = 96;
static float g_scale = 1.0f;
static int g_clientW = 0, g_clientH = 0;
static int g_page = 0;
static int g_selected = 0;
static int g_launching = 0;
#define LAUNCH_IDLE     0
#define LAUNCH_STARTING 1
#define LAUNCH_RUNNING  2
#define LAUNCH_FAILED   3
static int g_launchState = LAUNCH_IDLE;
static DWORD g_launchStarted = 0;
static HANDLE g_gameProcess = 0;
static DWORD g_gamePid = 0;

typedef struct GameWindowSearch {
    DWORD pid;
    int found;
} GameWindowSearch;

static BOOL CALLBACK find_game_window(HWND hwnd, LPARAM lp) {
    GameWindowSearch *search = (GameWindowSearch *)lp;
    DWORD pid = 0;
    WCHAR title[8];
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != search->pid || !IsWindowVisible(hwnd)) return TRUE;
    title[0] = 0;
    if (GetWindowTextW(hwnd, title, 8) > 0) {
        search->found = 1;
        return FALSE;
    }
    return TRUE;
}

static int game_window_exists(DWORD pid) {
    GameWindowSearch search;
    if (!pid) return 0;
    search.pid = pid;
    search.found = 0;
    EnumWindows(find_game_window, (LPARAM)&search);
    return search.found;
}
static LauncherSettings g_settings;
static WCHAR g_versionId[128];
static int g_versionFound = 0;
static HICON g_appIcon = 0;
static HICON g_appIconSm = 0;

static HICON load_app_icon(int cx, int cy) {
    WCHAR path[MAX_PATH];
    HICON icon = 0;
    UINT iconId = 0;
    if (!GetModuleFileNameW(0, path, MAX_PATH)) return 0;
    if (PrivateExtractIconsW(path, 0, cx, cy, &icon, &iconId, 1, 0) != 1) return 0;
    return icon;
}

static void scopy(WCHAR *dst, const WCHAR *src, int cap) {
    int i = 0;
    if (!dst || cap <= 0) return;
    if (!src) { dst[0] = 0; return; }
    while (src[i] && i < cap - 1) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

static int dir_exists(const WCHAR *path) {
    DWORD a;
    if (!path || !path[0]) return 0;
    a = GetFileAttributesW(path);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

static HFONT g_font24, g_font22, g_font20, g_font16, g_font36;
static float g_fontScale = -1.0f;
static VersionList g_versions;
static int g_versionsLoading = 1;
static WCHAR g_versionsError[256];

typedef struct LaunchResult {
    WCHAR message[512];
    int ok;
    HANDLE process;
    DWORD pid;
} LaunchResult;

static LaunchResult g_launchResult;
static LaunchRequest g_launchRequest;

static void launch_status(const WCHAR *text) {
    (void)text;
}

static void remote_version_id(const VersionEntry *entry, WCHAR *out, int cap) {
    const WCHAR *name;
    int i = 0;
    if (!out || cap <= 0) return;
    out[0] = 0;
    if (!entry || !entry->jarUrl[0]) {
        scopy(out, L"Novoline", cap);
        return;
    }
    name = wcsrchr(entry->jarUrl, L'/');
    name = name ? name + 1 : entry->jarUrl;
    while (name[i] && i < cap - 1) {
        out[i] = name[i];
        i++;
    }
    out[i] = 0;
    if (i > 4 && !wcscmp(out + i - 4, L".jar")) out[i - 4] = 0;
    if (!out[0]) scopy(out, L"Novoline", cap);
}

/* ---------- GDI+ helpers ---------- */

static DWORD gp_color(COLORREF c, int a) {
    return ((DWORD)(a & 255) << 24) | ((DWORD)GetRValue(c) << 16) |
           ((DWORD)GetGValue(c) << 8) | (DWORD)GetBValue(c);
}

static GpPath *round_path(float x, float y, float w, float h, float r) {
    GpPath *p = 0;
    if (w <= 0 || h <= 0) return 0;
    GdipCreatePath(GpFillModeWinding, &p);
    if (!p) return 0;
    if (r <= 0.2f) {
        GdipAddPathRectangle(p, x, y, w, h);
        return p;
    }
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    GdipAddPathArc(p, x, y, r * 2, r * 2, 180, 90);
    GdipAddPathLine(p, x + r, y, x + w - r, y);
    GdipAddPathArc(p, x + w - r * 2, y, r * 2, r * 2, 270, 90);
    GdipAddPathLine(p, x + w, y + r, x + w, y + h - r);
    GdipAddPathArc(p, x + w - r * 2, y + h - r * 2, r * 2, r * 2, 0, 90);
    GdipAddPathLine(p, x + w - r, y + h, x + r, y + h);
    GdipAddPathArc(p, x, y + h - r * 2, r * 2, r * 2, 90, 90);
    GdipClosePathFigure(p);
    return p;
}

static void fill_rect(GpGraphics *g, COLORREF color, float x, float y, float w, float h) {
    GpSolidFill *b = 0;
    GdipCreateSolidFill(gp_color(color, 255), &b);
    if (b) {
        GdipFillRectangle(g, b, x * g_scale, y * g_scale, w * g_scale, h * g_scale);
        GdipDeleteBrush((GpBrush *)b);
    }
}

static void panel(GpGraphics *g, COLORREF fill, COLORREF border, float x, float y,
                  float w, float h, float r, float borderWidth) {
    GpPath *p = round_path(x * g_scale, y * g_scale, w * g_scale, h * g_scale, r * g_scale);
    GpSolidFill *b = 0;
    GpPen *pen = 0;
    if (!p) return;
    GdipCreateSolidFill(gp_color(fill, 255), &b);
    if (b) {
        GdipFillPath(g, b, p);
        GdipDeleteBrush((GpBrush *)b);
    }
    if (borderWidth > 0) {
        GdipCreatePen1(gp_color(border, 255), borderWidth * g_scale, GpUnitPixel, &pen);
        if (pen) {
            GdipSetPenLineJoin(pen, GpLineJoinRound);
            GdipDrawPath(g, pen, p);
            GdipDeletePen(pen);
        }
    }
    GdipDeletePath(p);
}

/* ---------- fonts and text ---------- */

static HFONT create_ui_font(int px, int weight) {
    int h = (int)(px * g_scale + 0.5f);
    if (h < 1) h = 1;
    return CreateFontW(-h, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                       DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

static void destroy_fonts(void) {
    if (g_font24) DeleteObject(g_font24);
    if (g_font22) DeleteObject(g_font22);
    if (g_font20) DeleteObject(g_font20);
    if (g_font16) DeleteObject(g_font16);
    if (g_font36) DeleteObject(g_font36);
    g_font24 = g_font22 = g_font20 = g_font16 = g_font36 = 0;
}

static void ensure_fonts(void) {
    float diff = g_fontScale - g_scale;
    if (diff < 0) diff = -diff;
    if (g_font36 && g_font24 && g_font22 && g_font20 && g_font16 && diff < 0.005f) return;
    destroy_fonts();
    g_font24 = create_ui_font(12, FW_NORMAL);
    g_font22 = create_ui_font(12, FW_NORMAL);
    g_font20 = create_ui_font(12, FW_NORMAL);
    g_font16 = create_ui_font(10, FW_NORMAL);
    g_font36 = create_ui_font(20, FW_BOLD);
    g_fontScale = g_scale;
}

static void draw_text(HDC dc, const WCHAR *text, float x, float y, float w, float h,
                      HFONT font, COLORREF color, UINT flags) {
    RECT r;
    HGDIOBJ old;
    if (!text || !dc) return;
    if (!font) font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    old = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    r.left = (LONG)(x * g_scale);
    r.top = (LONG)(y * g_scale);
    r.right = (LONG)((x + w) * g_scale) + 1;
    r.bottom = (LONG)((y + h) * g_scale) + 1;
    DrawTextW(dc, text, -1, &r, flags | DT_NOPREFIX);
    SelectObject(dc, old);
}

/* ---------- layout ---------- */

static Layout make_layout(void) {
    Layout l;
    float H = (float)g_clientH / g_scale;
    float W = (float)g_clientW / g_scale;
    int i;
    ZeroMemory(&l, sizeof(l));
    l.W = W;
    l.H = H;
    l.mainX = 210.0f;
    l.mainW = W - l.mainX - 10.0f;
    if (l.mainW < 80) l.mainW = 80;
    for (i = 0; i < 4; i++) {
        l.items[i].x = 10.0f;
        l.items[i].y = 10.0f + i * 22.0f;
        l.items[i].w = 180.0f;
        l.items[i].h = 22.0f;
    }
    l.settingsBtn.x = 10.0f;
    l.settingsBtn.y = H - 68.0f;
    l.settingsBtn.w = 180.0f;
    l.settingsBtn.h = 24.0f;
    l.logoutBtn.x = 10.0f;
    l.logoutBtn.y = H - 34.0f;
    l.logoutBtn.w = 180.0f;
    l.logoutBtn.h = 24.0f;
    l.launchBtn.x = 210.0f;
    l.launchBtn.y = H - 34.0f;
    l.launchBtn.w = W - 210.0f - 10.0f;
    l.launchBtn.h = 24.0f;
    l.card.x = 210.0f;
    l.card.y = 58.0f;
    l.card.w = l.mainW;
    l.card.h = 196.0f;
    l.openBtn.x = l.card.x + l.card.w - 70.0f;
    l.openBtn.y = 72.0f;
    l.openBtn.w = 60.0f;
    l.openBtn.h = 24.0f;
    l.detectBtn.x = l.card.x + l.card.w - 70.0f;
    l.detectBtn.y = 112.0f;
    l.detectBtn.w = 60.0f;
    l.detectBtn.h = 24.0f;
    l.memEdit.x = l.card.x + 8.0f;
    l.memEdit.y = 160.0f;
    l.memEdit.w = 90.0f;
    l.memEdit.h = 22.0f;
    return l;
}


/* ---------- native controls ---------- */

static void set_control_theme(HWND hwnd) {
    HMODULE ux = LoadLibraryW(L"uxtheme.dll");
    if (ux) {
        typedef HRESULT (WINAPI *PFN_SetWindowTheme)(HWND, LPCWSTR, LPCWSTR);
        PFN_SetWindowTheme fn = (PFN_SetWindowTheme)GetProcAddress(ux, "SetWindowTheme");
        if (fn) fn(hwnd, L"DarkMode_Explorer", 0);
        FreeLibrary(ux);
    }
}

static void set_window_theme_none(HWND hwnd) {
    HMODULE ux = LoadLibraryW(L"uxtheme.dll");
    if (ux) {
        typedef HRESULT (WINAPI *PFN_SetWindowTheme)(HWND, LPCWSTR, LPCWSTR);
        PFN_SetWindowTheme fn = (PFN_SetWindowTheme)GetProcAddress(ux, "SetWindowTheme");
        if (fn) fn(hwnd, L"", L"");
        FreeLibrary(ux);
    }
}

static int is_owner_button(HWND hwnd) {
    return hwnd && (hwnd == g_settingsBtn || hwnd == g_logoutBtn || hwnd == g_launchBtn ||
                    hwnd == g_openBtn || hwnd == g_detectBtn);
}

static void update_hover_states(void) {
    POINT pt;
    HWND target;
    HWND old;
    int item = -1;
    GetCursorPos(&pt);
    target = WindowFromPoint(pt);
    if (!is_owner_button(target) || !IsWindowVisible(target) || !IsWindowEnabled(target)) target = 0;
    if (GetForegroundWindow() != g_hwnd) target = 0;
    if (GetForegroundWindow() == g_hwnd && g_list && IsWindowVisible(g_list)) {
        POINT cp = pt;
        RECT lr;
        ScreenToClient(g_list, &cp);
        GetClientRect(g_list, &lr);
        if (PtInRect(&lr, cp)) {
            LRESULT hit = SendMessageW(g_list, LB_ITEMFROMPOINT, 0, MAKELPARAM(cp.x, cp.y));
            if (HIWORD(hit) == 0) item = (int)LOWORD(hit);
        }
    }
    if (item != g_listHover) {
        g_listHover = item;
        InvalidateRect(g_list, 0, FALSE);
    }
    if (target == g_hoverButton) return;
    old = g_hoverButton;
    g_hoverButton = target;
    if (old) InvalidateRect(old, 0, FALSE);
    if (target) InvalidateRect(target, 0, FALSE);
    if (g_page == 0 && g_launchState == LAUNCH_RUNNING && (target == g_launchBtn || old == g_launchBtn)) {
        SetWindowTextW(g_launchBtn, target == g_launchBtn ? L"Shutdown" : L"Running");
    }
}

static void populate_version_list(void) {
    int i;
    if (!g_list) return;
    SendMessageW(g_list, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_versions.count; i++) {
        SendMessageW(g_list, LB_ADDSTRING, 0, (LPARAM)g_versions.items[i].title);
    }
    if (g_versions.count <= 0) g_selected = 0;
    else if (g_selected >= g_versions.count) g_selected = g_versions.count - 1;
    if (g_versions.count > 0) {
        SendMessageW(g_list, LB_SETCURSEL, (WPARAM)g_selected, 0);
        ShowWindow(g_list, SW_SHOW);
    } else {
        ShowWindow(g_list, SW_HIDE);
    }
    InvalidateRect(g_list, 0, FALSE);
}

static void create_controls(HWND hwnd) {
    HINSTANCE inst = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);
    if (!g_listBrush) g_listBrush = CreateSolidBrush(RGB_BG);
    g_list = CreateWindowExW(0, L"LISTBOX", 0,
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT |
                                 LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
                             0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_LIST, inst, 0);
    g_settingsBtn = CreateWindowExW(0, L"BUTTON", L"Settings",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                    0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_SETTINGS, inst, 0);
    g_logoutBtn = CreateWindowExW(0, L"BUTTON", L"Log out",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                  0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_LOGOUT, inst, 0);
    g_launchBtn = CreateWindowExW(0, L"BUTTON", L"Launch",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                  0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_LAUNCH, inst, 0);
    g_openBtn = CreateWindowExW(0, L"BUTTON", L"Browse",
                                WS_CHILD | WS_TABSTOP | BS_OWNERDRAW,
                                0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_OPEN, inst, 0);
    g_detectBtn = CreateWindowExW(0, L"BUTTON", L"Browse",
                                  WS_CHILD | WS_TABSTOP | BS_OWNERDRAW,
                                  0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_DETECT, inst, 0);
    g_memEdit = CreateWindowExW(0, L"EDIT", L"",
                                WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER,
                                0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_MEM_EDIT, inst, 0);
    if (g_memEdit) {
        WCHAR memBuf[32];
        set_window_theme_none(g_memEdit);
        wsprintfW(memBuf, L"%d", g_settings.memoryMB > 0 ? g_settings.memoryMB : 4096);
        SetWindowTextW(g_memEdit, memBuf);
        SendMessageW(g_memEdit, EM_SETLIMITTEXT, 6, 0);
        SendMessageW(g_memEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(6, 6));
    }
    if (g_list) {
        SendMessageW(g_list, LB_SETITEMHEIGHT, 0, (LPARAM)(int)(22 * g_scale + 0.5f));
    }
    populate_version_list();
    if (g_list) set_control_theme(g_list);
    SetTimer(hwnd, 2, 40, 0);
}

static void apply_control_fonts(void) {
    HWND controls[7];
    int i;
    controls[0] = g_list;
    controls[1] = g_settingsBtn;
    controls[2] = g_logoutBtn;
    controls[3] = g_launchBtn;
    controls[4] = g_openBtn;
    controls[5] = g_detectBtn;
    controls[6] = g_memEdit;
    for (i = 0; i < 7; i++) {
        if (controls[i]) SendMessageW(controls[i], WM_SETFONT, (WPARAM)g_font20, TRUE);
    }
    if (g_list) SendMessageW(g_list, LB_SETITEMHEIGHT, 0, (LPARAM)(int)(22 * g_scale + 0.5f));
}

static void layout_controls(void) {
    Layout l = make_layout();
    int x, y, w, h;
    if (!g_list) return;
    x = (int)(l.items[0].x * g_scale + 0.5f);
    y = (int)(l.items[0].y * g_scale + 0.5f);
    w = (int)(l.items[0].w * g_scale + 0.5f);
    h = (int)SendMessageW(g_list, LB_GETITEMHEIGHT, 0, 0);
    if (h < 1) h = (int)(22 * g_scale + 0.5f);
    h = h * 4 + 2;
    SetWindowPos(g_list, 0, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_settingsBtn, 0,
                 (int)(l.settingsBtn.x * g_scale + 0.5f), (int)(l.settingsBtn.y * g_scale + 0.5f),
                 (int)(l.settingsBtn.w * g_scale + 0.5f), (int)(l.settingsBtn.h * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_logoutBtn, 0,
                 (int)(l.logoutBtn.x * g_scale + 0.5f), (int)(l.logoutBtn.y * g_scale + 0.5f),
                 (int)(l.logoutBtn.w * g_scale + 0.5f), (int)(l.logoutBtn.h * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_launchBtn, 0,
                 (int)(l.launchBtn.x * g_scale + 0.5f), (int)(l.launchBtn.y * g_scale + 0.5f),
                 (int)(l.launchBtn.w * g_scale + 0.5f), (int)(l.launchBtn.h * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_openBtn, 0,
                 (int)(l.openBtn.x * g_scale + 0.5f), (int)(l.openBtn.y * g_scale + 0.5f),
                 (int)(l.openBtn.w * g_scale + 0.5f), (int)(l.openBtn.h * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_detectBtn, 0,
                 (int)(l.detectBtn.x * g_scale + 0.5f), (int)(l.detectBtn.y * g_scale + 0.5f),
                 (int)(l.detectBtn.w * g_scale + 0.5f), (int)(l.detectBtn.h * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_memEdit, 0,
                 (int)(l.memEdit.x * g_scale + 0.5f), (int)(l.memEdit.y * g_scale + 0.5f),
                 (int)(l.memEdit.w * g_scale + 0.5f), (int)(l.memEdit.h * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

static void sync_page_controls(void) {
    if (g_launchBtn) {
        const WCHAR *label = L"Launch";
        if (g_page == 1) label = L"Done";
        else if (g_launchState == LAUNCH_STARTING) label = L"Waiting for the game";
        else if (g_launchState == LAUNCH_RUNNING)
            label = (g_hoverButton == g_launchBtn) ? L"Shutdown" : L"Running";
        else if (g_launchState == LAUNCH_FAILED) label = L"Retry";
        SetWindowTextW(g_launchBtn, label);
        EnableWindow(g_launchBtn, g_page == 1 || g_launchState == LAUNCH_RUNNING ||
                                 (g_launchState != LAUNCH_STARTING && !g_launching &&
                                  g_versions.count > 0));
    }
    if (g_settingsBtn) EnableWindow(g_settingsBtn, !g_launching);
    if (g_logoutBtn) EnableWindow(g_logoutBtn, !g_launching);
    if (g_openBtn) {
        ShowWindow(g_openBtn, g_page == 1 ? SW_SHOW : SW_HIDE);
        EnableWindow(g_openBtn, !g_launching);
    }
    if (g_detectBtn) {
        ShowWindow(g_detectBtn, g_page == 1 ? SW_SHOW : SW_HIDE);
        EnableWindow(g_detectBtn, !g_launching);
    }
    if (g_memEdit) {
        ShowWindow(g_memEdit, g_page == 1 ? SW_SHOW : SW_HIDE);
        EnableWindow(g_memEdit, !g_launching);
    }
    if (g_page == 0 && (g_hoverButton == g_openBtn || g_hoverButton == g_detectBtn)) {
        g_hoverButton = 0;
    }
}

/* ---------- painting ---------- */

static void draw_sidebar(GpGraphics *g, const Layout *l) {
    fill_rect(g, RGB_SEP, 200, 10, 1, l->H - 20);
}

static void draw_release_shapes(GpGraphics *g, const Layout *l) {
    (void)g;
    (void)l;
}

static void draw_settings_shapes(GpGraphics *g, const Layout *l) {
    panel(g, RGB_PANEL, RGB(47, 51, 60), l->card.x, l->card.y, l->card.w, l->card.h, 10, 1);
}

static COLORREF blend_rgb(COLORREF a, COLORREF b, float t) {
    int r = (int)(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * t);
    int g = (int)(GetGValue(a) + (GetGValue(b) - GetGValue(a)) * t);
    int bl = (int)(GetBValue(a) + (GetBValue(b) - GetBValue(a)) * t);
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (bl < 0) bl = 0; if (bl > 255) bl = 255;
    return RGB(r, g, bl);
}

static void draw_launch_bar(GpGraphics *g, const Layout *l) {
    COLORREF base = RGB(53, 49, 77);
    COLORREF bright = RGB(142, 132, 232);
    float x, y, w, h;
    float i;
    if (g_page != 0 || g_launchState == LAUNCH_IDLE) return;
    x = l->launchBtn.x;
    y = l->launchBtn.y - 15.0f;
    w = l->launchBtn.w;
    h = 5.0f;
    if (g_launchState == LAUNCH_RUNNING) {
        static const unsigned char breathTab[32] = {
            127,152,176,198,217,233,245,253,254,253,245,233,217,198,176,152,
            127,102,78,56,37,21,9,1,0,1,9,21,37,56,78,102
        };
        int idx = (int)((GetTickCount() / 100) & 31);
        float breath = 0.18f + 0.78f * ((float)breathTab[idx] / 255.0f);
        base = RGB(24, 62, 43);
        bright = RGB(92, 214, 139);
        for (i = 0; i < w; i += 1.0f) {
            float t = (w > 1.0f) ? (i / (w - 1.0f)) * 2.0f - 1.0f : 0.0f;
            float edge = 1.0f - (t < 0.0f ? -t : t) * 0.55f;
            float k = breath * edge;
            fill_rect(g, blend_rgb(base, bright, k), x + i, y, 1.0f, h);
        }
        return;
    }
    if (g_launchState == LAUNCH_FAILED) {
        base = RGB(74, 28, 31);
        bright = RGB(224, 96, 96);
    }
    {
        int barW = (int)w;
        int head = barW > 0 ? (int)((GetTickCount() / 9) % (DWORD)barW) : 0;
        int radius = barW / 3;
        if (radius < 12) radius = 12;
        fill_rect(g, base, x, y, w, h);
        for (i = 0; i < w; i += 1.0f) {
            int px = (int)i;
            int dist = px > head ? px - head : head - px;
            float k;
            if (dist > barW / 2) dist = barW - dist;
            if (dist >= radius) continue;
            k = 1.0f - (float)dist / (float)radius;
            k = k * k * 0.95f;
            fill_rect(g, blend_rgb(base, bright, k), x + i, y, 1.0f, h);
        }
    }
}
static void draw_shapes(GpGraphics *g, const Layout *l) {
    fill_rect(g, RGB_BG, 0, 0, l->W, l->H);
    draw_sidebar(g, l);
    if (g_page == 0) draw_release_shapes(g, l);
    else draw_settings_shapes(g, l);
    draw_launch_bar(g, l);
}

static void draw_description(HDC dc, const Layout *l, const WCHAR *description) {
    WCHAR buffer[VERSION_DESC_CAP];
    WCHAR *p;
    int y = 30;
    int lines = 0;
    if (!description || !description[0]) {
        draw_text(dc, L"No description available.", l->mainX, y, l->mainW, 18, g_font24,
                  RGB(158, 161, 168), DT_LEFT | DT_TOP | DT_SINGLELINE);
        return;
    }
    scopy(buffer, description, VERSION_DESC_CAP);
    p = buffer;
    while (*p && y < l->H - 44 && lines < 16) {
        WCHAR *end = wcschr(p, L'\n');
        WCHAR *line = p;
        int n;
        if (end) *end = 0;
        n = (int)wcslen(line);
        while (n > 0 && (line[n - 1] == L'\r' || line[n - 1] == L' ')) line[--n] = 0;
        if (line[0]) {
            if (line[0] == L'[' && line[1] && line[2] == L']') {
                WCHAR prefix[4];
                WCHAR *body = line + 3;
                SIZE sz;
                HGDIOBJ oldFont;
                float bodyX;
                prefix[0] = line[0];
                prefix[1] = line[1];
                prefix[2] = line[2];
                prefix[3] = 0;
                while (*body == L' ') body++;
                oldFont = SelectObject(dc, g_font24);
                GetTextExtentPoint32W(dc, prefix, 3, &sz);
                SelectObject(dc, oldFont);
                bodyX = l->mainX + (float)sz.cx / g_scale;
                draw_text(dc, prefix, l->mainX, y, 18, 18, g_font24, RGB(150, 153, 161),
                          DT_LEFT | DT_TOP | DT_SINGLELINE);
                if (*body) {
                    draw_text(dc, body, bodyX, y, l->mainW - (bodyX - l->mainX), 18, g_font24,
                              RGB(196, 198, 204), DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
                }
            } else {
                draw_text(dc, line, l->mainX, y, l->mainW, 18, g_font24,
                          RGB(196, 198, 204), DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
            }
            y += 18;
            lines++;
        }
        if (!end) break;
        p = end + 1;
    }
}

static void draw_texts(HDC dc, const Layout *l) {
    if (g_versions.count == 0) {
        draw_text(dc, g_versionsLoading ? L"Loading..." : L"Failed to load",
                  10, 10, 180, 18, g_font24, RGB(158, 161, 168),
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    if (g_page == 0) {
        if (g_versions.count == 0) {
            if (g_versionsLoading) {
                draw_text(dc, L"Loading versions...", l->mainX, 4, l->mainW, 24, g_font36, RGB_HEADLINE,
                          DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
                draw_text(dc, L"Fetching version data from GitHub...", l->mainX, 30, l->mainW, 18,
                          g_font24, RGB(158, 161, 168), DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
            } else {
                draw_text(dc, L"Unable to load versions", l->mainX, 4, l->mainW, 24, g_font36, RGB_HEADLINE,
                          DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
                draw_text(dc, g_versionsError[0] ? g_versionsError : L"Please check your network connection.",
                          l->mainX, 30, l->mainW, 18, g_font24, RGB(158, 161, 168),
                          DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
            }
            return;
        }
        {
            const VersionEntry *entry = &g_versions.items[g_selected];
            draw_text(dc, entry->title, l->mainX, 4, l->mainW, 24, g_font36, RGB_HEADLINE,
                      DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
            draw_description(dc, l, entry->description);
        }
    } else {
        WCHAR memText[64];
        WCHAR profile[96];
        draw_text(dc, L"Settings", l->mainX, 4, l->mainW, 24, g_font36, RGB_HEADLINE,
                  DT_LEFT | DT_TOP | DT_SINGLELINE);
        draw_text(dc, L"Launcher configuration", l->mainX, 30, l->mainW, 18, g_font24, RGB(158, 161, 168),
                  DT_LEFT | DT_TOP | DT_SINGLELINE);
        draw_text(dc, L"GAME PATH", l->card.x + 8, 66, l->card.w - 88, 12, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        draw_text(dc, g_settings.gameDir, l->card.x + 8, 82, l->card.w - 88, 18, g_font22, RGB(197, 199, 205),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_PATH_ELLIPSIS);

        draw_text(dc, L"JAVA PATH", l->card.x + 8, 106, l->card.w - 88, 12, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        draw_text(dc, g_settings.javaPath[0] ? g_settings.javaPath : L"Select the Java folder",
                  l->card.x + 8, 122, l->card.w - 88, 18, g_font22, RGB(197, 199, 205),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_PATH_ELLIPSIS);

        draw_text(dc, L"MEMORY", l->card.x + 8, 146, l->card.w - 88, 12, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        draw_text(dc, L"MB", l->card.x + 106, 162, 30, 18, g_font22, RGB(197, 199, 205),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        draw_text(dc, L"(1024 - 32768)", l->card.x + 138, 164, 140, 16, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);

        draw_text(dc, L"PROFILE", l->card.x + 8, 186, l->card.w - 88, 12, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        wsprintfW(profile, L"%s  (fixed)", g_settings.username);
        draw_text(dc, profile, l->card.x + 8, 202, l->card.w - 16, 18, g_font22, RGB(197, 199, 205),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        draw_text(dc, g_versionFound ? g_versionId : L"No Minecraft version found",
                  l->card.x + 8, 224, l->card.w - 16, 14, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    }


}


static void paint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    int w, h;
    HDC mem;
    HBITMAP bmp, oldbmp;
    BITMAPINFO bi;
    void *bits = 0;
    GpGraphics *g = 0;
    Layout l;
    GetClientRect(hwnd, &rc);
    w = rc.right - rc.left;
    h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) {
        EndPaint(hwnd, &ps);
        return;
    }
    g_clientW = w;
    g_clientH = h;
    ensure_fonts();
    l = make_layout();
    mem = CreateCompatibleDC(hdc);
    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bmp = CreateDIBSection(mem, &bi, DIB_RGB_COLORS, &bits, 0, 0);
    if (!bmp) {
        DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return;
    }
    oldbmp = (HBITMAP)SelectObject(mem, bmp);
    GdipCreateFromHDC(mem, &g);
    if (g) {
        GdipSetSmoothingMode(g, GpSmoothingModeAntiAlias);
        GdipSetPixelOffsetMode(g, GpPixelOffsetModeHalf);
        GdipSetCompositingQuality(g, GpCompositingQualityHighQuality);
    }
    if (g) draw_shapes(g, &l);
    if (g) {
        GdipDeleteGraphics(g);
        g = 0;
    }
    draw_texts(mem, &l);
    BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldbmp);
    DeleteObject(bmp);
    DeleteDC(mem);
    EndPaint(hwnd, &ps);
}

/* ---------- app actions ---------- */

static void apply_initial_size(HWND hwnd, int dpi) {
    RECT work;
    RECT frame;
    int cw, ch, sw, sh, x, y;
    float ds, fitX, fitY, s;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    ds = (float)dpi / 96.0f;
    fitX = (float)(work.right - work.left - 60) / DESIGN_W;
    fitY = (float)(work.bottom - work.top - 60) / DESIGN_H;
    s = ds;
    if (fitX < s) s = fitX;
    if (fitY < s) s = fitY;
    if (s < 0.68f) s = 0.68f;
    g_scale = s;
    g_dpi = dpi;
    cw = (int)(DESIGN_W * s + 0.5f);
    ch = (int)(DESIGN_H * s + 0.5f);
    frame.left = 0;
    frame.top = 0;
    frame.right = cw;
    frame.bottom = ch;
    AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);
    sw = frame.right - frame.left;
    sh = frame.bottom - frame.top;
    x = work.left + (work.right - work.left - sw) / 2;
    y = work.top + (work.bottom - work.top - sh) / 2;
    SetWindowPos(hwnd, 0, x, y, sw, sh, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    g_clientW = cw;
    g_clientH = ch;
}


static DWORD WINAPI versions_thread(LPVOID param) {
    VersionList *list = (VersionList *)calloc(1, sizeof(VersionList));
    (void)param;
    if (!list) {
        PostMessageW(g_hwnd, WM_APP_VERSIONS_DONE, 0, 0);
        return 0;
    }
    if (versions_fetch(list)) {
        PostMessageW(g_hwnd, WM_APP_VERSIONS_DONE, 1, (LPARAM)list);
    } else {
        PostMessageW(g_hwnd, WM_APP_VERSIONS_DONE, 0, (LPARAM)list);
    }
    return 0;
}

static DWORD WINAPI launch_thread(LPVOID param) {
    LaunchResult *r = &g_launchResult;
    (void)param;
    r->ok = launch_minecraft(&g_launchRequest, r->message, 512, launch_status,
                             &r->process, &r->pid);
    PostMessageW(g_hwnd, WM_APP_LAUNCH_DONE, (WPARAM)r->ok, 0);
    return 0;
}

static void shutdown_game(void) {
    if (g_gameProcess) {
        TerminateProcess(g_gameProcess, 0);
        WaitForSingleObject(g_gameProcess, 2000);
        CloseHandle(g_gameProcess);
        g_gameProcess = 0;
    }
    g_gamePid = 0;
    g_launching = 0;
    g_launchState = LAUNCH_IDLE;
    g_launchStarted = 0;
    sync_page_controls();
    InvalidateRect(g_hwnd, 0, FALSE);
}

static void start_launch(void) {
    HANDLE th;
    if (g_launchState == LAUNCH_RUNNING) {
        shutdown_game();
        return;
    }
    if (g_launchState == LAUNCH_STARTING || g_launching) return;
    if (g_versions.count <= 0) return;
    g_launching = 1;
    g_launchState = LAUNCH_STARTING;
    g_launchStarted = 0;
    ZeroMemory(&g_launchResult, sizeof(g_launchResult));
    ZeroMemory(&g_launchRequest, sizeof(g_launchRequest));
    g_launchRequest.settings = g_settings;
    remote_version_id(&g_versions.items[g_selected], g_launchRequest.versionId, 128);
    scopy(g_launchRequest.jarUrl, g_versions.items[g_selected].jarUrl, PATH_CAP);
    scopy(g_launchRequest.jsonUrl, g_versions.items[g_selected].jsonUrl, PATH_CAP);
    sync_page_controls();
    th = CreateThread(0, 8 * 1024 * 1024, launch_thread, &g_launchResult, 0, 0);
    if (th) CloseHandle(th);
    else {
        g_launching = 0;
        g_launchState = LAUNCH_FAILED;
        sync_page_controls();
    }
    InvalidateRect(g_hwnd, 0, FALSE);
}

static void browse_game_path(void) {
    BROWSEINFOW bi;
    WCHAR display[MAX_PATH];
    WCHAR path[PATH_CAP];
    WCHAR versionJson[PATH_CAP];
    LPITEMIDLIST pidl;
    ZeroMemory(&bi, sizeof(bi));
    ZeroMemory(display, sizeof(display));
    bi.hwndOwner = g_hwnd;
    bi.pszDisplayName = display;
    bi.lpszTitle = L"Select the Minecraft game folder (.minecraft)";
    bi.ulFlags = 0x00000001; /* BIF_RETURNONLYFSDIRS */
    pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return;
    path[0] = 0;
    if (SHGetPathFromIDListW(pidl, path) && path[0]) {
        scopy(g_settings.gameDir, path, PATH_CAP);
        settings_save(&g_settings);
        g_versionFound = detect_version(&g_settings, g_versionId, 128, versionJson, PATH_CAP);
        InvalidateRect(g_hwnd, 0, FALSE);
    }
    CoTaskMemFree(pidl);
}

static void browse_java(void) {
    BROWSEINFOW bi;
    WCHAR display[MAX_PATH];
    WCHAR path[PATH_CAP];
    LPITEMIDLIST pidl;
    ZeroMemory(&bi, sizeof(bi));
    ZeroMemory(display, sizeof(display));
    bi.hwndOwner = g_hwnd;
    bi.pszDisplayName = display;
    bi.lpszTitle = L"Select the Java folder (contains bin\\java.exe)";
    bi.ulFlags = 0x00000001; /* BIF_RETURNONLYFSDIRS */
    pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return;
    path[0] = 0;
    if (SHGetPathFromIDListW(pidl, path) && path[0]) {
        int n = (int)wcslen(path);
        if (n > 4 && !_wcsicmp(path + n - 4, L"\\bin")) path[n - 4] = 0;
        scopy(g_settings.javaPath, path, PATH_CAP);
        settings_save(&g_settings);
        InvalidateRect(g_hwnd, 0, FALSE);
    }
    CoTaskMemFree(pidl);
}

static void sync_memory_from_edit(int clamp) {
    WCHAR buf[32];
    int val;
    if (!g_memEdit) return;
    GetWindowTextW(g_memEdit, buf, 32);
    val = _wtoi(buf);
    if (clamp) {
        if (val < 1024) val = 1024;
        if (val > 32768) val = 32768;
        wsprintfW(buf, L"%d", val);
        SetWindowTextW(g_memEdit, buf);
    }
    if (val >= 512 && val <= 65536) {
        g_settings.memoryMB = val;
        settings_save(&g_settings);
    }
}

static void handle_command(int id) {
    if (g_launching && id != IDC_LAUNCH) {
        return;
    }
    switch (id) {
        case IDC_SETTINGS:
            g_page = 1;
            if (g_memEdit) {
                WCHAR memBuf[32];
                wsprintfW(memBuf, L"%d", g_settings.memoryMB > 0 ? g_settings.memoryMB : 4096);
                SetWindowTextW(g_memEdit, memBuf);
            }
            sync_page_controls();
            InvalidateRect(g_hwnd, 0, FALSE);
            break;
        case IDC_LOGOUT:
            scopy(g_settings.username, L"novoline_rat", 64);
            settings_save(&g_settings);
            InvalidateRect(g_hwnd, 0, FALSE);
            break;
        case IDC_LAUNCH:
            sync_memory_from_edit(1);
            if (g_page == 1) {
                g_page = 0;
                sync_page_controls();
                InvalidateRect(g_hwnd, 0, FALSE);
            } else {
                start_launch();
            }
            break;
        case IDC_OPEN:
            browse_game_path();
            break;
        case IDC_DETECT:
            browse_java();
            break;
        default:
            break;
    }
}

/* ---------- window plumbing ---------- */

static void apply_window_look(HWND hwnd) {
    HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    if (dwm) {
        typedef HRESULT (WINAPI *PFN_DwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
        PFN_DwmSetWindowAttribute setAttr =
            (PFN_DwmSetWindowAttribute)GetProcAddress(dwm, "DwmSetWindowAttribute");
        if (setAttr) {
            BOOL dark = TRUE;
            COLORREF caption = RGB_BG;
            COLORREF text = RGB_HEADLINE;
            COLORREF border = RGB_BG;
            setAttr(hwnd, 20, &dark, sizeof(dark));
            /* Windows 11+: make the native caption match the client background.
               Windows 10 has no per-window caption color; it keeps the system accent. */
            setAttr(hwnd, 35, &caption, sizeof(caption));
            setAttr(hwnd, 36, &text, sizeof(text));
            setAttr(hwnd, 34, &border, sizeof(border));
        }
        FreeLibrary(dwm);
    }
}

static void draw_list_item(const DRAWITEMSTRUCT *d) {
    WCHAR label[256];
    COLORREF fill;
    COLORREF text;
    HBRUSH brush;
    RECT r;
    HGDIOBJ oldFont;
    int selected;
    if (!d || d->itemID == (UINT)-1) return;
    label[0] = 0;
    SendMessageW(d->hwndItem, LB_GETTEXT, d->itemID, (LPARAM)label);
    selected = (d->itemState & ODS_SELECTED) != 0;
    fill = selected ? RGB_BUTTON : RGB_BG;
    if ((int)d->itemID == g_listHover) {
        fill = selected ? RGB_BUTTON_H : RGB(40, 44, 52);
    }
    text = (selected || (int)d->itemID == g_listHover) ? RGB(225, 227, 232) : RGB(196, 198, 204);
    brush = CreateSolidBrush(fill);
    FillRect(d->hDC, &d->rcItem, brush);
    DeleteObject(brush);
    r = d->rcItem;
    r.left += (LONG)(6 * g_scale);
    SetBkMode(d->hDC, TRANSPARENT);
    SetTextColor(d->hDC, text);
    oldFont = SelectObject(d->hDC, g_font20);
    DrawTextW(d->hDC, label, -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(d->hDC, oldFont);
}

static void draw_play_icon(HDC dc, int cx, int cy, int size) {
    GpGraphics *graphics = 0;
    GpPen *pen = 0;
    GpPath *path = 0;
    float s = size / 24.0f;
    float x = (float)cx - size / 2.0f;
    float y = (float)cy - size / 2.0f;
    float p0x = x + 7.0f * s, p0y = y + 4.8f * s;
    float p1x = x + 19.0f * s, p1y = y + 12.0f * s;
    float p2x = x + 7.0f * s, p2y = y + 19.2f * s;
    if (GdipCreateFromHDC(dc, &graphics) != 0) return;
    GdipSetSmoothingMode(graphics, GpSmoothingModeAntiAlias);
    if (GdipCreatePen1((ARGB)0xFFD8D2F0, 2.0f * s, GpUnitPixel, &pen) == 0) {
        GdipSetPenLineCap197819(pen, GpLineCapRound, GpLineCapRound, GpLineCapFlat);
        GdipSetPenLineJoin(pen, GpLineJoinRound);
        if (GdipCreatePath(GpFillModeWinding, &path) == 0) {
            GdipAddPathLine(path, p0x, p0y, p1x, p1y);
            GdipAddPathLine(path, p1x, p1y, p2x, p2y);
            GdipAddPathLine(path, p2x, p2y, p0x, p0y);
            GdipClosePathFigure(path);
            GdipDrawPath(graphics, pen, path);
            GdipDeletePath(path);
        }
        GdipDeletePen(pen);
    }
    GdipDeleteGraphics(graphics);
}

static void draw_spinner_icon(HDC dc, int cx, int cy, int size) {
    GpGraphics *graphics = 0;
    GpPen *pen = 0;
    GpPath *path = 0;
    float s = size / 24.0f;
    float radius = 9.0f * s;
    int phase = (int)((GetTickCount() / 40) % 360);
    if (GdipCreateFromHDC(dc, &graphics) != 0) return;
    GdipSetSmoothingMode(graphics, GpSmoothingModeAntiAlias);
    if (GdipCreatePen1((ARGB)0xFFD8D2F0, 2.0f * s, GpUnitPixel, &pen) == 0) {
        GdipSetPenLineCap197819(pen, GpLineCapRound, GpLineCapRound, GpLineCapFlat);
        if (GdipCreatePath(GpFillModeWinding, &path) == 0) {
            GdipAddPathArc(path, (float)cx - radius, (float)cy - radius,
                           radius * 2.0f, radius * 2.0f, (float)(270 + phase), 270.0f);
            GdipDrawPath(graphics, pen, path);
            GdipDeletePath(path);
        }
        GdipDeletePen(pen);
    }
    GdipDeleteGraphics(graphics);
}

static void draw_close_icon(HDC dc, int cx, int cy, int size) {
    GpGraphics *graphics = 0;
    GpPen *pen = 0;
    GpPath *path = 0;
    float s = size / 24.0f;
    if (GdipCreateFromHDC(dc, &graphics) != 0) return;
    GdipSetSmoothingMode(graphics, GpSmoothingModeAntiAlias);
    if (GdipCreatePen1((ARGB)0xFFD8D2F0, 2.0f * s, GpUnitPixel, &pen) == 0) {
        GdipSetPenLineCap197819(pen, GpLineCapRound, GpLineCapRound, GpLineCapFlat);
        GdipDrawLine(graphics, pen, (float)cx, (float)cy - 9.0f * s,
                     (float)cx, (float)cy - 1.0f * s);
        if (GdipCreatePath(GpFillModeWinding, &path) == 0) {
            GdipAddPathArc(path, (float)cx - 8.0f * s, (float)cy - 8.0f * s,
                           16.0f * s, 16.0f * s, 315.0f, 270.0f);
            GdipDrawPath(graphics, pen, path);
            GdipDeletePath(path);
        }
        GdipDeletePen(pen);
    }
    GdipDeleteGraphics(graphics);
}

static void draw_owner_button(const DRAWITEMSTRUCT *d) {
    WCHAR label[128];
    COLORREF fill = RGB_BUTTON;
    COLORREF border = RGB_BTN_BORD;
    COLORREF text = RGB(210, 212, 218);
    HBRUSH brush;
    RECT r;
    HGDIOBJ oldFont;
    int hovered;
    if (!d) return;
    hovered = (g_hoverButton == d->hwndItem);
    if (d->itemState & ODS_DISABLED) {
        fill = RGB(43, 47, 56);
        border = RGB(55, 59, 69);
        text = RGB(124, 128, 136);
    } else if (d->itemState & ODS_SELECTED) {
        fill = RGB_BUTTON_P;
        border = RGB(62, 67, 79);
    } else if (d->CtlID == IDC_SETTINGS && g_page == 1) {
        fill = hovered ? RGB(64, 69, 82) : RGB(56, 61, 73);
        if (hovered) border = RGB_BTN_BORD_H;
    } else if (hovered) {
        fill = RGB_BUTTON_H;
        border = RGB_BTN_BORD_H;
        text = RGB(235, 237, 242);
    }
    brush = CreateSolidBrush(fill);
    FillRect(d->hDC, &d->rcItem, brush);
    DeleteObject(brush);
    brush = CreateSolidBrush(border);
    FrameRect(d->hDC, &d->rcItem, brush);
    DeleteObject(brush);
    label[0] = 0;
    GetWindowTextW(d->hwndItem, label, 128);
    r = d->rcItem;
    SetBkMode(d->hDC, TRANSPARENT);
    SetTextColor(d->hDC, text);
    oldFont = SelectObject(d->hDC, g_font20);
    {
        int iconKind = 0;
        if (d->CtlID == IDC_LAUNCH && g_page == 0) {
            if (g_launchState == LAUNCH_IDLE || g_launchState == LAUNCH_FAILED) iconKind = 1;
            else if (g_launchState == LAUNCH_STARTING) iconKind = 2;
            else if (g_launchState == LAUNCH_RUNNING && hovered) iconKind = 3;
        }
        if (iconKind) {
            SIZE textSize;
            int iconSize = (int)(20 * g_scale + 0.5f);
            int gap = (int)(8 * g_scale + 0.5f);
            int total, left, cy;
            RECT tr;
            GetTextExtentPoint32W(d->hDC, label, (int)wcslen(label), &textSize);
            total = iconSize + gap + textSize.cx;
            left = d->rcItem.left + ((d->rcItem.right - d->rcItem.left) - total) / 2;
            cy = (d->rcItem.top + d->rcItem.bottom) / 2;
            tr.left = left + iconSize + gap;
            tr.top = d->rcItem.top;
            tr.right = d->rcItem.right;
            tr.bottom = d->rcItem.bottom;
            DrawTextW(d->hDC, label, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            if (iconKind == 1) draw_play_icon(d->hDC, left + iconSize / 2, cy, iconSize);
            else if (iconKind == 2) draw_spinner_icon(d->hDC, left + iconSize / 2, cy, iconSize);
            else draw_close_icon(d->hDC, left + iconSize / 2, cy, iconSize);
        } else {
            DrawTextW(d->hDC, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
    }
    SelectObject(d->hDC, oldFont);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE:
            g_hwnd = hwnd;
            break;
        case WM_ACTIVATE:
            if (LOWORD(wp) == WA_INACTIVE && g_hoverButton) {
                HWND hovered = g_hoverButton;
                g_hoverButton = 0;
                InvalidateRect(hovered, 0, FALSE);
            }
            break;
        case WM_PAINT:
            paint(hwnd);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_SIZE:
            g_clientW = LOWORD(lp);
            g_clientH = HIWORD(lp);
            layout_controls();
            InvalidateRect(hwnd, 0, FALSE);
            return 0;
        case WM_DPICHANGED: {
            RECT *r = (RECT *)lp;
            g_dpi = HIWORD(wp);
            g_scale = (float)g_dpi / 96.0f;
            SetWindowPos(hwnd, 0, r->left, r->top, r->right - r->left, r->bottom - r->top,
                         SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            ensure_fonts();
            apply_control_fonts();
            layout_controls();
            InvalidateRect(hwnd, 0, FALSE);
            return 0;
        }
        case WM_GETMINMAXINFO: {
            MINMAXINFO *mmi = (MINMAXINFO *)lp;
            RECT r;
            r.left = 0;
            r.top = 0;
            r.right = (LONG)(520 * g_scale);
            r.bottom = (LONG)(360 * g_scale);
            AdjustWindowRectEx(&r, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);
            mmi->ptMinTrackSize.x = r.right - r.left;
            mmi->ptMinTrackSize.y = r.bottom - r.top;
            return 0;
        }
        case WM_DRAWITEM:
            if (((DRAWITEMSTRUCT *)lp)->CtlType == ODT_BUTTON) {
                draw_owner_button((const DRAWITEMSTRUCT *)lp);
                return TRUE;
            }
            if (((DRAWITEMSTRUCT *)lp)->CtlType == ODT_LISTBOX) {
                draw_list_item((const DRAWITEMSTRUCT *)lp);
                return TRUE;
            }
            break;
        case WM_MEASUREITEM: {
            MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lp;
            if (mis->CtlID == IDC_LIST) {
                mis->itemHeight = (UINT)(22 * g_scale + 0.5f);
                return TRUE;
            }
            break;
        }
        case WM_COMMAND: {
            int id = LOWORD(wp);
            int code = HIWORD(wp);
            if (id == IDC_LIST && code == LBN_SELCHANGE) {
                int sel = (int)SendMessageW(g_list, LB_GETCURSEL, 0, 0);
                if (sel >= 0) {
                    g_selected = sel;
                    if (g_page != 0) {
                        g_page = 0;
                        sync_page_controls();
                    }
                    InvalidateRect(hwnd, 0, FALSE);
                }
            } else if (id == IDC_MEM_EDIT) {
                if (code == EN_CHANGE) {
                    sync_memory_from_edit(0);
                } else if (code == EN_KILLFOCUS) {
                    sync_memory_from_edit(1);
                }
            } else if (code == BN_CLICKED) {
                handle_command(id);
            }
            return 0;
        }
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wp;
            SetTextColor(hdc, RGB(225, 227, 232));
            SetBkColor(hdc, RGB(43, 47, 56));
            if (!g_editBrush) g_editBrush = CreateSolidBrush(RGB(43, 47, 56));
            return (LRESULT)g_editBrush;
        }
        case WM_CTLCOLORSTATIC: {
            if ((HWND)lp == g_memEdit) {
                HDC hdc = (HDC)wp;
                SetTextColor(hdc, RGB(124, 128, 136));
                SetBkColor(hdc, RGB(43, 47, 56));
                if (!g_editBrush) g_editBrush = CreateSolidBrush(RGB(43, 47, 56));
                return (LRESULT)g_editBrush;
            }
            break;
        }
        case WM_CTLCOLORLISTBOX: {
            HDC hdc = (HDC)wp;
            SetTextColor(hdc, RGB(198, 200, 205));
            SetBkColor(hdc, RGB_BG);
            if (!g_listBrush) g_listBrush = CreateSolidBrush(RGB_BG);
            return (LRESULT)g_listBrush;
        }
        case WM_SETCURSOR:
            if (LOWORD(lp) == HTCLIENT) {
                SetCursor(LoadCursorW(0, IDC_ARROW));
                return TRUE;
            }
            break;
        case WM_TIMER:
            if (wp == 2) {
                update_hover_states();
                if ((g_launchState == LAUNCH_STARTING || g_launchState == LAUNCH_RUNNING) &&
                    g_gameProcess) {
                    if (WaitForSingleObject(g_gameProcess, 0) == WAIT_OBJECT_0) {
                        DWORD code = 0;
                        GetExitCodeProcess(g_gameProcess, &code);
                        CloseHandle(g_gameProcess);
                        g_gameProcess = 0;
                        g_gamePid = 0;
                        if (g_launchState == LAUNCH_STARTING)
                            g_launchState = LAUNCH_FAILED;
                        else
                            g_launchState = (code != 0) ? LAUNCH_FAILED : LAUNCH_IDLE;
                        sync_page_controls();
                        InvalidateRect(hwnd, 0, FALSE);
                    } else if (g_launchState == LAUNCH_STARTING &&
                               game_window_exists(g_gamePid)) {
                        g_launchState = LAUNCH_RUNNING;
                        sync_page_controls();
                    }
                }
                if (g_launchState != LAUNCH_IDLE) InvalidateRect(hwnd, 0, FALSE);
                if (g_launchState == LAUNCH_STARTING && g_launchBtn)
                    InvalidateRect(g_launchBtn, 0, FALSE);
            }
            return 0;
        case WM_APP_LAUNCH_STATUS:
            if (lp) free((void *)lp);
            return 0;
        case WM_APP_LAUNCH_DONE:
            g_launching = 0;
            if (g_gameProcess) {
                CloseHandle(g_gameProcess);
                g_gameProcess = 0;
            }
            if (g_launchResult.ok && g_launchResult.process) {
                g_gameProcess = g_launchResult.process;
                g_gamePid = g_launchResult.pid;
                g_launchStarted = GetTickCount();
                g_launchState = LAUNCH_STARTING;
                g_launchResult.process = 0;
                settings_save(&g_settings);
            } else {
                if (g_launchResult.process) CloseHandle(g_launchResult.process);
                g_launchResult.process = 0;
                g_launchState = LAUNCH_FAILED;
            }
            sync_page_controls();
            InvalidateRect(hwnd, 0, FALSE);
            return 0;
        case WM_APP_VERSIONS_DONE: {
            VersionList *remote = (VersionList *)lp;
            int ok = (int)wp;
            g_versionsLoading = 0;
            if (ok && remote && remote->count > 0) {
                memcpy(&g_versions, remote, sizeof(VersionList));
                g_selected = 0;
                g_versionsError[0] = 0;
            } else {
                g_versions.count = 0;
                if (remote && remote->error[0]) scopy(g_versionsError, remote->error, 256);
                else scopy(g_versionsError, L"Please check your network connection.", 256);
            }
            populate_version_list();
            sync_page_controls();
            if (remote) free(remote);
            InvalidateRect(hwnd, 0, FALSE);
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            sync_memory_from_edit(1);
            KillTimer(hwnd, 2);
            if (g_listBrush) {
                DeleteObject(g_listBrush);
                g_listBrush = 0;
            }
            if (g_editBrush) {
                DeleteObject(g_editBrush);
                g_editBrush = 0;
            }
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void set_dpi_awareness(void) {
    HMODULE user32 = LoadLibraryW(L"user32.dll");
    if (user32) {
        typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(HANDLE);
        PFN_SetProcessDpiAwarenessContext fn =
            (PFN_SetProcessDpiAwarenessContext)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
        if (fn) {
            if (fn((HANDLE)-4)) { FreeLibrary(user32); return; }
        }
        FreeLibrary(user32);
    }
    SetProcessDPIAware();
}

static int get_window_dpi(HWND hwnd) {
    HMODULE user32 = LoadLibraryW(L"user32.dll");
    if (user32) {
        typedef UINT (WINAPI *PFN_GetDpiForWindow)(HWND);
        PFN_GetDpiForWindow fn = (PFN_GetDpiForWindow)GetProcAddress(user32, "GetDpiForWindow");
        if (fn) {
            UINT d = fn(hwnd);
            FreeLibrary(user32);
            if (d >= 72 && d <= 480) return (int)d;
            return 96;
        }
        FreeLibrary(user32);
    }
    {
        HDC dc = GetDC(0);
        int dpi = GetDeviceCaps(dc, LOGPIXELSX);
        ReleaseDC(0, dc);
        if (dpi >= 72 && dpi <= 480) return dpi;
    }
    return 96;
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmdline, int show) {
    WNDCLASSEXW wc;
    MSG msg;
    GdiplusStartupInput si;
    int dpi;
    (void)hp;
    (void)cmdline;
    set_dpi_awareness();
    ZeroMemory(&si, sizeof(si));
    si.GdiplusVersion = 1;
    if (GdiplusStartup(&g_gdiplusToken, &si, 0) != 0) return 0;
    settings_load(&g_settings);
    g_selected = 0;
    g_appIcon = load_app_icon(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    g_appIconSm = load_app_icon(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    if (!g_appIcon) g_appIcon = CopyIcon(LoadIconW(0, IDI_APPLICATION));
    if (!g_appIconSm) g_appIconSm = g_appIcon;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.hIcon = g_appIcon;
    wc.hIconSm = g_appIconSm;
    wc.hCursor = LoadCursorW(0, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB_BG);
    wc.lpszClassName = APP_CLASS;
    if (!RegisterClassExW(&wc)) return 0;
    g_hwnd = CreateWindowExW(WS_EX_APPWINDOW, APP_CLASS, APP_TITLE,
                             WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                             CW_USEDEFAULT, CW_USEDEFAULT, 1200, 800, 0, 0, hi, 0);
    if (!g_hwnd) return 0;
    SetWindowTextW(g_hwnd, APP_TITLE);
    SendMessageW(g_hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_appIcon);
    SendMessageW(g_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_appIconSm);
    dpi = get_window_dpi(g_hwnd);
    g_scale = (float)dpi / 96.0f;
    apply_initial_size(g_hwnd, dpi);
    ensure_fonts();
    create_controls(g_hwnd);
    apply_control_fonts();
    layout_controls();
    sync_page_controls();
    apply_window_look(g_hwnd);
    {
        WCHAR versionJson[PATH_CAP];
        g_versionFound = detect_version(&g_settings, g_versionId, 128, versionJson, PATH_CAP);
    }
    g_selected = 0;
    ShowWindow(g_hwnd, show);
    UpdateWindow(g_hwnd);
    apply_window_look(g_hwnd);
    {
        HANDLE th = CreateThread(0, 0, versions_thread, 0, 0, 0);
        if (th) CloseHandle(th);
        else {
            g_versionsLoading = 0;
            scopy(g_versionsError, L"Could not start version fetch thread.", 256);
            populate_version_list();
            sync_page_controls();
            InvalidateRect(g_hwnd, 0, FALSE);
        }
    }
    while (GetMessageW(&msg, 0, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    destroy_fonts();
    if (g_appIconSm && g_appIconSm != g_appIcon) DestroyIcon(g_appIconSm);
    if (g_appIcon) DestroyIcon(g_appIcon);
    GdiplusShutdown(g_gdiplusToken);
    return 0;
}
