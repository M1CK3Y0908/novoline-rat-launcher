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
#define IDC_MEM_MINUS  1007
#define IDC_MEM_PLUS   1008
#define DESIGN_W 584.0f
#define DESIGN_H 361.0f
#define WM_APP_LAUNCH_DONE (WM_APP + 17)
#define WM_APP_VERSIONS_DONE (WM_APP + 18)

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
#define RGB_BTN_BORD  RGB(65, 69, 81)
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
    TRect openBtn, detectBtn, memMinus, memPlus;
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
static HWND g_memMinusBtn;
static HWND g_memPlusBtn;
static HWND g_hoverButton;
static int g_listHover = -1;
static HBRUSH g_listBrush;
static int g_dpi = 96;
static float g_scale = 1.0f;
static int g_clientW = 0, g_clientH = 0;
static int g_page = 0;
static int g_selected = 0;
static int g_launching = 0;
static LauncherSettings g_settings;
static WCHAR g_toast[512];
static int g_toastError = 0;
static DWORD g_toastUntil = 0;
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
} LaunchResult;

static LaunchResult g_launchResult;

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
    l.mainX = 214.0f;
    l.mainW = W - l.mainX - 8.0f;
    if (l.mainW < 80) l.mainW = 80;
    for (i = 0; i < 4; i++) {
        l.items[i].x = 8.0f;
        l.items[i].y = 8.0f + i * 22.0f;
        l.items[i].w = 184.0f;
        l.items[i].h = 22.0f;
    }
    l.settingsBtn.x = 8.0f;
    l.settingsBtn.y = H - 66.0f;
    l.settingsBtn.w = 184.0f;
    l.settingsBtn.h = 24.0f;
    l.logoutBtn.x = 8.0f;
    l.logoutBtn.y = H - 36.0f;
    l.logoutBtn.w = 184.0f;
    l.logoutBtn.h = 24.0f;
    l.launchBtn.x = 214.0f;
    l.launchBtn.y = H - 36.0f;
    l.launchBtn.w = W - 214.0f - 8.0f;
    l.launchBtn.h = 24.0f;
    l.card.x = 214.0f;
    l.card.y = 58.0f;
    l.card.w = l.mainW;
    l.card.h = 210.0f;
    l.openBtn.x = l.card.x + l.card.w - 72.0f;
    l.openBtn.y = 76.0f;
    l.openBtn.w = 60.0f;
    l.openBtn.h = 24.0f;
    l.detectBtn.x = l.card.x + l.card.w - 72.0f;
    l.detectBtn.y = 124.0f;
    l.detectBtn.w = 60.0f;
    l.detectBtn.h = 24.0f;
    l.memMinus.x = l.card.x + l.card.w - 60.0f;
    l.memMinus.y = 172.0f;
    l.memMinus.w = 26.0f;
    l.memMinus.h = 24.0f;
    l.memPlus.x = l.card.x + l.card.w - 30.0f;
    l.memPlus.y = 172.0f;
    l.memPlus.w = 26.0f;
    l.memPlus.h = 24.0f;
    return l;
}

static void set_toast(const WCHAR *text, DWORD ms, int error) {
    scopy(g_toast, text ? text : L"", 512);
    g_toastError = error;
    g_toastUntil = GetTickCount() + ms;
    if (g_hwnd) {
        SetTimer(g_hwnd, 1, 250, 0);
        InvalidateRect(g_hwnd, 0, FALSE);
    }
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

static int is_owner_button(HWND hwnd) {
    return hwnd && (hwnd == g_settingsBtn || hwnd == g_logoutBtn || hwnd == g_launchBtn ||
                    hwnd == g_openBtn || hwnd == g_detectBtn ||
                    hwnd == g_memMinusBtn || hwnd == g_memPlusBtn);
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
    g_openBtn = CreateWindowExW(0, L"BUTTON", L"Open",
                                WS_CHILD | WS_TABSTOP | BS_OWNERDRAW,
                                0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_OPEN, inst, 0);
    g_detectBtn = CreateWindowExW(0, L"BUTTON", L"Detect",
                                  WS_CHILD | WS_TABSTOP | BS_OWNERDRAW,
                                  0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_DETECT, inst, 0);
    g_memMinusBtn = CreateWindowExW(0, L"BUTTON", L"-",
                                    WS_CHILD | WS_TABSTOP | BS_OWNERDRAW,
                                    0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_MEM_MINUS, inst, 0);
    g_memPlusBtn = CreateWindowExW(0, L"BUTTON", L"+",
                                   WS_CHILD | WS_TABSTOP | BS_OWNERDRAW,
                                   0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_MEM_PLUS, inst, 0);
    if (g_list) {
        SendMessageW(g_list, LB_SETITEMHEIGHT, 0, (LPARAM)(int)(22 * g_scale + 0.5f));
    }
    populate_version_list();
    if (g_list) set_control_theme(g_list);
    SetTimer(hwnd, 2, 40, 0);
}

static void apply_control_fonts(void) {
    HWND controls[8];
    int i;
    controls[0] = g_list;
    controls[1] = g_settingsBtn;
    controls[2] = g_logoutBtn;
    controls[3] = g_launchBtn;
    controls[4] = g_openBtn;
    controls[5] = g_detectBtn;
    controls[6] = g_memMinusBtn;
    controls[7] = g_memPlusBtn;
    for (i = 0; i < 8; i++) {
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
    SetWindowPos(g_memMinusBtn, 0,
                 (int)(l.memMinus.x * g_scale + 0.5f), (int)(l.memMinus.y * g_scale + 0.5f),
                 (int)(l.memMinus.w * g_scale + 0.5f), (int)(l.memMinus.h * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_memPlusBtn, 0,
                 (int)(l.memPlus.x * g_scale + 0.5f), (int)(l.memPlus.y * g_scale + 0.5f),
                 (int)(l.memPlus.w * g_scale + 0.5f), (int)(l.memPlus.h * g_scale + 0.5f),
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

static void sync_page_controls(void) {
    if (g_launchBtn) {
        SetWindowTextW(g_launchBtn, g_page == 1 ? L"Done" : (g_launching ? L"Launching..." : L"Launch"));
        EnableWindow(g_launchBtn, g_page == 1 || (!g_launching && g_versions.count > 0));
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
    if (g_memMinusBtn) {
        ShowWindow(g_memMinusBtn, g_page == 1 ? SW_SHOW : SW_HIDE);
        EnableWindow(g_memMinusBtn, !g_launching);
    }
    if (g_memPlusBtn) {
        ShowWindow(g_memPlusBtn, g_page == 1 ? SW_SHOW : SW_HIDE);
        EnableWindow(g_memPlusBtn, !g_launching);
    }
    if (g_page == 0 && (g_hoverButton == g_openBtn || g_hoverButton == g_detectBtn ||
                        g_hoverButton == g_memMinusBtn || g_hoverButton == g_memPlusBtn)) {
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

static void draw_toast_shape(GpGraphics *g, const Layout *l) {
    float w = 420.0f;
    float x;
    if (!g_toast[0] || GetTickCount() >= g_toastUntil) return;
    if (w > l->mainW - 8) w = l->mainW - 8;
    x = l->mainX + (l->mainW - w) / 2;
    panel(g, RGB(42, 46, 54), g_toastError ? RGB(100, 58, 58) : RGB(58, 62, 73),
          x, l->H - 76, w, 26, 6, 1);
}

static void draw_shapes(GpGraphics *g, const Layout *l) {
    fill_rect(g, RGB_BG, 0, 0, l->W, l->H);
    draw_sidebar(g, l);
    if (g_page == 0) draw_release_shapes(g, l);
    else draw_settings_shapes(g, l);
    draw_toast_shape(g, l);
}

static void draw_description(HDC dc, const Layout *l, const WCHAR *description) {
    WCHAR buffer[VERSION_DESC_CAP];
    WCHAR *p;
    int y = 50;
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
                  10, 8, 184, 18, g_font24, RGB(158, 161, 168),
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
            draw_text(dc, entry->released[0] ? entry->released : L"Released ... days ago",
                      l->mainX, 30, l->mainW, 18, g_font24, RGB(158, 161, 168),
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
        draw_text(dc, L"GAME DIRECTORY", l->card.x + 8, 68, l->card.w - 88, 12, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        draw_text(dc, g_settings.gameDir, l->card.x + 8, 84, l->card.w - 88, 18, g_font22, RGB(197, 199, 205),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_PATH_ELLIPSIS);

        draw_text(dc, L"JAVA RUNTIME", l->card.x + 8, 116, l->card.w - 88, 12, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        draw_text(dc, g_settings.javaPath[0] ? g_settings.javaPath : L"Auto-detect when launching",
                  l->card.x + 8, 132, l->card.w - 88, 18, g_font22, RGB(197, 199, 205),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_PATH_ELLIPSIS);

        draw_text(dc, L"MEMORY", l->card.x + 8, 164, l->card.w - 88, 12, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        wsprintfW(memText, L"%d MB", g_settings.memoryMB);
        draw_text(dc, memText, l->card.x + 8, 180, 140, 18, g_font22, RGB(197, 199, 205),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);

        draw_text(dc, L"PROFILE", l->card.x + 8, 212, l->card.w - 88, 12, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        wsprintfW(profile, L"%s  (offline)", g_settings.username);
        draw_text(dc, profile, l->card.x + 8, 228, l->card.w - 16, 18, g_font22, RGB(197, 199, 205),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        draw_text(dc, g_versionFound ? g_versionId : L"No Minecraft version found",
                  l->card.x + 8, 248, l->card.w - 16, 14, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        draw_text(dc, L"Settings are saved automatically.", l->mainX, l->card.y + l->card.h + 8,
                  l->mainW, 16, g_font16, RGB(111, 115, 124), DT_LEFT | DT_SINGLELINE);
    }

    if (g_toast[0] && GetTickCount() < g_toastUntil) {
        float w = 420.0f;
        float x;
        if (w > l->mainW - 8) w = l->mainW - 8;
        x = l->mainX + (l->mainW - w) / 2;
        draw_text(dc, g_toast, x + 18, l->H - 76, w - 24, 26, g_font20,
                  g_toastError ? RGB(232, 180, 180) : RGB(198, 200, 206),
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
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
    LaunchResult *r = (LaunchResult *)param;
    r->ok = launch_minecraft(&g_settings, r->message, 512);
    PostMessageW(g_hwnd, WM_APP_LAUNCH_DONE, (WPARAM)r->ok, 0);
    return 0;
}

static void start_launch(void) {
    HANDLE th;
    if (g_launching) return;
    g_launching = 1;
    sync_page_controls();
    ZeroMemory(&g_launchResult, sizeof(g_launchResult));
    set_toast(L"Preparing Minecraft...", 10000, 0);
    th = CreateThread(0, 0, launch_thread, &g_launchResult, 0, 0);
    if (th) CloseHandle(th);
    else {
        g_launching = 0;
        sync_page_controls();
        set_toast(L"Could not start the launcher thread.", 5000, 1);
    }
    InvalidateRect(g_hwnd, 0, FALSE);
}

static void open_game_folder(void) {
    HINSTANCE r;
    if (!dir_exists(g_settings.gameDir)) {
        set_toast(L"The game directory does not exist.", 4500, 1);
        return;
    }
    r = ShellExecuteW(0, L"open", g_settings.gameDir, 0, 0, SW_SHOWNORMAL);
    if ((INT_PTR)r <= 32) set_toast(L"Could not open the game directory.", 4500, 1);
}

static void handle_command(int id) {
    if (g_launching && id != IDC_LAUNCH) {
        set_toast(L"Please wait for Minecraft to start.", 2500, 0);
        return;
    }
    switch (id) {
        case IDC_SETTINGS:
            g_page = 1;
            sync_page_controls();
            InvalidateRect(g_hwnd, 0, FALSE);
            break;
        case IDC_LOGOUT:
            scopy(g_settings.username, L"Player", 64);
            settings_save(&g_settings);
            set_toast(L"Logged out. Offline profile: Player.", 4000, 0);
            InvalidateRect(g_hwnd, 0, FALSE);
            break;
        case IDC_LAUNCH:
            if (g_page == 1) {
                g_page = 0;
                sync_page_controls();
                InvalidateRect(g_hwnd, 0, FALSE);
            } else {
                start_launch();
            }
            break;
        case IDC_OPEN:
            open_game_folder();
            break;
        case IDC_DETECT:
            if (detect_java(&g_settings, 8)) {
                settings_save(&g_settings);
                set_toast(L"Java runtime detected and saved.", 3500, 0);
            } else {
                set_toast(L"Java was not found. Install Java 8 or newer.", 5000, 1);
            }
            InvalidateRect(g_hwnd, 0, FALSE);
            break;
        case IDC_MEM_MINUS:
            if (g_settings.memoryMB > 1024) g_settings.memoryMB -= 512;
            settings_save(&g_settings);
            InvalidateRect(g_hwnd, 0, FALSE);
            break;
        case IDC_MEM_PLUS:
            if (g_settings.memoryMB < 16384) g_settings.memoryMB += 512;
            settings_save(&g_settings);
            InvalidateRect(g_hwnd, 0, FALSE);
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

static void draw_owner_button(const DRAWITEMSTRUCT *d) {
    WCHAR label[128];
    COLORREF fill = RGB_BUTTON;
    COLORREF text = RGB(210, 212, 218);
    HBRUSH brush;
    RECT r;
    HGDIOBJ oldFont;
    int hovered;
    if (!d) return;
    hovered = (g_hoverButton == d->hwndItem);
    if (d->itemState & ODS_DISABLED) {
        fill = RGB(43, 47, 56);
        text = RGB(124, 128, 136);
    } else if (d->itemState & ODS_SELECTED) {
        fill = RGB_BUTTON_P;
    } else if (d->CtlID == IDC_SETTINGS && g_page == 1) {
        fill = hovered ? RGB(64, 69, 82) : RGB(56, 61, 73);
    } else if (hovered) {
        fill = RGB_BUTTON_H;
        text = RGB(235, 237, 242);
    }
    brush = CreateSolidBrush(fill);
    FillRect(d->hDC, &d->rcItem, brush);
    DeleteObject(brush);
    label[0] = 0;
    GetWindowTextW(d->hwndItem, label, 128);
    r = d->rcItem;
    SetBkMode(d->hDC, TRANSPARENT);
    SetTextColor(d->hDC, text);
    oldFont = SelectObject(d->hDC, g_font20);
    DrawTextW(d->hDC, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
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
            } else if (code == BN_CLICKED) {
                handle_command(id);
            }
            return 0;
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
            if (wp == 1) {
                if (GetTickCount() >= g_toastUntil) {
                    g_toast[0] = 0;
                    KillTimer(hwnd, 1);
                    InvalidateRect(hwnd, 0, FALSE);
                }
            } else if (wp == 2) {
                update_hover_states();
            }
            return 0;
        case WM_APP_LAUNCH_DONE:
            g_launching = 0;
            sync_page_controls();
            set_toast(g_launchResult.message, g_launchResult.ok ? 6000 : 7000, g_launchResult.ok ? 0 : 1);
            if (g_launchResult.ok) settings_save(&g_settings);
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
            KillTimer(hwnd, 2);
            if (g_listBrush) {
                DeleteObject(g_listBrush);
                g_listBrush = 0;
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
