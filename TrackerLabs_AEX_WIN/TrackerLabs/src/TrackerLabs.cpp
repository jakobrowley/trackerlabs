#include "TrackerLabs.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include "activation.h"
#include <regex>
// Helpers
bool g_IsActivated = false;

bool isRedXPixel(int x, int y, int width, int height) {
    // 1. Bounds check: ensure the pixel is inside the rectangle
    if (x < 0 || x >= width || y < 0 || y >= height) return false;

    // Use long long to prevent overflow during multiplication
    long long x_h = (long long)x * height;
    long long y_w = (long long)y * width;
    long long inv_y_w = (long long)(height - 1 - y) * width;

    // Use a tolerance for thickness (e.g., width / 2) 
    // to make sure the X is visible and solid
    long long tolerance = width;

    // Check Main Diagonal (top-left to bottom-right)
    bool mainDiag = std::abs(x_h - y_w) < tolerance;

    // Check Anti-Diagonal (bottom-left to top-right)
    bool antiDiag = std::abs(x_h - inv_y_w) < tolerance;

    return mainDiag || antiDiag;
}

// --- DRAWING HELPERS ---
static void PlotPixel(PF_EffectWorld* world, PF_PixelFormat		format, int x, int y, PF_Pixel8 color) {
    if (x < 0 || x >= world->width || y < 0 || y >= world->height) return;
    float r = color.red / 255.f;
    float g = color.green / 255.f;
    float b = color.blue / 255.f;

    if (format == PF_PixelFormat_ARGB32)
    {
        PF_Pixel8* p = (PF_Pixel8*)((char*)world->data + y * world->rowbytes + x * sizeof(PF_Pixel8));
        p->red = color.red; p->green = color.green; p->blue = color.blue; p->alpha = 255;
    }
    else if (format == PF_PixelFormat_ARGB64)
    {
        PF_Pixel16* p = (PF_Pixel16*)((char*)world->data + y * world->rowbytes + x * sizeof(PF_Pixel16));
        p->red =r* PF_MAX_CHAN16;
        p->green =g* PF_MAX_CHAN16;
        p->blue = b* PF_MAX_CHAN16; 
        p->alpha = PF_MAX_CHAN16;
    }
    else if (format == PF_PixelFormat_ARGB128)
    {
        PF_PixelFloat* p = (PF_PixelFloat*)((char*)world->data + y * world->rowbytes + x * sizeof(PF_PixelFloat));
        p->red = r;
        p->green = g;
        p->blue = b;
        p->alpha = 1;
    }
}

// Blend pixel with alpha (0-255)
static void PlotPixelAlpha(PF_EffectWorld* world, PF_PixelFormat		format, int x, int y, PF_Pixel8 color, int alpha) {
    if (x < 0 || x >= world->width || y < 0 || y >= world->height) return;
    float a = alpha / 255.0f;
    float inv = 1.0f - a;
  
    float r = color.red / 255.f;
    float g = color.green / 255.f;
    float b = color.blue / 255.f;


    if (format == PF_PixelFormat_ARGB32)
    {
        PF_Pixel8* p = (PF_Pixel8*)((char*)world->data + y * world->rowbytes + x * sizeof(PF_Pixel8));

        r = r * a + p->red / 255.f * inv;
        g = g * a + p->green / 255.f * inv;
        b = b * a + p->blue / 255.f * inv;

        p->red = (unsigned char)(r* PF_MAX_CHAN8);
        p->green = (unsigned char)(g* PF_MAX_CHAN8);
        p->blue = (unsigned char)(b* PF_MAX_CHAN8);

    }
    else if (format == PF_PixelFormat_ARGB64)
    {
        PF_Pixel16* p = (PF_Pixel16*)((char*)world->data + y * world->rowbytes + x * sizeof(PF_Pixel16));

        r = r * a + p->red / (float)PF_MAX_CHAN16 * inv;
        g = g * a + p->green / (float)PF_MAX_CHAN16 * inv;
        b = b * a + p->blue / (float)PF_MAX_CHAN16 * inv;

        p->red = (unsigned short)(r * PF_MAX_CHAN16);
        p->green = (unsigned short)(g * PF_MAX_CHAN16);
        p->blue = (unsigned short)(b * PF_MAX_CHAN16);

    }
    else if (format == PF_PixelFormat_ARGB128)
    {
        PF_PixelFloat* p = (PF_PixelFloat*)((char*)world->data + y * world->rowbytes + x * sizeof(PF_PixelFloat));

        r = r * a + p->red * inv;
        g = g * a + p->green * inv;
        b = b * a + p->blue  * inv;

        p->red = r;
        p->green = g;
        p->blue = b;
    }
}

// Draw filled circle
static void DrawFilledCircle(PF_EffectWorld* world, PF_PixelFormat		format, int cx, int cy, int radius, PF_Pixel8 color, int alpha = 255) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                if (alpha >= 255) {
                    PlotPixel(world, format, cx + x, cy + y, color);
                }
                else {
                    PlotPixelAlpha(world,format, cx + x, cy + y, color, alpha);
                }
            }
        }
    }
}

// Draw circle outline
static void DrawCircleOutline(PF_EffectWorld* world, PF_PixelFormat		format, int cx, int cy, int radius, PF_Pixel8 color,int thickness) {

    if (thickness < 1) thickness = 1;
    int half = thickness / 2;
    int x = radius, y = 0, err = 0;
    while (x >= y) {
        // Plot each outline pixel as a small filled square for "stroke" thickness.
        for (int oy = -half; oy <= half; oy++) {
            for (int ox = -half; ox <= half; ox++) {
                PlotPixel(world,format, cx + x + ox, cy + y + oy, color);
                PlotPixel(world,format, cx + y + ox, cy + x + oy, color);
                PlotPixel(world,format, cx - y + ox, cy + x + oy, color);
                PlotPixel(world,format, cx - x + ox, cy + y + oy, color);
                PlotPixel(world,format, cx - x + ox, cy - y + oy, color);
                PlotPixel(world,format, cx - y + ox, cy - x + oy, color);
                PlotPixel(world,format, cx + y + ox, cy - x + oy, color);
                PlotPixel(world,format, cx + x + ox, cy - y + oy, color);
            }
        }
        y++;
        if (err <= 0) { err += 2 * y + 1; }
        if (err > 0) { x--; err -= 2 * x + 1; }
    }
}

// Draw filled box with alpha
static void DrawFilledBox(PF_EffectWorld* world, PF_PixelFormat		format, int cx, int cy, int halfW, int halfH, PF_Pixel8 color, int alpha) {
    int left = cx - halfW, right = cx + halfW;
    int top = cy - halfH, bottom = cy + halfH;
    for (int y = top; y <= bottom; y++) {
        for (int x = left; x <= right; x++) {
            PlotPixelAlpha(world,format, x, y, color, alpha);
        }
    }
}

static void DrawLine(PF_EffectWorld* world, PF_PixelFormat		format, int x0, int y0, int x1, int y1, PF_Pixel8 color,int thickness) {

    if (thickness <= 0) thickness=1;

    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    int half = thickness / 2;
    int start = -half;
    int end = thickness - half - 1;

    while (1) {
        // Thicken the line by plotting a small square around each Bresenham step.
        for (int oy = start; oy <= end; oy++) {
            for (int ox = start; ox <= end; ox++) {
                PlotPixel(world,format, x0 + ox, y0 + oy, color);
            }
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
static void DrawCurvedLine(PF_EffectWorld* world, PF_PixelFormat		format, int x0, int y0, int x1, int y1, PF_Pixel8 color, int thickness, float curvature01, int curveSign) {

    if (thickness <= 0) thickness = 1;
    if (curvature01 <= 0.0f) {
        DrawLine(world, format,x0, y0, x1, y1, color,thickness);
        return;
    }

    float dx = (float)(x1 - x0);
    float dy = (float)(y1 - y0);
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 0.001f) {
        DrawLine(world,format, x0, y0, x1, y1, color, thickness);
        return;
    }

    // Perpendicular unit vector.
    float nx = -dy / dist;
    float ny = dx / dist;

    // Quadratic Bezier: P0 -> P1 -> P2
    float midx = (x0 + x1) * 0.5f;
    float midy = (y0 + y1) * 0.5f;
    float mag = curvature01 * dist * 0.25f * (float)curveSign;
    float cx = midx + nx * mag;
    float cy = midy + ny * mag;

    // Sample enough segments so the curve feels smooth.
    int samples = (int)(dist / 3.0f) + 2;
    if (samples < 2) samples = 2;
    if (samples > 200) samples = 200;

    float prevx = (float)x0;
    float prevy = (float)y0;

    for (int s = 1; s <= samples; s++) {
        float t = (float)s / (float)samples;
        float omt = 1.0f - t;

        float x = omt * omt * (float)x0 + 2.0f * omt * t * cx + t * t * (float)x1;
        float y = omt * omt * (float)y0 + 2.0f * omt * t * cy + t * t * (float)y1;

        int xi0 = (int)(prevx + (prevx >= 0.0f ? 0.5f : -0.5f));
        int yi0 = (int)(prevy + (prevy >= 0.0f ? 0.5f : -0.5f));
        int xi1 = (int)(x + (x >= 0.0f ? 0.5f : -0.5f));
        int yi1 = (int)(y + (y >= 0.0f ? 0.5f : -0.5f));

        if (xi0 != xi1 || yi0 != yi1) {
            DrawLine(world,format, xi0, yi0, xi1, yi1, color,thickness);
        }

        prevx = x;
        prevy = y;
    }
}
static void DrawDottedLine(PF_EffectWorld* world, PF_PixelFormat		format, int x0, int y0, int x1, int y1, PF_Pixel8 color, int dashLen, int gapLen,int& step, int thickness) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    if (thickness <= 0) thickness = 1;

    int half = thickness / 2;
    int start = -half;
    int end = thickness - half - 1;

    int cycleLen = dashLen + gapLen;
    if (cycleLen <= 0) cycleLen = 1; // Prevent modulo by zero.

    while (1) {
        // Draw only during dash portion (keep `step` continuous across segments).
        if ((step % cycleLen) < dashLen) {
            for (int oy = start; oy <= end; oy++) {
                for (int ox = start; ox <= end; ox++) {
                    PlotPixel(world,format, x0 + ox, y0 + oy, color);
                }
            }
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
        step++;
    }
}
static void DrawCurvedDottedLine(PF_EffectWorld* world, PF_PixelFormat		format, int x0, int y0, int x1, int y1, PF_Pixel8 color, int dashLen, int gapLen, int& step, int thickness, float curvature01, int curveSign) {
    
    if (thickness <= 0) thickness = 1;


    if (curvature01 <= 0.0f) {
        DrawDottedLine(world,format, x0, y0, x1, y1, color, dashLen, gapLen,step,thickness);
        return;
    }

    float dx = (float)(x1 - x0);
    float dy = (float)(y1 - y0);
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 0.001f) {
        DrawDottedLine(world,format, x0, y0, x1, y1, color, dashLen, gapLen, step, thickness);
        return;
    }

    float nx = -dy / dist;
    float ny = dx / dist;

    float midx = (x0 + x1) * 0.5f;
    float midy = (y0 + y1) * 0.5f;
    float mag = curvature01 * dist * 0.25f * (float)curveSign;
    float cx = midx + nx * mag;
    float cy = midy + ny * mag;

    int samples = (int)(dist / 3.0f) + 2;
    if (samples < 2) samples = 2;
    if (samples > 200) samples = 200;

    step = 0;
    float prevx = (float)x0;
    float prevy = (float)y0;

    for (int s = 1; s <= samples; s++) {
        float t = (float)s / (float)samples;
        float omt = 1.0f - t;

        float x = omt * omt * (float)x0 + 2.0f * omt * t * cx + t * t * (float)x1;
        float y = omt * omt * (float)y0 + 2.0f * omt * t * cy + t * t * (float)y1;

        int xi0 = (int)(prevx + (prevx >= 0.0f ? 0.5f : -0.5f));
        int yi0 = (int)(prevy + (prevy >= 0.0f ? 0.5f : -0.5f));
        int xi1 = (int)(x + (x >= 0.0f ? 0.5f : -0.5f));
        int yi1 = (int)(y + (y >= 0.0f ? 0.5f : -0.5f));

        if (xi0 != xi1 || yi0 != yi1) {
            DrawDottedLine(world,format, xi0, yi0, xi1, yi1, color, dashLen, gapLen, step, thickness);
        }

        prevx = x;
        prevy = y;
    }
}
static void DrawBox(PF_EffectWorld* world, PF_PixelFormat		format, int cx, int cy, int halfW, int halfH, PF_Pixel8 color, int thickness) {
    int left = cx - halfW, right = cx + halfW;
    int top = cy - halfH, bottom = cy + halfH;
    DrawLine(world,format, left, top, right, top, color, thickness);
    DrawLine(world,format, right, top, right, bottom, color, thickness);
    DrawLine(world,format, right, bottom, left, bottom, color, thickness);
    DrawLine(world, format,left, bottom, left, top, color, thickness);
}

// Invert pixels inside a rectangular region (for inverted fill effect)
static void InvertBoxRegion(PF_EffectWorld* world, PF_PixelFormat		format, int cx, int cy, int halfW, int halfH) {
    int left = cx - halfW + 1;   // +1 to not invert the outline pixels
    int right = cx + halfW - 1;  // -1 to not invert the outline pixels
    int top = cy - halfH + 1;
    int bottom = cy + halfH - 1;

    // Clamp to world bounds
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right >= world->width) right = world->width - 1;
    if (bottom >= world->height) bottom = world->height - 1;

    for (int y = top; y <= bottom; y++) {
        for (int x = left; x <= right; x++) {


            if (x < 0 || x >= world->width || y < 0 || y >= world->height) return;
         
            if (format == PF_PixelFormat_ARGB32)
            {
                PF_Pixel8* p = (PF_Pixel8*)((char*)world->data + y * world->rowbytes + x * sizeof(PF_Pixel8));
                p->red = 255 - p->red;
                p->green = 255 - p->green;
                p->blue = 255 - p->blue;

            }
            else if (format == PF_PixelFormat_ARGB64)
            {
                PF_Pixel16* p = (PF_Pixel16*)((char*)world->data + y * world->rowbytes + x * sizeof(PF_Pixel16));
                p->red = PF_MAX_CHAN16 - p->red;
                p->green = PF_MAX_CHAN16 - p->green;
                p->blue = PF_MAX_CHAN16 - p->blue;
            }
            else if (format == PF_PixelFormat_ARGB128)
            {
                PF_PixelFloat* p = (PF_PixelFloat*)((char*)world->data + y * world->rowbytes + x * sizeof(PF_PixelFloat));
                p->red = 1 - p->red;
                p->green = 1 - p->green;
                p->blue = 1 - p->blue;
            }
           
            // Keep alpha unchanged
        }
    }
}

// Simple 3x5 digit font
static const unsigned char DIGITS[10][5] = {
    {0x7,0x5,0x5,0x5,0x7}, {0x2,0x6,0x2,0x2,0x7}, {0x7,0x1,0x7,0x4,0x7},
    {0x7,0x1,0x7,0x1,0x7}, {0x5,0x5,0x7,0x1,0x1}, {0x7,0x4,0x7,0x1,0x7},
    {0x7,0x4,0x7,0x5,0x7}, {0x7,0x1,0x1,0x1,0x1}, {0x7,0x5,0x7,0x5,0x7},
    {0x7,0x5,0x7,0x1,0x7}
};
static const unsigned char HEX_CHARS[6][5] = {
    {0x2,0x5,0x7,0x5,0x5}, {0x6,0x5,0x6,0x5,0x6}, {0x3,0x4,0x4,0x4,0x3},
    {0x6,0x5,0x5,0x5,0x6}, {0x7,0x4,0x6,0x4,0x7}, {0x7,0x4,0x6,0x4,0x4}
};

// 8x8 pixel art for CJK characters (each row is a byte, MSB left)
// Japanese Kanji
static const unsigned char KANJI_TSUI[8] = { 0x00,0x7E,0x42,0x7E,0x42,0x7E,0x24,0x42 };  // ?
static const unsigned char KANJI_SEKI[8] = { 0x20,0x7C,0x44,0xFE,0x44,0x7C,0x44,0x44 };  // ?
static const unsigned char KANJI_HYO[8] = { 0x10,0xFE,0x38,0x54,0xFE,0x10,0x38,0x54 };   // ?
static const unsigned char KANJI_TEKI[8] = { 0x78,0x48,0x78,0x48,0x78,0x0A,0x7A,0x0A };  // ?
static const unsigned char KANJI_SUO[8] = { 0x24,0x7E,0x24,0x00,0x7E,0x42,0x7E,0x42 };   // ?
static const unsigned char KANJI_DING[8] = { 0xFE,0x10,0x7C,0x10,0xFE,0x10,0x10,0x10 };  // ?
// Extra kanji for variety
static const unsigned char KANJI_SEN[8] = { 0x10,0x10,0xFE,0x10,0x10,0xFE,0x10,0x10 };   // ? (line)
static const unsigned char KANJI_TEN[8] = { 0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00 };   // ? (point)
static const unsigned char KANJI_KEN[8] = { 0x10,0x7C,0x54,0x7C,0x54,0x7C,0x10,0x10 };   // ? (see)
static const unsigned char KANJI_SHI[8] = { 0x7E,0x40,0x7E,0x02,0x02,0x7E,0x00,0x00 };   // ? (view)
static const unsigned char KANJI_MEI[8] = { 0x24,0x7E,0x24,0x7E,0x24,0x7E,0x24,0x00 };   // ? (bright)
static const unsigned char KANJI_AN[8] = { 0xFE,0x82,0xBA,0xAA,0xBA,0x82,0xFE,0x00 };    // ? (dark)

// Katakana characters
static const unsigned char KATA_TO[8] = { 0x40,0x40,0x7C,0x40,0x20,0x10,0x08,0x00 };     // ?
static const unsigned char KATA_RA[8] = { 0x7E,0x02,0x02,0x7E,0x02,0x04,0x08,0x30 };     // ?
static const unsigned char KATA_TSU[8] = { 0x00,0x24,0x42,0x00,0x00,0x08,0x10,0x00 };    // ?
static const unsigned char KATA_KU[8] = { 0x3E,0x22,0x02,0x04,0x08,0x10,0x20,0x00 };     // ?
static const unsigned char KATA_RU[8] = { 0x7E,0x42,0x42,0x7E,0x04,0x08,0x10,0x20 };     // ?
static const unsigned char KATA_BU[8] = { 0x7E,0x08,0x7E,0x08,0x08,0x14,0x22,0x00 };     // ?
static const unsigned char KATA_RO[8] = { 0x7E,0x42,0x42,0x42,0x42,0x42,0x7E,0x00 };     // ?
static const unsigned char KATA_N[8] = { 0x00,0x42,0x42,0x22,0x12,0x0A,0x06,0x02 };      // ?
static const unsigned char KATA_A[8] = { 0x7E,0x08,0x08,0x7E,0x08,0x14,0x22,0x00 };      // ?
static const unsigned char KATA_I[8] = { 0x00,0x24,0x24,0x24,0x04,0x08,0x10,0x00 };      // ?
static const unsigned char KATA_SU[8] = { 0x7E,0x08,0x08,0x08,0x14,0x22,0x41,0x00 };     // ?
static const unsigned char KATA_KA[8] = { 0x10,0x7E,0x12,0x12,0x22,0x42,0x02,0x00 };     // ?
static const unsigned char KATA_TE[8] = { 0x7E,0x00,0x7E,0x10,0x10,0x10,0x10,0x10 };     // ?
static const unsigned char KATA_PU[8] = { 0x24,0x00,0x7E,0x08,0x08,0x14,0x22,0x00 };     // ?

// Word arrays for variety (pointers to character pairs/quads)
struct KanjiWord { const unsigned char* chars[4]; int len; };
static const KanjiWord JP_TRACK_WORDS[] = {
    {{KANJI_TSUI, KANJI_SEKI, NULL, NULL}, 2},  // ?? track
    {{KANJI_SEN, KANJI_TEN, NULL, NULL}, 2},    // ?? line-point
    {{KANJI_KEN, KANJI_SHI, NULL, NULL}, 2},    // ?? see-view
    {{KANJI_MEI, KANJI_AN, NULL, NULL}, 2},     // ?? light-dark
};
static const KanjiWord JP_TARGET_WORDS[] = {
    {{KANJI_HYO, KANJI_TEKI, NULL, NULL}, 2},   // ?? target
    {{KANJI_TEN, KANJI_KEN, NULL, NULL}, 2},    // ?? point-see
    {{KANJI_SHI, KANJI_SEN, NULL, NULL}, 2},    // ?? line of sight
};
static const KanjiWord KATA_WORDS[] = {
    {{KATA_TO, KATA_RA, KATA_TSU, KATA_KU}, 4}, // ???? track
    {{KATA_RU, KATA_BU, KATA_SU, NULL}, 3},     // ??? labs
    {{KATA_BU, KATA_RO, KATA_N, NULL}, 3},      // ??? blob
    {{KATA_RA, KATA_I, KATA_N, NULL}, 3},       // ??? line
    {{KATA_TE, KATA_PU, KATA_SU, NULL}, 3},     // ??? tapes
    {{KATA_SU, KATA_KA, KATA_N, NULL}, 3},      // ??? scan
};
static const KanjiWord CN_LOCK_WORDS[] = {
    {{KANJI_SUO, KANJI_DING, NULL, NULL}, 2},   // ?? lock
    {{KANJI_KEN, KANJI_TSUI, NULL, NULL}, 2},   // ?? see-chase
    {{KANJI_SEN, KANJI_MEI, NULL, NULL}, 2},    // ?? line-bright
};

static void DrawChar(PF_EffectWorld* world, PF_PixelFormat		format, char c, int x, int y, PF_Pixel8 color, int scale) {
    const unsigned char* glyph = NULL;
    if (c >= '0' && c <= '9') glyph = DIGITS[c - '0'];
    else if (c >= 'A' && c <= 'F') glyph = HEX_CHARS[c - 'A'];
    else if (c == 'X' || c == 'x') {
        for (int i = 0; i < 5 * scale; i++) {
            PlotPixel(world,format,x + i, y + i, color);
            PlotPixel(world,format, x + 4 * scale - i, y + i, color);
        }
        return;
    }
    else if (c == ':') {
        // Colon for timecode
        for (int sy = 0; sy < scale; sy++) {
            for (int sx = 0; sx < scale; sx++) {
                PlotPixel(world,format, x + scale + sx, y + scale + sy, color);
                PlotPixel(world,format, x + scale + sx, y + 3 * scale + sy, color);
            }
        }
        return;
    }
    if (!glyph) return;
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            if (glyph[row] & (1 << (2 - col))) {
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++)
                        PlotPixel(world,format, x + col * scale + sx, y + row * scale + sy, color);
            }
        }
    }
}

// Draw 8x8 CJK character
static void DrawKanji(PF_EffectWorld* world, PF_PixelFormat		format, const unsigned char* glyph, int x, int y, PF_Pixel8 color, int scale) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (glyph[row] & (1 << (7 - col))) {
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++)
                        PlotPixel(world,format, x + col * scale + sx, y + row * scale + sy, color);
            }
        }
    }
}

// Label type enumeration
enum LabelType {
    LABEL_COORDINATES = 1,
    LABEL_RANDOM_HEX = 2,
    LABEL_CUSTOM = 3,
    LABEL_SEQUENTIAL = 4,
    LABEL_TIMECODE = 5,
    LABEL_BINARY = 6,
    LABEL_HEX_MEMORY = 7,
    LABEL_JP_TRACK = 8,
    LABEL_JP_TARGET = 9,
    LABEL_KATAKANA = 10,
    LABEL_CN_LOCK = 11
};

static void DrawHexLabel(PF_EffectWorld* world, PF_PixelFormat		format, int id, int x, int y, PF_Pixel8 color, int scale) {
    char hex[8];
    snprintf(hex, sizeof(hex), "0X%04X", id & 0xFFFF);
    int px = x;
    int charWidth = 4 * scale;
    for (int i = 0; hex[i]; i++) {
        DrawChar(world,format, hex[i], px, y, color, scale);
        px += charWidth;
    }
}

// English word variations for sequential labels
static const char* SEQ_WORDS[] = { "TRK", "LAB", "BLB", "LNE", "TPE", "NOD", "PTR", "SRC" };
static const int SEQ_WORDS_COUNT = 8;

// Draw label based on type - with animation and variety
static void DrawLabel(PF_EffectWorld* world, PF_PixelFormat		format, int id, int index, float time, int x, int y, PF_Pixel8 color, int scale, int labelType) {
    char buf[32];
    int px = x;
    int charWidth = 4 * scale;
    int kanjiWidth = 9 * scale;

    // Animation cycle - changes every ~2 seconds
    int animCycle = (int)(time * 0.5f) + index;

    switch (labelType) {
    case LABEL_COORDINATES:
    case LABEL_RANDOM_HEX:
    default:
        // Animated hex - shifts over time
    {
        int animatedId = id + (int)(time * 10) * (index + 1);
        DrawHexLabel(world,format, animatedId, x, y, color, scale);
    }
    break;

    case LABEL_SEQUENTIAL:
        // TRK-001, LAB-002, etc - varies word and animates number
    {
        const char* word = SEQ_WORDS[(index + animCycle) % SEQ_WORDS_COUNT];
        int num = ((index + 1) + (int)(time * 2)) % 1000;
        snprintf(buf, sizeof(buf), "%s%03d", word, num);
        for (int i = 0; buf[i]; i++) {
            char c = buf[i];
            // Draw letters too (simple versions)
            if (c >= 'A' && c <= 'Z') {
                // Simple letter rendering - use hex chars for A-F, skip others
                if (c <= 'F') DrawChar(world, format,c, px, y, color, scale);
                else {
                    // Draw a dot for other letters
                    for (int sy = 0; sy < scale * 2; sy++)
                        for (int sx = 0; sx < scale * 2; sx++)
                            PlotPixel(world,format, px + sx, y + 2 * scale + sy, color);
                }
            }
            else {
                DrawChar(world,format, c, px, y, color, scale);
            }
            px += charWidth;
        }
    }
    break;

    case LABEL_TIMECODE:
        // Animated timecode - each node shows different time
    {
        int baseFrames = (int)(time * 24);
        int offset = index * 127 + (id % 100) * 37;  // Unique offset per node
        int frames = baseFrames + offset;
        int ff = frames % 24;
        int ss = (frames / 24) % 60;
        int mm = (frames / 24 / 60) % 60;
        int hh = (frames / 24 / 60 / 60) % 24;
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d:%02d", hh, mm, ss, ff);
        for (int i = 0; buf[i]; i++) {
            DrawChar(world,format, buf[i], px, y, color, scale);
            px += charWidth;
        }
    }
    break;

    case LABEL_BINARY:
        // Animated binary - shifts and varies per node
    {
        int animVal = (id * 17 + index * 31 + (int)(time * 8)) & 0xFF;
        for (int i = 7; i >= 0; i--) {
            DrawChar(world,format, (animVal & (1 << i)) ? '1' : '0', px, y, color, scale);
            px += charWidth;
        }
    }
    break;

    case LABEL_HEX_MEMORY:
        // Animated memory address
    {
        unsigned int addr = (unsigned int)((id << 16) | ((index * 0x1234 + (int)(time * 100)) & 0xFFFF));
        snprintf(buf, sizeof(buf), "0X%08X", addr);
        for (int i = 0; buf[i]; i++) {
            DrawChar(world,format, buf[i], px, y, color, scale);
            px += charWidth;
        }
    }
    break;

    case LABEL_JP_TRACK:
        // Cycle through Japanese tracking words
    {
        int wordIdx = (index + animCycle) % 4;
        const KanjiWord& word = JP_TRACK_WORDS[wordIdx];
        for (int i = 0; i < word.len; i++) {
            DrawKanji(world,format, word.chars[i], px, y, color, scale);
            px += kanjiWidth;
        }
    }
    break;

    case LABEL_JP_TARGET:
        // Cycle through Japanese target words
    {
        int wordIdx = (index + animCycle) % 3;
        const KanjiWord& word = JP_TARGET_WORDS[wordIdx];
        for (int i = 0; i < word.len; i++) {
            DrawKanji(world,format, word.chars[i], px, y, color, scale);
            px += kanjiWidth;
        }
    }
    break;

    case LABEL_KATAKANA:
        // Cycle through Katakana words
    {
        int wordIdx = (index + animCycle) % 6;
        const KanjiWord& word = KATA_WORDS[wordIdx];
        for (int i = 0; i < word.len; i++) {
            DrawKanji(world,format, word.chars[i], px, y, color, scale);
            px += kanjiWidth;
        }
    }
    break;

    case LABEL_CN_LOCK:
        // Cycle through Chinese words
    {
        int wordIdx = (index + animCycle) % 3;
        const KanjiWord& word = CN_LOCK_WORDS[wordIdx];
        for (int i = 0; i < word.len; i++) {
            DrawKanji(world,format, word.chars[i], px, y, color, scale);
            px += kanjiWidth;
        }
    }
    break;
    }
}

// --- BLOB STRUCTURE ---
struct Blob {
    float x, y;
    float w, h;
    int id;
    float targetX, targetY;  // For smooth interpolation
};

// --- TRACKING PERSISTENCE ---
static std::vector<Blob> g_cachedBlobs;
static int g_lastUpdateFrame = -999;
static int g_lastTargetR = -1, g_lastTargetG = -1, g_lastTargetB = -1;
static PF_Err Render(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
    PF_Err err = PF_Err_NONE;
    PF_EffectWorld *input = &params[0]->u.ld;
    err = PF_COPY(input, output, NULL, NULL);

    int sampleStep = 1;
  
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    PF_WorldSuite2* wsP = NULL;

    ERR(AEFX_AcquireSuite(in_data,
        out_data,
        kPFWorldSuite,
        kPFWorldSuiteVersion2,
        "Couldn't load suite.",
        (void**)&wsP));

    PF_PixelFormat		format = PF_PixelFormat_INVALID;

    PF_ParamDef inputLayerParam;
    ERR(PF_CHECKOUT_PARAM(in_data,
        0,
        in_data->current_time,
        in_data->time_step,
        in_data->time_scale,
        &inputLayerParam));
    PF_EffectWorld* input_worldP = NULL;
    input_worldP = &(inputLayerParam.u.ld);
    ERR(wsP->PF_GetPixelFormat(input_worldP, &format));

    memcpy((char*)output->data, (char*)input->data, input->height * input->rowbytes);

    if (!g_IsActivated)
    {
        for (int y = 0; y < output->height; y += sampleStep) {
            for (int x = 0; x < output->width; x += sampleStep) {
                ;
                if (isRedXPixel(x, y, output->width, output->height))
                {
                    PF_Pixel8 out_color;
                    out_color.red = 255;
                    out_color.green = 0;
                    out_color.blue = 0;
                    PlotPixel(output, format, x, y, out_color);
                }               
            }
        }
    }
    else
    {   
        // Get parameters
        PF_Pixel tgt = params[TRACKERLABS_TARGET_COLOR]->u.cd.value;
        float tolerance = params[TRACKERLABS_TOLERANCE]->u.fs_d.value;
        int maxObjects = params[TRACKERLABS_MAX_OBJECTS]->u.sd.value;
        float boxSize = params[TRACKERLABS_TARGET_SIZE]->u.fs_d.value;

        // Base colors
        PF_Pixel8 lineColor;
        lineColor.red = params[TRACKERLABS_LINE_COLOR]->u.cd.value.red;
        lineColor.green = params[TRACKERLABS_LINE_COLOR]->u.cd.value.green;
        lineColor.blue = params[TRACKERLABS_LINE_COLOR]->u.cd.value.blue;
        lineColor.alpha = 255;

        PF_Pixel8 nodeColor;
        nodeColor.red = params[TRACKERLABS_NODE_COLOR]->u.cd.value.red;
        nodeColor.green = params[TRACKERLABS_NODE_COLOR]->u.cd.value.green;
        nodeColor.blue = params[TRACKERLABS_NODE_COLOR]->u.cd.value.blue;
        nodeColor.alpha = 255;

        PF_Pixel8 textColor;
        textColor.red = params[TRACKERLABS_TEXT_COLOR]->u.cd.value.red;
        textColor.green = params[TRACKERLABS_TEXT_COLOR]->u.cd.value.green;
        textColor.blue = params[TRACKERLABS_TEXT_COLOR]->u.cd.value.blue;
        textColor.alpha = 255;

        // Tri-tone colors
        bool triToneEnabled = params[TRACKERLABS_TRITONE_ENABLED]->u.bd.value != 0;
        PF_Pixel8 triColors[3];
        triColors[0] = nodeColor;  // Color 1 is the node/line color
        triColors[1].red = params[TRACKERLABS_COLOR_2]->u.cd.value.red;
        triColors[1].green = params[TRACKERLABS_COLOR_2]->u.cd.value.green;
        triColors[1].blue = params[TRACKERLABS_COLOR_2]->u.cd.value.blue;
        triColors[1].alpha = 255;
        triColors[2].red = params[TRACKERLABS_COLOR_3]->u.cd.value.red;
        triColors[2].green = params[TRACKERLABS_COLOR_3]->u.cd.value.green;
        triColors[2].blue = params[TRACKERLABS_COLOR_3]->u.cd.value.blue;
        triColors[2].alpha = 255;

        // Detection parameters
        int W = output->width;
        int H = output->height;
        int marginX = W / 20;  // Smaller margin for more coverage
        int marginY = H / 20;
        int step = (maxObjects > 30) ? 2 : 4;  // Finer scan for high object counts
        float clusterRadius = (maxObjects > 30) ? 20.0f : 40.0f;  // Smaller clusters for chaos mode

        // Target RGB values
        int tgtR = tgt.red;
        int tgtG = tgt.green;
        int tgtB = tgt.blue;

        // Speed parameter - MAX PIXELS a box can move per frame
        float speedPercent = params[TRACKERLABS_TRACKING_SPEED]->u.fs_d.value;
        float maxMovePerFrame = (speedPercent >= 99.0f) ? 99999.0f : (speedPercent * 2.0f + 5.0f);

        // STEP 1: Find ALL matching pixels
        std::vector<std::pair<float, float>> matchingPixels;

        for (int py = marginY; py < H - marginY; py += step) {
            for (int px = marginX; px < W - marginX; px += step) {
                PF_Pixel8 pixel;

                if (format == PF_PixelFormat_ARGB32)
                {
                    PF_Pixel8* src_pixel = (PF_Pixel8*)((char*)input->data + py * input->rowbytes + px * sizeof(PF_Pixel8));

                    pixel.red = src_pixel->red;
                    pixel.green = src_pixel->green;
                    pixel.blue = src_pixel->blue;
                }
                else if (format == PF_PixelFormat_ARGB64)
                {
                    PF_Pixel16* src_pixel= (PF_Pixel16*)((char*)input->data + py * input->rowbytes + px * sizeof(PF_Pixel16));
                    pixel.red = (unsigned char)(PF_MAX_CHAN8*src_pixel->red / (float)PF_MAX_CHAN16);
                    pixel.green = (unsigned char)(PF_MAX_CHAN8 * src_pixel->green / (float)PF_MAX_CHAN16);
                    pixel.blue = (unsigned char)(PF_MAX_CHAN8 * src_pixel->blue / (float)PF_MAX_CHAN16);
                }
                else if (format == PF_PixelFormat_ARGB128)
                {
                    PF_PixelFloat* src_pixel = (PF_PixelFloat*)((char*)input->data + py * input->rowbytes + px * sizeof(PF_PixelFloat));
                
                    pixel.red = (unsigned char)(PF_MAX_CHAN8 * src_pixel->red);
                    pixel.green = (unsigned char)(PF_MAX_CHAN8 * src_pixel->green);
                    pixel.blue = (unsigned char)(PF_MAX_CHAN8 * src_pixel->blue);
                }
                int dr = pixel.red - tgtR;
                int dg = pixel.green - tgtG;
                int db = pixel.blue - tgtB;
                float dist = sqrtf((float)(dr * dr + dg * dg + db * db));

                if (dist < tolerance) {
                    matchingPixels.push_back({ (float)px, (float)py });
                }
            }
        }

        // STEP 2: Cluster matching pixels into blobs (find centroids)
        std::vector<Blob> detectedBlobs;
        std::vector<bool> used(matchingPixels.size(), false);

        for (size_t i = 0; i < matchingPixels.size(); i++) {
            if (used[i]) continue;

            // Start a new cluster
            float sumX = matchingPixels[i].first;
            float sumY = matchingPixels[i].second;
            int count = 1;
            used[i] = true;

            // Find all nearby pixels and add to cluster
            for (size_t j = i + 1; j < matchingPixels.size(); j++) {
                if (used[j]) continue;
                float dx = matchingPixels[j].first - matchingPixels[i].first;
                float dy = matchingPixels[j].second - matchingPixels[i].second;
                if (sqrtf(dx * dx + dy * dy) < clusterRadius) {
                    sumX += matchingPixels[j].first;
                    sumY += matchingPixels[j].second;
                    count++;
                    used[j] = true;
                }
            }

            // Create blob at centroid
            Blob blob;
            blob.x = sumX / count;
            blob.y = sumY / count;
            blob.w = boxSize;
            blob.h = boxSize;
            blob.id = ((int)(blob.x / 20) * 31 + (int)(blob.y / 20) * 17) & 0xFFFF;
            detectedBlobs.push_back(blob);
        }

        // STEP 3: Select blobs spread across the frame
        std::vector<Blob> targets;

        if (!detectedBlobs.empty()) {
            // Minimum spacing - scale down for high object counts (chaos mode!)
            float minSpacing;
            if (maxObjects >= 50) {
                minSpacing = 15.0f;  // Chaos mode - very tight spacing allowed
            }
            else if (maxObjects >= 20) {
                minSpacing = 30.0f;  // Dense mode
            }
            else {
                minSpacing = fmaxf(W, H) / (float)(maxObjects + 1) * 0.5f;
                if (minSpacing < 50.0f) minSpacing = 50.0f;
            }

            // First, pick one blob near each region of the frame
            // Divide frame into grid and pick best blob from each cell
            int gridCols = (int)ceilf(sqrtf((float)maxObjects * W / H));
            int gridRows = (int)ceilf((float)maxObjects / gridCols);
            float cellW = (float)W / gridCols;
            float cellH = (float)H / gridRows;

            for (int gy = 0; gy < gridRows && (int)targets.size() < maxObjects; gy++) {
                for (int gx = 0; gx < gridCols && (int)targets.size() < maxObjects; gx++) {
                    float cellCenterX = (gx + 0.5f) * cellW;
                    float cellCenterY = (gy + 0.5f) * cellH;

                    // Find blob closest to this cell center
                    float bestDist = 999999.0f;
                    int bestIdx = -1;
                    for (size_t i = 0; i < detectedBlobs.size(); i++) {
                        float dx = detectedBlobs[i].x - cellCenterX;
                        float dy = detectedBlobs[i].y - cellCenterY;
                        float d = sqrtf(dx * dx + dy * dy);

                        // Check not too close to already selected targets
                        bool tooClose = false;
                        for (auto& t : targets) {
                            float td = sqrtf((detectedBlobs[i].x - t.x) * (detectedBlobs[i].x - t.x) +
                                (detectedBlobs[i].y - t.y) * (detectedBlobs[i].y - t.y));
                            if (td < minSpacing) { tooClose = true; break; }
                        }

                        if (!tooClose && d < bestDist) {
                            bestDist = d;
                            bestIdx = (int)i;
                        }
                    }

                    if (bestIdx >= 0) {
                        targets.push_back(detectedBlobs[bestIdx]);
                    }
                }
            }

            // If we still need more targets and have detected blobs, add them more aggressively
            if ((int)targets.size() < maxObjects && detectedBlobs.size() > targets.size()) {
                std::vector<bool> used(detectedBlobs.size(), false);
                for (auto& t : targets) {
                    for (size_t i = 0; i < detectedBlobs.size(); i++) {
                        if (detectedBlobs[i].x == t.x && detectedBlobs[i].y == t.y) {
                            used[i] = true;
                            break;
                        }
                    }
                }
                // Add remaining blobs with minimal spacing check
                for (size_t i = 0; i < detectedBlobs.size() && (int)targets.size() < maxObjects; i++) {
                    if (used[i]) continue;
                    bool tooClose = false;
                    for (auto& t : targets) {
                        float td = sqrtf((detectedBlobs[i].x - t.x) * (detectedBlobs[i].x - t.x) +
                            (detectedBlobs[i].y - t.y) * (detectedBlobs[i].y - t.y));
                        if (td < minSpacing * 0.5f) { tooClose = true; break; }
                    }
                    if (!tooClose) {
                        targets.push_back(detectedBlobs[i]);
                    }
                }
            }
        }

        // STEP 4: Stable matching - each box tracks nearest target consistently
        std::vector<Blob> blobs;

        if (g_cachedBlobs.empty() || targets.empty()) {
            blobs = targets;
            // Assign IDs
            for (size_t i = 0; i < blobs.size(); i++) {
                blobs[i].id = ((int)(blobs[i].x / 20) * 31 + (int)(blobs[i].y / 20) * 17) & 0xFFFF;
            }
        }
        else {
            // Keep existing boxes, update toward new targets
            blobs = g_cachedBlobs;

            // Adjust count
            while (blobs.size() < targets.size()) {
                Blob newBlob = targets[blobs.size()];
                newBlob.id = ((int)(newBlob.x / 20) * 31 + (int)(newBlob.y / 20) * 17) & 0xFFFF;
                blobs.push_back(newBlob);
            }
            while (blobs.size() > targets.size() && !blobs.empty()) {
                blobs.pop_back();
            }

            // Better matching: sort targets by x then y for consistency
            std::vector<Blob> sortedTargets = targets;
            std::sort(sortedTargets.begin(), sortedTargets.end(), [](const Blob& a, const Blob& b) {
                if (abs(a.y - b.y) > 50) return a.y < b.y;
                return a.x < b.x;
                });

            // Sort blobs the same way
            std::sort(blobs.begin(), blobs.end(), [](const Blob& a, const Blob& b) {
                if (abs(a.y - b.y) > 50) return a.y < b.y;
                return a.x < b.x;
                });

            // Now match sorted lists
            for (size_t i = 0; i < blobs.size() && i < sortedTargets.size(); i++) {
                float targetX = sortedTargets[i].x;
                float targetY = sortedTargets[i].y;
                float dx = targetX - blobs[i].x;
                float dy = targetY - blobs[i].y;
                float distance = sqrtf(dx * dx + dy * dy);

                if (distance <= maxMovePerFrame || speedPercent >= 99.0f) {
                    blobs[i].x = targetX;
                    blobs[i].y = targetY;
                }
                else {
                    float ratio = maxMovePerFrame / distance;
                    blobs[i].x += dx * ratio;
                    blobs[i].y += dy * ratio;
                }
            }
        }

        // Save for next frame
        g_cachedBlobs = blobs;

        // STEP 3: Draw connections based on type
        bool linesEnabled = params[TRACKERLABS_LINES_ENABLED]->u.bd.value != 0;
        int connType = params[TRACKERLABS_CONN_TYPE]->u.pd.value;  // 1=Lines, 2=Dotted, 3=None
        float lineWidth = params[TRACKERLABS_LINE_WIDTH]->u.fs_d.value;
        float lineCurvature = params[TRACKERLABS_LINE_CURVATURE]->u.fs_d.value;
        float fadeByDistance = params[TRACKERLABS_FADE_DIST]->u.fs_d.value;
        float maxDist = params[TRACKERLABS_MAX_DIST]->u.fs_d.value;
        int maxConnections = params[TRACKERLABS_MAX_CONNECTIONS]->u.sd.value;
        float connChance = params[TRACKERLABS_CONN_CHANCE]->u.fs_d.value / 100.0f;  // Convert to 0-1

        // Track connection count per node
        std::vector<int> connectionCount(blobs.size(), 0);

        // Only draw if enabled and not "None"
        if (linesEnabled && connType != 3) {

            float curvature01 = lineCurvature / 100.0f;
            bool useCurved = curvature01 > 0.0f;
            float fade01 = fadeByDistance / 100.0f;
            if (fade01 < 0.0f) fade01 = 0.0f;
            if (fade01 > 1.0f) fade01 = 1.0f;
            float invMaxDist = (maxDist > 0.0f) ? (1.0f / maxDist) : 0.0f;

            for (size_t i = 0; i < blobs.size(); i++) {
                for (size_t j = i + 1; j < blobs.size(); j++) {
                    // Check if either node has reached max connections
                    if (connectionCount[i] >= maxConnections || connectionCount[j] >= maxConnections) {
                        continue;
                    }

                    // Calculate distance between nodes
                    float dx = blobs[j].x - blobs[i].x;
                    float dy = blobs[j].y - blobs[i].y;
                    float dist = sqrtf(dx * dx + dy * dy);

                    // Skip if beyond max distance
                    if (dist > maxDist) {
                        continue;
                    }

                    // Fade based on normalized distance (0..1)
                    float fadeFactor = 1.0f - fade01 * (dist * invMaxDist);
                    if (fadeFactor < 0.0f) fadeFactor = 0.0f;
                    if (fadeFactor > 1.0f) fadeFactor = 1.0f;
                    bool drawThisConnection = fadeFactor > 0.001f;

                    // Apply connection chance (use deterministic pseudo-random based on IDs for stability)
                    if (connChance < 1.0f) {
                        int seed = (blobs[i].id * 31 + blobs[j].id * 17) & 0xFFFF;
                        float chance = (seed % 1000) / 1000.0f;
                        if (chance > connChance) {
                            continue;
                        }
                    }

                    // Draw the connection - pick color based on first node for tri-tone
                    PF_Pixel8 thisLineColor = lineColor;
                    if (triToneEnabled) {
                        int colorIdx = (blobs[i].id * 13 + (int)i * 11) % 3;
                        thisLineColor = triColors[colorIdx];
                    }

                    // Apply fade by distance to the line color (no alpha blending in this buffer).
                    thisLineColor.red *= fadeFactor;
                    thisLineColor.green *= fadeFactor;
                    thisLineColor.blue *= fadeFactor;

                    if (drawThisConnection) {
                        if (connType == 1) {
                            // Solid lines
                            int curveSign = ((blobs[i].id + blobs[j].id) & 1) ? -1 : 1;
                            if (useCurved) {
                                DrawCurvedLine(output,format, (int)blobs[i].x, (int)blobs[i].y,
                                    (int)blobs[j].x, (int)blobs[j].y, thisLineColor, lineWidth, curvature01,curveSign);
                            }
                            else {

                                DrawLine(output,format, (int)blobs[i].x, (int)blobs[i].y,
                                    (int)blobs[j].x, (int)blobs[j].y, thisLineColor, lineWidth);
                            }
                        }
                        else if (connType == 2) {
                            // Dotted lines (8px dash, 8px gap)
                            int curveSign = ((blobs[i].id + blobs[j].id) & 1) ? -1 : 1;
                            if (useCurved) {

                                DrawCurvedDottedLine(output,format, (int)blobs[i].x, (int)blobs[i].y,
                                    (int)blobs[j].x, (int)blobs[j].y, thisLineColor, 8, 8, step, lineWidth, curvature01, curveSign);

                            }
                            else {

                                DrawDottedLine(output,format, (int)blobs[i].x, (int)blobs[i].y,
                                    (int)blobs[j].x, (int)blobs[j].y, thisLineColor, 8, 8, step, lineWidth);
                            }
                        }
                    }

                    // Increment connection counts
                    connectionCount[i]++;
                    connectionCount[j]++;
                }
            }
        }

        // STEP 4: Draw shapes and labels
        float currentTime = (float)in_data->current_time / (float)in_data->time_scale;

        // Get shape settings
        int nodeShape = params[TRACKERLABS_NODE_SHAPE]->u.pd.value;  // 1-6
        float nodeSize = params[TRACKERLABS_NODE_SIZE]->u.fs_d.value;
        float fillOpacity = params[TRACKERLABS_FILL_OPACITY]->u.fs_d.value;
        int fillAlpha = (int)(fillOpacity * 2.55f);  // 0-255
        int textScale = (int)params[TRACKERLABS_TEXT_SIZE]->u.fs_d.value;
        if (textScale < 1) textScale = 1;
        bool showLabels = params[TRACKERLABS_SHOW_LABELS]->u.bd.value != 0;
        int labelType = params[TRACKERLABS_LABEL_CONTENT]->u.pd.value;
        float invertFillPercent = params[TRACKERLABS_INVERT_FILL]->u.fs_d.value;

        for (size_t i = 0; i < blobs.size(); i++) {
            Blob& b = blobs[i];

            // Pick color for this node (tri-tone or single)
            PF_Pixel8 thisNodeColor = nodeColor;
            if (triToneEnabled) {
                int colorIdx = (b.id * 17 + (int)i * 7) % 3;
                thisNodeColor = triColors[colorIdx];
            }

            // Create dramatic size variation - some boxes much larger than others
            int sizeClass = (b.id * 7) % 10;  // 0-9
            float sizeVariation;
            if (sizeClass < 2) {
                sizeVariation = 2.5f + ((b.id % 50) / 50.0f) * 1.5f;
            }
            else if (sizeClass < 5) {
                sizeVariation = 1.2f + ((b.id % 40) / 40.0f) * 0.8f;
            }
            else {
                sizeVariation = 0.5f + ((b.id % 30) / 30.0f) * 0.5f;
            }

            float baseSize = boxSize * sizeVariation;

            // Oscillate between taller and wider over time
            float phase = (float)i * 0.7f;
            float oscillation = sinf(currentTime * 1.5f + phase);

            float aspectRange = (sizeClass < 2) ? 0.5f : 0.35f;
            float aspectShift = 1.0f + oscillation * aspectRange;

            int halfW = (int)(baseSize * aspectShift / 2.0f);
            int halfH = (int)(baseSize / aspectShift / 2.0f);

            // Minimum sizes
            if (halfW < 15) halfW = 15;
            if (halfH < 12) halfH = 12;

            int radius = (halfW + halfH) / 2;

            // Draw shape based on selection
            switch (nodeShape) {
            case 1:  // Filled Circle
                DrawFilledCircle(output, format,(int)b.x, (int)b.y, radius, thisNodeColor, fillAlpha);
                break;
            case 2:  // Circle Outline
                DrawCircleOutline(output, format,(int)b.x, (int)b.y, radius, thisNodeColor, nodeSize);
                break;
            case 3:  // Filled Box
                DrawFilledBox(output,format,(int)b.x, (int)b.y, halfW, halfH, thisNodeColor, fillAlpha);
                break;
            case 4:  // Outlined Box
            {
                // Check if this box should have inverted fill
                bool shouldInvert = false;
                if (invertFillPercent >= 100.0f) {
                    shouldInvert = true;  // 100% = all boxes inverted
                }
                else if (invertFillPercent > 0.0f) {
                    // Use deterministic pseudo-random based on node ID for stability
                    int invertRand = (b.id * 59 + (int)i * 43) % 100;
                    shouldInvert = (invertRand < (int)invertFillPercent);
                }

                // Invert the inside of the box first (before drawing outline)
                if (shouldInvert) {
                    InvertBoxRegion(output, format,(int)b.x, (int)b.y, halfW, halfH);
                }

                // Then draw the box outline on top
                DrawBox(output,format, (int)b.x, (int)b.y, halfW, halfH, thisNodeColor, nodeSize);
            }
            break;
            case 5:  // Text Only - no shape drawn
                break;
            case 6:  // None - no shape or label
                continue;  // Skip label too
            }

            // Draw label (if enabled and not None shape)
            // Label Density controls what percentage of nodes show labels
            float labelDensity = params[TRACKERLABS_LABEL_DENSITY]->u.fs_d.value;
            bool showThisLabel = false;

            if (showLabels && labelDensity > 0) {
                if (labelDensity >= 100.0f) {
                    showThisLabel = true;  // 100% = all labels
                }
                else {
                    // Use deterministic random based on node ID for stability
                    int labelRand = (b.id * 73 + (int)i * 37) % 100;
                    showThisLabel = (labelRand < (int)labelDensity);
                }
            }

            if (showThisLabel) {
                int labelX, labelY;
                int actualTextScale = textScale;

                if (nodeShape == 5) {
                    // Text Only - vary size per node for visual interest
                    int sizeVar = (b.id * 13 + (int)i * 7) % 100;
                    float scaleMult;
                    if (sizeVar < 15) {
                        scaleMult = 2.5f + (sizeVar / 15.0f) * 1.0f;  // Large: 2.5x - 3.5x
                    }
                    else if (sizeVar < 40) {
                        scaleMult = 1.5f + ((sizeVar - 15) / 25.0f) * 0.8f;  // Medium: 1.5x - 2.3x
                    }
                    else if (sizeVar < 70) {
                        scaleMult = 1.0f + ((sizeVar - 40) / 30.0f) * 0.4f;  // Normal: 1.0x - 1.4x
                    }
                    else {
                        scaleMult = 0.6f + ((sizeVar - 70) / 30.0f) * 0.3f;  // Small: 0.6x - 0.9x
                    }
                    actualTextScale = (int)(textScale * scaleMult);
                    if (actualTextScale < 1) actualTextScale = 1;

                    // Center text at the tracking point
                    int textWidth = 6 * 4 * actualTextScale;
                    labelX = (int)b.x - textWidth / 2;
                    labelY = (int)b.y - (5 * actualTextScale) / 2;
                }
                else {
                    // Other shapes - text above the shape
                    labelX = (int)b.x - halfW;
                    labelY = (int)b.y - halfH - 8 * actualTextScale;
                }
                DrawLabel(output,format, b.id, (int)i, currentTime, labelX, labelY, textColor, actualTextScale, labelType);
            }
        }
    }
   
    return err;
}

static PF_Err ParamsSetup(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
    PF_Err err = PF_Err_NONE;
    PF_ParamDef def;

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDER("Master Intensity %", 0, 100, 0, 100, 0, 100.0, 1, 0, 0, TRACKERLABS_MASTER_INTENSITY);

    AEFX_CLR_STRUCT(def); PF_ADD_TOPIC("Tracking Engine", TRACKERLABS_TOPIC_TRACKING);
    AEFX_CLR_STRUCT(def); PF_ADD_COLOR("Target Color", 255, 100, 100, TRACKERLABS_TARGET_COLOR);
    AEFX_CLR_STRUCT(def); PF_ADD_FLOAT_SLIDER("Color Tolerance", 0, 255, 0, 255, 0, 80.0, 1, 0, 0, TRACKERLABS_TOLERANCE);
    AEFX_CLR_STRUCT(def); PF_ADD_FLOAT_SLIDER("Min Blob Size", 0, 500, 0, 500, 0, 45.0, 1, 0, 0, TRACKERLABS_MIN_BLOB_SIZE);
    AEFX_CLR_STRUCT(def); PF_ADD_SLIDER("Max Objects", 1, 100, 1, 100, 5, TRACKERLABS_MAX_OBJECTS);
    AEFX_CLR_STRUCT(def); PF_ADD_FLOAT_SLIDER("Tracking Speed %", 0, 100, 0, 100, 0, 100.0, 1, 0, 0, TRACKERLABS_TRACKING_SPEED);
    PF_END_TOPIC(TRACKERLABS_TOPIC_TRACKING_END);

    AEFX_CLR_STRUCT(def); PF_ADD_TOPIC("Connections", TRACKERLABS_TOPIC_CONNECTIONS);
    AEFX_CLR_STRUCT(def); PF_ADD_CHECKBOX("Show Lines", "Enabled", TRUE, 0, TRACKERLABS_LINES_ENABLED);
    AEFX_CLR_STRUCT(def); PF_ADD_POPUP("Type", 3, 1, "Lines|Dotted|None", TRACKERLABS_CONN_TYPE);
    AEFX_CLR_STRUCT(def); PF_ADD_FLOAT_SLIDER("Max Distance", 0, 2000, 0, 2000, 0, 2000.0, 1, 0, 0, TRACKERLABS_MAX_DIST);
    AEFX_CLR_STRUCT(def); PF_ADD_SLIDER("Max Connections", 1, 20, 1, 20, 10, TRACKERLABS_MAX_CONNECTIONS);
    AEFX_CLR_STRUCT(def); PF_ADD_FLOAT_SLIDER("Connection Chance %", 0, 100, 0, 100, 0, 100.0, 1, 0, 0, TRACKERLABS_CONN_CHANCE);
    PF_END_TOPIC(TRACKERLABS_TOPIC_CONNECTIONS_END);

    AEFX_CLR_STRUCT(def); PF_ADD_TOPIC("Visuals", TRACKERLABS_TOPIC_VISUALS);
    AEFX_CLR_STRUCT(def); PF_ADD_CHECKBOX("Tri-Tone Colors", "Enabled", FALSE, 0, TRACKERLABS_TRITONE_ENABLED);
    AEFX_CLR_STRUCT(def); PF_ADD_COLOR("Color 2", 0, 200, 255, TRACKERLABS_COLOR_2);
    AEFX_CLR_STRUCT(def); PF_ADD_COLOR("Color 3", 255, 100, 200, TRACKERLABS_COLOR_3);
    AEFX_CLR_STRUCT(def); PF_ADD_FLOAT_SLIDER("Line Width", 0, 50, 0, 50, 0, 1.0, 1, 0, 0, TRACKERLABS_LINE_WIDTH);
    AEFX_CLR_STRUCT(def); PF_ADD_COLOR("Line Color", 255, 255, 255, TRACKERLABS_LINE_COLOR);
    AEFX_CLR_STRUCT(def); PF_ADD_FLOAT_SLIDER("Line Curvature %", 0, 100, 0, 100, 0, 0.0, 1, 0, 0, TRACKERLABS_LINE_CURVATURE);
    AEFX_CLR_STRUCT(def); PF_ADD_FLOAT_SLIDER("Fade by Distance %", 0, 100, 0, 100, 0, 0, 1, 0, 0, TRACKERLABS_FADE_DIST);
    AEFX_CLR_STRUCT(def); PF_ADD_FLOAT_SLIDER("Node Size", 0, 50, 0, 50, 0, 2.0, 1, 0, 0, TRACKERLABS_NODE_SIZE);
    AEFX_CLR_STRUCT(def); PF_ADD_COLOR("Node Color", 255, 255, 255, TRACKERLABS_NODE_COLOR);
    AEFX_CLR_STRUCT(def); PF_ADD_POPUP("Node Shape", 6, 4, "Filled Circle|Circle Outline|Filled Box|Outlined Box|Text Only|None", TRACKERLABS_NODE_SHAPE);
    AEFX_CLR_STRUCT(def); PF_ADD_FLOAT_SLIDER("Fill Opacity %", 0, 100, 0, 100, 0, 50.0, 1, 0, 0, TRACKERLABS_FILL_OPACITY);
    AEFX_CLR_STRUCT(def); PF_ADD_FLOAT_SLIDER("Box Padding %", -50, 200, -50, 200, 0, 0.0, 1, 0, 0, TRACKERLABS_BOX_PADDING);
    AEFX_CLR_STRUCT(def); PF_ADD_CHECKBOX("Uniform Box Size", "Force Uniform", FALSE, 0, TRACKERLABS_UNIFORM_BOX);
    AEFX_CLR_STRUCT(def); PF_ADD_FLOAT_SLIDER("Target Size", 5, 300, 5, 300, 0, 60.0, 1, 0, 0, TRACKERLABS_TARGET_SIZE);
    AEFX_CLR_STRUCT(def); PF_ADD_FLOAT_SLIDER("Label Density %", 0, 100, 0, 100, 0, 23.0, 1, 0, 0, TRACKERLABS_LABEL_DENSITY);
    AEFX_CLR_STRUCT(def); PF_ADD_POPUP("Label Content", 11, 2, "Coordinates|Random Hex|Custom|Sequential 001|Timecode|Binary|Hex Memory|Japanese Track|Japanese Target|Katakana|Chinese Lock", TRACKERLABS_LABEL_CONTENT);
    AEFX_CLR_STRUCT(def); PF_ADD_CHECKBOX("Show Labels", "Enabled", TRUE, 1, TRACKERLABS_SHOW_LABELS);
    AEFX_CLR_STRUCT(def); PF_ADD_FLOAT_SLIDER("Text Size", 1, 20, 1, 20, 0, 2.0, 1, 0, 0, TRACKERLABS_TEXT_SIZE);
    AEFX_CLR_STRUCT(def); PF_ADD_COLOR("Text Color", 255, 255, 255, TRACKERLABS_TEXT_COLOR);
    AEFX_CLR_STRUCT(def); PF_ADD_FLOAT_SLIDER("Invert Fill %", 0, 100, 0, 100, 0, 0.0, 1, 0, 0, TRACKERLABS_INVERT_FILL);
    PF_END_TOPIC(TRACKERLABS_TOPIC_VISUALS_END);

    PF_ADD_BUTTON("",
        "License",
        0,
        PF_ParamFlag_SUPERVISE, // Critical to receive change events
        TRACKERLABS_LICENSE);

    out_data->num_params = TRACKERLABS_NUM_PARAMS;
    return err;
}

// MINIMAL EffectMain - WITH GlobalSetup to match PiPL version
static PF_Err GlobalSetup(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
    // Set version to EXACTLY match PiPL: 524289
    out_data->my_version = 524289;
    out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE;
    out_data->out_flags2 = 0;
    strncpy(out_data->name, TRACKERLABS_DISPLAY_NAME, sizeof(out_data->name));
    return PF_Err_NONE;
}
void trim(std::string& s) {
    // Trim from start
    size_t first = s.find_first_not_of(" \t\n\r");
    if (std::string::npos == first) {
        s.clear();
        return;
    }
    // Trim from end
    size_t last = s.find_last_not_of(" \t\n\r");
    s = s.substr(first, (last - first + 1));
}
std::string strip_zero_width(const std::string& input) {
    // UTF-8 hex patterns for common zero-width characters:
    // U+200B (Zero Width Space): \xE2\x80\x8B
    // U+200C (Zero Width Non-Joiner): \xE2\x80\x8C
    // U+200D (Zero Width Joiner): \xE2\x80\x8D
    // U+FEFF (Byte Order Mark): \xEF\xBB\xBF

    // Create a regex to match any of these byte sequences
    std::regex zw_regex("(\xE2\x80[\x8B-\x8D]|\xEF\xBB\xBF)");

    // Replace all matches with an empty string
    return std::regex_replace(input, zw_regex, "");
}
static PF_Err
UserChangedParam(
    PF_InData* in_data,
    PF_OutData* out_data,
    PF_ParamDef* params[],
    PF_LayerDef* outputP,
    const PF_UserChangedParamExtra* which_hitP)
{
    PF_Err	err = PF_Err_NONE;
    A_Err	err2 = A_Err_NONE;

    AEGP_SuiteHandler		suites(in_data->pica_basicP);




    if (which_hitP->param_index == TRACKERLABS_LICENSE)
    {
        // 1. Get the AEGP Utility Suite
        AEGP_SuiteHandler suites(in_data->pica_basicP);
        AEGP_MemHandle result_memH = NULL;

        // 2. Define the Script (with logic to return the value)
        // We wrap the ScriptUI in a function that returns the input text
        const char* script =
            "function sanitizeLicense(str){"
            "return str.toUpperCase();"
            "}"
            "function getLicense() {"
            "  var win = new Window('dialog', 'Plugin Registration');"
            "  win.orientation = 'column';"
            "  win.add('statictext', undefined, 'Please enter your license key:');"
            "  var input = win.add('edittext', [0,0,200,20], '');"
            "  var btnGroup = win.add('group');"
            "  btnGroup.add('button', undefined, 'Cancel', {name:'cancel'});"
            "  btnGroup.add('button', undefined, 'Register', {name:'ok'});"
            "  if (win.show() == 1) return sanitizeLicense(input.text);"
            "  return 'CANCEL_PRESSED';"
            "}"
            "getLicense();";

        if (in_data->appl_id != 'PrMr')
        {
            // 3. Execute the Script
            A_Err err = suites.UtilitySuite6()->AEGP_ExecuteScript(NULL, script, false, &result_memH, NULL);

            // 4. Retrieve the String Result
            if (!err && result_memH) {
                char* result_str = nullptr;
                suites.MemorySuite1()->AEGP_LockMemHandle(result_memH, (void**)&result_str);

                if (result_str && strcmp(result_str, "CANCEL_PRESSED") != 0) {

                    std::string license = result_str;
                    trim(license);
                    strip_zero_width(license);
                    license = std::regex_replace(license, std::regex("\\s+"), "");

                    std::string prefix = "TRACK";
                    std::string suffix = "TT";
                    std::string patternStr = prefix + R"([A-Z0-9]{16})" + suffix;
                    std::regex licensePattern(patternStr);

                    int res_code = 501;
                    if (std::regex_match(license, licensePattern))
                    {
                        res_code = useVerifyOnline(license.c_str(), true);
                    }                     
                    if (res_code == 0) g_IsActivated = true;
                    char* err_message = "alert('Your message here');";
                    switch (res_code)
                    {
                    case 501:
                        err_message = "alert('Invalid license key format');";
                        break;
                    case 500:
                        err_message = "alert('Unknown error verifying license. Please contact support.');";
                        break;
                    case 502:
                        err_message = "alert('License key not found.');";
                        break;
                    case 509:
                        err_message = "alert('Maximum number of activations reached.');";
                        break;
                    case 503:
                        err_message = "alert('Network error. Check your internet connection.');";
                        break;
                    case 0:
                        err_message = "alert('Licesne was activated.');";
                        break;
                    }

                    suites.UtilitySuite6()->AEGP_ExecuteScript(NULL, err_message, false, &result_memH, NULL);
                }

                suites.MemorySuite1()->AEGP_UnlockMemHandle(result_memH);
                suites.MemorySuite1()->AEGP_FreeMemHandle(result_memH);
            }
        }
        else
        {
        }
       
       
    }


    out_data->out_flags |= PF_OutFlag_REFRESH_UI | PF_OutFlag_FORCE_RERENDER;

    return err;
}
extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd, PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output, void* extra) {
    PF_Err err = PF_Err_NONE;
    try {
        switch (cmd) {
            case PF_Cmd_GLOBAL_SETUP:
                if (useVerifyOffline() == 0)
                {
                    g_IsActivated = true;
                }
                err = GlobalSetup(in_data, out_data, params, output);
                break;
            case PF_Cmd_PARAMS_SETUP: 
                err = ParamsSetup(in_data, out_data, params, output); 
                break;
            case PF_Cmd_RENDER:
                err = Render(in_data, out_data, params, output); 
                break;
            case PF_Cmd_USER_CHANGED_PARAM:
                ERR(UserChangedParam(in_data,
                    out_data,
                    params,
                    output,
                    reinterpret_cast<const PF_UserChangedParamExtra*>(extra)));
                break;
        }
    } catch (...) { err = PF_Err_INTERNAL_STRUCT_DAMAGED; }
    return err;
}


