// clay_ui.h - Unified Clay UI with Raylib renderer
// Single-file include for simplified Clay UI
//
// Usage:
//   #define CLAY_UI_IMPLEMENTATION
//   #include "clay_ui.h"

#ifndef CLAY_UI_H
#define CLAY_UI_H

#include "clay_types.h"
#include <stdlib.h>
#include <math.h>

#ifdef CLAY_USE_RAYLIB
#include "raylib.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// CONFIGURATION
// =============================================================================

#ifndef CLAY_MAX_ELEMENTS
#define CLAY_MAX_ELEMENTS 8192
#endif

#ifndef CLAY_MAX_RENDER_COMMANDS  
#define CLAY_MAX_RENDER_COMMANDS 16384
#endif

#ifndef CLAY_MAX_TEXT_CACHE
#define CLAY_MAX_TEXT_CACHE 4096
#endif

// =============================================================================
// API FUNCTIONS
// =============================================================================

void ClayInit(float width, float height, ClayMeasureTextFn measureText, void *measureTextUserData);
void ClayShutdown(void);
void ClaySetErrorCallback(ClayErrorFn callback, void *userData);
void ClayBeginFrame(float width, float height);
ClayRenderCommandArray ClayEndFrame(void);
void ClaySetPointer(float x, float y, bool down);
void ClaySetScroll(float deltaX, float deltaY, float deltaTime);
void BeginClay(ClayOptions options);
void EndClay(void);
void ClayText(const char *text, ClayTextOptions options);
void ClayTextEx(ClayString text, ClayTextOptions options);
bool ClayHovered(void);
bool ClayHoveredId(const char *id);
bool ClayPressedId(const char *id);
bool ClayPressed(void);
uint32_t ClayHashId(const char *id);
ClayElementData ClayGetElement(uint32_t idHash);
ClayScrollData ClayGetScroll(uint32_t idHash);
bool ClayIsPointerOver(uint32_t idHash);
void ClaySetDebug(bool enabled);
bool ClayIsDebugEnabled(void);

// =============================================================================
// RAYLIB RENDERER (only when CLAY_USE_RAYLIB is defined)
// =============================================================================

#ifdef CLAY_USE_RAYLIB
void ClayRaylibInit(int width, int height, Font *fonts, int fontCount);
void ClayRaylibRender(ClayRenderCommandArray commands, Font *fonts);
ClayVec2 ClayRaylibMeasureText(ClayString text, uint16_t fontId, uint16_t fontSize, uint16_t letterSpacing, void *userData);

static inline Color ClayToRaylibColor(ClayColor c) {
    return (Color){ c.r, c.g, c.b, c.a };
}

static inline Rectangle ClayToRaylibRect(ClayRect r) {
    return (Rectangle){ r.x, r.y, r.width, r.height };
}
#endif

#ifdef __cplusplus
}
#endif

// =============================================================================
// IMPLEMENTATION
// =============================================================================

#ifdef CLAY_UI_IMPLEMENTATION

// -----------------------------------------------------------------------------
// Internal Types
// -----------------------------------------------------------------------------

typedef struct ClayElement {
    uint32_t id;
    ClayOptions options;
    ClayRect bounds;
    ClayVec2 contentSize;
    int32_t parentIndex;
    int32_t firstChild;
    int32_t nextSibling;
    int32_t childCount;
} ClayElement;

typedef struct ClayTextElement {
    ClayString text;
    ClayTextOptions options;
    int32_t parentIndex;
    ClayVec2 measuredSize;
} ClayTextElement;

typedef struct ClayScrollState {
    uint32_t elementId;
    ClayVec2 position;
    ClayVec2 velocity;
    ClayRect bounds;
    ClayVec2 contentSize;
    bool active;
} ClayScrollState;

// Store previous frame element data for hover/click detection
typedef struct ClayPrevElement {
    uint32_t id;
    ClayRect bounds;
} ClayPrevElement;

typedef struct ClayContext {
    ClayElement *elements;
    int32_t elementCount;
    int32_t elementCapacity;
    
    ClayTextElement *textElements;
    int32_t textCount;
    int32_t textCapacity;
    
    ClayRenderCommand *renderCommands;
    int32_t renderCount;
    int32_t renderCapacity;
    
    ClayScrollState *scrollStates;
    int32_t scrollCount;
    int32_t scrollCapacity;
    
    // Previous frame element data (for hover/click detection)
    ClayPrevElement *prevElements;
    int32_t prevElementCount;
    int32_t prevElementCapacity;
    
    int32_t *elementStack;
    int32_t stackDepth;
    int32_t stackCapacity;
    
    ClayMeasureTextFn measureText;
    void *measureTextUserData;
    ClayErrorFn errorCallback;
    void *errorUserData;
    
    ClayVec2 pointerPos;
    ClayVec2 pointerPrevPos;
    bool pointerDown;
    bool pointerPrevDown;
    ClayVec2 scrollDelta;
    float deltaTime;
    
    float layoutWidth;
    float layoutHeight;
    
    int32_t currentElement;
    uint32_t hoveredElement;
    
    bool debugEnabled;
    bool initialized;
} ClayContext;

static ClayContext g_clay = {0};

// -----------------------------------------------------------------------------
// Hash Function
// -----------------------------------------------------------------------------

static uint32_t clay_hash_string(const char *str) {
    if (!str) return 0;
    uint32_t hash = 2166136261u;
    while (*str) {
        hash ^= (uint8_t)*str++;
        hash *= 16777619u;
    }
    return hash;
}

// -----------------------------------------------------------------------------
// Error Reporting
// -----------------------------------------------------------------------------

static void clay_error(const char *id, const char *msg) {
    if (g_clay.errorCallback) {
        g_clay.errorCallback(id, msg, g_clay.errorUserData);
    }
}

// -----------------------------------------------------------------------------
// Memory Management
// -----------------------------------------------------------------------------

static void clay_grow_elements(void) {
    int32_t newCapacity = g_clay.elementCapacity * 2;
    ClayElement *newElements = (ClayElement*)realloc(g_clay.elements, newCapacity * sizeof(ClayElement));
    if (newElements) {
        g_clay.elements = newElements;
        g_clay.elementCapacity = newCapacity;
    } else {
        clay_error("MEMORY", "Failed to grow element array");
    }
}

static void clay_grow_text(void) {
    int32_t newCapacity = g_clay.textCapacity * 2;
    ClayTextElement *newText = (ClayTextElement*)realloc(g_clay.textElements, newCapacity * sizeof(ClayTextElement));
    if (newText) {
        g_clay.textElements = newText;
        g_clay.textCapacity = newCapacity;
    } else {
        clay_error("MEMORY", "Failed to grow text array");
    }
}

static void clay_grow_render(void) {
    int32_t newCapacity = g_clay.renderCapacity * 2;
    ClayRenderCommand *newRender = (ClayRenderCommand*)realloc(g_clay.renderCommands, newCapacity * sizeof(ClayRenderCommand));
    if (newRender) {
        g_clay.renderCommands = newRender;
        g_clay.renderCapacity = newCapacity;
    } else {
        clay_error("MEMORY", "Failed to grow render command array");
    }
}

// -----------------------------------------------------------------------------
// Forward Declarations
// -----------------------------------------------------------------------------

static void clay_calculate_layout(void);
static void clay_generate_render_commands(void);

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void ClayInit(float width, float height, ClayMeasureTextFn measureText, void *measureTextUserData) {
    if (g_clay.initialized) {
        ClayShutdown();
    }
    
    memset(&g_clay, 0, sizeof(ClayContext));
    
    g_clay.elementCapacity = CLAY_MAX_ELEMENTS;
    g_clay.elements = (ClayElement*)calloc(g_clay.elementCapacity, sizeof(ClayElement));
    
    g_clay.textCapacity = CLAY_MAX_TEXT_CACHE;
    g_clay.textElements = (ClayTextElement*)calloc(g_clay.textCapacity, sizeof(ClayTextElement));
    
    g_clay.renderCapacity = CLAY_MAX_RENDER_COMMANDS;
    g_clay.renderCommands = (ClayRenderCommand*)calloc(g_clay.renderCapacity, sizeof(ClayRenderCommand));
    
    g_clay.scrollCapacity = 64;
    g_clay.scrollStates = (ClayScrollState*)calloc(g_clay.scrollCapacity, sizeof(ClayScrollState));
    
    g_clay.prevElementCapacity = CLAY_MAX_ELEMENTS;
    g_clay.prevElements = (ClayPrevElement*)calloc(g_clay.prevElementCapacity, sizeof(ClayPrevElement));
    
    g_clay.stackCapacity = 256;
    g_clay.elementStack = (int32_t*)calloc(g_clay.stackCapacity, sizeof(int32_t));
    
    g_clay.measureText = measureText;
    g_clay.measureTextUserData = measureTextUserData;
    g_clay.layoutWidth = width;
    g_clay.layoutHeight = height;
    g_clay.currentElement = -1;
    
    g_clay.initialized = true;
}

void ClayShutdown(void) {
    free(g_clay.elements);
    free(g_clay.textElements);
    free(g_clay.renderCommands);
    free(g_clay.scrollStates);
    free(g_clay.prevElements);
    free(g_clay.elementStack);
    memset(&g_clay, 0, sizeof(ClayContext));
}

void ClaySetErrorCallback(ClayErrorFn callback, void *userData) {
    g_clay.errorCallback = callback;
    g_clay.errorUserData = userData;
}

void ClayBeginFrame(float width, float height) {
    g_clay.layoutWidth = width;
    g_clay.layoutHeight = height;
    g_clay.elementCount = 0;
    g_clay.textCount = 0;
    g_clay.renderCount = 0;
    g_clay.stackDepth = 0;
    g_clay.currentElement = -1;
    g_clay.hoveredElement = 0;
    
    if (g_clay.elementCount >= g_clay.elementCapacity) {
        clay_grow_elements();
    }
    
    ClayElement *root = &g_clay.elements[0];
    memset(root, 0, sizeof(ClayElement));
    root->bounds = (ClayRect){ 0, 0, width, height };
    root->parentIndex = -1;
    root->firstChild = -1;
    root->nextSibling = -1;
    g_clay.elementCount = 1;
    
    g_clay.elementStack[0] = 0;
    g_clay.stackDepth = 1;
    g_clay.currentElement = 0;
}

ClayRenderCommandArray ClayEndFrame(void) {
    while (g_clay.stackDepth > 1) {
        EndClay();
    }
    
    clay_calculate_layout();
    clay_generate_render_commands();
    
    // Store element data for next frame's hover/click detection
    g_clay.prevElementCount = g_clay.elementCount;
    for (int32_t i = 0; i < g_clay.elementCount; i++) {
        g_clay.prevElements[i].id = g_clay.elements[i].id;
        g_clay.prevElements[i].bounds = g_clay.elements[i].bounds;
    }
    
    for (int32_t i = 0; i < g_clay.scrollCount; i++) {
        ClayScrollState *scroll = &g_clay.scrollStates[i];
        scroll->velocity.x *= 0.95f;
        scroll->velocity.y *= 0.95f;
        if (fabsf(scroll->velocity.x) < 0.1f) scroll->velocity.x = 0;
        if (fabsf(scroll->velocity.y) < 0.1f) scroll->velocity.y = 0;
        scroll->position.x += scroll->velocity.x * g_clay.deltaTime;
        scroll->position.y += scroll->velocity.y * g_clay.deltaTime;
    }
    
    g_clay.pointerPrevPos = g_clay.pointerPos;
    g_clay.pointerPrevDown = g_clay.pointerDown;
    
    return (ClayRenderCommandArray){
        .commands = g_clay.renderCommands,
        .count = g_clay.renderCount,
        .capacity = g_clay.renderCapacity
    };
}

void ClaySetPointer(float x, float y, bool down) {
    g_clay.pointerPos = (ClayVec2){ x, y };
    g_clay.pointerDown = down;
}

void ClaySetScroll(float deltaX, float deltaY, float deltaTime) {
    g_clay.scrollDelta = (ClayVec2){ deltaX, deltaY };
    g_clay.deltaTime = deltaTime;
}

void BeginClay(ClayOptions options) {
    if (g_clay.elementCount >= g_clay.elementCapacity) {
        clay_grow_elements();
    }
    
    int32_t index = g_clay.elementCount++;
    ClayElement *elem = &g_clay.elements[index];
    memset(elem, 0, sizeof(ClayElement));
    
    if (options.id) {
        elem->id = clay_hash_string(options.id);
    } else if (options.idHash) {
        elem->id = options.idHash;
    } else {
        elem->id = (uint32_t)(index * 2654435761u);
    }
    
    elem->options = options;
    elem->parentIndex = g_clay.currentElement;
    elem->firstChild = -1;
    elem->nextSibling = -1;
    
    if (g_clay.currentElement >= 0) {
        ClayElement *parent = &g_clay.elements[g_clay.currentElement];
        if (parent->firstChild < 0) {
            parent->firstChild = index;
        } else {
            int32_t sibling = parent->firstChild;
            while (g_clay.elements[sibling].nextSibling >= 0) {
                sibling = g_clay.elements[sibling].nextSibling;
            }
            g_clay.elements[sibling].nextSibling = index;
        }
        parent->childCount++;
    }
    
    if (g_clay.stackDepth >= g_clay.stackCapacity) {
        clay_error("STACK", "Element stack overflow");
        return;
    }
    g_clay.elementStack[g_clay.stackDepth++] = index;
    g_clay.currentElement = index;
}

void EndClay(void) {
    if (g_clay.stackDepth <= 1) {
        clay_error("STACK", "EndClay called without matching BeginClay");
        return;
    }
    
    g_clay.stackDepth--;
    g_clay.currentElement = g_clay.elementStack[g_clay.stackDepth - 1];
}

void ClayText(const char *text, ClayTextOptions options) {
    ClayTextEx(ClayStr(text), options);
}

void ClayTextEx(ClayString text, ClayTextOptions options) {
    if (g_clay.currentElement < 0) {
        clay_error("TEXT", "ClayText called outside of BeginClay/EndClay");
        return;
    }
    
    if (g_clay.textCount >= g_clay.textCapacity) {
        clay_grow_text();
    }
    
    ClayTextElement *textElem = &g_clay.textElements[g_clay.textCount++];
    textElem->text = text;
    textElem->options = options;
    textElem->parentIndex = g_clay.currentElement;
    
    if (g_clay.measureText) {
        textElem->measuredSize = g_clay.measureText(
            text, options.fontId, options.fontSize, 
            options.letterSpacing, g_clay.measureTextUserData
        );
    } else {
        textElem->measuredSize = (ClayVec2){ 
            (float)(text.length * (options.fontSize * 0.6f)), 
            (float)options.fontSize 
        };
    }
    // Note: Don't add to contentSize here - layout phase handles it
}

static bool point_in_rect(ClayVec2 p, ClayRect r) {
    return p.x >= r.x && p.x <= r.x + r.width &&
           p.y >= r.y && p.y <= r.y + r.height;
}

// Find element bounds from previous frame by ID
static ClayRect* clay_get_prev_bounds(uint32_t id) {
    for (int32_t i = 0; i < g_clay.prevElementCount; i++) {
        if (g_clay.prevElements[i].id == id) {
            return &g_clay.prevElements[i].bounds;
        }
    }
    return NULL;
}

bool ClayHovered(void) {
    if (g_clay.currentElement < 0) return false;
    ClayElement *elem = &g_clay.elements[g_clay.currentElement];
    
    // Use previous frame's bounds (current frame not yet calculated)
    ClayRect *prevBounds = clay_get_prev_bounds(elem->id);
    if (!prevBounds) return false;
    
    return point_in_rect(g_clay.pointerPos, *prevBounds);
}

bool ClayHoveredId(const char *id) {
    if (!id) return false;
    uint32_t hash = clay_hash_string(id);
    
    ClayRect *prevBounds = clay_get_prev_bounds(hash);
    if (!prevBounds) return false;
    
    return point_in_rect(g_clay.pointerPos, *prevBounds);
}

bool ClayPressed(void) {
    return ClayHovered() && g_clay.pointerDown && !g_clay.pointerPrevDown;
}

bool ClayPressedId(const char *id) {
    return ClayHoveredId(id) && g_clay.pointerDown && !g_clay.pointerPrevDown;
}

uint32_t ClayHashId(const char *id) {
    return clay_hash_string(id);
}

ClayElementData ClayGetElement(uint32_t idHash) {
    // Use previous frame data (current frame not yet laid out during draw)
    for (int32_t i = 0; i < g_clay.prevElementCount; i++) {
        if (g_clay.prevElements[i].id == idHash) {
            return (ClayElementData){
                .bounds = g_clay.prevElements[i].bounds,
                .found = true
            };
        }
    }
    return (ClayElementData){ .found = false };
}

ClayScrollData ClayGetScroll(uint32_t idHash) {
    for (int32_t i = 0; i < g_clay.scrollCount; i++) {
        if (g_clay.scrollStates[i].elementId == idHash) {
            ClayScrollState *s = &g_clay.scrollStates[i];
            return (ClayScrollData){
                .scrollPosition = &s->position,
                .contentSize = s->contentSize,
                .containerSize = (ClayVec2){ s->bounds.width, s->bounds.height },
                .found = true
            };
        }
    }
    return (ClayScrollData){ .found = false };
}

bool ClayIsPointerOver(uint32_t idHash) {
    ClayElementData data = ClayGetElement(idHash);
    if (!data.found) return false;
    return point_in_rect(g_clay.pointerPos, data.bounds);
}

void ClaySetDebug(bool enabled) {
    g_clay.debugEnabled = enabled;
}

bool ClayIsDebugEnabled(void) {
    return g_clay.debugEnabled;
}

// -----------------------------------------------------------------------------
// Layout Calculation
// -----------------------------------------------------------------------------

static float clay_resolve_size(ClaySizeAxis axis, float parentSize, float contentSize) {
    switch (axis.type) {
        case CLAY_SIZE_FIT: {
            float size = contentSize;
            // Clamp to parent size if content is larger (prevent overflow)
            if (size > parentSize && parentSize > 0) size = parentSize;
            if (axis.min > 0 && size < axis.min) size = axis.min;
            if (axis.max > 0 && size > axis.max) size = axis.max;
            return size;
        }
        case CLAY_SIZE_GROW: {
            float size = parentSize;
            if (axis.min > 0 && size < axis.min) size = axis.min;
            if (axis.max > 0 && size > axis.max) size = axis.max;
            return size;
        }
        case CLAY_SIZE_FIXED:
            return axis.value;
        case CLAY_SIZE_PERCENT:
            return parentSize * axis.value;
        default:
            return contentSize;
    }
}

static void clay_layout_element(int32_t index, ClayRect parentBounds) {
    ClayElement *elem = &g_clay.elements[index];
    ClayOptions *opts = &elem->options;
    
    float padLeft = opts->padding.left;
    float padRight = opts->padding.right;
    float padTop = opts->padding.top;
    float padBottom = opts->padding.bottom;
    
    // Pre-calculate our own width/height for FIXED and GROW (known upfront)
    float preWidth = parentBounds.width;
    float preHeight = parentBounds.height;
    
    if (opts->width.type == CLAY_SIZE_FIXED) {
        preWidth = opts->width.value;
    } else if (opts->width.type == CLAY_SIZE_PERCENT) {
        preWidth = parentBounds.width * opts->width.value;
    }
    
    if (opts->height.type == CLAY_SIZE_FIXED) {
        preHeight = opts->height.value;
    } else if (opts->height.type == CLAY_SIZE_PERCENT) {
        preHeight = parentBounds.height * opts->height.value;
    }
    
    float availWidth = preWidth - padLeft - padRight;
    float availHeight = preHeight - padTop - padBottom;
    
    // Count text elements first
    float textWidth = 0;
    float textHeight = 0;
    int32_t textCountForElem = 0;
    for (int32_t i = 0; i < g_clay.textCount; i++) {
        if (g_clay.textElements[i].parentIndex == index) {
            ClayTextElement *text = &g_clay.textElements[i];
            if (opts->direction == CLAY_TOP_TO_BOTTOM) {
                textHeight += text->measuredSize.y;
                if (text->measuredSize.x > textWidth) textWidth = text->measuredSize.x;
            } else {
                textWidth += text->measuredSize.x;
                if (text->measuredSize.y > textHeight) textHeight = text->measuredSize.y;
            }
            textCountForElem++;
        }
    }
    
    // Add gaps between text elements
    if (textCountForElem > 1 && opts->childGap > 0) {
        if (opts->direction == CLAY_TOP_TO_BOTTOM) {
            textHeight += opts->childGap * (textCountForElem - 1);
        } else {
            textWidth += opts->childGap * (textCountForElem - 1);
        }
    }
    
    // First pass: measure all children
    int32_t child = elem->firstChild;
    while (child >= 0) {
        ClayElement *childElem = &g_clay.elements[child];
        ClayOptions *childOpts = &childElem->options;
        
        // For wrap containers, measure children at their minimum/intrinsic size
        // so multiple items can fit per row. GROW children use their min value.
        float childAvailW = availWidth;
        float childAvailH = availHeight;
        
        if (opts->wrap) {
            if (opts->direction == CLAY_LEFT_TO_RIGHT) {
                // Horizontal wrap: constrain width to min or content
                if (childOpts->width.type == CLAY_SIZE_GROW) {
                    childAvailW = childOpts->width.min > 0 ? childOpts->width.min : 0;
                }
            } else {
                // Vertical wrap: constrain height to min or content
                if (childOpts->height.type == CLAY_SIZE_GROW) {
                    childAvailH = childOpts->height.min > 0 ? childOpts->height.min : 0;
                }
            }
        }
        
        clay_layout_element(child, (ClayRect){ 0, 0, childAvailW, childAvailH });
        child = childElem->nextSibling;
    }
    
    // Calculate content size (with or without wrapping)
    float contentWidth = 0;
    float contentHeight = 0;
    int32_t growCountX = 0;
    int32_t growCountY = 0;
    
    if (opts->wrap && elem->childCount > 0) {
        // WRAPPING LAYOUT
        if (opts->direction == CLAY_LEFT_TO_RIGHT) {
            // Horizontal with wrap: items flow left-to-right, wrap to new rows
            float rowWidth = 0;
            float rowHeight = 0;
            float totalHeight = 0;
            float maxWidth = 0;
            int itemsInRow = 0;
            
            child = elem->firstChild;
            while (child >= 0) {
                ClayElement *childElem = &g_clay.elements[child];
                float childW = childElem->bounds.width;
                float childH = childElem->bounds.height;
                
                // Check if child fits in current row
                float neededWidth = rowWidth + childW + (itemsInRow > 0 ? opts->childGap : 0);
                
                if (itemsInRow > 0 && neededWidth > availWidth) {
                    // Wrap to new row
                    if (rowWidth > maxWidth) maxWidth = rowWidth;
                    totalHeight += rowHeight + opts->childGap;
                    rowWidth = childW;
                    rowHeight = childH;
                    itemsInRow = 1;
                } else {
                    // Add to current row
                    if (itemsInRow > 0) rowWidth += opts->childGap;
                    rowWidth += childW;
                    if (childH > rowHeight) rowHeight = childH;
                    itemsInRow++;
                }
                
                child = childElem->nextSibling;
            }
            
            // Add final row
            if (rowWidth > maxWidth) maxWidth = rowWidth;
            totalHeight += rowHeight;
            
            contentWidth = maxWidth;
            contentHeight = totalHeight;
            
        } else {
            // Vertical with wrap: items flow top-to-bottom, wrap to new columns
            float colWidth = 0;
            float colHeight = 0;
            float totalWidth = 0;
            float maxHeight = 0;
            int itemsInCol = 0;
            
            child = elem->firstChild;
            while (child >= 0) {
                ClayElement *childElem = &g_clay.elements[child];
                float childW = childElem->bounds.width;
                float childH = childElem->bounds.height;
                
                float neededHeight = colHeight + childH + (itemsInCol > 0 ? opts->childGap : 0);
                
                if (itemsInCol > 0 && neededHeight > availHeight) {
                    // Wrap to new column
                    if (colHeight > maxHeight) maxHeight = colHeight;
                    totalWidth += colWidth + opts->childGap;
                    colHeight = childH;
                    colWidth = childW;
                    itemsInCol = 1;
                } else {
                    // Add to current column
                    if (itemsInCol > 0) colHeight += opts->childGap;
                    colHeight += childH;
                    if (childW > colWidth) colWidth = childW;
                    itemsInCol++;
                }
                
                child = childElem->nextSibling;
            }
            
            // Add final column
            if (colHeight > maxHeight) maxHeight = colHeight;
            totalWidth += colWidth;
            
            contentWidth = totalWidth;
            contentHeight = maxHeight;
        }
        
        // Add text size
        if (opts->direction == CLAY_TOP_TO_BOTTOM) {
            if (textWidth > contentWidth) contentWidth = textWidth;
            contentHeight += textHeight;
            if (textCountForElem > 0 && elem->childCount > 0) contentHeight += opts->childGap;
        } else {
            if (textHeight > contentHeight) contentHeight = textHeight;
            contentWidth += textWidth;
            if (textCountForElem > 0 && elem->childCount > 0) contentWidth += opts->childGap;
        }
        
    } else {
        // NON-WRAPPING LAYOUT (original behavior)
        if (opts->direction == CLAY_TOP_TO_BOTTOM) {
            contentHeight = textHeight;
            contentWidth = textWidth;
        } else {
            contentWidth = textWidth;
            contentHeight = textHeight;
        }
        
        child = elem->firstChild;
        while (child >= 0) {
            ClayElement *childElem = &g_clay.elements[child];
            ClayOptions *childOpts = &childElem->options;
            
            if (opts->direction == CLAY_TOP_TO_BOTTOM) {
                contentHeight += childElem->bounds.height;
                if (childElem->bounds.width > contentWidth) contentWidth = childElem->bounds.width;
                if (childOpts->height.type == CLAY_SIZE_GROW) growCountY++;
            } else {
                contentWidth += childElem->bounds.width;
                if (childElem->bounds.height > contentHeight) contentHeight = childElem->bounds.height;
                if (childOpts->width.type == CLAY_SIZE_GROW) growCountX++;
            }
            
            child = childElem->nextSibling;
        }
        
        // Add gaps
        if (elem->childCount > 0 && opts->childGap > 0) {
            if (elem->childCount > 1) {
                if (opts->direction == CLAY_TOP_TO_BOTTOM) {
                    contentHeight += opts->childGap * (elem->childCount - 1);
                } else {
                    contentWidth += opts->childGap * (elem->childCount - 1);
                }
            }
            if (textCountForElem > 0) {
                if (opts->direction == CLAY_TOP_TO_BOTTOM) {
                    contentHeight += opts->childGap;
                } else {
                    contentWidth += opts->childGap;
                }
            }
        }
    }
    
    elem->contentSize = (ClayVec2){ contentWidth, contentHeight };
    
    float selfWidth = clay_resolve_size(opts->width, parentBounds.width, contentWidth + padLeft + padRight);
    float selfHeight = clay_resolve_size(opts->height, parentBounds.height, contentHeight + padTop + padBottom);
    
    float x = parentBounds.x;
    float y = parentBounds.y;
    
    if (opts->floating) {
        x = opts->floatOffset.x;
        y = opts->floatOffset.y;
    }
    
    elem->bounds = (ClayRect){ x, y, selfWidth, selfHeight };
    
    float innerWidth = selfWidth - padLeft - padRight;
    float innerHeight = selfHeight - padTop - padBottom;
    
    float extraSpaceX = (growCountX > 0) ? (innerWidth - contentWidth) / growCountX : 0;
    float extraSpaceY = (growCountY > 0) ? (innerHeight - contentHeight) / growCountY : 0;
    
    float startX = x + padLeft;
    float startY = y + padTop;
    
    // Apply alignment for non-wrap or for the content block
    if (!opts->wrap) {
        if (opts->alignX == CLAY_ALIGN_X_CENTER) {
            startX += (innerWidth - contentWidth) / 2;
        } else if (opts->alignX == CLAY_ALIGN_X_RIGHT) {
            startX += innerWidth - contentWidth;
        }
        
        if (opts->alignY == CLAY_ALIGN_Y_CENTER) {
            startY += (innerHeight - contentHeight) / 2;
        } else if (opts->alignY == CLAY_ALIGN_Y_BOTTOM) {
            startY += innerHeight - contentHeight;
        }
    }
    
    float curX = startX;
    float curY = startY;
    
    // Offset for text
    if (opts->direction == CLAY_TOP_TO_BOTTOM) {
        curY += textHeight;
        if (textCountForElem > 1) curY += opts->childGap * (textCountForElem - 1);
        if (textCountForElem > 0 && elem->childCount > 0) curY += opts->childGap;
    } else {
        curX += textWidth;
        if (textCountForElem > 1) curX += opts->childGap * (textCountForElem - 1);
        if (textCountForElem > 0 && elem->childCount > 0) curX += opts->childGap;
    }
    
    // Position children
    if (opts->wrap && elem->childCount > 0) {
        // WRAPPING POSITIONING
        if (opts->direction == CLAY_LEFT_TO_RIGHT) {
            // First pass: count items per row and determine standard row size
            int firstRowCount = 0;
            float minItemWidth = 0;
            {
                float rowWidth = 0;
                int itemsInRow = 0;
                child = elem->firstChild;
                while (child >= 0) {
                    ClayElement *childElem = &g_clay.elements[child];
                    float childW = childElem->bounds.width;
                    
                    float neededWidth = rowWidth + childW + (itemsInRow > 0 ? opts->childGap : 0);
                    
                    if (itemsInRow > 0 && neededWidth > innerWidth) {
                        // Found first row
                        if (firstRowCount == 0) {
                            firstRowCount = itemsInRow;
                            minItemWidth = childW;
                        }
                        break;
                    }
                    
                    if (itemsInRow > 0) rowWidth += opts->childGap;
                    rowWidth += childW;
                    if (minItemWidth == 0 || childW < minItemWidth) minItemWidth = childW;
                    itemsInRow++;
                    
                    child = childElem->nextSibling;
                }
                // If we didn't wrap, first row is all items
                if (firstRowCount == 0) firstRowCount = itemsInRow;
            }
            
            // Calculate grow width for standard rows
            float growWidth = minItemWidth;
            if (opts->wrapGrow && firstRowCount > 0) {
                growWidth = (innerWidth - (firstRowCount - 1) * opts->childGap) / firstRowCount;
            }
            
            // Second pass: position items
            float rowX = startX;
            float rowY = curY;
            float rowHeight = 0;
            int itemsInRow = 0;
            int totalItemsPlaced = 0;
            int totalItems = elem->childCount;
            
            // Count remaining items for last row handling
            int remainingItems = totalItems;
            
            child = elem->firstChild;
            while (child >= 0) {
                ClayElement *childElem = &g_clay.elements[child];
                float childW = opts->wrapGrow ? growWidth : childElem->bounds.width;
                float childH = childElem->bounds.height;
                
                // Check if we need to wrap
                float neededWidth = (rowX - startX) + childW + (itemsInRow > 0 ? opts->childGap : 0);
                
                if (itemsInRow > 0 && neededWidth > innerWidth + 0.5f) {
                    // Wrap to new row
                    rowY += rowHeight + opts->childGap;
                    rowX = startX;
                    rowHeight = 0;
                    itemsInRow = 0;
                    remainingItems = totalItems - totalItemsPlaced;
                    
                    // Recalculate width for last row if wrapStretchLast
                    if (opts->wrapGrow && opts->wrapStretchLast && remainingItems < firstRowCount) {
                        childW = (innerWidth - (remainingItems - 1) * opts->childGap) / remainingItems;
                    }
                }
                
                if (itemsInRow > 0) rowX += opts->childGap;
                
                // Update child bounds
                if (opts->wrapGrow) {
                    // Check if this is in the last row and wrapStretchLast is enabled
                    if (opts->wrapStretchLast && remainingItems <= firstRowCount && remainingItems < firstRowCount) {
                        childW = (innerWidth - (remainingItems - 1) * opts->childGap) / remainingItems;
                    }
                    childElem->bounds.width = childW;
                }
                
                childElem->bounds.x = rowX;
                childElem->bounds.y = rowY;
                
                rowX += childW;
                if (childH > rowHeight) rowHeight = childH;
                itemsInRow++;
                totalItemsPlaced++;
                
                clay_layout_element(child, childElem->bounds);
                
                child = childElem->nextSibling;
            }
            
        } else {
            // Vertical wrap
            float colX = curX;
            float colY = startY;
            float colWidth = 0;
            int itemsInCol = 0;
            
            child = elem->firstChild;
            while (child >= 0) {
                ClayElement *childElem = &g_clay.elements[child];
                float childW = childElem->bounds.width;
                float childH = childElem->bounds.height;
                
                float neededHeight = (colY - startY) + childH + (itemsInCol > 0 ? opts->childGap : 0);
                
                if (itemsInCol > 0 && neededHeight > innerHeight) {
                    // Wrap to new column
                    colX += colWidth + opts->childGap;
                    colY = startY;
                    colWidth = 0;
                    itemsInCol = 0;
                }
                
                if (itemsInCol > 0) colY += opts->childGap;
                
                childElem->bounds.x = colX;
                childElem->bounds.y = colY;
                
                colY += childH;
                if (childW > colWidth) colWidth = childW;
                itemsInCol++;
                
                clay_layout_element(child, childElem->bounds);
                
                child = childElem->nextSibling;
            }
        }
        
    } else {
        // NON-WRAPPING POSITIONING (original)
        child = elem->firstChild;
        while (child >= 0) {
            ClayElement *childElem = &g_clay.elements[child];
            ClayOptions *childOpts = &childElem->options;
            
            if (opts->direction == CLAY_TOP_TO_BOTTOM) {
                if (childOpts->height.type == CLAY_SIZE_GROW) {
                    childElem->bounds.height += extraSpaceY;
                }
                if (childOpts->width.type == CLAY_SIZE_GROW) {
                    childElem->bounds.width = innerWidth;
                    childElem->bounds.x = x + padLeft;
                } else {
                    childElem->bounds.x = curX;
                    if (opts->alignX == CLAY_ALIGN_X_CENTER) {
                        childElem->bounds.x = x + padLeft + (innerWidth - childElem->bounds.width) / 2;
                    } else if (opts->alignX == CLAY_ALIGN_X_RIGHT) {
                        childElem->bounds.x = x + padLeft + innerWidth - childElem->bounds.width;
                    }
                }
                childElem->bounds.y = curY;
            } else {
                if (childOpts->width.type == CLAY_SIZE_GROW) {
                    childElem->bounds.width += extraSpaceX;
                }
                if (childOpts->height.type == CLAY_SIZE_GROW) {
                    childElem->bounds.height = innerHeight;
                    childElem->bounds.y = y + padTop;
                } else {
                    childElem->bounds.y = curY;
                    if (opts->alignY == CLAY_ALIGN_Y_CENTER) {
                        childElem->bounds.y = y + padTop + (innerHeight - childElem->bounds.height) / 2;
                    } else if (opts->alignY == CLAY_ALIGN_Y_BOTTOM) {
                        childElem->bounds.y = y + padTop + innerHeight - childElem->bounds.height;
                    }
                }
                childElem->bounds.x = curX;
            }
            
            if (opts->direction == CLAY_TOP_TO_BOTTOM) {
                curY += childElem->bounds.height + opts->childGap;
            } else {
                curX += childElem->bounds.width + opts->childGap;
            }
            
            clay_layout_element(child, childElem->bounds);
            
            child = childElem->nextSibling;
        }
    }
}

static void clay_calculate_layout(void) {
    if (g_clay.elementCount > 0) {
        ClayRect rootBounds = { 0, 0, g_clay.layoutWidth, g_clay.layoutHeight };
        g_clay.elements[0].bounds = rootBounds;
        clay_layout_element(0, rootBounds);
    }
}

// -----------------------------------------------------------------------------
// Render Command Generation
// -----------------------------------------------------------------------------

static void clay_add_render_command(ClayRenderCommand cmd) {
    if (g_clay.renderCount >= g_clay.renderCapacity) {
        clay_grow_render();
    }
    g_clay.renderCommands[g_clay.renderCount++] = cmd;
}

static void clay_render_element(int32_t index) {
    ClayElement *elem = &g_clay.elements[index];
    ClayOptions *opts = &elem->options;
    
    if (index == 0) {
        int32_t child = elem->firstChild;
        while (child >= 0) {
            clay_render_element(child);
            child = g_clay.elements[child].nextSibling;
        }
        return;
    }
    
    if (opts->scrollX || opts->scrollY) {
        clay_add_render_command((ClayRenderCommand){
            .bounds = elem->bounds,
            .type = CLAY_RENDER_SCISSOR_START,
            .zIndex = opts->zIndex,
            .id = elem->id
        });
    }
    
    if (opts->backgroundColor.a > 0) {
        clay_add_render_command((ClayRenderCommand){
            .bounds = elem->bounds,
            .type = CLAY_RENDER_RECTANGLE,
            .zIndex = opts->zIndex,
            .id = elem->id,
            .userData = opts->userData,
            .data = { .rectangle = {
                .color = opts->backgroundColor,
                .cornerRadius = opts->cornerRadius
            } }
        });
    }
    
    if (opts->imageData) {
        clay_add_render_command((ClayRenderCommand){
            .bounds = elem->bounds,
            .type = CLAY_RENDER_IMAGE,
            .zIndex = opts->zIndex,
            .id = elem->id,
            .userData = opts->userData,
            .data = { .image = {
                .imageData = opts->imageData,
                .tint = (ClayColor){ 255, 255, 255, 255 },
                .cornerRadius = opts->cornerRadius
            } }
        });
    }
    
    if (opts->customData) {
        clay_add_render_command((ClayRenderCommand){
            .bounds = elem->bounds,
            .type = CLAY_RENDER_CUSTOM,
            .zIndex = opts->zIndex,
            .id = elem->id,
            .userData = opts->userData,
            .data = { .custom = {
                .customData = opts->customData,
                .backgroundColor = opts->backgroundColor,
                .cornerRadius = opts->cornerRadius
            } }
        });
    }
    
    float textY = elem->bounds.y + opts->padding.top;
    float textX = elem->bounds.x + opts->padding.left;
    
    // Calculate total text height/width for alignment
    float totalTextHeight = 0;
    float totalTextWidth = 0;
    int textCountForElem = 0;
    for (int32_t i = 0; i < g_clay.textCount; i++) {
        if (g_clay.textElements[i].parentIndex == index) {
            if (opts->direction == CLAY_TOP_TO_BOTTOM) {
                totalTextHeight += g_clay.textElements[i].measuredSize.y;
                if (g_clay.textElements[i].measuredSize.x > totalTextWidth) {
                    totalTextWidth = g_clay.textElements[i].measuredSize.x;
                }
            } else {
                totalTextWidth += g_clay.textElements[i].measuredSize.x;
                if (g_clay.textElements[i].measuredSize.y > totalTextHeight) {
                    totalTextHeight = g_clay.textElements[i].measuredSize.y;
                }
            }
            textCountForElem++;
        }
    }
    
    // Add gaps to total
    if (textCountForElem > 1 && opts->childGap > 0) {
        if (opts->direction == CLAY_TOP_TO_BOTTOM) {
            totalTextHeight += opts->childGap * (textCountForElem - 1);
        } else {
            totalTextWidth += opts->childGap * (textCountForElem - 1);
        }
    }
    
    float innerWidth = elem->bounds.width - opts->padding.left - opts->padding.right;
    float innerHeight = elem->bounds.height - opts->padding.top - opts->padding.bottom;
    
    // For text-only elements, render each text item aligned within the container
    for (int32_t i = 0; i < g_clay.textCount; i++) {
        if (g_clay.textElements[i].parentIndex == index) {
            ClayTextElement *text = &g_clay.textElements[i];
            
            float thisTextX = textX;
            float thisTextY = textY;
            
            // Apply horizontal alignment
            if (opts->alignX == CLAY_ALIGN_X_CENTER && innerWidth > 0) {
                float offset = (innerWidth - text->measuredSize.x) / 2;
                if (offset < 0) offset = 0;  // Clamp: don't push left if text too wide
                thisTextX = elem->bounds.x + opts->padding.left + offset;
            } else if (opts->alignX == CLAY_ALIGN_X_RIGHT && innerWidth > 0) {
                float offset = innerWidth - text->measuredSize.x;
                if (offset < 0) offset = 0;
                thisTextX = elem->bounds.x + opts->padding.left + offset;
            }
            
            // Apply vertical alignment
            if (opts->alignY == CLAY_ALIGN_Y_CENTER && innerHeight > 0) {
                float offset = (innerHeight - text->measuredSize.y) / 2;
                if (offset < 0) offset = 0;  // Clamp: don't push up if text too tall
                thisTextY = elem->bounds.y + opts->padding.top + offset;
            } else if (opts->alignY == CLAY_ALIGN_Y_BOTTOM && innerHeight > 0) {
                float offset = innerHeight - text->measuredSize.y;
                if (offset < 0) offset = 0;
                thisTextY = elem->bounds.y + opts->padding.top + offset;
            }
            
            clay_add_render_command((ClayRenderCommand){
                .bounds = { thisTextX, thisTextY, text->measuredSize.x, text->measuredSize.y },
                .type = CLAY_RENDER_TEXT,
                .zIndex = opts->zIndex,
                .id = elem->id,
                .data = { .text = {
                    .text = text->text,
                    .color = text->options.color,
                    .fontId = text->options.fontId,
                    .fontSize = text->options.fontSize,
                    .letterSpacing = text->options.letterSpacing
                } }
            });
            
            // Advance position for next text element
            if (opts->direction == CLAY_TOP_TO_BOTTOM) {
                textY += text->measuredSize.y + opts->childGap;
            } else {
                textX += text->measuredSize.x + opts->childGap;
            }
        }
    }
    
    int32_t child = elem->firstChild;
    while (child >= 0) {
        clay_render_element(child);
        child = g_clay.elements[child].nextSibling;
    }
    
    if (opts->border.left > 0 || opts->border.right > 0 || 
        opts->border.top > 0 || opts->border.bottom > 0) {
        clay_add_render_command((ClayRenderCommand){
            .bounds = elem->bounds,
            .type = CLAY_RENDER_BORDER,
            .zIndex = opts->zIndex,
            .id = elem->id,
            .userData = opts->userData,
            .data = { .border = {
                .color = opts->border.color,
                .cornerRadius = opts->cornerRadius,
                .left = opts->border.left,
                .right = opts->border.right,
                .top = opts->border.top,
                .bottom = opts->border.bottom
            } }
        });
    }
    
    if (opts->scrollX || opts->scrollY) {
        clay_add_render_command((ClayRenderCommand){
            .bounds = elem->bounds,
            .type = CLAY_RENDER_SCISSOR_END,
            .zIndex = opts->zIndex,
            .id = elem->id
        });
    }
}

static void clay_generate_render_commands(void) {
    g_clay.renderCount = 0;
    if (g_clay.elementCount > 0) {
        clay_render_element(0);
    }
}

// -----------------------------------------------------------------------------
// Raylib Renderer
// -----------------------------------------------------------------------------

#ifdef CLAY_USE_RAYLIB

typedef struct {
    Font *fonts;
    int fontCount;
} ClayRaylibContext;

static ClayRaylibContext g_clayRaylib = {0};
static char *g_textBuffer = NULL;
static int g_textBufferSize = 0;

static char* clay_ensure_null_terminated(ClayString str) {
    int needed = str.length + 1;
    if (needed > g_textBufferSize) {
        g_textBuffer = (char*)realloc(g_textBuffer, needed);
        g_textBufferSize = needed;
    }
    memcpy(g_textBuffer, str.chars, str.length);
    g_textBuffer[str.length] = '\0';
    return g_textBuffer;
}

ClayVec2 ClayRaylibMeasureText(ClayString text, uint16_t fontId, uint16_t fontSize, uint16_t letterSpacing, void *userData) {
    (void)userData;
    
    Font font = (g_clayRaylib.fonts && fontId < g_clayRaylib.fontCount) 
        ? g_clayRaylib.fonts[fontId] 
        : GetFontDefault();
    
    if (!font.glyphs) {
        font = GetFontDefault();
    }
    
    float scaleFactor = (float)fontSize / (float)font.baseSize;
    float maxWidth = 0;
    float lineWidth = 0;
    float height = fontSize;
    
    for (int32_t i = 0; i < text.length; i++) {
        char c = text.chars[i];
        
        if (c == '\n') {
            if (lineWidth > maxWidth) maxWidth = lineWidth;
            lineWidth = 0;
            height += fontSize;
            continue;
        }
        
        int index = c - 32;
        if (index < 0 || index >= font.glyphCount) index = 0;
        
        if (font.glyphs[index].advanceX != 0) {
            lineWidth += font.glyphs[index].advanceX * scaleFactor;
        } else {
            lineWidth += (font.recs[index].width + font.glyphs[index].offsetX) * scaleFactor;
        }
        
        // Only add letter spacing between characters, not after the last one
        if (i < text.length - 1) {
            lineWidth += letterSpacing;
        }
    }
    
    if (lineWidth > maxWidth) maxWidth = lineWidth;
    
    return (ClayVec2){ maxWidth, height };
}

void ClayRaylibInit(int width, int height, Font *fonts, int fontCount) {
    g_clayRaylib.fonts = fonts;
    g_clayRaylib.fontCount = fontCount;
    ClayInit((float)width, (float)height, ClayRaylibMeasureText, NULL);
}

void ClayRaylibRender(ClayRenderCommandArray commands, Font *fonts) {
    for (int32_t i = 0; i < commands.count; i++) {
        ClayRenderCommand *cmd = &commands.commands[i];
        Rectangle bounds = ClayToRaylibRect(cmd->bounds);
        
        switch (cmd->type) {
            case CLAY_RENDER_RECTANGLE: {
                ClayRenderRect *rect = &cmd->data.rectangle;
                Color color = ClayToRaylibColor(rect->color);
                
                if (rect->cornerRadius.topLeft > 0) {
                    float radius = (rect->cornerRadius.topLeft * 2.0f) / 
                        fminf(bounds.width, bounds.height);
                    DrawRectangleRounded(bounds, radius, 8, color);
                } else {
                    DrawRectangleRec(bounds, color);
                }
                break;
            }
            
            case CLAY_RENDER_BORDER: {
                ClayRenderBorder *border = &cmd->data.border;
                Color color = ClayToRaylibColor(border->color);
                ClayCorners cr = border->cornerRadius;
                
                if (border->left > 0) {
                    DrawRectangle(
                        (int)bounds.x,
                        (int)(bounds.y + cr.topLeft),
                        border->left,
                        (int)(bounds.height - cr.topLeft - cr.bottomLeft),
                        color
                    );
                }
                if (border->right > 0) {
                    DrawRectangle(
                        (int)(bounds.x + bounds.width - border->right),
                        (int)(bounds.y + cr.topRight),
                        border->right,
                        (int)(bounds.height - cr.topRight - cr.bottomRight),
                        color
                    );
                }
                if (border->top > 0) {
                    DrawRectangle(
                        (int)(bounds.x + cr.topLeft),
                        (int)bounds.y,
                        (int)(bounds.width - cr.topLeft - cr.topRight),
                        border->top,
                        color
                    );
                }
                if (border->bottom > 0) {
                    DrawRectangle(
                        (int)(bounds.x + cr.bottomLeft),
                        (int)(bounds.y + bounds.height - border->bottom),
                        (int)(bounds.width - cr.bottomLeft - cr.bottomRight),
                        border->bottom,
                        color
                    );
                }
                
                if (cr.topLeft > 0) {
                    DrawRing(
                        (Vector2){ bounds.x + cr.topLeft, bounds.y + cr.topLeft },
                        cr.topLeft - border->top, cr.topLeft,
                        180, 270, 10, color
                    );
                }
                if (cr.topRight > 0) {
                    DrawRing(
                        (Vector2){ bounds.x + bounds.width - cr.topRight, bounds.y + cr.topRight },
                        cr.topRight - border->top, cr.topRight,
                        270, 360, 10, color
                    );
                }
                if (cr.bottomLeft > 0) {
                    DrawRing(
                        (Vector2){ bounds.x + cr.bottomLeft, bounds.y + bounds.height - cr.bottomLeft },
                        cr.bottomLeft - border->bottom, cr.bottomLeft,
                        90, 180, 10, color
                    );
                }
                if (cr.bottomRight > 0) {
                    DrawRing(
                        (Vector2){ bounds.x + bounds.width - cr.bottomRight, bounds.y + bounds.height - cr.bottomRight },
                        cr.bottomRight - border->bottom, cr.bottomRight,
                        0, 90, 10, color
                    );
                }
                break;
            }
            
            case CLAY_RENDER_TEXT: {
                ClayRenderText *text = &cmd->data.text;
                // Use stored fonts (same as measuring) - ignore parameter for consistency
                Font font = (g_clayRaylib.fonts && text->fontId < g_clayRaylib.fontCount)
                    ? g_clayRaylib.fonts[text->fontId]
                    : GetFontDefault();
                    
                if (!font.glyphs) font = GetFontDefault();
                
                char *str = clay_ensure_null_terminated(text->text);
                DrawTextEx(
                    font, str,
                    (Vector2){ bounds.x, bounds.y },
                    text->fontSize,
                    text->letterSpacing,
                    ClayToRaylibColor(text->color)
                );
                break;
            }
            
            case CLAY_RENDER_IMAGE: {
                ClayRenderImage *img = &cmd->data.image;
                if (img->imageData) {
                    Texture2D *tex = (Texture2D*)img->imageData;
                    DrawTexturePro(
                        *tex,
                        (Rectangle){ 0, 0, (float)tex->width, (float)tex->height },
                        bounds,
                        (Vector2){ 0, 0 },
                        0,
                        ClayToRaylibColor(img->tint)
                    );
                }
                break;
            }
            
            case CLAY_RENDER_SCISSOR_START: {
                BeginScissorMode(
                    (int)bounds.x, (int)bounds.y,
                    (int)bounds.width, (int)bounds.height
                );
                break;
            }
            
            case CLAY_RENDER_SCISSOR_END: {
                EndScissorMode();
                break;
            }
            
            default:
                break;
        }
    }
}

#endif // CLAY_USE_RAYLIB

#endif // CLAY_UI_IMPLEMENTATION

#endif // CLAY_UI_H