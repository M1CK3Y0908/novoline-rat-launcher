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
#define DESIGN_W 1237.0f
#define DESIGN_H 779.0f
#define WM_APP_LAUNCH_DONE (WM_APP + 17)

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

static HWND g_hwnd;static ULONG_PTR g_gdiplusToken;
static int g_dpi = 96;
static float g_scale = 1.0f;
static int g_clientW = 0, g_clientH = 0;
static int g_page = 0;
static int g_selected = 0;
static int g_hot = 0;
static int g_pressed = 0;
static int g_launching = 0;
static int g_tracking = 0;
static LauncherSettings g_settings;
static WCHAR g_toast[512];
static int g_toastError = 0;
static DWORD g_toastUntil = 0;
static WCHAR g_versionId[128];
static int g_versionFound = 0;
static HICON g_appIcon = 0;

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

static HFONT g_font24, g_font24b, g_font23, g_font22, g_font20, g_font18, g_font16, g_font36;
static float g_fontScale = -1.0f;

static const WCHAR *g_items[4] = {
    L"Novoline Release #100421",
    L"Novoline Release #100421",
    L"Novoline Beta #100221",
    L"Novoline Beta #100121"
};

static const WCHAR *g_notes[4][3] = {
    {
        L"Fixed crashes caused by loading scripts",
        L"Fixed connection timeout issue on Hypixel (note: please test this)",
        L"Fixed 'module may not be registered twice' error"
    },
    {
        L"Fixed crashes caused by loading scripts",
        L"Fixed connection timeout issue on Hypixel (note: please test this)",
        L"Fixed 'module may not be registered twice' error"
    },
    {
        L"Added new module manager",
        L"Improved render performance",
        L"Fixed several crashes"
    },
    {
        L"Initial beta release",
        L"Added script loader",
        L"Various stability fixes"
    }
};

typedef enum HitId {
    HIT_NONE = 0,
    HIT_ITEM1 = 10,
    HIT_ITEM2 = 11,
    HIT_ITEM3 = 12,
    HIT_ITEM4 = 13,
    HIT_SETTINGS = 20,
    HIT_LOGOUT = 21,
    HIT_LAUNCH = 22,
    HIT_OPEN = 30,
    HIT_DETECT = 31,
    HIT_MEM_MINUS = 32,
    HIT_MEM_PLUS = 33
} HitId;

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

static void stroke_round(GpGraphics *g, COLORREF color, float x, float y, float w, float h,
                         float r, float width) {
    GpPath *p = round_path(x * g_scale, y * g_scale, w * g_scale, h * g_scale, r * g_scale);
    GpPen *pen = 0;
    if (!p) return;
    GdipCreatePen1(gp_color(color, 255), width * g_scale, GpUnitPixel, &pen);
    if (pen) {
        GdipSetPenLineJoin(pen, GpLineJoinRound);
        GdipDrawPath(g, pen, p);
        GdipDeletePen(pen);
    }
    GdipDeletePath(p);
}

static void draw_line(GpGraphics *g, COLORREF color, float x1, float y1, float x2, float y2, float width) {
    GpPen *pen = 0;
    GdipCreatePen1(gp_color(color, 255), width * g_scale, GpUnitPixel, &pen);
    if (pen) {
        GdipSetPenLineCap197819(pen, GpLineCapRound, GpLineCapRound, GpLineCapRound);
        GdipDrawLine(g, pen, x1 * g_scale, y1 * g_scale, x2 * g_scale, y2 * g_scale);
        GdipDeletePen(pen);
    }
}

static void fill_circle(GpGraphics *g, COLORREF color, float x, float y, float size) {
    GpSolidFill *b = 0;
    GdipCreateSolidFill(gp_color(color, 255), &b);
    if (b) {
        GdipFillEllipse(g, b, x * g_scale, y * g_scale, size * g_scale, size * g_scale);
        GdipDeleteBrush((GpBrush *)b);
    }
}

static void stroke_circle(GpGraphics *g, COLORREF color, float x, float y, float size, float width) {
    GpPen *pen = 0;
    GdipCreatePen1(gp_color(color, 255), width * g_scale, GpUnitPixel, &pen);
    if (pen) {
        GdipDrawEllipse(g, pen, x * g_scale, y * g_scale, size * g_scale, size * g_scale);
        GdipDeletePen(pen);
    }
}

static void fill_poly(GpGraphics *g, COLORREF color, const float *pts, int count) {
    GpPointF p[16];
    GpSolidFill *b = 0;
    int i;
    if (count > 16) count = 16;
    for (i = 0; i < count; i++) {
        p[i].X = pts[i * 2] * g_scale;
        p[i].Y = pts[i * 2 + 1] * g_scale;
    }
    GdipCreateSolidFill(gp_color(color, 255), &b);
    if (b) {
        GdipFillPolygon(g, b, p, count, GpFillModeWinding);
        GdipDeleteBrush((GpBrush *)b);
    }
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
    if (g_font23) DeleteObject(g_font23);
    if (g_font24b) DeleteObject(g_font24b);
    if (g_font22) DeleteObject(g_font22);
    if (g_font20) DeleteObject(g_font20);
    if (g_font18) DeleteObject(g_font18);
    if (g_font16) DeleteObject(g_font16);
    if (g_font36) DeleteObject(g_font36);
    g_font24 = g_font24b = g_font23 = g_font22 = g_font20 = g_font18 = g_font16 = g_font36 = 0;
}

static void ensure_fonts(void) {
    float diff = g_fontScale - g_scale;
    if (diff < 0) diff = -diff;
    if (g_font36 && g_font24 && g_font24b && g_font23 && g_font22 && g_font20 && g_font18 && g_font16 &&
        diff < 0.005f) return;
    destroy_fonts();
    g_font24 = create_ui_font(24, FW_NORMAL);
    g_font23 = create_ui_font(23, FW_NORMAL);
    g_font24b = create_ui_font(24, FW_SEMIBOLD);
    g_font22 = create_ui_font(22, FW_NORMAL);
    g_font20 = create_ui_font(20, FW_NORMAL);
    g_font18 = create_ui_font(18, FW_NORMAL);
    g_font16 = create_ui_font(16, FW_NORMAL);
    g_font36 = create_ui_font(37, FW_BOLD);
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

static float text_width(HDC dc, HFONT font, const WCHAR *text) {
    HGDIOBJ old;
    SIZE sz;
    sz.cx = 0;
    sz.cy = 0;
    if (!dc || !text) return 0;
    old = SelectObject(dc, font ? font : (HFONT)GetStockObject(DEFAULT_GUI_FONT));
    GetTextExtentPoint32W(dc, text, (int)wcslen(text), &sz);
    SelectObject(dc, old);
    return (float)sz.cx / g_scale;
}

/* ---------- icons ---------- */

static void draw_gear(GpGraphics *g, float cx, float cy, float size, COLORREF color) {
    int i;
    float outer = size * 0.49f;
    float inner = size * 0.36f;
    float ring = size * 0.31f;
    for (i = 0; i < 8; i++) {
        float a = (float)i * 3.14159265f / 4.0f;
        float x1 = cx + (float)cos((double)a) * inner;
        float y1 = cy + (float)sin((double)a) * inner;
        float x2 = cx + (float)cos((double)a) * outer;
        float y2 = cy + (float)sin((double)a) * outer;
        draw_line(g, color, x1, y1, x2, y2, 2.2f);
    }
    stroke_circle(g, color, cx - ring, cy - ring, ring * 2, 2.0f);
    stroke_circle(g, color, cx - size * 0.10f, cy - size * 0.10f, size * 0.20f, 2.0f);
}

static void draw_logout_icon(GpGraphics *g, float cx, float cy, float size, COLORREF color) {
    GpPath *p = 0;
    GpPen *pen = 0;
    float r = size * 0.39f;
    GdipCreatePath(GpFillModeWinding, &p);
    if (p) {
        GdipAddPathArc(p, (cx - r) * g_scale, (cy - r) * g_scale, r * 2 * g_scale, r * 2 * g_scale, 200, 320);
        GdipCreatePen1(gp_color(color, 255), 2.0f * g_scale, GpUnitPixel, &pen);
        if (pen) {
            GdipSetPenLineCap197819(pen, GpLineCapRound, GpLineCapRound, GpLineCapRound);
            GdipDrawPath(g, pen, p);
            GdipDeletePen(pen);
        }
        GdipDeletePath(p);
    }
    draw_line(g, color, cx - r - size * 0.14f, cy, cx - r + size * 0.08f, cy, 2.0f);
    draw_line(g, color, cx - r - size * 0.14f, cy, cx - r + size * 0.02f, cy - size * 0.16f, 1.9f);
    draw_line(g, color, cx - r - size * 0.14f, cy, cx - r + size * 0.02f, cy + size * 0.16f, 1.9f);
}

static void draw_play(GpGraphics *g, float x, float y, float w, float h, COLORREF color) {
    float pts[6];
    pts[0] = x + w * 0.18f; pts[1] = y + h * 0.10f;
    pts[2] = x + w * 0.18f; pts[3] = y + h * 0.90f;
    pts[4] = x + w * 0.92f; pts[5] = y + h * 0.50f;
    fill_poly(g, color, pts, 3);
}

static void draw_check(GpGraphics *g, float cx, float cy, float size, COLORREF color) {
    draw_line(g, color, cx - size * 0.42f, cy + size * 0.02f, cx - size * 0.10f, cy + size * 0.34f, 2.3f);
    draw_line(g, color, cx - size * 0.10f, cy + size * 0.34f, cx + size * 0.46f, cy - size * 0.34f, 2.3f);
}

/* ---------- layout and hit testing ---------- */

static Layout make_layout(void) {
    Layout l;
    float H = (float)g_clientH / g_scale;
    float W = (float)g_clientW / g_scale;
    int i;
    ZeroMemory(&l, sizeof(l));
    l.W = W;
    l.H = H;
    l.mainX = 343.0f;
    l.mainW = W - l.mainX - 27.0f;
    if (l.mainW < 80) l.mainW = 80;
    for (i = 0; i < 4; i++) {
        l.items[i].x = 12.0f;
        l.items[i].y = 44.0f + i * 42.0f;
        l.items[i].w = 296.0f;
        l.items[i].h = 42.0f;
    }
    l.settingsBtn.x = 17.0f;
    l.settingsBtn.y = H - 127.0f;
    l.settingsBtn.w = 288.0f;
    l.settingsBtn.h = 51.0f;
    l.logoutBtn.x = 17.0f;
    l.logoutBtn.y = H - 69.0f;
    l.logoutBtn.w = 288.0f;
    l.logoutBtn.h = 51.0f;
    l.launchBtn.x = 346.0f;
    l.launchBtn.y = H - 69.0f;
    l.launchBtn.w = W - 346.0f - 27.0f;
    l.launchBtn.h = 51.0f;
    l.card.x = 345.0f;
    l.card.y = 130.0f;
    l.card.w = l.mainW;
    l.card.h = 330.0f;
    l.openBtn.x = l.card.x + l.card.w - 112.0f;
    l.openBtn.y = 162.0f;
    l.openBtn.w = 92.0f;
    l.openBtn.h = 32.0f;
    l.detectBtn.x = l.card.x + l.card.w - 112.0f;
    l.detectBtn.y = 238.0f;
    l.detectBtn.w = 92.0f;
    l.detectBtn.h = 32.0f;
    l.memMinus.x = l.card.x + l.card.w - 92.0f;
    l.memMinus.y = 314.0f;
    l.memMinus.w = 34.0f;
    l.memMinus.h = 32.0f;
    l.memPlus.x = l.card.x + l.card.w - 50.0f;
    l.memPlus.y = 314.0f;
    l.memPlus.w = 34.0f;
    l.memPlus.h = 32.0f;
    return l;
}

static int rect_hit(const TRect *r, float x, float y) {
    return x >= r->x && x <= r->x + r->w && y >= r->y && y <= r->y + r->h;
}

static int hit_test(float x, float y) {
    Layout l = make_layout();
    int i;
    if (g_page == 1) {
        if (rect_hit(&l.openBtn, x, y)) return HIT_OPEN;
        if (rect_hit(&l.detectBtn, x, y)) return HIT_DETECT;
        if (rect_hit(&l.memMinus, x, y)) return HIT_MEM_MINUS;
        if (rect_hit(&l.memPlus, x, y)) return HIT_MEM_PLUS;
    }
    if (rect_hit(&l.settingsBtn, x, y)) return HIT_SETTINGS;
    if (rect_hit(&l.logoutBtn, x, y)) return HIT_LOGOUT;
    if (rect_hit(&l.launchBtn, x, y)) return HIT_LAUNCH;
    for (i = 0; i < 4; i++) {
        if (rect_hit(&l.items[i], x, y)) return HIT_ITEM1 + i;
    }
    return HIT_NONE;
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

/* ---------- painting ---------- */

static void draw_buttons(GpGraphics *g, const Layout *l) {
    int ids[3];
    TRect *rs[3];
    int i;
    ids[0] = HIT_SETTINGS; rs[0] = (TRect *)&l->settingsBtn;
    ids[1] = HIT_LOGOUT; rs[1] = (TRect *)&l->logoutBtn;
    ids[2] = HIT_LAUNCH; rs[2] = (TRect *)&l->launchBtn;
    for (i = 0; i < 3; i++) {
        int hot = (g_hot == ids[i]);
        int pressed = (g_pressed == ids[i]);
        COLORREF fill = pressed ? RGB_BUTTON_P : (hot ? RGB_BUTTON_H : RGB_BUTTON);
        COLORREF border = hot ? RGB(76, 81, 94) : RGB_BTN_BORD;
        if (i == 0 && g_page == 1) fill = RGB(56, 61, 73);
        panel(g, fill, border, rs[i]->x, rs[i]->y, rs[i]->w, rs[i]->h, 7, 1);
    }
    if (g_page == 1) {
        int hot = (g_hot == HIT_OPEN);
        panel(g, hot ? RGB_BUTTON_H : RGB_BUTTON, hot ? RGB(80, 85, 98) : RGB_BTN_BORD,
              l->openBtn.x, l->openBtn.y, l->openBtn.w, l->openBtn.h, 6, 1);
        hot = (g_hot == HIT_DETECT);
        panel(g, hot ? RGB_BUTTON_H : RGB_BUTTON, hot ? RGB(80, 85, 98) : RGB_BTN_BORD,
              l->detectBtn.x, l->detectBtn.y, l->detectBtn.w, l->detectBtn.h, 6, 1);
        hot = (g_hot == HIT_MEM_MINUS);
        panel(g, hot ? RGB_BUTTON_H : RGB_BUTTON, hot ? RGB(80, 85, 98) : RGB_BTN_BORD,
              l->memMinus.x, l->memMinus.y, l->memMinus.w, l->memMinus.h, 6, 1);
        hot = (g_hot == HIT_MEM_PLUS);
        panel(g, hot ? RGB_BUTTON_H : RGB_BUTTON, hot ? RGB(80, 85, 98) : RGB_BTN_BORD,
              l->memPlus.x, l->memPlus.y, l->memPlus.w, l->memPlus.h, 6, 1);
    }
}

static void draw_sidebar(GpGraphics *g, const Layout *l) {
    int i;
    fill_rect(g, RGB_SEP, 320, 0, 1, l->H);
    for (i = 0; i < 4; i++) {
        int selected = (i == g_selected);
        int hot = (g_hot == HIT_ITEM1 + i);
        if (selected) {
            panel(g, RGB(45, 49, 58), RGB(45, 49, 58), l->items[i].x, l->items[i].y,
                  l->items[i].w, l->items[i].h, 5, 0);
        } else if (hot) {
            panel(g, RGB(40, 44, 52), RGB(40, 44, 52), l->items[i].x, l->items[i].y,
                  l->items[i].w, l->items[i].h, 5, 0);
        }
    }
}

static void draw_release_shapes(GpGraphics *g, const Layout *l) {
    (void)g;
    (void)l;
}

static void draw_settings_shapes(GpGraphics *g, const Layout *l) {
    panel(g, RGB_PANEL, RGB(47, 51, 60), l->card.x, l->card.y, l->card.w, l->card.h, 10, 1);
}

static void draw_toast_shape(GpGraphics *g, const Layout *l) {
    float w = 580.0f;
    float x;
    if (!g_toast[0] || GetTickCount() >= g_toastUntil) return;
    if (w > l->mainW - 20) w = l->mainW - 20;
    x = (l->W - w) / 2;
    panel(g, RGB(42, 46, 54), g_toastError ? RGB(100, 58, 58) : RGB(58, 62, 73),
          x, l->H - 132, w, 44, 9, 1);
}

static void draw_shapes(GpGraphics *g, const Layout *l) {
    fill_rect(g, RGB_BG, 0, 0, l->W, l->H);
    draw_sidebar(g, l);
    if (g_page == 0) draw_release_shapes(g, l);
    else draw_settings_shapes(g, l);
    draw_buttons(g, l);
    draw_toast_shape(g, l);
}

static void draw_texts(HDC dc, const Layout *l) {
    int i;

    for (i = 0; i < 4; i++) {
        draw_text(dc, g_items[i], l->items[i].x + 9, l->items[i].y, l->items[i].w - 10, l->items[i].h,
                  g_font24, i == g_selected ? RGB(200, 202, 208) : RGB(180, 183, 190),
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    {
        const WCHAR *settings = L"Settings";
        float tw = text_width(dc, g_font24, settings);
        float groupW = 30.0f + 10.0f + tw;
        float gx = l->settingsBtn.x + (l->settingsBtn.w - groupW) / 2;
        draw_text(dc, settings, gx + 40, l->settingsBtn.y, tw + 3, l->settingsBtn.h, g_font24,
                  RGB(198, 200, 206), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    {
        const WCHAR *logout = L"Log out";
        float tw = text_width(dc, g_font24, logout);
        float groupW = 30.0f + 10.0f + tw;
        float gx = l->settingsBtn.x + (l->settingsBtn.w - groupW) / 2;
        draw_text(dc, logout, gx + 40, l->logoutBtn.y, tw + 3, l->logoutBtn.h, g_font24,
                  RGB(198, 200, 206), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    {
        const WCHAR *label = (g_page == 1) ? L"Done" : (g_launching ? L"Launching..." : L"Launch");
        float tw = text_width(dc, g_font24, label);
        float groupW = 20.0f + 12.0f + tw;
        float gx = l->launchBtn.x + (l->launchBtn.w - groupW) / 2;
        draw_text(dc, label, gx + 32, l->launchBtn.y, tw + 4, l->launchBtn.h, g_font24,
                  RGB(199, 201, 207), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    if (g_page == 0) {
        draw_text(dc, g_items[g_selected], l->mainX, 25, l->mainW, 48, g_font36, RGB_HEADLINE,
                  DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        draw_text(dc, L"Released ... days ago", l->mainX, 77, l->mainW, 34, g_font24, RGB(158, 161, 168),
                  DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        for (i = 0; i < 3; i++) {
            float y = 109.0f + i * 33.0f;
            draw_text(dc, L"[/]", l->mainX, y, 32, 34, g_font24, RGB(150, 153, 161),
                      DT_LEFT | DT_TOP | DT_SINGLELINE);
            draw_text(dc, g_notes[g_selected][i], l->mainX + 34, y, l->mainW - 34, 34, g_font24,
                      RGB(187, 190, 196), DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
    } else {
        WCHAR memText[64];
        WCHAR profile[96];
        draw_text(dc, L"Settings", l->mainX, 30, l->mainW, 48, g_font36, RGB_HEADLINE,
                  DT_LEFT | DT_TOP | DT_SINGLELINE);
        draw_text(dc, L"Launcher configuration", l->mainX, 80, l->mainW, 28, g_font24, RGB(158, 161, 168),
                  DT_LEFT | DT_TOP | DT_SINGLELINE);
        draw_text(dc, L"GAME DIRECTORY", l->card.x + 24, 153, l->card.w - 160, 22, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        draw_text(dc, g_settings.gameDir, l->card.x + 24, 174, l->card.w - 160, 32, g_font22, RGB(197, 199, 205),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_PATH_ELLIPSIS);
        draw_text(dc, L"Open", l->openBtn.x, l->openBtn.y, l->openBtn.w, l->openBtn.h, g_font18,
                  RGB(195, 197, 203), DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        draw_text(dc, L"JAVA RUNTIME", l->card.x + 24, 229, l->card.w - 160, 22, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        draw_text(dc, g_settings.javaPath[0] ? g_settings.javaPath : L"Auto-detect when launching",
                  l->card.x + 24, 250, l->card.w - 160, 32, g_font22, RGB(197, 199, 205),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_PATH_ELLIPSIS);
        draw_text(dc, L"Detect", l->detectBtn.x, l->detectBtn.y, l->detectBtn.w, l->detectBtn.h, g_font18,
                  RGB(195, 197, 203), DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        draw_text(dc, L"MEMORY", l->card.x + 24, 305, l->card.w - 200, 22, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        wsprintfW(memText, L"%d MB", g_settings.memoryMB);
        draw_text(dc, memText, l->card.x + 24, 326, 200, 32, g_font22, RGB(197, 199, 205),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        draw_text(dc, L"-", l->memMinus.x, l->memMinus.y, l->memMinus.w, l->memMinus.h, g_font20,
                  RGB(205, 207, 212), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        draw_text(dc, L"+", l->memPlus.x, l->memPlus.y, l->memPlus.w, l->memPlus.h, g_font20,
                  RGB(205, 207, 212), DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        draw_text(dc, L"PROFILE", l->card.x + 24, 381, l->card.w - 200, 22, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        wsprintfW(profile, L"%s  (offline)", g_settings.username);
        draw_text(dc, profile, l->card.x + 24, 402, l->card.w - 48, 32, g_font22, RGB(197, 199, 205),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        draw_text(dc, g_versionFound ? g_versionId : L"No Minecraft version found",
                  l->card.x + 24, 430, l->card.w - 48, 24, g_font16, RGB(126, 130, 138),
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        draw_text(dc, L"Settings are saved automatically.", l->mainX, l->card.y + l->card.h + 14,
                  l->mainW, 24, g_font16, RGB(111, 115, 124), DT_LEFT | DT_SINGLELINE);
    }

    if (g_toast[0] && GetTickCount() < g_toastUntil) {
        float w = 580.0f;
        float x;
        if (w > l->mainW - 20) w = l->mainW - 20;
        x = (l->W - w) / 2;
        draw_text(dc, g_toast, x + 18, l->H - 132, w - 36, 44, g_font20,
                  g_toastError ? RGB(232, 180, 180) : RGB(198, 200, 206),
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
}


/* We need exact text widths for centering icons; draw them after text measurement.
   This function is called from paint after text is measured, before BitBlt. */
static void draw_icons_with_dc(GpGraphics *g, HDC dc, const Layout *l) {
    {
        const WCHAR *settings = L"Settings";
        float tw = text_width(dc, g_font24, settings);
        float groupW = 30.0f + 10.0f + tw;
        float gx = l->settingsBtn.x + (l->settingsBtn.w - groupW) / 2;
        draw_gear(g, gx + 15, l->settingsBtn.y + l->settingsBtn.h / 2, 28, RGB(184, 187, 194));
    }
    {
        const WCHAR *logout = L"Log out";
        float tw = text_width(dc, g_font24, logout);
        float groupW = 30.0f + 10.0f + tw;
        float gx = l->settingsBtn.x + (l->settingsBtn.w - groupW) / 2;
        draw_logout_icon(g, gx + 15, l->logoutBtn.y + l->logoutBtn.h / 2, 28, RGB(184, 187, 194));
    }
    {
        const WCHAR *label = (g_page == 1) ? L"Done" : (g_launching ? L"Launching..." : L"Launch");
        float tw = text_width(dc, g_font24, label);
        float groupW = 20.0f + 12.0f + tw;
        float gx = l->launchBtn.x + (l->launchBtn.w - groupW) / 2;
        float cy = l->launchBtn.y + l->launchBtn.h / 2;
        if (g_page == 1) draw_check(g, gx + 10, cy, 18, RGB_GREEN);
        else draw_play(g, gx, cy - 11, 20, 22, RGB_GREEN);
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
    /* Icons need text widths; draw them now while GDI+ is alive. */
    if (g) draw_icons_with_dc(g, mem, &l);
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
    g_pressed = 0;
    ZeroMemory(&g_launchResult, sizeof(g_launchResult));
    set_toast(L"Preparing Minecraft...", 10000, 0);
    th = CreateThread(0, 0, launch_thread, &g_launchResult, 0, 0);
    if (th) CloseHandle(th);
    else {
        g_launching = 0;
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

static void activate(int id) {
    if (g_launching && id != HIT_ITEM1 && id != HIT_ITEM2 && id != HIT_ITEM3 && id != HIT_ITEM4) {
        set_toast(L"Please wait for Minecraft to start.", 2500, 0);
        return;
    }
    switch (id) {
        case HIT_ITEM1:
        case HIT_ITEM2:
        case HIT_ITEM3:
        case HIT_ITEM4:
            g_selected = id - HIT_ITEM1;
            g_page = 0;
            InvalidateRect(g_hwnd, 0, FALSE);
            break;
        case HIT_SETTINGS:
            g_page = 1;
            InvalidateRect(g_hwnd, 0, FALSE);
            break;
        case HIT_LOGOUT:
            scopy(g_settings.username, L"Player", 64);
            settings_save(&g_settings);
            set_toast(L"Logged out. Offline profile: Player.", 4000, 0);
            break;
        case HIT_LAUNCH:
            if (g_page == 1) {
                g_page = 0;
                InvalidateRect(g_hwnd, 0, FALSE);
            } else {
                start_launch();
            }
            break;
        case HIT_OPEN:
            open_game_folder();
            break;
        case HIT_DETECT:
            if (detect_java(&g_settings, 8)) {
                settings_save(&g_settings);
                set_toast(L"Java runtime detected and saved.", 3500, 0);
            } else {
                set_toast(L"Java was not found. Install Java 8 or newer.", 5000, 1);
            }
            InvalidateRect(g_hwnd, 0, FALSE);
            break;
        case HIT_MEM_MINUS:
            if (g_settings.memoryMB > 1024) g_settings.memoryMB -= 512;
            settings_save(&g_settings);
            InvalidateRect(g_hwnd, 0, FALSE);
            break;
        case HIT_MEM_PLUS:
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
            COLORREF caption = RGB(36, 37, 42);
            COLORREF text = RGB(200, 202, 208);
            COLORREF border = RGB(51, 54, 62);
            setAttr(hwnd, 20, &dark, sizeof(dark));
            setAttr(hwnd, 35, &caption, sizeof(caption));
            setAttr(hwnd, 36, &text, sizeof(text));
            setAttr(hwnd, 34, &border, sizeof(border));
        }
        FreeLibrary(dwm);
    }
}

static void set_window_icon(void) {
    HDC screen, mem;
    HBITMAP bmp, mask, old;
    void *bits = 0;
    BITMAPINFO bi;
    ICONINFO ii;
    int x, y;
    int size = 32;
    screen = GetDC(0);
    mem = CreateCompatibleDC(screen);
    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = size;
    bi.bmiHeader.biHeight = -size;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bmp = CreateDIBSection(mem, &bi, DIB_RGB_COLORS, &bits, 0, 0);
    mask = CreateBitmap(size, size, 1, 1, 0);
    if (bmp) {
        old = (HBITMAP)SelectObject(mem, bmp);
        {
            HBRUSH bg = CreateSolidBrush(RGB(30, 33, 39));
            RECT r;
            r.left = 0; r.top = 0; r.right = size; r.bottom = size;
            FillRect(mem, &r, bg);
            DeleteObject(bg);
        }
        {
            HPEN pen = CreatePen(PS_SOLID, 5, RGB(102, 90, 235));
            HGDIOBJ op = SelectObject(mem, pen);
            MoveToEx(mem, 7, 28, 0); LineTo(mem, 11, 4);
            LineTo(mem, 21, 29); LineTo(mem, 27, 4);
            SelectObject(mem, op);
            DeleteObject(pen);
        }
        if (bits) {
            unsigned char *p = (unsigned char *)bits;
            for (y = 0; y < size; y++) for (x = 0; x < size; x++) p[(y * size + x) * 4 + 3] = 255;
        }
        SelectObject(mem, old);
        ZeroMemory(&ii, sizeof(ii));
        ii.fIcon = TRUE;
        ii.hbmColor = bmp;
        ii.hbmMask = mask;
        g_appIcon = CreateIconIndirect(&ii);
        DeleteObject(bmp);
    }
    if (mask) DeleteObject(mask);
    DeleteDC(mem);
    ReleaseDC(0, screen);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE:
            g_hwnd = hwnd;
            break;
        case WM_PAINT:
            paint(hwnd);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_SIZE:
            g_clientW = LOWORD(lp);
            g_clientH = HIWORD(lp);
            InvalidateRect(hwnd, 0, FALSE);
            return 0;
        case WM_DPICHANGED: {
            RECT *r = (RECT *)lp;
            g_dpi = HIWORD(wp);
            g_scale = (float)g_dpi / 96.0f;
            SetWindowPos(hwnd, 0, r->left, r->top, r->right - r->left, r->bottom - r->top,
                         SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            ensure_fonts();
            InvalidateRect(hwnd, 0, FALSE);
            return 0;
        }
        case WM_GETMINMAXINFO: {
            MINMAXINFO *mmi = (MINMAXINFO *)lp;
            RECT r;
            r.left = 0;
            r.top = 0;
            r.right = (LONG)(980 * g_scale);
            r.bottom = (LONG)(650 * g_scale);
            AdjustWindowRectEx(&r, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);
            mmi->ptMinTrackSize.x = r.right - r.left;
            mmi->ptMinTrackSize.y = r.bottom - r.top;
            return 0;
        }
        case WM_MOUSEMOVE: {
            float x = (float)GET_X_LPARAM(lp) / g_scale;
            float y = (float)GET_Y_LPARAM(lp) / g_scale;
            int id = hit_test(x, y);
            if (!g_tracking) {
                TRACKMOUSEEVENT tme;
                ZeroMemory(&tme, sizeof(tme));
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
                g_tracking = 1;
            }
            if (id != g_hot) {
                g_hot = id;
                InvalidateRect(hwnd, 0, FALSE);
            }
            return 0;
        }
        case WM_MOUSELEAVE:
            g_tracking = 0;
            if (g_hot) {
                g_hot = HIT_NONE;
                InvalidateRect(hwnd, 0, FALSE);
            }
            return 0;
        case WM_SETCURSOR:
            if (LOWORD(lp) == HTCLIENT) {
                SetCursor(LoadCursorW(0, IDC_ARROW));
                return TRUE;
            }
            break;
        case WM_LBUTTONDOWN: {
            float x = (float)GET_X_LPARAM(lp) / g_scale;
            float y = (float)GET_Y_LPARAM(lp) / g_scale;
            int id = hit_test(x, y);
            if (id) {
                g_pressed = id;
                SetCapture(hwnd);
                InvalidateRect(hwnd, 0, FALSE);
            }
            return 0;
        }
        case WM_LBUTTONUP: {
            int id;
            float x = (float)GET_X_LPARAM(lp) / g_scale;
            float y = (float)GET_Y_LPARAM(lp) / g_scale;
            id = hit_test(x, y);
            if (g_pressed) {
                int was = g_pressed;
                g_pressed = 0;
                ReleaseCapture();
                if (id == was) activate(id);
                InvalidateRect(hwnd, 0, FALSE);
            }
            return 0;
        }
        case WM_CAPTURECHANGED:
            if (g_pressed && (HWND)lp != hwnd) {
                g_pressed = 0;
                InvalidateRect(hwnd, 0, FALSE);
            }
            return 0;
        case WM_TIMER:
            if (wp == 1) {
                if (GetTickCount() >= g_toastUntil) {
                    g_toast[0] = 0;
                    KillTimer(hwnd, 1);
                    InvalidateRect(hwnd, 0, FALSE);
                }
            }
            return 0;
        case WM_APP_LAUNCH_DONE:
            g_launching = 0;
            set_toast(g_launchResult.message, g_launchResult.ok ? 6000 : 7000, g_launchResult.ok ? 0 : 1);
            if (g_launchResult.ok) settings_save(&g_settings);
            InvalidateRect(hwnd, 0, FALSE);
            return 0;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
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
    g_appIcon = (HICON)LoadImageW(hi, MAKEINTRESOURCEW(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    if (!g_appIcon) set_window_icon();
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.hIcon = g_appIcon ? g_appIcon : LoadIconW(0, IDI_APPLICATION);
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursorW(0, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB_BG);
    wc.lpszClassName = APP_CLASS;
    if (!RegisterClassExW(&wc)) return 0;
    g_hwnd = CreateWindowExW(WS_EX_APPWINDOW, APP_CLASS, APP_TITLE,
                             WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                             CW_USEDEFAULT, CW_USEDEFAULT, 1200, 800, 0, 0, hi, 0);
    if (!g_hwnd) return 0;
    SetWindowTextW(g_hwnd, APP_TITLE);
    dpi = get_window_dpi(g_hwnd);
    g_scale = (float)dpi / 96.0f;
    apply_initial_size(g_hwnd, dpi);
    apply_window_look(g_hwnd);
    {
        WCHAR versionJson[PATH_CAP];
        g_versionFound = detect_version(&g_settings, g_versionId, 128, versionJson, PATH_CAP);
    }
    g_selected = 0;
    ShowWindow(g_hwnd, show);
    UpdateWindow(g_hwnd);
    apply_window_look(g_hwnd);
    while (GetMessageW(&msg, 0, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    destroy_fonts();
    if (g_appIcon) DestroyIcon(g_appIcon);
    GdiplusShutdown(g_gdiplusToken);
    return 0;
}
