/* Minimal GDI+ flat API declarations for TinyCC.
   Only what the launcher needs; links against the Windows system gdiplus.dll. */
#ifndef GDI_PLUS_FLAT_H
#define GDI_PLUS_FLAT_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int GpStatus;
typedef INT GpFillMode;
typedef INT GpLineJoin;
typedef INT GpLineCap;
typedef INT GpUnit;
typedef INT GpSmoothingMode;
typedef INT GpPixelOffsetMode;
typedef INT GpCompositingQuality;
typedef INT GpInterpolationMode;

typedef DWORD ARGB;

typedef struct GpGraphics GpGraphics;
typedef struct GpBrush GpBrush;
typedef struct GpSolidFill GpSolidFill;
typedef struct GpPen GpPen;
typedef struct GpPath GpPath;
typedef struct GpMatrix GpMatrix;
typedef struct GpImage GpImage;
typedef struct GpBitmap GpBitmap;

typedef struct GpPointF {
    float X;
    float Y;
} GpPointF;

typedef struct GpRectF {
    float X;
    float Y;
    float Width;
    float Height;
} GpRectF;

typedef struct GdiplusStartupInput {
    UINT32 GdiplusVersion;
    void *DebugEventCallback;
    BOOL SuppressBackgroundThread;
    BOOL SuppressExternalCodecs;
} GdiplusStartupInput;

#define GpSmoothingModeNone 3
#define GpSmoothingModeAntiAlias 4
#define GpSmoothingModeHighQuality 2

#define GpPixelOffsetModeNone 3
#define GpPixelOffsetModeHalf 4
#define GpPixelOffsetModeHighQuality 2

#define GpCompositingQualityHighQuality 2

#define GpFillModeAlternate 0
#define GpFillModeWinding 1

#define GpLineCapFlat 0
#define GpLineCapRound 2

#define GpLineJoinRound 2

#define GpUnitWorld 0
#define GpUnitPixel 2

GpStatus WINAPI GdiplusStartup(ULONG_PTR *token, const GdiplusStartupInput *input, void *output);
void WINAPI GdiplusShutdown(ULONG_PTR token);

GpStatus WINAPI GdipCreateFromHDC(HDC hdc, GpGraphics **graphics);
GpStatus WINAPI GdipDeleteGraphics(GpGraphics *graphics);
GpStatus WINAPI GdipSetSmoothingMode(GpGraphics *graphics, GpSmoothingMode mode);
GpStatus WINAPI GdipSetPixelOffsetMode(GpGraphics *graphics, GpPixelOffsetMode mode);
GpStatus WINAPI GdipSetCompositingQuality(GpGraphics *graphics, GpCompositingQuality quality);
GpStatus WINAPI GdipSetInterpolationMode(GpGraphics *graphics, GpInterpolationMode mode);

GpStatus WINAPI GdipCreateSolidFill(ARGB color, GpSolidFill **brush);
GpStatus WINAPI GdipDeleteBrush(GpBrush *brush);
GpStatus WINAPI GdipCreatePen1(ARGB color, float width, GpUnit unit, GpPen **pen);
GpStatus WINAPI GdipCreatePen2(GpBrush *brush, float width, GpUnit unit, GpPen **pen);
GpStatus WINAPI GdipDeletePen(GpPen *pen);
GpStatus WINAPI GdipSetPenLineCap197819(GpPen *pen, GpLineCap startCap, GpLineCap endCap, GpLineCap dashCap);
GpStatus WINAPI GdipSetPenLineJoin(GpPen *pen, GpLineJoin lineJoin);
GpStatus WINAPI GdipSetPenWidth(GpPen *pen, float width);

GpStatus WINAPI GdipCreatePath(GpFillMode brushMode, GpPath **path);
GpStatus WINAPI GdipDeletePath(GpPath *path);
GpStatus WINAPI GdipAddPathLine(GpPath *path, float x1, float y1, float x2, float y2);
GpStatus WINAPI GdipAddPathLine2(GpPath *path, const GpPointF *points, INT count);
GpStatus WINAPI GdipAddPathArc(GpPath *path, float x, float y, float width, float height, float startAngle, float sweepAngle);
GpStatus WINAPI GdipAddPathRectangle(GpPath *path, float x, float y, float width, float height);
GpStatus WINAPI GdipClosePathFigure(GpPath *path);
GpStatus WINAPI GdipFillPath(GpGraphics *graphics, GpBrush *brush, GpPath *path);
GpStatus WINAPI GdipDrawPath(GpGraphics *graphics, GpPen *pen, GpPath *path);

GpStatus WINAPI GdipFillRectangle(GpGraphics *graphics, GpBrush *brush, float x, float y, float width, float height);
GpStatus WINAPI GdipDrawRectangle(GpGraphics *graphics, GpPen *pen, float x, float y, float width, float height);
GpStatus WINAPI GdipFillEllipse(GpGraphics *graphics, GpBrush *brush, float x, float y, float width, float height);
GpStatus WINAPI GdipDrawEllipse(GpGraphics *graphics, GpPen *pen, float x, float y, float width, float height);
GpStatus WINAPI GdipDrawLine(GpGraphics *graphics, GpPen *pen, float x1, float y1, float x2, float y2);
GpStatus WINAPI GdipFillPolygon(GpGraphics *graphics, GpBrush *brush, const GpPointF *points, INT count, GpFillMode fillMode);

#ifdef __cplusplus
}
#endif

#endif


