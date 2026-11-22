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
#include <stdint.h>
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
#define APP_TITLE L"Novoline.Rat Launcher"
#define IDC_LIST       1001
#define IDC_SETTINGS   1002
#define IDC_LAUNCH     1004
#define IDC_OPEN       1005
#define IDC_DETECT     1006
#define IDC_MEM_EDIT   1007
#define IDC_RESET_GAME 1009
#define IDC_RESET_JAVA 1010
#define IDC_SETTINGS_TAB 1011
#define IDC_GAME_LABEL 1012
#define IDC_GAME_VALUE 1013
#define IDC_JAVA_LABEL 1014
#define IDC_JAVA_VALUE 1015
#define IDC_MEM_LABEL 1016
#define IDC_MEM_MB 1017
#define IDC_MEM_RANGE 1018
#define EM_SETCUEBANNER_LOCAL 0x1501
typedef struct { NMHDR hdr; DWORD dwDrawStage; HDC hdc; RECT rc; DWORD dwItemSpec; UINT uItemState; LPARAM lItemlParam; COLORREF clrText; } NMCUSTOMDRAW_LOCAL;
#define CDDS_PREPAINT_LOCAL 0x00000001
#define CDRF_DODEFAULT_LOCAL 0x00000000
#define TCIF_TEXT_LOCAL 0x0001
#define TCM_GETCURSEL_LOCAL 0x130B
#define TCM_SETCURSEL_LOCAL 0x130C
#define TCM_SETITEMSIZE_LOCAL 0x1329
#define TCM_INSERTITEMW_LOCAL 0x133E
#define TCM_GETITEMRECT_LOCAL 0x130A
#define TCM_GETITEMCOUNT_LOCAL 0x1304
#define TCM_HITTEST_LOCAL 0x130D
#define NM_CUSTOMDRAW_LOCAL (-12)
#define CDDS_PREPAINT_LOCAL 0x00000001
#define CDDS_ITEMPREPAINT_LOCAL 0x00010001
#define CDRF_NOTIFYITEMDRAW_LOCAL 0x00000020
#define CDRF_NEWFONT_LOCAL 0x00000002
#define CDIS_SELECTED_LOCAL 0x0001
#define TCN_SELCHANGE_LOCAL (-551)
#define ODT_TAB_LOCAL 101
#define DESIGN_W 660.0f
#define DESIGN_H 440.0f
#define WM_APP_LAUNCH_DONE (WM_APP + 17)
#define WM_APP_VERSIONS_DONE (WM_APP + 18)
#define WM_APP_LAUNCH_STATUS (WM_APP + 19)
#define WM_APP_TAB_RESET (WM_APP + 20)

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
    TRect launchBtn;
    TRect card;
    TRect openBtn, detectBtn, resetGameBtn, resetJavaBtn, tab;
    TRect memEdit;
    float mainX, mainW;
} Layout;

static HWND g_hwnd;
static ULONG_PTR g_gdiplusToken;
static HWND g_list;
static HWND g_settingsBtn;
static HWND g_launchBtn;
static HWND g_openBtn;
static HWND g_detectBtn;
static HWND g_resetGameBtn;
static HWND g_resetJavaBtn;
static HWND g_tab;
static WNDPROC g_tabOldProc;
static int g_settingsTab = 0;
static int g_tabHover = -1;
static int g_tabTracking = 0;
static HWND g_gameLabel, g_gameValue, g_javaLabel, g_javaValue;
static HWND g_memLabel, g_memMb, g_memRange;
#define MAX_NATIVE_BUTTONS 16
static HWND g_nativeButtons[MAX_NATIVE_BUTTONS];
static WNDPROC g_nativeButtonProcs[MAX_NATIVE_BUTTONS];
static int g_nativeButtonCount;
static HWND g_memEdit;
static HBRUSH g_editBrush;
static HBRUSH g_panelBrush;
static HWND g_hoverButton;
static int g_listHover = -1;
static HBRUSH g_listBrush;
static int g_dpi = 96;
static float g_scale = 1.0f;
static int g_clientW = 0, g_clientH = 0;
static int g_page = 0;
static int g_selected = 0;
static HWND g_descView;
static WCHAR g_descText[VERSION_DESC_CAP];
static int g_descScrollX = 0;
static int g_descScrollY = 0;
static int g_descHoverV = 0;
static int g_descHoverH = 0;
static int g_descDragV = 0;
static int g_descDragH = 0;
static int g_descDragOffset = 0;
static int g_descTracking = 0;
static RECT g_descVThumb;
static RECT g_descHThumb;
static int g_descNeedV = 0;
static int g_descNeedH = 0;
typedef struct DescMetrics {
    int contentW, contentH;
    int viewW, viewH;
    int maxX, maxY;
    int thick, gap;
    int trackVTop, trackVHeight;
    int trackHLeft, trackHWidth;
    RECT vThumb, hThumb;
} DescMetrics;
static DescMetrics g_descMetrics;
#define DESC_CLASS L"NovolineDescriptionView"
static HWND g_verScroll;
#define VER_SCROLL_CLASS L"NovolineVersionScroll"
typedef struct VerMetrics {
    int need;
    int viewH;
    int thick, gap;
    int trackTop, trackHeight;
    int count, visible, top, maxTop;
    RECT thumb;
} VerMetrics;
static VerMetrics g_verMetrics;
static int g_verHover = 0;
static int g_verDrag = 0;
static int g_verDragOffset = 0;
static int g_verTracking = 0;

static LRESULT CALLBACK version_scroll_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static void version_scroll_sync(void);

static LRESULT CALLBACK description_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static void description_set_text(const WCHAR *text);
static void description_update_scrollbars(HWND hwnd);

static int system_panel_radius(void) {
    return 5;
}

static int system_button_radius(void) {
    return 3;
}

static int system_tab_radius(void) {
    return 3;
}

static void apply_control_region(HWND hwnd, int radius) {
    RECT rc;
    HRGN region;
    if (!hwnd) return;
    if (radius <= 0) {
        SetWindowRgn(hwnd, 0, TRUE);
        return;
    }
    GetClientRect(hwnd, &rc);
    region = CreateRoundRectRgn(0, 0, rc.right + 1, rc.bottom + 1,
                                radius * 2, radius * 2);
    if (region) SetWindowRgn(hwnd, region, TRUE);
}

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
    if (!out || cap <= 0) return;
    out[0] = 0;
    if (entry && entry->type[0] && entry->date[0]) {
        int n = 0, i;
        for (i = 0; entry->type[i] && n < cap - 1; i++) {
            WCHAR c = entry->type[i];
            if ((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9'))
                out[n++] = c;
        }
        for (i = 0; entry->date[i] && n < cap - 1; i++) {
            WCHAR c = entry->date[i];
            if ((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9'))
                out[n++] = c;
        }
        out[n] = 0;
    }
    if (!out[0]) {
        if (entry && entry->jarUrl[0]) {
            const WCHAR *name = wcsrchr(entry->jarUrl, L'/');
            int i = 0;
            name = name ? name + 1 : entry->jarUrl;
            while (name[i] && i < cap - 1) { out[i] = name[i]; i++; }
            out[i] = 0;
            if (i > 4 && !wcscmp(out + i - 4, L".jar")) out[i - 4] = 0;
        }
    }
    if (!out[0]) scopy(out, L"Release111526", cap);
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
    l.settingsBtn.y = H - 34.0f;
    l.settingsBtn.w = 180.0f;
    l.settingsBtn.h = 24.0f;
    l.launchBtn.x = 210.0f;
    l.launchBtn.y = H - 34.0f;
    l.launchBtn.w = W - 210.0f - 10.0f;
    l.launchBtn.h = 24.0f;
    l.card.x = 210.0f;
    l.card.y = 58.0f;
    l.card.w = l.mainW;
    l.card.h = 196.0f;
    l.openBtn.x = l.card.x + l.card.w - 70.0f;
    l.openBtn.y = 110.0f;
    l.openBtn.w = 60.0f;
    l.openBtn.h = 24.0f;
    l.detectBtn.x = l.card.x + l.card.w - 70.0f;
    l.detectBtn.y = 148.0f;
    l.detectBtn.w = 60.0f;
    l.detectBtn.h = 24.0f;
    l.resetGameBtn.x = l.card.x + l.card.w - 130.0f;
    l.resetGameBtn.y = 110.0f;
    l.resetGameBtn.w = 54.0f;
    l.resetGameBtn.h = 24.0f;
    l.resetJavaBtn.x = l.card.x + l.card.w - 130.0f;
    l.resetJavaBtn.y = 148.0f;
    l.resetJavaBtn.w = 54.0f;
    l.resetJavaBtn.h = 24.0f;
    l.tab.x = l.card.x;
    l.tab.y = l.card.y + 8.0f;
    l.tab.w = 336.0f;
    l.tab.h = 30.0f;
    l.memEdit.x = l.card.x + 8.0f;
    l.memEdit.y = 120.0f;
    l.memEdit.w = 90.0f;
    l.memEdit.h = 22.0f;
    return l;
}


/* ---------- native controls ---------- */

static void set_control_theme(HWND hwnd) {
    HMODULE ux = LoadLibraryW(L"uxtheme.dll");
    if (ux) {
        typedef HRESULT (WINAPI *PFN_SetWindowTheme)(HWND, LPCWSTR, LPCWSTR);
        typedef BOOL (WINAPI *PFN_AllowDarkModeForWindow)(HWND, BOOL);
        PFN_SetWindowTheme fn = (PFN_SetWindowTheme)GetProcAddress(ux, "SetWindowTheme");
        PFN_AllowDarkModeForWindow allow =
            (PFN_AllowDarkModeForWindow)GetProcAddress(ux, (LPCSTR)133);
        if (allow) allow(hwnd, TRUE);
        if (fn) fn(hwnd, L"DarkMode_Explorer", 0);
        FreeLibrary(ux);
    }
}

static void enable_dark_mode(HWND hwnd) {
    HMODULE ux = LoadLibraryW(L"uxtheme.dll");
    if (ux) {
        typedef int (WINAPI *PFN_SetPreferredAppMode)(int);
        typedef BOOL (WINAPI *PFN_AllowDarkModeForWindow)(HWND, BOOL);
        typedef void (WINAPI *PFN_FlushMenuThemes)(void);
        PFN_SetPreferredAppMode preferred =
            (PFN_SetPreferredAppMode)GetProcAddress(ux, (LPCSTR)135);
        PFN_AllowDarkModeForWindow allow =
            (PFN_AllowDarkModeForWindow)GetProcAddress(ux, (LPCSTR)133);
        PFN_FlushMenuThemes flush = (PFN_FlushMenuThemes)GetProcAddress(ux, (LPCSTR)136);
        if (preferred) preferred(2);
        if (allow) allow(hwnd, TRUE);
        if (flush) flush();
        FreeLibrary(ux);
    }
    {
        HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
        if (dwm) {
            typedef HRESULT (WINAPI *PFN_DwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
            PFN_DwmSetWindowAttribute setAttr =
                (PFN_DwmSetWindowAttribute)GetProcAddress(dwm, "DwmSetWindowAttribute");
            if (setAttr) {
                BOOL dark = TRUE;
                setAttr(hwnd, 20, &dark, sizeof(dark));
            }
            FreeLibrary(dwm);
        }
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

typedef struct NMCustomDrawLocal {
    NMHDR nmcd;
    DWORD dwDrawStage;
    HDC hdc;
    RECT rc;
    DWORD_PTR dwItemSpec;
    UINT uItemState;
    LPARAM lItemlParam;
} NMCustomDrawLocal;

typedef struct NMTBCustomDrawLocal {
    NMCustomDrawLocal nmcd;
    HBRUSH hbrMonoDither;
    HBRUSH hbrLines;
    HPEN hpnLines;
    int nLinesWidth;
    COLORREF clrText;
    COLORREF clrMark;
    COLORREF clrTextHighlight;
    COLORREF clrBtnFace;
    COLORREF clrBtnHighlight;
    COLORREF clrHighlightHotTrack;
    RECT rcText;
    int nStringBkMode;
    int nHLStringBkMode;
    int iListGap;
} NMTBCustomDrawLocal;

typedef struct TabItemWLocal {
    UINT mask;
    DWORD dwState;
    DWORD dwStateMask;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
} TabItemWLocal;

static void init_tab_control_support(void) {
    HMODULE lib = LoadLibraryW(L"comctl32.dll");
    typedef BOOL (WINAPI *PFN_InitCommonControlsEx)(const void *);
    PFN_InitCommonControlsEx init;
    struct { DWORD size; DWORD icc; } icc;
    if (!lib) return;
    init = (PFN_InitCommonControlsEx)GetProcAddress(lib, "InitCommonControlsEx");
    if (!init) return;
    icc.size = sizeof(icc);
    icc.icc = 0x000000FF;
    init(&icc);
}

static void overdraw_tabs(HWND hwnd, HDC dc) {
    static const WCHAR *labels[2] = { L"Paths", L"Game" };
    int count = (int)SendMessageW(hwnd, TCM_GETITEMCOUNT_LOCAL, 0, 0);
    int selected = g_settingsTab;
    RECT client;
    int i;
    GetClientRect(hwnd, &client);
    for (i = 0; i < count; i++) {
        RECT r;
        RECT bottom;
        int hovered = (i == g_tabHover);
        COLORREF fill = (i == selected) ? RGB_PANEL : (hovered ? RGB(55, 60, 72) : RGB_BG);
        COLORREF border = (i == selected) ? RGB(72, 77, 90) :
                          (hovered ? RGB(82, 88, 102) : RGB_BORDER);
        COLORREF text = (i == selected) ? RGB(232, 234, 239) :
                        (hovered ? RGB(214, 217, 224) : RGB(150, 154, 162));
        HBRUSH brush;
        HPEN pen;
        HGDIOBJ oldPen;
        HGDIOBJ oldFont;
        if (!SendMessageW(hwnd, TCM_GETITEMRECT_LOCAL, (WPARAM)i, (LPARAM)&r)) continue;
        r.bottom = client.bottom;
        {
            int radius = (int)(system_tab_radius() * g_scale + 0.5f);
            if (radius > 0) {
                HGDIOBJ oldBrush;
                brush = CreateSolidBrush(fill);
                pen = CreatePen(PS_SOLID, 1, border);
                oldPen = SelectObject(dc, pen);
                oldBrush = SelectObject(dc, brush);
                RoundRect(dc, r.left, r.top, r.right, r.bottom + radius,
                          radius * 2, radius * 2);
                SelectObject(dc, oldBrush);
                SelectObject(dc, oldPen);
                DeleteObject(pen);
                DeleteObject(brush);
                if (i != selected) {
                    pen = CreatePen(PS_SOLID, 1, border);
                    oldPen = SelectObject(dc, pen);
                    MoveToEx(dc, r.left, r.bottom - 1, 0);
                    LineTo(dc, r.right - 1, r.bottom - 1);
                    SelectObject(dc, oldPen);
                    DeleteObject(pen);
                }
            } else {
                brush = CreateSolidBrush(fill);
                FillRect(dc, &r, brush);
                DeleteObject(brush);
                pen = CreatePen(PS_SOLID, 1, border);
                oldPen = SelectObject(dc, pen);
                MoveToEx(dc, r.left, r.top, 0); LineTo(dc, r.right - 1, r.top);
                MoveToEx(dc, r.left, r.top, 0); LineTo(dc, r.left, r.bottom - 1);
                MoveToEx(dc, r.right - 1, r.top, 0); LineTo(dc, r.right - 1, r.bottom - 1);
                if (i != selected) {
                    MoveToEx(dc, r.left, r.bottom - 1, 0);
                    LineTo(dc, r.right - 1, r.bottom - 1);
                }
                SelectObject(dc, oldPen);
                DeleteObject(pen);
            }
        }
        if (i == selected) {
            bottom.left = r.left;
            bottom.top = r.bottom - 1;
            bottom.right = r.right;
            bottom.bottom = r.bottom;
            brush = CreateSolidBrush(RGB_PANEL);
            FillRect(dc, &bottom, brush);
            DeleteObject(brush);
        }
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, text);
        oldFont = SelectObject(dc, g_font20);
        DrawTextW(dc, labels[i], -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(dc, oldFont);
    }
}
static LRESULT CALLBACK settings_tab_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_ERASEBKGND) {
        HDC dc = (HDC)wp;
        RECT r;
        HBRUSH brush;
        GetClientRect(hwnd, &r);
        brush = CreateSolidBrush(RGB_BG);
        FillRect(dc, &r, brush);
        DeleteObject(brush);
        return 1;
    }
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT r;
        HBRUSH brush;
        GetClientRect(hwnd, &r);
        brush = CreateSolidBrush(RGB_BG);
        FillRect(dc, &r, brush);
        DeleteObject(brush);
        overdraw_tabs(hwnd, dc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    if (msg == WM_PRINTCLIENT || msg == WM_PRINT) {
        RECT r;
        HBRUSH brush;
        HDC dc = (HDC)wp;
        GetClientRect(hwnd, &r);
        brush = CreateSolidBrush(RGB_BG);
        FillRect(dc, &r, brush);
        DeleteObject(brush);
        overdraw_tabs(hwnd, dc);
        return 0;
    }
    if (msg == WM_NCPAINT) return 0;
    if (msg == WM_MOUSEMOVE) {
        POINT pt;
        int count = (int)SendMessageW(hwnd, TCM_GETITEMCOUNT_LOCAL, 0, 0);
        int hover = -1;
        int i;
        pt.x = (short)LOWORD(lp);
        pt.y = (short)HIWORD(lp);
        for (i = 0; i < count; i++) {
            RECT r;
            if (SendMessageW(hwnd, TCM_GETITEMRECT_LOCAL, (WPARAM)i, (LPARAM)&r) && PtInRect(&r, pt)) {
                hover = i;
                break;
            }
        }
        if (hover != g_tabHover) {
            g_tabHover = hover;
            InvalidateRect(hwnd, 0, FALSE);
        }
        if (!g_tabTracking) {
            struct { DWORD cbSize; DWORD dwFlags; HWND hwndTrack; DWORD dwHoverTime; } tme;
            tme.cbSize = sizeof(tme);
            tme.dwFlags = 0x00000002; /* TME_LEAVE */
            tme.hwndTrack = hwnd;
            tme.dwHoverTime = 0;
            TrackMouseEvent((void *)&tme);
            g_tabTracking = 1;
        }
        return 0;
    }
    if (msg == 0x02A3) { /* WM_MOUSELEAVE */
        g_tabTracking = 0;
        if (g_tabHover != -1) {
            g_tabHover = -1;
            InvalidateRect(hwnd, 0, FALSE);
        }
        return 0;
    }
    return CallWindowProcW(g_tabOldProc, hwnd, msg, wp, lp);
}

static void add_settings_tabs(void) {
    static const WCHAR *labels[2] = { L"Paths", L"Game" };
    int i;
    if (!g_tab) return;
    for (i = 0; i < 2; i++) {
        TabItemWLocal item;
        ZeroMemory(&item, sizeof(item));
        item.mask = TCIF_TEXT_LOCAL;
        item.pszText = (LPWSTR)labels[i];
        item.cchTextMax = (int)wcslen(labels[i]);
        SendMessageW(g_tab, TCM_INSERTITEMW_LOCAL, (WPARAM)i, (LPARAM)&item);
    }
    SendMessageW(g_tab, TCM_SETCURSEL_LOCAL, 0, 0);
}

static void draw_tab_item(const DRAWITEMSTRUCT *d) {
    static const WCHAR *labels[2] = { L"Paths", L"Game" };
    RECT r;
    COLORREF fill;
    COLORREF border;
    COLORREF text;
    HBRUSH brush;
    HPEN pen;
    HGDIOBJ oldPen;
    HGDIOBJ oldFont;
    int selected;
    int left, top, right, bottom;
    if (!d || d->itemID >= 3) return;
    selected = (d->itemState & ODS_SELECTED) != 0;
    fill = selected ? RGB_PANEL : RGB(30, 33, 38);
    border = selected ? RGB(58, 62, 74) : RGB_BORDER;
    text = selected ? RGB(232, 234, 239) : RGB(150, 154, 162);
    r = d->rcItem;
    brush = CreateSolidBrush(fill);
    FillRect(d->hDC, &r, brush);
    DeleteObject(brush);
    left = r.left;
    top = r.top;
    right = r.right - 1;
    bottom = r.bottom - 1;
    pen = CreatePen(PS_SOLID, 1, border);
    oldPen = SelectObject(d->hDC, pen);
    MoveToEx(d->hDC, left, top, 0);
    LineTo(d->hDC, right, top);
    MoveToEx(d->hDC, left, top, 0);
    LineTo(d->hDC, left, bottom);
    MoveToEx(d->hDC, right, top, 0);
    LineTo(d->hDC, right, bottom);
    if (!selected) {
        MoveToEx(d->hDC, left, bottom, 0);
        LineTo(d->hDC, right, bottom);
    }
    SelectObject(d->hDC, oldPen);
    DeleteObject(pen);
    r.left += (LONG)(4 * g_scale);
    r.right -= (LONG)(4 * g_scale);
    SetBkMode(d->hDC, TRANSPARENT);
    SetTextColor(d->hDC, text);
    oldFont = SelectObject(d->hDC, g_font20);
    DrawTextW(d->hDC, labels[d->itemID], -1, &r,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(d->hDC, oldFont);
}
static int is_owner_button(HWND hwnd) {
    return hwnd && (hwnd == g_settingsBtn || hwnd == g_launchBtn ||
                    hwnd == g_openBtn || hwnd == g_detectBtn ||
                    hwnd == g_resetGameBtn || hwnd == g_resetJavaBtn);
}

static void update_hover_states(void) {
    version_scroll_sync();
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
    {
        int tabHover = -1;
        if (g_page == 1 && g_tab && IsWindowVisible(g_tab)) {
            POINT cp = pt;
            struct { POINT pt; UINT flags; } hit;
            ScreenToClient(g_tab, &cp);
            hit.pt = cp;
            hit.flags = 0;
            tabHover = (int)SendMessageW(g_tab, TCM_HITTEST_LOCAL, 0, (LPARAM)&hit);
        }
        if (tabHover != g_tabHover) {
            g_tabHover = tabHover;
            if (g_tab) InvalidateRect(g_tab, 0, FALSE);
        }
    }
    if (target == g_hoverButton) return;
    old = g_hoverButton;
    g_hoverButton = target;
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
    ShowScrollBar(g_list, SB_VERT, FALSE);
    version_scroll_sync();
    if (g_descView)
        description_set_text(g_versions.count > 0 ? g_versions.items[g_selected].description : L"");
}

static LRESULT CALLBACK native_button_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static void install_native_button(HWND hwnd);

static void create_controls(HWND hwnd) {
    HINSTANCE inst = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);
    if (!g_listBrush) g_listBrush = CreateSolidBrush(RGB_BG);
    g_list = CreateWindowExW(0, L"LISTBOX", 0,
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT |
                                 LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
                             0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_LIST, inst, 0);
    if (g_list) ShowScrollBar(g_list, SB_VERT, FALSE);
    g_verScroll = CreateWindowExW(0, VER_SCROLL_CLASS, L"",
                                  WS_CHILD | WS_CLIPSIBLINGS,
                                  0, 0, 0, 0, hwnd, 0, inst, 0);
    g_settingsBtn = CreateWindowExW(0, L"BUTTON", L"Settings",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                    0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_SETTINGS, inst, 0);
    g_launchBtn = CreateWindowExW(0, L"BUTTON", L"Launch",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                  0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_LAUNCH, inst, 0);
    g_openBtn = CreateWindowExW(0, L"BUTTON", L"Browse",
                                WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_OPEN, inst, 0);
    g_detectBtn = CreateWindowExW(0, L"BUTTON", L"Browse",
                                  WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                  0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_DETECT, inst, 0);
    g_resetGameBtn = CreateWindowExW(0, L"BUTTON", L"Reset",
                                     WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                     0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_RESET_GAME, inst, 0);
    g_resetJavaBtn = CreateWindowExW(0, L"BUTTON", L"Reset",
                                     WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                     0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_RESET_JAVA, inst, 0);
    g_tab = CreateWindowExW(0, L"SysTabControl32", L"",
                            WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS |
                                0x8000 /* TCS_FOCUSNEVER */,
                            0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_SETTINGS_TAB, inst, 0);
    if (g_tab) {
        set_window_theme_none(g_tab);
        g_tabOldProc = (WNDPROC)SetWindowLongPtrW(g_tab, GWLP_WNDPROC, (LONG_PTR)settings_tab_proc);
        add_settings_tabs();
    }
    g_memEdit = CreateWindowExW(0, L"EDIT", L"",
                                WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER,
                                0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_MEM_EDIT, inst, 0);
    g_gameLabel = CreateWindowExW(0, L"STATIC", L"GAME PATH",
                                  WS_CHILD | WS_CLIPSIBLINGS | 0x00004000 | 0x00000080,
                                  0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_GAME_LABEL, inst, 0);
    g_gameValue = CreateWindowExW(0, L"STATIC", L"",
                                  WS_CHILD | WS_CLIPSIBLINGS | 0x00004000 | 0x00000080,
                                  0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_GAME_VALUE, inst, 0);
    g_javaLabel = CreateWindowExW(0, L"STATIC", L"JAVA PATH",
                                  WS_CHILD | WS_CLIPSIBLINGS | 0x00004000 | 0x00000080,
                                  0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_JAVA_LABEL, inst, 0);
    g_javaValue = CreateWindowExW(0, L"STATIC", L"",
                                  WS_CHILD | WS_CLIPSIBLINGS | 0x00004000 | 0x00000080,
                                  0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_JAVA_VALUE, inst, 0);
    g_memLabel = CreateWindowExW(0, L"STATIC", L"MEMORY",
                                 WS_CHILD | WS_CLIPSIBLINGS | 0x00004000 | 0x00000080,
                                 0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_MEM_LABEL, inst, 0);
    g_memMb = CreateWindowExW(0, L"STATIC", L"MB",
                              WS_CHILD | WS_CLIPSIBLINGS | 0x00004000 | 0x00000080,
                              0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_MEM_MB, inst, 0);
    g_memRange = CreateWindowExW(0, L"STATIC", L"(1024 - 32768)",
                                 WS_CHILD | WS_CLIPSIBLINGS | 0x00004000 | 0x00000080,
                                 0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_MEM_RANGE, inst, 0);
    {
        HWND buttons[] = {
            g_settingsBtn, g_launchBtn, g_openBtn, g_detectBtn,
            g_resetGameBtn, g_resetJavaBtn
        };
        int i;
        for (i = 0; i < (int)(sizeof(buttons) / sizeof(buttons[0])); i++)
            install_native_button(buttons[i]);
    }
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
    g_descView = CreateWindowExW(0, DESC_CLASS, L"",
                                 WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP,
                                 0, 0, 0, 0, hwnd, 0, inst, 0);
    SetTimer(hwnd, 2, 40, 0);
}

static WNDPROC native_button_old_proc(HWND hwnd) {
    int i;
    for (i = 0; i < g_nativeButtonCount; i++) {
        if (g_nativeButtons[i] == hwnd) return g_nativeButtonProcs[i];
    }
    return 0;
}

static void paint_native_button(HWND hwnd, HDC target) {
    RECT rc;
    HDC mem;
    HBITMAP bmp, oldbmp;
    HBRUSH brush;
    HGDIOBJ oldFont;
    WCHAR label[128];
    COLORREF fill = RGB_BUTTON;
    COLORREF border = RGB_BTN_BORD;
    COLORREF textColor = RGB(210, 212, 218);
    int id = GetDlgCtrlID(hwnd);
    int hovered = GetPropW(hwnd, L"NV_BUTTON_HOVER") != 0;
    int pressed = GetCapture() == hwnd && (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
    if (!IsWindowEnabled(hwnd)) {
        fill = RGB(43, 47, 56);
        border = RGB(55, 59, 69);
        textColor = RGB(124, 128, 136);
    } else if (pressed) {
        fill = RGB_BUTTON_P;
        border = RGB(62, 67, 79);
    } else if (id == IDC_SETTINGS && g_page == 1) {
        fill = hovered ? RGB(64, 69, 82) : RGB(56, 61, 73);
        if (hovered) border = RGB_BTN_BORD_H;
    } else if (hovered) {
        fill = RGB_BUTTON_H;
        border = RGB_BTN_BORD_H;
        textColor = RGB(235, 237, 242);
    }
    GetClientRect(hwnd, &rc);
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    mem = CreateCompatibleDC(target);
    bmp = CreateCompatibleBitmap(target, rc.right, rc.bottom);
    if (!mem || !bmp) {
        if (bmp) DeleteObject(bmp);
        if (mem) DeleteDC(mem);
        return;
    }
    oldbmp = SelectObject(mem, bmp);
    {
        int radius = (int)(system_button_radius() * g_scale + 0.5f);
        COLORREF parent = (id == IDC_SETTINGS || id == IDC_LAUNCH)
                              ? RGB_BG : RGB_PANEL;
        HGDIOBJ oldPen, oldBrush;
        HPEN pen;
        brush = CreateSolidBrush(parent);
        FillRect(mem, &rc, brush);
        DeleteObject(brush);
        brush = CreateSolidBrush(fill);
        pen = CreatePen(PS_SOLID, 1, border);
        oldPen = SelectObject(mem, pen);
        oldBrush = SelectObject(mem, brush);
        RoundRect(mem, 0, 0, rc.right, rc.bottom, radius * 2, radius * 2);
        SelectObject(mem, oldBrush);
        SelectObject(mem, oldPen);
        DeleteObject(pen);
        DeleteObject(brush);
    }
    label[0] = 0;
    GetWindowTextW(hwnd, label, 128);
    if (pressed) OffsetRect(&rc, 0, (int)(1 * g_scale));
    SetBkMode(mem, TRANSPARENT);
    SetTextColor(mem, textColor);
    oldFont = SelectObject(mem, g_font20);
    DrawTextW(mem, label, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(mem, oldFont);
    BitBlt(target, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldbmp);
    DeleteObject(bmp);
    DeleteDC(mem);
}

static LRESULT CALLBACK native_button_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    WNDPROC oldProc = native_button_old_proc(hwnd);
    if (msg == WM_ERASEBKGND) return 1;
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        paint_native_button(hwnd, dc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    if (msg == WM_PRINT || msg == WM_PRINTCLIENT) {
        paint_native_button(hwnd, (HDC)wp);
        return 0;
    }
    if (msg == WM_MOUSEMOVE) {
        LRESULT result = oldProc ? CallWindowProcW(oldProc, hwnd, msg, wp, lp) : 0;
        if (!GetPropW(hwnd, L"NV_BUTTON_HOVER")) {
            struct { DWORD cbSize; DWORD dwFlags; HWND hwndTrack; DWORD dwHoverTime; } tme;
            SetPropW(hwnd, L"NV_BUTTON_HOVER", (HANDLE)1);
            tme.cbSize = sizeof(tme);
            tme.dwFlags = 0x00000002;
            tme.hwndTrack = hwnd;
            tme.dwHoverTime = 0;
            TrackMouseEvent((void *)&tme);
            InvalidateRect(hwnd, 0, FALSE);
        }
        return result;
    }
    if (msg == 0x02A3) {
        RemovePropW(hwnd, L"NV_BUTTON_HOVER");
        InvalidateRect(hwnd, 0, FALSE);
    }
    if (msg == WM_ENABLE || msg == WM_SETFOCUS || msg == WM_KILLFOCUS || msg == WM_SETTEXT ||
        msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP) {
        LRESULT result = oldProc ? CallWindowProcW(oldProc, hwnd, msg, wp, lp) : 0;
        InvalidateRect(hwnd, 0, FALSE);
        return result;
    }
    if (msg == WM_DESTROY) RemovePropW(hwnd, L"NV_BUTTON_HOVER");
    return oldProc ? CallWindowProcW(oldProc, hwnd, msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
}

static void install_native_button(HWND hwnd) {
    WNDPROC oldProc;
    if (!hwnd || g_nativeButtonCount >= MAX_NATIVE_BUTTONS) return;
    oldProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)native_button_proc);
    if (!oldProc) return;
    g_nativeButtons[g_nativeButtonCount] = hwnd;
    g_nativeButtonProcs[g_nativeButtonCount] = oldProc;
    g_nativeButtonCount++;
}


static void apply_control_fonts(void) {
    HWND controls[8];
    int i;
    controls[0] = g_list;
    controls[1] = g_settingsBtn;
    controls[2] = g_launchBtn;
    controls[3] = g_openBtn;
    controls[4] = g_detectBtn;
    controls[5] = g_memEdit;
    controls[6] = g_resetGameBtn;
    controls[7] = g_resetJavaBtn;
    for (i = 0; i < 8; i++) {
        if (controls[i]) SendMessageW(controls[i], WM_SETFONT, (WPARAM)g_font20, TRUE);
    }
    if (g_list) SendMessageW(g_list, LB_SETITEMHEIGHT, 0, (LPARAM)(int)(22 * g_scale + 0.5f));
    if (g_gameLabel) SendMessageW(g_gameLabel, WM_SETFONT, (WPARAM)g_font16, TRUE);
    if (g_javaLabel) SendMessageW(g_javaLabel, WM_SETFONT, (WPARAM)g_font16, TRUE);
    if (g_memLabel) SendMessageW(g_memLabel, WM_SETFONT, (WPARAM)g_font16, TRUE);
    if (g_memMb) SendMessageW(g_memMb, WM_SETFONT, (WPARAM)g_font22, TRUE);
    if (g_memRange) SendMessageW(g_memRange, WM_SETFONT, (WPARAM)g_font16, TRUE);
    if (g_gameValue) SendMessageW(g_gameValue, WM_SETFONT, (WPARAM)g_font22, TRUE);
    if (g_javaValue) SendMessageW(g_javaValue, WM_SETFONT, (WPARAM)g_font22, TRUE);
    if (g_tab) SendMessageW(g_tab, WM_SETFONT, (WPARAM)g_font20, TRUE);
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
    h = (int)((l.settingsBtn.y - 10.0f) * g_scale + 0.5f) - y;
    if (h < (int)(22 * g_scale + 0.5f) * 3) h = (int)(22 * g_scale + 0.5f) * 3;
    {
        int barW = (int)(6 * g_scale + 0.5f);
        int gap = (int)(4 * g_scale + 0.5f);
        int listW;
        if (barW < 4) barW = 4;
        if (gap < 2) gap = 2;
        listW = w - barW - gap;
        SetWindowPos(g_list, 0, x, y, listW, h, SWP_NOZORDER | SWP_NOACTIVATE);
        ShowScrollBar(g_list, SB_VERT, FALSE);
        if (g_verScroll)
            SetWindowPos(g_verScroll, 0, x + listW + gap, y, barW, h,
                         SWP_NOZORDER | SWP_NOACTIVATE);
    }
    version_scroll_sync();
    SetWindowPos(g_settingsBtn, 0,
                 (int)(l.settingsBtn.x * g_scale + 0.5f), (int)(l.settingsBtn.y * g_scale + 0.5f),
                 (int)(l.settingsBtn.w * g_scale + 0.5f), (int)(l.settingsBtn.h * g_scale + 0.5f),
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
    SetWindowPos(g_resetGameBtn, 0,
                 (int)(l.resetGameBtn.x * g_scale + 0.5f), (int)(l.resetGameBtn.y * g_scale + 0.5f),
                 (int)(l.resetGameBtn.w * g_scale + 0.5f), (int)(l.resetGameBtn.h * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_resetJavaBtn, 0,
                 (int)(l.resetJavaBtn.x * g_scale + 0.5f), (int)(l.resetJavaBtn.y * g_scale + 0.5f),
                 (int)(l.resetJavaBtn.w * g_scale + 0.5f), (int)(l.resetJavaBtn.h * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_tab, 0,
                 (int)(l.tab.x * g_scale + 0.5f), (int)(l.tab.y * g_scale + 0.5f),
                 (int)(l.tab.w * g_scale + 0.5f), (int)(l.tab.h * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_memEdit, 0,
                 (int)(l.memEdit.x * g_scale + 0.5f), (int)(l.memEdit.y * g_scale + 0.5f),
                 (int)(l.memEdit.w * g_scale + 0.5f), (int)(l.memEdit.h * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_gameLabel, 0,
                 (int)((l.card.x + 8) * g_scale + 0.5f), (int)(104 * g_scale + 0.5f),
                 (int)((l.card.w - 144) * g_scale + 0.5f), (int)(18 * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_gameValue, 0,
                 (int)((l.card.x + 8) * g_scale + 0.5f), (int)(120 * g_scale + 0.5f),
                 (int)((l.card.w - 144) * g_scale + 0.5f), (int)(20 * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_javaLabel, 0,
                 (int)((l.card.x + 8) * g_scale + 0.5f), (int)(140 * g_scale + 0.5f),
                 (int)((l.card.w - 144) * g_scale + 0.5f), (int)(18 * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_javaValue, 0,
                 (int)((l.card.x + 8) * g_scale + 0.5f), (int)(156 * g_scale + 0.5f),
                 (int)((l.card.w - 144) * g_scale + 0.5f), (int)(20 * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_memLabel, 0,
                 (int)((l.card.x + 8) * g_scale + 0.5f), (int)(104 * g_scale + 0.5f),
                 (int)(220 * g_scale + 0.5f), (int)(18 * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_memMb, 0,
                 (int)((l.card.x + 106) * g_scale + 0.5f), (int)(122 * g_scale + 0.5f),
                 (int)(30 * g_scale + 0.5f), (int)(20 * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_memRange, 0,
                 (int)((l.card.x + 138) * g_scale + 0.5f), (int)(124 * g_scale + 0.5f),
                 (int)(140 * g_scale + 0.5f), (int)(18 * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    {
        int editRadius = (int)(system_button_radius() * g_scale + 0.5f);
        apply_control_region(g_memEdit, editRadius);
    }
    if (g_descView) {
        int descX = (int)(l.mainX * g_scale + 0.5f);
        int descY = (int)(30 * g_scale + 0.5f);
        int descW = (int)(l.mainW * g_scale + 0.5f);
        int descH = (int)((l.launchBtn.y - 20.0f) * g_scale + 0.5f) - descY;
        if (descH < 40) descH = 40;
        SetWindowPos(g_descView, 0, descX, descY, descW, descH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

static void invalidate_settings_card(HWND hwnd) {
    Layout l = make_layout();
    RECT r;
    r.left = (LONG)(l.card.x * g_scale);
    r.top = (LONG)(l.card.y * g_scale);
    r.right = (LONG)((l.card.x + l.card.w) * g_scale) + 1;
    r.bottom = (LONG)((l.card.y + l.card.h) * g_scale) + 1;
    InvalidateRect(hwnd, &r, FALSE);
    if (g_tab) InvalidateRect(g_tab, 0, FALSE);
}

static void sync_page_controls(void) {
    if (g_descView) {
        int showDesc = g_page == 0 && g_versions.count > 0;
        ShowWindow(g_descView, showDesc ? SW_SHOW : SW_HIDE);
        if (showDesc) {
            if (g_selected >= 0 && g_selected < g_versions.count)
                description_set_text(g_versions.items[g_selected].description);
            description_update_scrollbars(g_descView);
            InvalidateRect(g_descView, 0, FALSE);
        }
    }
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
    if (g_tab) {
        ShowWindow(g_tab, g_page == 1 ? SW_SHOW : SW_HIDE);
        EnableWindow(g_tab, !g_launching);
    }
    if (g_openBtn) {
        ShowWindow(g_openBtn, (g_page == 1 && g_settingsTab == 0) ? SW_SHOW : SW_HIDE);
        EnableWindow(g_openBtn, !g_launching);
    }
    if (g_detectBtn) {
        ShowWindow(g_detectBtn, (g_page == 1 && g_settingsTab == 0) ? SW_SHOW : SW_HIDE);
        EnableWindow(g_detectBtn, !g_launching);
    }
    if (g_memEdit) {
        ShowWindow(g_memEdit, (g_page == 1 && g_settingsTab == 1) ? SW_SHOW : SW_HIDE);
        EnableWindow(g_memEdit, !g_launching);
    }
    if (g_gameLabel) ShowWindow(g_gameLabel, (g_page == 1 && g_settingsTab == 0) ? SW_SHOW : SW_HIDE);
    if (g_gameValue) ShowWindow(g_gameValue, (g_page == 1 && g_settingsTab == 0) ? SW_SHOW : SW_HIDE);
    if (g_javaLabel) ShowWindow(g_javaLabel, (g_page == 1 && g_settingsTab == 0) ? SW_SHOW : SW_HIDE);
    if (g_javaValue) ShowWindow(g_javaValue, (g_page == 1 && g_settingsTab == 0) ? SW_SHOW : SW_HIDE);
    if (g_memLabel) ShowWindow(g_memLabel, (g_page == 1 && g_settingsTab == 1) ? SW_SHOW : SW_HIDE);
    if (g_memMb) ShowWindow(g_memMb, (g_page == 1 && g_settingsTab == 1) ? SW_SHOW : SW_HIDE);
    if (g_memRange) ShowWindow(g_memRange, (g_page == 1 && g_settingsTab == 1) ? SW_SHOW : SW_HIDE);
    if (g_resetGameBtn) {
        ShowWindow(g_resetGameBtn, (g_page == 1 && g_settingsTab == 0) ? SW_SHOW : SW_HIDE);
        EnableWindow(g_resetGameBtn, !g_launching);
    }
    if (g_resetJavaBtn) {
        ShowWindow(g_resetJavaBtn, (g_page == 1 && g_settingsTab == 0) ? SW_SHOW : SW_HIDE);
        EnableWindow(g_resetJavaBtn, !g_launching);
    }
    if (g_page == 0 && (g_hoverButton == g_openBtn || g_hoverButton == g_detectBtn ||
                        g_hoverButton == g_resetGameBtn || g_hoverButton == g_resetJavaBtn)) {
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
    panel(g, RGB_PANEL, RGB(47, 51, 60), l->card.x, 96.0f, l->card.w,
          l->card.y + l->card.h - 96.0f, system_panel_radius(), 1);
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
    GpPath *clip;
    GpSolidFill *baseFill = 0;
    float x, y, w, h;
    float i;
    if (g_page != 0 || g_launchState == LAUNCH_IDLE) return;
    x = l->launchBtn.x;
    y = l->launchBtn.y - 15.0f;
    w = l->launchBtn.w;
    h = 5.0f;
    if (g_launchState == LAUNCH_RUNNING) {
        base = RGB(24, 62, 43);
        bright = RGB(92, 214, 139);
    } else if (g_launchState == LAUNCH_FAILED) {
        base = RGB(74, 28, 31);
        bright = RGB(224, 96, 96);
    }
    clip = round_path(x * g_scale, y * g_scale, w * g_scale, h * g_scale,
                      (h * 0.5f) * g_scale);
    if (!clip) return;
    if (GdipCreateSolidFill(gp_color(base, 255), &baseFill) == 0 && baseFill) {
        GdipFillPath(g, (GpBrush *)baseFill, clip);
        GdipDeleteBrush((GpBrush *)baseFill);
    }
    GdipSetClipPath(g, clip, 0);
    if (g_launchState == LAUNCH_RUNNING) {
        static const unsigned char breathTab[32] = {
            127,152,176,198,217,233,245,253,254,253,245,233,217,198,176,152,
            127,102,78,56,37,21,9,1,0,1,9,21,37,56,78,102
        };
        int idx = (int)((GetTickCount() / 100) & 31);
        float breath = 0.18f + 0.78f * ((float)breathTab[idx] / 255.0f);
        for (i = 0; i < w; i += 1.0f) {
            float t = (w > 1.0f) ? (i / (w - 1.0f)) * 2.0f - 1.0f : 0.0f;
            float edge = 1.0f - (t < 0.0f ? -t : t) * 0.55f;
            float k = breath * edge;
            fill_rect(g, blend_rgb(base, bright, k), x + i, y, 1.0f, h);
        }
    } else {
        int barW = (int)w;
        int head = barW > 0 ? (int)(((GetTickCount() * 2) / 9) % (DWORD)barW) : 0;
        int glowRadius = barW / 3;
        if (glowRadius < 12) glowRadius = 12;
        for (i = 0; i < w; i += 1.0f) {
            int px = (int)i;
            int dist = px > head ? px - head : head - px;
            float k;
            if (dist > barW / 2) dist = barW - dist;
            if (dist >= glowRadius) continue;
            k = 1.0f - (float)dist / (float)glowRadius;
            k = k * k * 0.95f;
            fill_rect(g, blend_rgb(base, bright, k), x + i, y, 1.0f, h);
        }
    }
    GdipResetClip(g);
    GdipDeletePath(clip);
}
static void draw_page_dividers(GpGraphics *g, const Layout *l) {
    fill_rect(g, RGB_SEP, l->launchBtn.x, l->launchBtn.y - 20.0f,
              l->launchBtn.w, 1.0f);
}

static void draw_shapes(GpGraphics *g, const Layout *l) {
    fill_rect(g, RGB_BG, 0, 0, l->W, l->H);
    draw_sidebar(g, l);
    if (g_page == 0) draw_release_shapes(g, l);
    else if (g_page == 1) draw_settings_shapes(g, l);
    draw_page_dividers(g, l);
    draw_launch_bar(g, l);
}

static void description_normalize(const WCHAR *description, WCHAR *buffer) {
    WCHAR *src;
    WCHAR *dst = buffer;
    scopy(buffer, description ? description : L"", VERSION_DESC_CAP);
    src = buffer;
    while (*src) {
        if (src[0] == L'\\' && (src[1] == L'n' || src[1] == L'N')) {
            *dst++ = L'\n';
            src += 2;
        } else if (src[0] == L'\\' && (src[1] == L'r' || src[1] == L'R')) {
            src += 2;
        } else if (src[0] == L'\r') {
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = 0;
}

static int description_line_height(void) {
    int height = (int)(18 * g_scale + 0.5f);
    return height > 1 ? height : 1;
}

static int description_padding(void) {
    int pad = (int)(6 * g_scale + 0.5f);
    return pad > 1 ? pad : 1;
}

static void description_measure(HDC dc, int *contentW, int *contentH) {
    WCHAR buffer[VERSION_DESC_CAP];
    WCHAR *p;
    int maxW = 0, lines = 0;
    int lineH = description_line_height();
    HGDIOBJ oldFont;
    description_normalize(g_descText, buffer);
    p = buffer;
    oldFont = SelectObject(dc, g_font24);
    if (!buffer[0]) {
        SIZE sz;
        GetTextExtentPoint32W(dc, L"No description available.", 24, &sz);
        maxW = sz.cx;
        lines = 1;
    } else {
        while (*p) {
            WCHAR *end = wcschr(p, L'\n');
            WCHAR *line = p;
            int n;
            SIZE sz;
            if (end) *end = 0;
            n = (int)wcslen(line);
            while (n > 0 && (line[n - 1] == L'\r' || line[n - 1] == L' ')) line[--n] = 0;
            if (n > 0) {
                GetTextExtentPoint32W(dc, line, n, &sz);
                if (sz.cx > maxW) maxW = sz.cx;
            }
            lines++;
            if (!end) break;
            p = end + 1;
        }
    }
    SelectObject(dc, oldFont);
    *contentW = maxW + description_padding() + 2;
    *contentH = lines * lineH + description_padding() * 2 + 2;
}

static void description_refresh_metrics(HWND hwnd) {
    RECT rc;
    HDC dc;
    DescMetrics *m = &g_descMetrics;
    int minThumb = (int)(24 * g_scale + 0.5f);
    int vTrackBottom, hTrackRight, thumbVH, thumbHW;
    GetClientRect(hwnd, &rc);
    ZeroMemory(m, sizeof(*m));
    m->viewW = rc.right - rc.left;
    m->viewH = rc.bottom - rc.top;
    dc = GetDC(hwnd);
    if (dc) {
        description_measure(dc, &m->contentW, &m->contentH);
        ReleaseDC(hwnd, dc);
    }
    m->thick = (int)(6 * g_scale + 0.5f);
    if (m->thick < 4) m->thick = 4;
    m->gap = (int)(4 * g_scale + 0.5f);
    if (m->gap < 2) m->gap = 2;
    if (minThumb < m->thick * 2) minThumb = m->thick * 2;
    g_descNeedV = m->contentH > m->viewH;
    g_descNeedH = m->contentW > m->viewW;
    m->maxX = m->contentW > m->viewW ? m->contentW - m->viewW : 0;
    m->maxY = m->contentH > m->viewH ? m->contentH - m->viewH : 0;
    if (g_descScrollX > m->maxX) g_descScrollX = m->maxX;
    if (g_descScrollY > m->maxY) g_descScrollY = m->maxY;
    if (g_descScrollX < 0) g_descScrollX = 0;
    if (g_descScrollY < 0) g_descScrollY = 0;

    vTrackBottom = m->viewH - m->gap - (g_descNeedH ? m->thick + m->gap : 0);
    m->trackVTop = m->gap;
    m->trackVHeight = vTrackBottom - m->trackVTop;
    if (m->trackVHeight < 1) m->trackVHeight = 1;
    hTrackRight = m->viewW - m->gap - (g_descNeedV ? m->thick + m->gap : 0);
    m->trackHLeft = m->gap;
    m->trackHWidth = hTrackRight - m->trackHLeft;
    if (m->trackHWidth < 1) m->trackHWidth = 1;

    thumbVH = g_descNeedV ? m->trackVHeight * m->viewH / m->contentH : 0;
    if (thumbVH < minThumb) thumbVH = minThumb;
    if (thumbVH > m->trackVHeight) thumbVH = m->trackVHeight;
    thumbHW = g_descNeedH ? m->trackHWidth * m->viewW / m->contentW : 0;
    if (thumbHW < minThumb) thumbHW = minThumb;
    if (thumbHW > m->trackHWidth) thumbHW = m->trackHWidth;

    m->vThumb.left = m->viewW - m->gap - m->thick;
    m->vThumb.right = m->vThumb.left + m->thick;
    m->vThumb.top = m->trackVTop;
    m->vThumb.bottom = m->trackVTop + thumbVH;
    if (m->maxY > 0 && m->trackVHeight > thumbVH)
        m->vThumb.top += g_descScrollY * (m->trackVHeight - thumbVH) / m->maxY;
    m->vThumb.bottom = m->vThumb.top + thumbVH;

    m->hThumb.top = m->viewH - m->gap - m->thick;
    m->hThumb.bottom = m->hThumb.top + m->thick;
    m->hThumb.left = m->trackHLeft;
    m->hThumb.right = m->trackHLeft + thumbHW;
    if (m->maxX > 0 && m->trackHWidth > thumbHW)
        m->hThumb.left += g_descScrollX * (m->trackHWidth - thumbHW) / m->maxX;
    m->hThumb.right = m->hThumb.left + thumbHW;
}

static void description_update_scrollbars(HWND hwnd) {
    description_refresh_metrics(hwnd);
}

static void description_set_text(const WCHAR *text) {
    if (text && !wcscmp(g_descText, text)) {
        if (g_descView) {
            description_update_scrollbars(g_descView);
            InvalidateRect(g_descView, 0, FALSE);
        }
        return;
    }
    scopy(g_descText, text ? text : L"", VERSION_DESC_CAP);
    g_descScrollX = 0;
    g_descScrollY = 0;
    if (g_descView) {
        description_update_scrollbars(g_descView);
        InvalidateRect(g_descView, 0, FALSE);
    }
}

static void description_scroll_by(HWND hwnd, int dx, int dy) {
    DescMetrics *m;
    description_refresh_metrics(hwnd);
    m = &g_descMetrics;
    g_descScrollX += dx;
    g_descScrollY += dy;
    if (g_descScrollX < 0) g_descScrollX = 0;
    if (g_descScrollY < 0) g_descScrollY = 0;
    if (g_descScrollX > m->maxX) g_descScrollX = m->maxX;
    if (g_descScrollY > m->maxY) g_descScrollY = m->maxY;
    description_refresh_metrics(hwnd);
    InvalidateRect(hwnd, 0, FALSE);
}

static void description_scroll(HWND hwnd, int bar, int code) {
    DescMetrics *m;
    int line = description_line_height();
    description_refresh_metrics(hwnd);
    m = &g_descMetrics;
    if (bar == SB_VERT) {
        int pos = g_descScrollY;
        switch (code) {
            case SB_LINEUP: pos -= line; break;
            case SB_LINEDOWN: pos += line; break;
            case SB_PAGEUP: pos -= m->viewH; break;
            case SB_PAGEDOWN: pos += m->viewH; break;
            case SB_TOP: pos = 0; break;
            case SB_BOTTOM: pos = m->maxY; break;
            default: return;
        }
        description_scroll_by(hwnd, 0, pos - g_descScrollY);
    } else {
        int pos = g_descScrollX;
        switch (code) {
            case SB_LINEUP: pos -= 24; break;
            case SB_LINEDOWN: pos += 24; break;
            case SB_PAGEUP: pos -= m->viewW; break;
            case SB_PAGEDOWN: pos += m->viewW; break;
            case SB_TOP: pos = 0; break;
            case SB_BOTTOM: pos = m->maxX; break;
            default: return;
        }
        description_scroll_by(hwnd, pos - g_descScrollX, 0);
    }
}

static void draw_scroll_thumb(HDC dc, const RECT *r, int pressed, int hovered) {
    GpGraphics *g = 0;
    COLORREF color = pressed ? RGB(198, 202, 212) :
                     (hovered ? RGB(158, 163, 174) : RGB(118, 122, 132));
    float radius = 3.0f;
    if (r->right <= r->left || r->bottom <= r->top) return;
    if (GdipCreateFromHDC(dc, &g) != 0) return;
    GdipSetSmoothingMode(g, GpSmoothingModeAntiAlias);
    panel(g, color, color,
          r->left / g_scale, r->top / g_scale,
          (r->right - r->left) / g_scale, (r->bottom - r->top) / g_scale,
          radius, 0);
    GdipDeleteGraphics(g);
}

static void description_paint(HWND hwnd, HDC target) {
    WCHAR buffer[VERSION_DESC_CAP];
    WCHAR *p;
    RECT rc;
    HDC mem;
    HBITMAP bmp, oldbmp;
    HGDIOBJ oldFont;
    HBRUSH brush;
    int x, y, lineH = description_line_height();
    GetClientRect(hwnd, &rc);
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    description_refresh_metrics(hwnd);
    mem = CreateCompatibleDC(target);
    bmp = CreateCompatibleBitmap(target, rc.right, rc.bottom);
    if (!mem || !bmp) {
        if (bmp) DeleteObject(bmp);
        if (mem) DeleteDC(mem);
        return;
    }
    oldbmp = SelectObject(mem, bmp);
    brush = CreateSolidBrush(RGB_BG);
    FillRect(mem, &rc, brush);
    DeleteObject(brush);
    SetBkMode(mem, TRANSPARENT);
    oldFont = SelectObject(mem, g_font24);
    description_normalize(g_descText, buffer);
    x = -g_descScrollX;
    y = description_padding() - g_descScrollY;
    if (!buffer[0]) {
        SetTextColor(mem, RGB(158, 161, 168));
        TextOutW(mem, x, y, L"No description available.", 24);
    } else {
        p = buffer;
        while (*p) {
            WCHAR *end = wcschr(p, L'\n');
            WCHAR *line = p;
            int n;
            if (end) *end = 0;
            n = (int)wcslen(line);
            while (n > 0 && (line[n - 1] == L'\r' || line[n - 1] == L' ')) line[--n] = 0;
            if (n > 0) {
                if (line[0] == L'[' && line[1] && line[2] == L']') {
                    WCHAR *body = line + 3;
                    SIZE sz;
                    int prefixX = x;
                    GetTextExtentPoint32W(mem, line, 3, &sz);
                    SetTextColor(mem, RGB(150, 153, 161));
                    TextOutW(mem, prefixX, y, line, 3);
                    while (*body == L' ') body++;
                    SetTextColor(mem, RGB(196, 198, 204));
                    if (*body) TextOutW(mem, prefixX + sz.cx, y, body, (int)wcslen(body));
                } else {
                    SetTextColor(mem, RGB(196, 198, 204));
                    TextOutW(mem, x, y, line, n);
                }
            }
            y += lineH;
            if (!end) break;
            p = end + 1;
        }
    }
    SelectObject(mem, oldFont);
    description_refresh_metrics(hwnd);
    if (g_descNeedV)
        draw_scroll_thumb(mem, &g_descMetrics.vThumb, g_descDragV, g_descHoverV);
    if (g_descNeedH)
        draw_scroll_thumb(mem, &g_descMetrics.hThumb, g_descDragH, g_descHoverH);
    BitBlt(target, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldbmp);
    DeleteObject(bmp);
    DeleteDC(mem);
}

static void description_update_hover(HWND hwnd, int x, int y) {
    POINT pt;
    int hoverV = 0, hoverH = 0;
    description_refresh_metrics(hwnd);
    pt.x = x;
    pt.y = y;
    if (g_descNeedV && PtInRect(&g_descMetrics.vThumb, pt)) hoverV = 1;
    if (g_descNeedH && PtInRect(&g_descMetrics.hThumb, pt)) hoverH = 1;
    if (hoverV != g_descHoverV || hoverH != g_descHoverH) {
        g_descHoverV = hoverV;
        g_descHoverH = hoverH;
        InvalidateRect(hwnd, 0, FALSE);
    }
    if (!g_descTracking) {
        struct { DWORD cbSize; DWORD dwFlags; HWND hwndTrack; DWORD dwHoverTime; } tme;
        tme.cbSize = sizeof(tme);
        tme.dwFlags = 0x00000002;
        tme.hwndTrack = hwnd;
        tme.dwHoverTime = 0;
        TrackMouseEvent((void *)&tme);
        g_descTracking = 1;
    }
}

static void description_mouse_down(HWND hwnd, int x, int y) {
    POINT pt;
    DescMetrics *m;
    pt.x = x;
    pt.y = y;
    description_refresh_metrics(hwnd);
    m = &g_descMetrics;
    SetFocus(hwnd);
    if (g_descNeedV && PtInRect(&m->vThumb, pt)) {
        g_descDragV = 1;
        g_descDragOffset = y - m->vThumb.top;
        SetCapture(hwnd);
        InvalidateRect(hwnd, 0, FALSE);
    } else if (g_descNeedH && PtInRect(&m->hThumb, pt)) {
        g_descDragH = 1;
        g_descDragOffset = x - m->hThumb.left;
        SetCapture(hwnd);
        InvalidateRect(hwnd, 0, FALSE);
    } else if (g_descNeedV && x >= m->vThumb.left && x <= m->vThumb.right) {
        int thumbH = m->vThumb.bottom - m->vThumb.top;
        int movable = m->trackVHeight - thumbH;
        int top = y - thumbH / 2;
        if (top < m->trackVTop) top = m->trackVTop;
        if (top > m->trackVTop + movable) top = m->trackVTop + movable;
        g_descScrollY = movable > 0 ? (top - m->trackVTop) * m->maxY / movable : 0;
        g_descDragV = 1;
        g_descDragOffset = thumbH / 2;
        SetCapture(hwnd);
        description_refresh_metrics(hwnd);
        InvalidateRect(hwnd, 0, FALSE);
    } else if (g_descNeedH && y >= m->hThumb.top && y <= m->hThumb.bottom) {
        int thumbW = m->hThumb.right - m->hThumb.left;
        int movable = m->trackHWidth - thumbW;
        int left = x - thumbW / 2;
        if (left < m->trackHLeft) left = m->trackHLeft;
        if (left > m->trackHLeft + movable) left = m->trackHLeft + movable;
        g_descScrollX = movable > 0 ? (left - m->trackHLeft) * m->maxX / movable : 0;
        g_descDragH = 1;
        g_descDragOffset = thumbW / 2;
        SetCapture(hwnd);
        description_refresh_metrics(hwnd);
        InvalidateRect(hwnd, 0, FALSE);
    }
}

static void description_mouse_move(HWND hwnd, int x, int y) {
    DescMetrics *m;
    if (g_descDragV) {
        int thumbH, movable, top;
        description_refresh_metrics(hwnd);
        m = &g_descMetrics;
        thumbH = m->vThumb.bottom - m->vThumb.top;
        movable = m->trackVHeight - thumbH;
        top = y - g_descDragOffset;
        if (top < m->trackVTop) top = m->trackVTop;
        if (top > m->trackVTop + movable) top = m->trackVTop + movable;
        g_descScrollY = movable > 0 ? (top - m->trackVTop) * m->maxY / movable : 0;
        description_refresh_metrics(hwnd);
        InvalidateRect(hwnd, 0, FALSE);
    } else if (g_descDragH) {
        int thumbW, movable, left;
        description_refresh_metrics(hwnd);
        m = &g_descMetrics;
        thumbW = m->hThumb.right - m->hThumb.left;
        movable = m->trackHWidth - thumbW;
        left = x - g_descDragOffset;
        if (left < m->trackHLeft) left = m->trackHLeft;
        if (left > m->trackHLeft + movable) left = m->trackHLeft + movable;
        g_descScrollX = movable > 0 ? (left - m->trackHLeft) * m->maxX / movable : 0;
        description_refresh_metrics(hwnd);
        InvalidateRect(hwnd, 0, FALSE);
    } else {
        description_update_hover(hwnd, x, y);
    }
}

static void description_mouse_up(HWND hwnd) {
    if (g_descDragV || g_descDragH) {
        g_descDragV = 0;
        g_descDragH = 0;
        if (GetCapture() == hwnd) ReleaseCapture();
        InvalidateRect(hwnd, 0, FALSE);
    }
}

static LRESULT CALLBACK description_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            description_paint(hwnd, dc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_PRINT:
        case WM_PRINTCLIENT:
            description_paint(hwnd, (HDC)wp);
            return 0;
        case WM_SIZE:
            description_update_scrollbars(hwnd);
            InvalidateRect(hwnd, 0, FALSE);
            return 0;
        case WM_MOUSEWHEEL: {
            int delta = (short)HIWORD(wp);
            int lines = delta / WHEEL_DELTA;
            description_scroll_by(hwnd, 0, -lines * description_line_height() * 3);
            return 0;
        }
        case WM_MOUSEMOVE:
            description_mouse_move(hwnd, (short)LOWORD(lp), (short)HIWORD(lp));
            return 0;
        case WM_LBUTTONDOWN:
            description_mouse_down(hwnd, (short)LOWORD(lp), (short)HIWORD(lp));
            return 0;
        case WM_LBUTTONUP:
            description_mouse_up(hwnd);
            return 0;
        case 0x02A3: /* WM_MOUSELEAVE */
            g_descTracking = 0;
            if (g_descHoverV || g_descHoverH) {
                g_descHoverV = 0;
                g_descHoverH = 0;
                InvalidateRect(hwnd, 0, FALSE);
            }
            return 0;
        case WM_KEYDOWN: {
            int code = 0;
            switch (wp) {
                case VK_UP: code = SB_LINEUP; break;
                case VK_DOWN: code = SB_LINEDOWN; break;
                case VK_PRIOR: code = SB_PAGEUP; break;
                case VK_NEXT: code = SB_PAGEDOWN; break;
                case VK_HOME: code = SB_TOP; break;
                case VK_END: code = SB_BOTTOM; break;
                case VK_LEFT: description_scroll(hwnd, SB_HORZ, SB_LINEUP); return 0;
                case VK_RIGHT: description_scroll(hwnd, SB_HORZ, SB_LINEDOWN); return 0;
                default: return 0;
            }
            description_scroll(hwnd, SB_VERT, code);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void version_scroll_set_top(int top) {
    int count, visible;
    if (!g_list) return;
    count = (int)SendMessageW(g_list, LB_GETCOUNT, 0, 0);
    visible = g_verMetrics.visible > 0 ? g_verMetrics.visible : 1;
    if (top < 0) top = 0;
    if (top > count - visible) top = count - visible;
    if (top < 0) top = 0;
    SendMessageW(g_list, LB_SETTOPINDEX, (WPARAM)top, 0);
    version_scroll_sync();
}

static void version_scroll_sync(void) {
    RECT rc;
    int itemH, count, top, visible, maxTop, minThumb;
    VerMetrics *m = &g_verMetrics;
    VerMetrics old;
    int wasVisible;
    if (!g_verScroll || !g_list) return;
    old = *m;
    wasVisible = IsWindowVisible(g_verScroll);
    GetClientRect(g_verScroll, &rc);
    itemH = (int)SendMessageW(g_list, LB_GETITEMHEIGHT, 0, 0);
    if (itemH < 1) itemH = (int)(22 * g_scale + 0.5f);
    count = (int)SendMessageW(g_list, LB_GETCOUNT, 0, 0);
    top = (int)SendMessageW(g_list, LB_GETTOPINDEX, 0, 0);
    visible = rc.bottom > 0 ? rc.bottom / itemH : 1;
    if (visible < 1) visible = 1;
    maxTop = count - visible;
    if (maxTop < 0) maxTop = 0;
    ZeroMemory(m, sizeof(*m));
    m->viewH = rc.bottom;
    m->count = count;
    m->visible = visible;
    m->top = top;
    m->maxTop = maxTop;
    m->need = maxTop > 0;
    if (!m->need) {
        if (IsWindowVisible(g_verScroll)) ShowWindow(g_verScroll, SW_HIDE);
        ShowScrollBar(g_list, SB_VERT, FALSE);
        return;
    }
    if (count <= 0) return;
    ShowScrollBar(g_list, SB_VERT, FALSE);
    if (!IsWindowVisible(g_verScroll)) ShowWindow(g_verScroll, SW_SHOW);
    m->thick = rc.right - rc.left;
    m->gap = (int)(4 * g_scale + 0.5f);
    if (m->gap < 2) m->gap = 2;
    m->trackTop = m->gap;
    m->trackHeight = m->viewH - m->gap * 2;
    if (m->trackHeight < 1) m->trackHeight = 1;
    minThumb = (int)(24 * g_scale + 0.5f);
    if (minThumb > m->trackHeight) minThumb = m->trackHeight;
    m->thumb.left = 0;
    m->thumb.right = rc.right;
    m->thumb.top = m->trackTop;
    {
        int height = m->viewH - m->gap * 2;
        int thumbH = height * visible / count;
        if (thumbH < minThumb) thumbH = minThumb;
        if (thumbH > height) thumbH = height;
        if (m->maxTop > 0 && height > thumbH)
            m->thumb.top += top * (height - thumbH) / m->maxTop;
        m->thumb.bottom = m->thumb.top + thumbH;
    }
    if (memcmp(&old, m, sizeof(*m)) != 0 || wasVisible != IsWindowVisible(g_verScroll))
        InvalidateRect(g_verScroll, 0, FALSE);
}

static void version_scroll_paint(HWND hwnd, HDC target) {
    RECT rc;
    HBRUSH brush;
    GetClientRect(hwnd, &rc);
    brush = CreateSolidBrush(RGB_BG);
    FillRect(target, &rc, brush);
    DeleteObject(brush);
    if (g_verMetrics.need)
        draw_scroll_thumb(target, &g_verMetrics.thumb, g_verDrag, g_verHover);
}

static void version_scroll_mouse_down(HWND hwnd, int x, int y) {
    POINT pt;
    VerMetrics *m = &g_verMetrics;
    (void)x;
    version_scroll_sync();
    if (!m->need) return;
    pt.x = x;
    pt.y = y;
    SetFocus(hwnd);
    if (PtInRect(&m->thumb, pt)) {
        g_verDrag = 1;
        g_verDragOffset = y - m->thumb.top;
        SetCapture(hwnd);
        InvalidateRect(hwnd, 0, FALSE);
    } else {
        int thumbH = m->thumb.bottom - m->thumb.top;
        int movable = m->trackHeight - thumbH;
        int top = y - thumbH / 2;
        if (top < m->trackTop) top = m->trackTop;
        if (top > m->trackTop + movable) top = m->trackTop + movable;
        if (movable > 0)
            version_scroll_set_top((top - m->trackTop) * m->maxTop / movable);
        g_verDrag = 1;
        g_verDragOffset = thumbH / 2;
        SetCapture(hwnd);
    }
}

static void version_scroll_mouse_move(HWND hwnd, int x, int y) {
    VerMetrics *m = &g_verMetrics;
    if (g_verDrag) {
        int thumbH, movable, top;
        version_scroll_sync();
        m = &g_verMetrics;
        thumbH = m->thumb.bottom - m->thumb.top;
        movable = m->trackHeight - thumbH;
        top = y - g_verDragOffset;
        if (top < m->trackTop) top = m->trackTop;
        if (top > m->trackTop + movable) top = m->trackTop + movable;
        if (movable > 0)
            version_scroll_set_top((top - m->trackTop) * m->maxTop / movable);
    } else {
        POINT pt;
        int hovered;
        version_scroll_sync();
        m = &g_verMetrics;
        pt.x = x;
        pt.y = y;
        hovered = m->need && PtInRect(&m->thumb, pt);
        if (hovered != g_verHover) {
            g_verHover = hovered;
            InvalidateRect(hwnd, 0, FALSE);
        }
        if (!g_verTracking) {
            struct { DWORD cbSize; DWORD dwFlags; HWND hwndTrack; DWORD dwHoverTime; } tme;
            tme.cbSize = sizeof(tme);
            tme.dwFlags = 0x00000002;
            tme.hwndTrack = hwnd;
            tme.dwHoverTime = 0;
            TrackMouseEvent((void *)&tme);
            g_verTracking = 1;
        }
    }
}

static void version_scroll_mouse_up(HWND hwnd) {
    if (g_verDrag) {
        g_verDrag = 0;
        if (GetCapture() == hwnd) ReleaseCapture();
        InvalidateRect(hwnd, 0, FALSE);
    }
}

static LRESULT CALLBACK version_scroll_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            version_scroll_paint(hwnd, dc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_PRINT:
        case WM_PRINTCLIENT:
            version_scroll_paint(hwnd, (HDC)wp);
            return 0;
        case WM_MOUSEMOVE:
            version_scroll_mouse_move(hwnd, (short)LOWORD(lp), (short)HIWORD(lp));
            return 0;
        case WM_LBUTTONDOWN:
            version_scroll_mouse_down(hwnd, (short)LOWORD(lp), (short)HIWORD(lp));
            return 0;
        case WM_LBUTTONUP:
            version_scroll_mouse_up(hwnd);
            return 0;
        case 0x02A3:
            g_verTracking = 0;
            if (g_verHover) {
                g_verHover = 0;
                InvalidateRect(hwnd, 0, FALSE);
            }
            return 0;
        case WM_MOUSEWHEEL: {
            int lines = (short)HIWORD(wp) / WHEEL_DELTA;
            version_scroll_set_top(g_verMetrics.top - lines * 3);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
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
        }
    } else if (g_page == 1) {
        draw_text(dc, L"Settings", l->mainX, 4, l->mainW, 24, g_font36, RGB_HEADLINE,
                  DT_LEFT | DT_TOP | DT_SINGLELINE);
        draw_text(dc, L"Launcher configuration", l->mainX, 30, l->mainW, 18, g_font24, RGB(158, 161, 168),
                  DT_LEFT | DT_TOP | DT_SINGLELINE);
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

static void refresh_settings_statics(void);

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
        refresh_settings_statics();
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
        refresh_settings_statics();
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

static void refresh_settings_statics(void) {
    if (g_gameValue) SetWindowTextW(g_gameValue, g_settings.gameDir);
    if (g_javaValue)
        SetWindowTextW(g_javaValue, g_settings.javaPath[0] ? g_settings.javaPath : L"Select the Java folder");
}

static void reset_game_path(void) {
    WCHAR versionJson[PATH_CAP];
    scopy(g_settings.gameDir, default_game_dir(), PATH_CAP);
    settings_save(&g_settings);
    g_versionFound = detect_version(&g_settings, g_versionId, 128, versionJson, PATH_CAP);
    refresh_settings_statics();
    InvalidateRect(g_hwnd, 0, FALSE);
}

static void reset_java_path(void) {
    scopy(g_settings.javaPath, default_java_dir(), PATH_CAP);
    settings_save(&g_settings);
    refresh_settings_statics();
    InvalidateRect(g_hwnd, 0, FALSE);
}

static void handle_command(int id) {
    if (g_launching && id != IDC_LAUNCH) {
        return;
    }
    switch (id) {
        case IDC_SETTINGS:
            if (g_page == 1) {
                g_page = 0;
            } else {
                g_page = 1;
                if (g_tab) SendMessageW(g_tab, TCM_SETCURSEL_LOCAL, 0, 0);
                g_settingsTab = 0;
                refresh_settings_statics();
                if (g_memEdit) {
                    WCHAR memBuf[32];
                    wsprintfW(memBuf, L"%d", g_settings.memoryMB > 0 ? g_settings.memoryMB : 4096);
                    SetWindowTextW(g_memEdit, memBuf);
                }
            }
            sync_page_controls();
            InvalidateRect(g_hwnd, 0, FALSE);
            PostMessageW(g_hwnd, WM_APP_TAB_RESET, 0, 0);
            break;
        case IDC_LAUNCH:
            sync_memory_from_edit(1);
            if (g_page != 0) {
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
        case IDC_RESET_GAME:
            reset_game_path();
            break;
        case IDC_RESET_JAVA:
            reset_java_path();
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
    GpSolidFill *fill = 0;
    GpPen *pen = 0;
    GpPointF p[3];
    float height = (float)size;
    float width = height * 0.866f;
    if (GdipCreateFromHDC(dc, &graphics) != 0) return;
    GdipSetSmoothingMode(graphics, GpSmoothingModeAntiAlias);
    p[0].X = (float)cx - width / 2.0f; p[0].Y = (float)cy - height / 2.0f;
    p[1].X = (float)cx - width / 2.0f; p[1].Y = (float)cy + height / 2.0f;
    p[2].X = (float)cx + width / 2.0f; p[2].Y = (float)cy;
    if (GdipCreateSolidFill((ARGB)0xFF3FBF5F, &fill) == 0) {
        GdipFillPolygon(graphics, (GpBrush *)fill, p, 3, GpFillModeWinding);
        if (GdipCreatePen1((ARGB)0xFF1E5E34, 1.4f, GpUnitPixel, &pen) == 0) {
            GdipDrawLine(graphics, pen, p[0].X, p[0].Y, p[1].X, p[1].Y);
            GdipDrawLine(graphics, pen, p[1].X, p[1].Y, p[2].X, p[2].Y);
            GdipDrawLine(graphics, pen, p[2].X, p[2].Y, p[0].X, p[0].Y);
            GdipDeletePen(pen);
        }
        GdipDeleteBrush((GpBrush *)fill);
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
    if (GdipCreatePen1((ARGB)0xFFFF3B30, 2.0f * s, GpUnitPixel, &pen) == 0) {
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
            else if (g_launchState == LAUNCH_RUNNING && hovered) iconKind = 3;
        }
        if (iconKind) {
            SIZE textSize;
            TEXTMETRICW tm;
            int iconSize = (int)(20 * g_scale + 0.5f);
            int gap = (int)(8 * g_scale + 0.5f);
            int iconOffsetY = 0;
            int total, left, cy;
            RECT tr;
            GetTextExtentPoint32W(d->hDC, label, (int)wcslen(label), &textSize);
            if (iconKind == 1 && GetTextMetricsW(d->hDC, &tm)) {
                int capHeight = tm.tmAscent - tm.tmInternalLeading;
                if (capHeight >= 8) iconSize = capHeight;
            } else if (iconKind == 3) {
                iconSize = (iconSize * 4) / 5;
                iconOffsetY = -(int)(1 * g_scale + 0.5f);
            }
            total = iconSize + gap + textSize.cx;
            left = d->rcItem.left + ((d->rcItem.right - d->rcItem.left) - total) / 2;
            cy = (d->rcItem.top + d->rcItem.bottom) / 2;
            tr.left = left + iconSize + gap;
            tr.top = d->rcItem.top;
            tr.right = d->rcItem.right;
            tr.bottom = d->rcItem.bottom;
            DrawTextW(d->hDC, label, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            if (iconKind == 1) draw_play_icon(d->hDC, left + iconSize / 2, cy, iconSize);
            else draw_close_icon(d->hDC, left + iconSize / 2, cy + iconOffsetY, iconSize);
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
            r.right = (LONG)(660 * g_scale);
            r.bottom = (LONG)(440 * g_scale);
            AdjustWindowRectEx(&r, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);
            mmi->ptMinTrackSize.x = r.right - r.left;
            mmi->ptMinTrackSize.y = r.bottom - r.top;
            return 0;
        }
        case WM_DRAWITEM:
            if (((DRAWITEMSTRUCT *)lp)->CtlType == ODT_LISTBOX) {
                draw_list_item((const DRAWITEMSTRUCT *)lp);
                return TRUE;
            }
            if (((DRAWITEMSTRUCT *)lp)->CtlType == ODT_TAB_LOCAL) {
                draw_tab_item((const DRAWITEMSTRUCT *)lp);
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
        case WM_NOTIFY: {
            NMHDR *nh = (NMHDR *)lp;
            if (nh && nh->idFrom == IDC_SETTINGS_TAB && nh->code == TCN_SELCHANGE_LOCAL &&
                g_page == 1) {
                int newTab = (int)SendMessageW(g_tab, TCM_GETCURSEL_LOCAL, 0, 0);
                if (newTab >= 0 && newTab != g_settingsTab) {
                    g_settingsTab = newTab;
                    sync_page_controls();
                    invalidate_settings_card(hwnd);
                }
            }
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wp);
            int code = HIWORD(wp);
            if (id == IDC_LIST && code == LBN_SELCHANGE) {
                int sel = (int)SendMessageW(g_list, LB_GETCURSEL, 0, 0);
                if (sel >= 0) {
                    g_selected = sel;
                    if (g_descView) {
                        ShowWindow(g_descView, g_page == 0 ? SW_SHOW : SW_HIDE);
                        description_set_text(g_versions.items[sel].description);
                        description_update_scrollbars(g_descView);
                        InvalidateRect(g_descView, 0, FALSE);
                    }
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
            HWND ctl = (HWND)lp;
            HDC hdc = (HDC)wp;
            if (ctl == g_memEdit) {
                SetTextColor(hdc, RGB(124, 128, 136));
                SetBkColor(hdc, RGB(43, 47, 56));
                if (!g_editBrush) g_editBrush = CreateSolidBrush(RGB(43, 47, 56));
                return (LRESULT)g_editBrush;
            }
            if (ctl == g_gameLabel || ctl == g_javaLabel || ctl == g_memLabel ||
                ctl == g_memRange) {
                SetTextColor(hdc, RGB(126, 130, 138));
            } else if (ctl == g_gameValue || ctl == g_javaValue || ctl == g_memMb) {
                SetTextColor(hdc, RGB(197, 199, 205));
            } else {
                break;
            }
            SetBkColor(hdc, RGB_PANEL);
            if (!g_panelBrush) g_panelBrush = CreateSolidBrush(RGB_PANEL);
            return (LRESULT)g_panelBrush;
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
            }
            return 0;
        case WM_APP_TAB_RESET:
            if (g_tab) SendMessageW(g_tab, TCM_SETCURSEL_LOCAL, 0, 0);
            g_settingsTab = 0;
            sync_page_controls();
            invalidate_settings_card(hwnd);
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

static int register_description_class(HINSTANCE instance) {
    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = description_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(0, IDC_ARROW);
    wc.lpszClassName = DESC_CLASS;
    return RegisterClassExW(&wc) != 0;
}

static int register_version_scroll_class(HINSTANCE instance) {
    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = version_scroll_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(0, IDC_ARROW);
    wc.lpszClassName = VER_SCROLL_CLASS;
    return RegisterClassExW(&wc) != 0;
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmdline, int show) {
    WNDCLASSEXW wc;
    MSG msg;
    GdiplusStartupInput si;
    int dpi;
    (void)hp;
    (void)cmdline;
    init_tab_control_support();
    register_description_class(hi);
    register_version_scroll_class(hi);
    set_dpi_awareness();
    ZeroMemory(&si, sizeof(si));
    si.GdiplusVersion = 1;
    if (GdiplusStartup(&g_gdiplusToken, &si, 0) != 0) return 0;
    settings_load(&g_settings);
    settings_save(&g_settings);
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
    g_hwnd = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_COMPOSITED, APP_CLASS, APP_TITLE,
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
    refresh_settings_statics();
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
