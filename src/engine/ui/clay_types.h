// clay_types.h - Clay UI types, enums, and macros only
// Include this in headers that need Clay types without the implementation

#ifndef CLAY_TYPES_H
#define CLAY_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// BASIC TYPES
// =============================================================================

typedef struct ClayColor { 
    unsigned char r, g, b, a; 
} ClayColor;

typedef struct ClayVec2 { 
    float x, y; 
} ClayVec2;

typedef struct ClayRect { 
    float x, y, width, height; 
} ClayRect;

typedef struct ClayString {
    const char *chars;
    int32_t length;
} ClayString;

// =============================================================================
// STRING HELPERS
// =============================================================================

#define CLAY_STR(s) ((ClayString){ .chars = (s), .length = (int32_t)(sizeof(s) - 1) })

static inline ClayString ClayStr(const char *s) {
    return (ClayString){ .chars = s, .length = s ? (int32_t)strlen(s) : 0 };
}

// =============================================================================
// LAYOUT ENUMS
// =============================================================================

typedef enum ClayDirection {
    CLAY_LEFT_TO_RIGHT = 0,
    CLAY_TOP_TO_BOTTOM = 1
} ClayDirection;

typedef enum ClayAlignX {
    CLAY_ALIGN_X_LEFT = 0,
    CLAY_ALIGN_X_CENTER = 1,
    CLAY_ALIGN_X_RIGHT = 2
} ClayAlignX;

typedef enum ClayAlignY {
    CLAY_ALIGN_Y_TOP = 0,
    CLAY_ALIGN_Y_CENTER = 1,
    CLAY_ALIGN_Y_BOTTOM = 2
} ClayAlignY;

typedef enum ClaySizing {
    CLAY_SIZE_FIT = 0,
    CLAY_SIZE_GROW = 1,
    CLAY_SIZE_FIXED = 2,
    CLAY_SIZE_PERCENT = 3
} ClaySizing;

// =============================================================================
// ELEMENT OPTIONS
// =============================================================================

typedef struct ClayCorners {
    float topLeft, topRight, bottomLeft, bottomRight;
} ClayCorners;

typedef struct ClayPadding {
    uint16_t left, right, top, bottom;
} ClayPadding;

typedef struct ClaySizeAxis {
    ClaySizing type;
    float value;
    float min, max;
} ClaySizeAxis;

typedef struct ClayBorder {
    ClayColor color;
    uint16_t left, right, top, bottom;
} ClayBorder;

typedef struct ClayOptions {
    const char *id;
    uint32_t idHash;
    ClayDirection direction;
    ClayAlignX alignX;
    ClayAlignY alignY;
    ClaySizeAxis width;
    ClaySizeAxis height;
    ClayPadding padding;
    float childGap;
    ClayColor backgroundColor;
    ClayCorners cornerRadius;
    ClayBorder border;
    bool scrollX;
    bool scrollY;
    bool wrap;              // Enable flex-wrap (children wrap to next row/column)
    bool wrapGrow;          // Items grow to fill rows/cols evenly (requires wrap=true)
    bool wrapStretchLast;   // Last row/col stretches to fill (default: same size as others)
    bool floating;
    ClayVec2 floatOffset;
    int16_t zIndex;
    void *imageData;
    float imageWidth, imageHeight;
    void *customData;
    void *userData;
} ClayOptions;

typedef struct ClayTextOptions {
    ClayColor color;
    uint16_t fontId;
    uint16_t fontSize;
    uint16_t letterSpacing;
    uint16_t lineHeight;
    bool wrap;
} ClayTextOptions;

// =============================================================================
// RENDER COMMANDS
// =============================================================================

typedef enum ClayRenderType {
    CLAY_RENDER_NONE = 0,
    CLAY_RENDER_RECTANGLE,
    CLAY_RENDER_BORDER,
    CLAY_RENDER_TEXT,
    CLAY_RENDER_IMAGE,
    CLAY_RENDER_SCISSOR_START,
    CLAY_RENDER_SCISSOR_END,
    CLAY_RENDER_CUSTOM
} ClayRenderType;

typedef struct ClayRenderText {
    ClayString text;
    ClayColor color;
    uint16_t fontId;
    uint16_t fontSize;
    uint16_t letterSpacing;
} ClayRenderText;

typedef struct ClayRenderRect {
    ClayColor color;
    ClayCorners cornerRadius;
} ClayRenderRect;

typedef struct ClayRenderBorder {
    ClayColor color;
    ClayCorners cornerRadius;
    uint16_t left, right, top, bottom;
} ClayRenderBorder;

typedef struct ClayRenderImage {
    void *imageData;
    ClayColor tint;
    ClayCorners cornerRadius;
} ClayRenderImage;

typedef struct ClayRenderCustom {
    void *customData;
    ClayColor backgroundColor;
    ClayCorners cornerRadius;
} ClayRenderCustom;

typedef struct ClayRenderCommand {
    ClayRect bounds;
    ClayRenderType type;
    int16_t zIndex;
    uint32_t id;
    void *userData;
    union {
        ClayRenderRect rectangle;
        ClayRenderBorder border;
        ClayRenderText text;
        ClayRenderImage image;
        ClayRenderCustom custom;
    } data;
} ClayRenderCommand;

typedef struct ClayRenderCommandArray {
    ClayRenderCommand *commands;
    int32_t count;
    int32_t capacity;
} ClayRenderCommandArray;

// =============================================================================
// QUERY TYPES
// =============================================================================

typedef struct ClayElementData {
    ClayRect bounds;
    bool found;
} ClayElementData;

typedef struct ClayScrollData {
    ClayVec2 *scrollPosition;
    ClayVec2 contentSize;
    ClayVec2 containerSize;
    bool found;
} ClayScrollData;

// =============================================================================
// CALLBACKS
// =============================================================================

typedef ClayVec2 (*ClayMeasureTextFn)(ClayString text, uint16_t fontId, uint16_t fontSize, uint16_t letterSpacing, void *userData);
typedef void (*ClayErrorFn)(const char *errorId, const char *message, void *userData);

// =============================================================================
// CONVENIENCE MACROS
// =============================================================================

#define CLAY_FIT()              ((ClaySizeAxis){ .type = CLAY_SIZE_FIT })
#define CLAY_GROW()             ((ClaySizeAxis){ .type = CLAY_SIZE_GROW })
#define CLAY_FIXED(px)          ((ClaySizeAxis){ .type = CLAY_SIZE_FIXED, .value = (px) })
#define CLAY_PERCENT(p)         ((ClaySizeAxis){ .type = CLAY_SIZE_PERCENT, .value = (p) })
#define CLAY_FIT_MINMAX(mn,mx)  ((ClaySizeAxis){ .type = CLAY_SIZE_FIT, .min = (mn), .max = (mx) })
#define CLAY_GROW_MINMAX(mn,mx) ((ClaySizeAxis){ .type = CLAY_SIZE_GROW, .min = (mn), .max = (mx) })
#define CLAY_MIN(mn)            ((ClaySizeAxis){ .type = CLAY_SIZE_GROW, .min = (mn) })
#define CLAY_MAX(mx)            ((ClaySizeAxis){ .type = CLAY_SIZE_GROW, .max = (mx) })

#define CLAY_CORNERS(r)         ((ClayCorners){ (r), (r), (r), (r) })
#define CLAY_CORNERS_TOP(r)     ((ClayCorners){ (r), (r), 0, 0 })
#define CLAY_CORNERS_BOT(r)     ((ClayCorners){ 0, 0, (r), (r) })

#define CLAY_PAD(p)             ((ClayPadding){ (p), (p), (p), (p) })
#define CLAY_PAD_X(x)           ((ClayPadding){ (x), (x), 0, 0 })
#define CLAY_PAD_Y(y)           ((ClayPadding){ 0, 0, (y), (y) })
#define CLAY_PAD_XYWH(l,r,t,b)  ((ClayPadding){ (l), (r), (t), (b) })

#define CLAY_RGB(r,g,b)         ((ClayColor){ (r), (g), (b), 255 })
#define CLAY_RGBA(r,g,b,a)      ((ClayColor){ (r), (g), (b), (a) })

#define CLAY_BORDER_ALL(w, col) ((ClayBorder){ .color = (col), .left = (w), .right = (w), .top = (w), .bottom = (w) })

#define CLAY_DEFAULT            ((ClayOptions){0})

#ifdef __cplusplus
}
#endif

#endif // CLAY_TYPES_H