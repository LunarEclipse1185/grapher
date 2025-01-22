// A MacOS `Grapher` app clone
//#include <stdio.h>
#include <stdbool.h>
#include <stddef.h> // NULL, size_t
#include <math.h>
#include <stdlib.h>

#include <raylib.h>

#include "style.h"
#include "equation.h"
#include "dynarray.h"

typedef struct {
    int window_width, window_height;
    int sidebar_width; // list of equations
    int editor_height; // equation area
} LayoutStyle;

typedef struct {
    Equation * items;
    size_t count;
    size_t capacity;

    size_t selected;
} Equations;

// components
bool sidebar(Rectangle frame, Equations * eqs); // return true if should_redraw
void editor(Rectangle frame, String * eq);
void grapher(Rectangle frame, Equations * eqs, bool redraw);

Font g_font;

#define BeginScissorModeRec(rect) BeginScissorMode((rect).x, (rect).y, (rect).width, (rect).height);
int main(void) {
    
    LayoutStyle ls = {
        .window_width = 800,
        .window_height = 600,
        .sidebar_width = 200,
        .editor_height = 100,
    };

    Equations eqs = {
        .selected = -1,
    };
    da_append(&eqs, (Equation) {.editor = string_createEmpty()});

    
    // init
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(800, 600, "Grapher");
    SetTargetFPS(60);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(ls.sidebar_width + 20, ls.editor_height + 20); // TBD: set this again after resizing the components
    g_font = LoadFont("Iosevka.ttf");

    // main loop
    while (!WindowShouldClose()) {
        if (IsWindowResized()) {
            ls.window_width = GetScreenWidth();
            ls.window_height = GetScreenHeight();
        }
        
        BeginDrawing();
        ClearBackground(c_bg_primary);

        bool should_redraw = false;
        
        Rectangle sidebar_frame = {0, 0, ls.sidebar_width, ls.window_height};
        BeginScissorModeRec(sidebar_frame);
        should_redraw = sidebar(sidebar_frame, &eqs) || should_redraw;
        EndScissorMode();
        //DrawLineEx((Vector2) {ls.sidebar_width, 0}, (Vector2) {ls.sidebar_width, ls.window_height}, 2, c_separator);
        DrawRectangle(ls.sidebar_width - 1, 0, 1, ls.window_height, c_separator);

        Rectangle editor_frame = {ls.sidebar_width, 0, ls.window_width - ls.sidebar_width, ls.editor_height};
        BeginScissorModeRec(editor_frame);
        editor(editor_frame, eqs.selected == (size_t)-1 ? NULL : &eqs.items[eqs.selected].editor);
        EndScissorMode();
        DrawRectangle(ls.sidebar_width, ls.editor_height - 1, ls.window_width, 1, c_separator);

        Rectangle grapher_frame = {ls.sidebar_width, ls.editor_height, ls.window_width - ls.sidebar_width, ls.window_height - ls.editor_height};
        //BeginScissorModeRec(grapher_frame);
        grapher(grapher_frame, &eqs, should_redraw);
        //EndScissorMode();
        
        EndDrawing();
    }
    CloseWindow();

    // cleanup
    for (size_t i = 0; i < eqs.count; ++i) {
        equation_free(eqs.items + i);
    }
    da_free(&eqs);
    
    return 0;
}

void render_string(Vector2 pos, String text, int charh, bool show_cursor, Color c) {
    int charw = MeasureTextEx(g_font, " ", charh, 0).x;
    for (size_t i = 0; i + 1 < text.count; ++i) {
        DrawTextEx(g_font, TextFormat("%c", text.items[i]),
                   (Vector2) {pos.x + i * charw, pos.y},
                   charh, 0, c);
    }
    if (show_cursor) {
        DrawRectangle(pos.x + text.cursor * charw,
                      pos.y,
                      charw, charh, BLACK);
        DrawTextEx(g_font, TextFormat("%c", text.items[text.cursor]),
                   (Vector2) {pos.x + text.cursor * charw,
                              pos.y},
                   charh, 0, c_bg_primary);
    }
}
void DrawTextCentered(char * str, Rectangle rect, int fontSize, Color c) {
    DrawTextEx(g_font, str,
               (Vector2) {rect.x + rect.width / 2 - MeasureTextEx(g_font, str, fontSize, 0).x / 2,
                          rect.y + rect.height / 2 - fontSize / 2},
               fontSize, 0, c);
}

bool sidebar(Rectangle frame, Equations * eqs) {
    DrawRectangleRec(frame, c_bg_secondary);

    int padding = 10;
    int itemH = 30;
    Vector2 mp = GetMousePosition();

    bool should_redraw = false;
    
    // buttons ui
    int buttons_top = /*frame.y + (padding + itemH) * eqs->count + */padding;
    Rectangle frame_add = {
        frame.x + padding,
        buttons_top,
        (frame.width - padding * 3) * 0.5f,
        itemH,
    };
    Rectangle frame_remove = {
        frame.x + padding * 0.5f + frame.width * 0.5f,
        buttons_top,
        (frame.width - padding * 3) * 0.5f,
        itemH,
    };
    DrawRectangleRounded(frame_add, 0.5, 10,
                         IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mp, frame_add)
                             ? c_bg_quaternary : c_bg_tertiary);
    DrawTextCentered("+", frame_add, itemH, c_fg_primary);
    DrawRectangleRounded(frame_remove, 0.5, 10,
                         IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mp, frame_remove)
                             ? c_bg_quaternary : c_bg_tertiary);
    DrawTextCentered("-", frame_remove, itemH, c_fg_primary);
    // button interaction
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointRec(mp, frame_add)) {
        should_redraw = true;
        da_append(eqs, (Equation) {.editor = string_createEmpty()});
        eqs->selected = eqs->count - 1;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointRec(mp, frame_remove) &&
        eqs->count > 0) {
        should_redraw = true;
        if (eqs->selected == (size_t)-1) {
            equation_free(&eqs->items[eqs->count - 1]);
            eqs->count -= 1;
        } else {
            equation_free(&eqs->items[eqs->selected]);
            for (size_t i = eqs->selected + 1; i < eqs->count; ++i) {
                eqs->items[i-1] = eqs->items[i];
            }
            if (eqs->selected > 0) eqs->selected -= 1;
            eqs->count -= 1;
            if (eqs->count == 0) eqs->selected = -1;
        }
    }

    // scroll area
    static float scroffs = 0; // negative or 0
    float contentH = (padding + itemH) * eqs->count;
    float scrollH = frame.height - padding * 2 - itemH;
    float scrollTop = frame.y + padding * 2 + itemH;
    Rectangle scroll_frame = {frame.x, scrollTop, frame.width, scrollH};

    // scroll bar
    if (contentH > scrollH) { 
        DrawRectangle(frame.width - padding * 0.75f,
                      scrollTop - scroffs * scrollH / contentH,
                      padding * 0.5f,
                      scrollH * scrollH / contentH,
                      c_bg_quaternary);
        scroffs += GetMouseWheelMoveV().y;
        if (scroffs > 0) scroffs = 0;
        if (scroffs < scrollH - contentH) scroffs = scrollH - contentH;
    } else {
        scroffs = 0;
    }
    
    // scroll items
    for (size_t i = 0; i < eqs->count; ++i) {
        Equation * eq = eqs->items + i;
        Rectangle item_frame = (Rectangle) {frame.x + padding,
                                            frame.y + scroffs + (padding + itemH) * (i + 1) + padding,
                                            frame.width - padding * 2,
                                            itemH};
        BeginScissorModeRec(scroll_frame);
        DrawRectangleRounded(item_frame, 0.5, 10,
                             i == eqs->selected ? c_bg_highlighted :
                             c_bg_tertiary);
        // EndScissorMode();
        BeginScissorModeRec(item_frame); // double scissor, [!] need testing
        render_string((Vector2) {frame.x + padding * 2,
                                 frame.y + scroffs + (padding + itemH) * (i + 1) + padding * 1.5f},
            eq->editor, itemH - padding, false,
            eq->state == ES_INVALID ? c_fg_alarming : c_fg_primary);
        EndScissorMode();
        // item interaction
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(mp, item_frame) &&
            CheckCollisionPointRec(mp, scroll_frame) &&
            i != eqs->selected) {
            if (eqs->selected != (size_t)-1) {
                should_redraw = true;
                Equation * eq_sel = eqs->items + eqs->selected;
                da_copy(&eq_sel->text, &eq_sel->editor);
                equation_parse(eq_sel);
            }
            eqs->selected = i;
        }
    }
    // item interaction
    if (IsKeyPressed(KEY_ENTER) && eqs->selected != (size_t)-1) {
        should_redraw = true;
        Equation * eq_sel = eqs->items + eqs->selected;
        da_copy(&eq_sel->text, &eq_sel->editor);
        equation_parse(eq_sel);
    }

    return should_redraw;
}

void editor(Rectangle frame, String * text) {
    DrawRectangleRec(frame, c_bg_primary);

    if (!text) {
        char * str = "No equation is selected";
        int charh = 25;
        DrawTextCentered(str, frame, charh, c_fg_placeholder);
        return;
    }

    int charh = 25;
    int charw = MeasureTextEx(g_font, " ", charh, 0).x;
    int padleft = 8;
    
    // input
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointRec(GetMousePosition(),
                               (Rectangle) {frame.x + padleft,
                                            frame.y + frame.height / 2 - charh / 2,
                                            frame.width - padleft,
                                            charh})) {
        text->cursor = (int)(GetMousePosition().x - frame.x - padleft) / charw;
        if (text->cursor > text->count - 1) text->cursor = text->count - 1;
    }
    if ((IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) && text->cursor > 0) text->cursor -= 1;
    if ((IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) && text->cursor < text->count - 1) text->cursor += 1;
    if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))) string_backspace(text);
        
    int c;
    while ((c = GetCharPressed()) > 0) {
        string_insert(text, c);
    }

    // render
    render_string((Vector2) {frame.x + padleft, frame.y + frame.height / 2 - charh / 2}, *text, charh, true, c_fg_primary);
}


float lerpf(float x, float froml, float fromr, float tol, float tor) {
    return tol + (x - froml) / (fromr - froml) * (tor - tol);
}
void grapher(Rectangle frame, Equations * eqs, bool should_redraw) {
    // basically this is like a singleton class
    static RenderTexture2D cvs;
    static int width_old = 0;
    static int height_old = 0;
    static Vector2 * spline = NULL;
    int scale = 2;
    int width = (frame.width - 2) * scale;
    int height = (frame.height - 2) * scale;
    if (!cvs.id || width != width_old || height != height_old) { // empty or resized
        width_old = width;
        height_old = height;
        UnloadRenderTexture(cvs);
        cvs = LoadRenderTexture(width, height);
        should_redraw = true;
        if (spline = reallocf(spline, width * sizeof(Vector2)), !spline) exit(1);
    }
    if (should_redraw) {
        BeginTextureMode(cvs);
        ClearBackground(c_bg_primary);

        // axes
        int w = 5 * scale, l = 15 * scale, b = 10 * scale;
        DrawLineEx((Vector2) {0, height/2}, (Vector2) {width - b, height/2}, 2, c_fg_primary);
        DrawLineEx((Vector2) {width/2, 0}, (Vector2) {width/2, height - b}, 2, c_fg_primary);
        Vector2 uparrow[] = {
            {width/2 - w + 0.5f, height - l},
            {width/2 + 0.5f, height},
            {width/2 + 0.5f, height - b},
            {width/2 + w + 0.5f, height - l},
        };
        DrawTriangleStrip(uparrow, 4, c_fg_primary);
        Vector2 rightarrow[] = {
            {width - l, height/2 + w - 0.5f},
            {width, height/2 - 0.5f},
            {width - b, height/2 - 0.5f},
            {width - l, height/2 - w - 0.5f},
        };
        DrawTriangleStrip(rightarrow, 4, c_fg_primary);
        // todo: integer markers

        // plot
        float minvalX = -10, maxvalX = 10;
        float minvalY = -5, maxvalY = 5;

        for (size_t i = 0; i < eqs->count; ++i) {
            if (eqs->items[i].state != ES_VALID) continue;
            for (int x = 0; x < width; x += 2) {
                float valx = lerpf(x, 0, width, minvalX, maxvalX);
                float valy = expr_eval(eqs->items[i].expr, valx);
                float y = lerpf(valy, minvalY, maxvalY, 0, height);
                spline[x/2] = (Vector2) {x, y};
            }
            DrawSplineLinear(spline, width/2, i == eqs->selected ? 4 : 2, BLACK);
        }

        EndTextureMode();
    }
    DrawTexturePro(cvs.texture, (Rectangle) {0, 0, width, height}, frame, (Vector2) {0, 0}, 0.0f, WHITE);
}
