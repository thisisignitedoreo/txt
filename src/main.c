
#include "raylib.h"
#include <libgen.h>
#include <errno.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include "font.c"
#include "icon.c"
#define DA_IMPL
#include "da.h"

char* error;
#include "config.c"
#include "buffer.c"

void draw_text(int* str, Font font, size_t x, size_t y,
               size_t font_size, int posx, size_t line_num, Token* tokens) {
    size_t dx = 0;

    for (size_t i = 0; i < da_length(tokens); ++i) {
        Token token = tokens[i];
        size_t token_length = token.end - token.start;
        if (token.line == line_num && token_length > 0) {
            int dstr[token_length];
            memcpy(dstr, str + token.start, token_length*sizeof(int));
            char* utf8_string = LoadUTF8(dstr, token_length);
            DrawTextEx(font, utf8_string, (Vector2) {(float) dx+x+posx, (float) y}, font_size, 0, token.color);
            Vector2 text_size = MeasureTextEx(font, utf8_string, font_size, 0);
            UnloadUTF8(utf8_string);
            dx += text_size.x;
        }
    }
}

void print_tokens(Token* tokens) {
    printf("len(tokens) = %zu\n", da_length(tokens));
    printf("tokens = {\n");
    for (size_t i = 0; i < da_length(tokens); ++i) {
        printf("  [%zu] = (Token) {.start = %zu, .end = %zu, .line = %zu, .color = (Color) {%d %d %d %d}}\n",
                i, tokens[i].start, tokens[i].end, tokens[i].line, tokens[i].color.r, tokens[i].color.g, tokens[i].color.b, tokens[i].color.a);
    }
    printf("}\n");
}

void print_lines(Line* lines) {
    printf("len(lines) = %zu\n", da_length(lines));
    printf("lines = {\n");
    for (size_t i = 0; i < da_length(lines); ++i) {
        printf("  [%zu] = (Line) {.start = %zu, .end = %zu}\n",
                i, lines[i].start, lines[i].end);
    }
    printf("}\n");
}

void print_content(int* content) {
    char* ucontent = LoadUTF8(content, da_length(content));
    for (size_t i = 0; i < strlen(ucontent); ++i) putc(ucontent[i], stdout);
    putc('\n', stdout);
    UnloadUTF8(ucontent);
}

float lerp(float a, float b, float t) {
    return a + ((b - a) * t);
}

float ilerp(float a, float b, float v) {
    return (v - a) / (b - a);
}

Color* lerp_color(Color c1, Color c2, float t) {
    Color* a = malloc(sizeof(Color));
    a->r = lerp(c1.r, c2.r, t);
    a->g = lerp(c1.g, c2.g, t);
    a->b = lerp(c1.b, c2.b, t);
    a->a = lerp(c1.a, c2.a, t);
    return a;
}

#define ERROR_LENGTH (5*60)
#define ERROR_FADE (1*60)
size_t error_time = 0;

void draw_buffer(Buffer* buf, Font font, int font_size, int posy, int posx, int line_size, int pad, bool select_line, int inner_pad) {
    int y = pad - posy * font_size - posy * inner_pad;
    size_t cl, cc, sl, sc;
    bool selection = buf_get_selection_cursor(buf, &sl, &sc);
    buf_get_cursor(buf, &cl, &cc);
    for (size_t i = 0; i < da_length(buf->lines); ++i) {
        if (y < -font_size) {
            y += font_size + inner_pad;
            continue;
        }
        Line line = buf->lines[i];
        
        const char* lstr = TextFormat("%d ", i + 1);
        Vector2 lsize = MeasureTextEx(font, lstr, font_size, 0);
        DrawTextEx(font, lstr, (Vector2) {pad + posx - lsize.x + line_size, y}, font_size, 0, MIDDLEGROUND);

        if (cl == i && select_line && buf->is_searching == 0) {
            int str[line.end-line.start];
            memcpy(str, buf->content + line.start, (line.end-line.start)*sizeof(int));
            char* utf8_string = LoadUTF8(str, line.end - line.start);
            Vector2 size = {0};
            if (line.end-line.start == 0) size = (Vector2) {.x = 0, .y = font_size};
            else size = MeasureTextEx(font, utf8_string, font_size, 0);
            UnloadUTF8(utf8_string);
            DrawRectangle(pad + line_size + posx, y, size.x, size.y, FAINT_FG);
        }

        if (selection && buf->selection_origin > (int) buf->cursor && buf->is_searching == 0) {
            if (cl == i && cl == sl) {
                int str[sc-cc], strl = sc-cc;
                memcpy(str, buf->content + buf->cursor, strl*sizeof(int));
                char* utf8_string = LoadUTF8(str, strl);
                Vector2 size = {0};
                if (sc-cc == 0) size = (Vector2) {.x = 0, .y = font_size};
                else size = MeasureTextEx(font, utf8_string, font_size, 0);
                UnloadUTF8(utf8_string);
                
                int sstr[cc], sstrl = cc;
                memcpy(sstr, buf->content + line.start, sstrl*sizeof(int));
                utf8_string = LoadUTF8(sstr, sstrl);
                Vector2 ssize = {0};
                if (cc == 0) ssize = (Vector2) {.x = 0, .y = font_size};
                else ssize = MeasureTextEx(font, utf8_string, font_size, 0);
                UnloadUTF8(utf8_string);

                DrawRectangle(pad + line_size + posx + ssize.x, y, size.x, size.y, select_line?MIDDLEGROUND:FAINT_FG);
            } else if (cl == i && cl != sl) {
                int str[cc], strl = cc;
                memcpy(str, buf->content + line.start, strl*sizeof(int));
                char* utf8_string = LoadUTF8(str, strl);
                Vector2 size = {0};
                if (cc == 0) size = (Vector2) {.x = 0, .y = font_size};
                else size = MeasureTextEx(font, utf8_string, font_size, 0);
                UnloadUTF8(utf8_string);
                
                int sstr[(line.end-line.start)-cc], sstrl = (line.end-line.start)-cc;
                memcpy(sstr, buf->content + line.start + cc, sstrl*sizeof(int));
                utf8_string = LoadUTF8(sstr, sstrl);
                Vector2 ssize = {0};
                if (line.end-line.start-cc == 0) ssize = (Vector2) {.x = 0, .y = font_size};
                else ssize = MeasureTextEx(font, utf8_string, font_size, 0);
                UnloadUTF8(utf8_string);

                DrawRectangle(pad + line_size + posx + size.x, y, ssize.x, ssize.y, select_line?MIDDLEGROUND:FAINT_FG);
            } else if (sl == i && cl != sl) {
                int str[sc], strl = sc;
                memcpy(str, buf->content + line.start, strl*sizeof(int));
                char* utf8_string = LoadUTF8(str, strl);
                Vector2 size = {0};
                if (sc == 0) size = (Vector2) {.x = 0, .y = font_size};
                else size = MeasureTextEx(font, utf8_string, font_size, 0);
                UnloadUTF8(utf8_string);
                
                DrawRectangle(pad + line_size + posx, y, size.x, size.y, select_line?MIDDLEGROUND:FAINT_FG);
            } else if (cl < i && sl > i) {
                int str[line.end-line.start], strl = line.end-line.start;
                memcpy(str, buf->content + line.start, strl*sizeof(int));
                char* utf8_string = LoadUTF8(str, strl);
                Vector2 size = {0};
                if (line.end-line.start == 0) size = (Vector2) {.x = 0, .y = font_size};
                else size = MeasureTextEx(font, utf8_string, font_size, 0);
                UnloadUTF8(utf8_string);
                
                DrawRectangle(pad + line_size + posx, y, size.x, size.y, select_line?MIDDLEGROUND:FAINT_FG);
            }
        } else if (selection && buf->selection_origin < (int) buf->cursor && buf->is_searching == 0) {
            if (cl == i && cl == sl) {
                int str[cc-sc], strl = cc-sc;
                memcpy(str, buf->content + buf->selection_origin, strl*sizeof(int));
                char* utf8_string = LoadUTF8(str, strl);
                Vector2 size = {0};
                if (cc-sc == 0) size = (Vector2) {.x = 0, .y = font_size};
                else size = MeasureTextEx(font, utf8_string, font_size, 0);
                UnloadUTF8(utf8_string);
                
                int sstr[sc], sstrl = sc;
                memcpy(sstr, buf->content + line.start, sstrl*sizeof(int));
                utf8_string = LoadUTF8(sstr, sstrl);
                Vector2 ssize = {0};
                if (sc == 0) ssize = (Vector2) {.x = 0, .y = font_size};
                else ssize = MeasureTextEx(font, utf8_string, font_size, 0);
                UnloadUTF8(utf8_string);

                DrawRectangle(pad + line_size + posx + ssize.x, y, size.x, size.y, select_line?MIDDLEGROUND:FAINT_FG);
            } else if (cl == i && cl != sl) {
                int str[cc], strl = cc;
                memcpy(str, buf->content + line.start, strl*sizeof(int));
                char* utf8_string = LoadUTF8(str, strl);
                Vector2 size = {0};
                if (cc == 0) size = (Vector2) {.x = 0, .y = font_size};
                else size = MeasureTextEx(font, utf8_string, font_size, 0);
                UnloadUTF8(utf8_string);
                
                DrawRectangle(pad + line_size + posx, y, size.x, size.y, select_line?MIDDLEGROUND:FAINT_FG);
            } else if (sl == i && cl != sl) {
                int str[(line.end-line.start)-sc], strl = (line.end-line.start)-sc;
                memcpy(str, buf->content + line.start, strl*sizeof(int));
                char* utf8_string = LoadUTF8(str, strl);
                Vector2 size = {0};
                if (line.end-line.start-sc == 0) size = (Vector2) {.x = 0, .y = font_size};
                else size = MeasureTextEx(font, utf8_string, font_size, 0);
                UnloadUTF8(utf8_string);
                
                int sstr[sc], sstrl = sc;
                memcpy(sstr, buf->content + line.start, sstrl*sizeof(int));
                utf8_string = LoadUTF8(sstr, sstrl);
                Vector2 ssize = {0};
                if (sc == 0) ssize = (Vector2) {.x = 0, .y = font_size};
                else ssize = MeasureTextEx(font, utf8_string, font_size, 0);
                UnloadUTF8(utf8_string);
                
                DrawRectangle(pad + line_size + posx + ssize.x, y, size.x, size.y, select_line?MIDDLEGROUND:FAINT_FG);
            } else if (cl > i && sl < i) {
                int str[line.end-line.start], strl = line.end-line.start;
                memcpy(str, buf->content + line.start, strl*sizeof(int));
                char* utf8_string = LoadUTF8(str, strl);
                Vector2 size = {0};
                if (line.end-line.start == 0) size = (Vector2) {.x = 0, .y = font_size};
                else size = MeasureTextEx(font, utf8_string, font_size, 0);
                UnloadUTF8(utf8_string);
                
                DrawRectangle(pad + line_size + posx, y, size.x, size.y, select_line?MIDDLEGROUND:FAINT_FG);
            }
        }

        draw_text(buf->content + line.start,
                  font, pad+line_size, y, font_size, posx, i, buf->tokens);

        if (cl == i && (!selection || buf->cursor == (size_t) buf->selection_origin) && buf->is_searching == 0) {
            int str[cc];
            memcpy(str, buf->content + line.start, cc*sizeof(int));
            char* utf8_string = LoadUTF8(str, cc);
            size_t size = 0;
            if (cc == 0) size = 0;
            else size = MeasureTextEx(font, utf8_string, font_size, 0).x;
            UnloadUTF8(utf8_string);
            DrawRectangle(size + pad + line_size + posx, y, 2, font_size, FOREGROUND);
        }
    
        y += font_size + inner_pad;
        if (y > GetScreenHeight()) break;
    }

    if (error_time == 1) { error = NULL; error_time = 0; }
    if (error != NULL && error_time == 0) error_time = ERROR_LENGTH + ERROR_FADE;
    if (error != NULL) {
        float alpha = 0;
        if (error_time < ERROR_FADE) alpha = error_time / (float) ERROR_FADE;
        else alpha = 1.0f;

        long p = 8;
        Vector2 error_size = MeasureTextEx(font, error, font_size, 0);
        Rectangle rec = {
            .x = GetScreenWidth()-error_size.x-p*3,
            .y = p,
            .width = error_size.x + p*2,
            .height = error_size.y + p*2,
        };
        DrawRectangleRounded(rec, 0.6, 5, FAINT_FG_A(alpha*255));
        DrawTextEx(font, error, (Vector2) {rec.x+p, rec.y+p}, font_size, 0, FOREGROUND_A(alpha*255));

        error_time--;
    }
}

void print_sb(char* sb) {
    for (size_t i = 0; i < da_length(sb); ++i) printf("%c", sb[i]);
    printf("\n");
}

size_t key_presses[512] = {0};
size_t key_holds[512] = {0};
#define KEY_HOLD_DUR 400
#define KEY_REP_DUR 3

bool key_pressed(int key) {
    if (IsKeyDown(key) && key_presses[key] == 0) {
        key_holds[key] += GetFrameTime()*1000;
        key_presses[key]++;
        return true;
    } else if (IsKeyDown(key) && key_presses[key] > 0) {
        key_holds[key] += GetFrameTime()*1000;
        key_presses[key]++;
        return (key_holds[key] > KEY_HOLD_DUR) && (key_presses[key] % KEY_REP_DUR == 0);
    } else {
        key_presses[key] = 0;
        key_holds[key] = 0;
    }
    return false;
}

#define SEARCHING_NONE 0
#define SEARCHING_SEARCH 1
#define SEARCHING_GOTO 2

void update_buf(Buffer* buf, bool change_lines, bool read_only) {
    int key_char = GetCharPressed();
    while (key_char != 0 && !read_only) {
        if (change_lines) {
            if (key_char == '/') {
                key_char = GetCharPressed();
                continue;
            }
        }
        if (buf->is_searching != 0) {
            da_push(buf->search_buffer, key_char);
        } else {
            if (buf->selection_origin != -1) remove_selection(buf);
            push_at_cursor(buf, key_char);
            buf->changed = true;
            if (IsKeyUp(KEY_LEFT_SHIFT)) buf->selection_origin = -1;
        }
        key_char = GetCharPressed();
    }

    if (buf->is_searching != SEARCHING_NONE) {
        if (key_pressed(KEY_BACKSPACE) && da_length(buf->search_buffer) > 0) {
            da_pop(buf->search_buffer, 0);
        }
        if (key_pressed(KEY_ESCAPE)) {
            buf->is_searching = SEARCHING_NONE;
            da_free(buf->search_buffer);
            buf->search_buffer = da_new(int);
        }
        if (key_pressed(KEY_ENTER)) {
            if (buf->is_searching == SEARCHING_GOTO) {
                char* ustr = LoadUTF8(buf->search_buffer, da_length(buf->search_buffer));
                char* strp;
                size_t line = strtoull(ustr, &strp, 0);
                if (*strp == '\0') {
                    if (line <= 0) line = 1;
                    else if (line > da_length(buf->lines)) line = da_length(buf->lines);
                    buf->cursor = buf->lines[line-1].start;
                    buf->selection_origin = -1;
                }
                buf->is_searching = SEARCHING_NONE;
                da_free(buf->search_buffer);
                buf->search_buffer = da_new(int);
                UnloadUTF8(ustr);
            } else if (buf->is_searching == SEARCHING_SEARCH) {
                for (size_t i = buf->cursor; i < da_length(buf->content) - da_length(buf->search_buffer); ++i) {
                    if (memcmp(buf->content + i, buf->search_buffer, da_length(buf->search_buffer)*sizeof(int)) == 0) {
                        buf->cursor = i;
                        buf->selection_origin = i + da_length(buf->search_buffer);
                        break;
                    }
                }
                buf->is_searching = SEARCHING_NONE;
                da_free(buf->search_buffer);
                buf->search_buffer = da_new(int);
            }
        }

        return;
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && key_pressed(KEY_G)) {
        buf->is_searching = SEARCHING_GOTO;
        return;
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && key_pressed(KEY_A)) {
        buf->cursor = 0;
        buf->selection_origin = buf->lines[da_length(buf->lines)-1].end;
        return;
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && key_pressed(KEY_Q)) {
        size_t cursor_line, cursor_char;
        buf_get_cursor(buf, &cursor_line, &cursor_char);
        buf->cursor = buf->lines[cursor_line].end;
        buf->selection_origin = buf->lines[cursor_line].start;
        return;
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && key_pressed(KEY_F)) {
        buf->is_searching = SEARCHING_SEARCH;
        return;
    }

    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        if (buf->selection_origin == -1) buf->selection_origin = buf->cursor;
    }

    if (key_pressed(KEY_ENTER) && !change_lines && !read_only) {
        if (buf->selection_origin != -1) remove_selection(buf);
        size_t l, c;
        buf_get_cursor(buf, &l, &c);
        Line line = buf->lines[l];
        size_t spaces = 0;
        for (size_t i = line.start; i < line.end; ++i) {
            if (buf->content[i] == ' ') spaces++;
            else break;
        }
        push_at_cursor(buf, '\n');
        for (size_t i = 0; i < spaces; ++i) push_at_cursor(buf, ' ');
        buf->changed = true;
    } else if (key_pressed(KEY_DELETE) && !read_only) {
        if (buf->selection_origin != -1) remove_selection(buf);
        else if (buf->cursor < da_length(buf->content) && (change_lines ? buf->content[buf->cursor] != '\n' : true)) {
            int temp[da_length(buf->content) - buf->cursor - 1];
            memcpy(temp, buf->content + buf->cursor + 1, (da_length(buf->content) - buf->cursor - 1)*sizeof(int));
            memcpy(buf->content + buf->cursor, temp, (da_length(buf->content) - buf->cursor - 1)*sizeof(int));
            da_pop(buf->content, 0);
            buf->changed = true;
            if (IsKeyUp(KEY_LEFT_SHIFT)) buf->selection_origin = -1;
        }
    } else if (key_pressed(KEY_BACKSPACE) && !read_only) {
        if (buf->selection_origin != -1) remove_selection(buf);
        else if (buf->cursor > 0 && (change_lines ? buf->content[buf->cursor-1] != '\n' : true)) {
            int temp[da_length(buf->content) - buf->cursor];
            memcpy(temp, buf->content + buf->cursor, sizeof(int)*(da_length(buf->content) - buf->cursor));
            memcpy(buf->content + buf->cursor - 1, temp, sizeof(int)*(da_length(buf->content) - buf->cursor));
            da_pop(buf->content, 0);
            buf->cursor -= 1;
            buf->changed = true;
            if (IsKeyUp(KEY_LEFT_SHIFT)) buf->selection_origin = -1;
        }
    } else if (key_pressed(KEY_TAB) && !read_only) {
        if (buf->selection_origin != -1) remove_selection(buf);
        push_at_cursor(buf, ' ');
        push_at_cursor(buf, ' ');
        push_at_cursor(buf, ' ');
        push_at_cursor(buf, ' ');
        buf->changed = true;
        if (IsKeyUp(KEY_LEFT_SHIFT)) buf->selection_origin = -1;
    } else if (key_pressed(KEY_LEFT)) {
        if (buf->cursor > 0) {
            buf->cursor -= 1;
        }
        if (IsKeyUp(KEY_LEFT_SHIFT)) buf->selection_origin = -1;
    } else if (key_pressed(KEY_RIGHT)) {
        if (buf->cursor < da_length(buf->content)) {
            buf->cursor += 1;
        }
        if (IsKeyUp(KEY_LEFT_SHIFT)) buf->selection_origin = -1;
    } else if (key_pressed(KEY_UP)) {
        size_t l, c;
        buf_get_cursor(buf, &l, &c);
        if (l > 0) {
            Line next_line = buf->lines[l - 1];
            if (next_line.end - next_line.start < c) {
                buf->cursor = next_line.end;
            } else {
                buf->cursor = next_line.start + c;
            }
        }
        if (IsKeyUp(KEY_LEFT_SHIFT)) buf->selection_origin = -1;
    } else if (key_pressed(KEY_DOWN)) {
        size_t l, c;
        buf_get_cursor(buf, &l, &c);
        if (l < da_length(buf->lines)-1) {
            Line next_line = buf->lines[l + 1];
            if (next_line.end - next_line.start < c) {
                buf->cursor = next_line.end;
            } else {
                buf->cursor = next_line.start + c;
            }
        }
        if (IsKeyUp(KEY_LEFT_SHIFT)) buf->selection_origin = -1;
    }

    if (change_lines) buf->changed = false;

    update_newlines(buf);
    color_highlight(buf);
}

void draw_statusbar(Buffer* buf, Font font, size_t font_size) {
    int pad = 8;

    size_t l, c;
    buf_get_cursor(buf, &l, &c);

    int ww = GetScreenWidth();
    int wh = GetScreenHeight();
    
    DrawRectangle(0, wh - font_size - pad*2, ww, wh, FAINT_FG);
    
    char* str;
    if (buf->filename != 0) {
        str = LoadUTF8(buf->filename, buf->filenamel);
    }

    const char* lstatus;
    if (buf->is_searching == SEARCHING_NONE) lstatus = TextFormat("%s%s", buf->filename == 0 ? "<new file>" : basename(str), buf->changed ? "*" : "");
    else if (buf->is_searching == SEARCHING_GOTO) {
        char* ustr = LoadUTF8(buf->search_buffer, da_length(buf->search_buffer));
        lstatus = TextFormat("line: %s", ustr);
        UnloadUTF8(ustr);
    } else if (buf->is_searching == SEARCHING_SEARCH) {
        char* ustr = LoadUTF8(buf->search_buffer, da_length(buf->search_buffer));
        lstatus = TextFormat("find: %s", ustr);
        UnloadUTF8(ustr);
    }
    Vector2 lssize = MeasureTextEx(font, lstatus, font_size, 0);
    if (buf->is_searching == SEARCHING_GOTO || buf->is_searching == SEARCHING_SEARCH) {
        DrawRectangle(pad + lssize.x, wh-lssize.y-pad, 2, lssize.y, FOREGROUND);
    }
    DrawTextEx(font, lstatus, (Vector2) {pad, wh - lssize.y - pad}, font_size, 0, FOREGROUND);
    
    const char* rstatus = TextFormat("%ld:%ld", l+1, c+1);
    Vector2 rssize = MeasureTextEx(font, rstatus, font_size, 0);
    DrawTextEx(font, rstatus, (Vector2) {ww - rssize.x - pad, wh - lssize.y - pad}, font_size, 0, FOREGROUND);
}

#define STATE_TEXT 0
#define STATE_SAVE 1
#define STATE_OPEN 2
#define STATE_HELP 3

int state = STATE_TEXT;

Font load_font(size_t font_size) {
    int codepoints[512] = { 0 };
    for (int i = 0; i < 95; i++) codepoints[i] = 32 + i;
    for (int i = 0; i < 255; i++) codepoints[96 + i] = 0x400 + i;
    if (FileExists("font.ttf") && !DirectoryExists("font.ttf"))
        return LoadFontEx("font.ttf", font_size, codepoints, 512);
    else return LoadFontFromMemory(".ttf", __FONT_TTF, __FONT_TTF_LENGTH, font_size, codepoints, 512);
}

int main(int argc, char** argv) {
    Buffer buf = {0};
    if (argc == 2) {
        if (FileExists(argv[1]) && !DirectoryExists(argv[1])) {
            size_t flen = strlen(argv[1]);
            char* fname = malloc(flen+1);
            fname[flen] = 0;
            memcpy(fname, argv[1], flen);
            init_buf_from_file(&buf, fname);
        } else {
            init_buf(&buf);
        }
    } else {
        init_buf(&buf);
    }
    Buffer open_buffer = {0};
    Buffer save_buffer = {0};
    Buffer help_buffer = {0};

    bool debug = false;
    load_config();

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTraceLogLevel(LOG_NONE);
    InitWindow(800, 600, "txt");
    SetExitKey(0);
    init_open_buffer(&open_buffer);
    init_save_buffer(&save_buffer);
    init_help_buffer(&help_buffer);
    int font_size = 24;
    Font font = load_font(font_size);

    Image icon = LoadImageFromMemory(".png", _ICON_PNG, _ICON_PNG_LENGTH);
    SetWindowIcon(icon);
        
    int pad = 8, inner_pad = 2, pos = 0, posx = 0, lines_size = 80;

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        size_t l, c;
        size_t lp, cp;
        Buffer cursorbuf = state == STATE_TEXT ? buf : state == STATE_OPEN ? open_buffer : state == STATE_SAVE ? save_buffer : help_buffer;
        buf_get_cursor(&cursorbuf, &l, &c);
        buf_get_cursor_pos(&cursorbuf, font, font_size, &lp, &cp);

        Vector2 mouse_pos = GetMousePosition();
        if (mouse_pos.y >= GetScreenHeight() - font_size - pad*2) SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        else if (mouse_pos.x > lines_size) SetMouseCursor(MOUSE_CURSOR_IBEAM);
        else SetMouseCursor(MOUSE_CURSOR_DEFAULT);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            int tx = mouse_pos.x - lines_size - posx;
            if (tx > 0 && mouse_pos.y < GetScreenHeight() - font_size - pad*2) {
                int ty = (mouse_pos.y + pos*(font_size+inner_pad) - pad) / (font_size+inner_pad);
                if (ty < (int) da_length(buf.lines)) {
                    Line line = buf.lines[ty];
                    int str[line.end-line.start];
                    memcpy(str, buf.content + line.start, sizeof(int)*(line.end-line.start));
                    char* ustr = LoadUTF8(str, line.end-line.start);
                    Vector2 str_size = MeasureTextEx(font, ustr, font_size, 0);
                    UnloadUTF8(ustr);
                    if (str_size.x < tx) buf.cursor = line.end;
                    else {
                        for (int i = line.end-line.start-1; i >= 0; --i) {
                            ustr = LoadUTF8(str, i);
                            str_size = MeasureTextEx(font, ustr, font_size, 0);
                            UnloadUTF8(ustr);
                            if (str_size.x < tx) { buf.cursor = i + line.start; break; }
                        }
                    }
                }
            }
        } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            buf.selection_origin = buf.cursor;
            int tx = mouse_pos.x - lines_size - posx;
            if (tx > 0 && mouse_pos.y < GetScreenHeight() - font_size - pad*2) {
                int ty = (mouse_pos.y + pos*(font_size+inner_pad) - pad) / (font_size+inner_pad);
                if (ty < (int) da_length(buf.lines)) {
                    Line line = buf.lines[ty];
                    int str[line.end-line.start];
                    memcpy(str, buf.content + line.start, sizeof(int)*(line.end-line.start));
                    char* ustr = LoadUTF8(str, line.end-line.start);
                    Vector2 str_size = MeasureTextEx(font, ustr, font_size, 0);
                    UnloadUTF8(ustr);
                    if (str_size.x < tx) buf.selection_origin = line.end;
                    else {
                        for (int i = line.end-line.start-1; i >= 0; --i) {
                            ustr = LoadUTF8(str, i);
                            str_size = MeasureTextEx(font, ustr, font_size, 0);
                            UnloadUTF8(ustr);
                            if (str_size.x < tx) { buf.selection_origin = i + line.start; break; }
                        }
                    }
                }
            }
        }
        
        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            if (key_pressed(KEY_EQUAL) && font_size <= 64) {
                font_size += 2;
                UnloadFont(font);
                font = load_font(font_size);
            } else if (key_pressed(KEY_MINUS) && font_size >= 16) {
                font_size -= 2;
                UnloadFont(font);
                font = load_font(font_size);
            } else if (key_pressed(KEY_D)) {
                debug = !debug;
            } else if (key_pressed(KEY_L)) {
                if (lines_size == 80) lines_size = 0;
                else if (lines_size == 0) lines_size = 80;
            } else if (key_pressed(KEY_C) && buf.selection_origin != -1) {
                size_t start = buf.cursor;
                size_t end = buf.selection_origin;
                if (start > end) {
                    start = buf.selection_origin;
                    end = buf.cursor;
                }
                int clipboard[end-start];
                memcpy(clipboard, buf.content + start, sizeof(int)*(end-start));
                char* utf8string = LoadUTF8(clipboard, end-start);
                SetClipboardText(utf8string);
                UnloadUTF8(utf8string);
                update_newlines(&buf);
                color_highlight(&buf);
            }
        }

        if (state == STATE_TEXT) {
            if (IsKeyDown(KEY_LEFT_CONTROL)) {
                if (IsKeyDown(KEY_LEFT_SHIFT)) {
                    if (key_pressed(KEY_S)) {
                        state = STATE_SAVE;
                    }
                } else if (key_pressed(KEY_H)) {
                    state = STATE_HELP;
                } else if (key_pressed(KEY_O)) {
                    state = STATE_OPEN;
                } else if (key_pressed(KEY_S)) {
                    if (buf.filename == 0) {
                        state = STATE_SAVE;
                    } else {
                        save_file(&buf);
                    }
                } else if (key_pressed(KEY_V) && buf.readonly == false) {
                    if (buf.selection_origin != -1) {
                        remove_selection(&buf);
                    }
                    const char* clipboard = GetClipboardText();
                    int cliplen;
                    int* clipcodep = LoadCodepoints(clipboard, &cliplen);
                    for (int i = 0; i < cliplen; ++i) {
                        if (clipcodep[i] == 0x0d) continue;
                        push_at_cursor(&buf, clipcodep[i]);
                    }
                    update_newlines(&buf);
                    color_highlight(&buf);
                    UnloadCodepoints(clipcodep);
                } else if (key_pressed(KEY_X) && buf.readonly == false && buf.selection_origin != -1) {
                    size_t start = buf.cursor;
                    size_t end = buf.selection_origin;
                    if (start > end) {
                        start = buf.selection_origin;
                        end = buf.cursor;
                    }
                    int clipboard[end-start];
                    memcpy(clipboard, buf.content + start, sizeof(int)*(end-start));
                    char* utf8string = LoadUTF8(clipboard, end-start);
                    SetClipboardText(utf8string);
                    UnloadUTF8(utf8string);
                    remove_selection(&buf);
                    update_newlines(&buf);
                    color_highlight(&buf);
                }
            }
            if (buf.readonly) update_buf(&buf, false, true);
            else update_buf(&buf, false, false);
        } else if (state == STATE_OPEN) {
            if (key_pressed(KEY_ESCAPE)) {
                state = STATE_TEXT;
            } else if (key_pressed(KEY_ENTER)) {
                size_t l, c;
                buf_get_cursor(&open_buffer, &l, &c);
                if (l > 1) {
                    if (l == 2) {
                        ChangeDirectory("..");
                        deinit_buf(&open_buffer);
                        init_open_buffer(&open_buffer);
                    } else {
                        int* line = open_buffer.content + open_buffer.lines[l].start;
                        size_t line_length = open_buffer.lines[l].end - open_buffer.lines[l].start;
                        char* utf8_string = LoadUTF8(line, line_length);
                        if (utf8_string[line_length-1] == '/') {
                            ChangeDirectory(utf8_string);
                            deinit_buf(&open_buffer);
                            init_open_buffer(&open_buffer);
                        } else {
                            deinit_buf(&buf);
                            init_buf_from_file(&buf, utf8_string);
                            state = STATE_TEXT;
                        }
                        UnloadUTF8(utf8_string);
                    }
                }
            }
            update_buf(&open_buffer, false, false);
        } else if (state == STATE_SAVE) {
            size_t l, c;
            buf_get_cursor(&save_buffer, &l, &c);
            if (key_pressed(KEY_ESCAPE)) {
                state = STATE_TEXT;
            } else if (key_pressed(KEY_ENTER)) {
                Line line = save_buffer.lines[l];
                if (l == 1) {
                    const char* cwd = GetWorkingDirectory();
                    size_t cwdl = strlen(cwd);
                    int utfl;
                    int* utfs = LoadCodepoints(cwd, &utfl);
                    int strl = cwdl + 1 + line.end - line.start;
                    if (line.end - line.start == 0) {
                        UnloadCodepoints(utfs);
                    } else {
                        int* str = malloc(strl*sizeof(int));
                        memcpy(str + utfl + 1, save_buffer.content + line.start, sizeof(int)*(line.end-line.start));
                        memcpy(str, utfs, utfl*sizeof(int));
                        str[utfl] = '/';
                        
                        free(buf.filename);
                        buf.filename = str;
                        buf.filenamel = strl;
                        save_file(&buf);
                        state = STATE_TEXT;
                    }
                } else if (l > 1) {
                    if (l == 2) {
                        ChangeDirectory("..");
                        deinit_buf(&save_buffer);
                        init_save_buffer(&save_buffer);
                    } else if (save_buffer.content[line.end - 1] == '/') {
                        int str[line.end-line.start];
                        memcpy(str, save_buffer.content + line.start, line.end-line.start);
                        char* ustr = LoadUTF8(str, line.end-line.start);
                        ChangeDirectory(ustr);
                        UnloadUTF8(ustr);
                        deinit_buf(&save_buffer);
                        init_save_buffer(&save_buffer);
                    }
                }
            }
            if (l == 1) {
                if (key_pressed(KEY_V) && save_buffer.readonly == false) {
                    if (save_buffer.selection_origin != -1) {
                        remove_selection(&save_buffer);
                    }
                    const char* clipboard = GetClipboardText();
                    int cliplen;
                    int* clipcodep = LoadCodepoints(clipboard, &cliplen);
                    for (int i = 0; i < cliplen; ++i) {
                        push_at_cursor(&save_buffer, clipcodep[i]);
                    }
                    update_newlines(&save_buffer);
                    color_highlight(&save_buffer);
                    UnloadCodepoints(clipcodep);
                } else if (key_pressed(KEY_X) && save_buffer.readonly == false && save_buffer.selection_origin != -1) {
                    size_t start = save_buffer.cursor;
                    size_t end = save_buffer.selection_origin;
                    if (start > end) {
                        start = save_buffer.selection_origin;
                        end = save_buffer.cursor;
                    }
                    int clipboard[end-start];
                    memcpy(clipboard, save_buffer.content + start, sizeof(int)*(end-start));
                    char* utf8string = LoadUTF8(clipboard, end-start);
                    SetClipboardText(utf8string);
                    UnloadUTF8(utf8string);
                    remove_selection(&save_buffer);
                    update_newlines(&save_buffer);
                    color_highlight(&save_buffer);
                }
                update_buf(&save_buffer, true, false);
            } update_buf(&save_buffer, true, true);
        } else if (state == STATE_HELP) {
            if (key_pressed(KEY_ESCAPE)) state = STATE_TEXT;
            update_buf(&save_buffer, true, true);
        }

        cp += lines_size + pad;
        
        if ((int)cp+posx > GetScreenWidth() - pad) {
            posx = GetScreenWidth()-cp-pad;
        }
        if ((int)cp < -posx + lines_size + pad) {
            posx = -cp + lines_size + pad;
        }

        while (l > GetScreenHeight()/font_size*0.8+pos) {
            pos += 1;
        }
        while (l < (size_t) pos) {
            pos -= 1;
        }

        BeginDrawing();
            ClearBackground(BACKGROUND);
            if (debug) DrawFPS(10, 10);
            if (state == STATE_TEXT) {
                draw_buffer(&buf, font, font_size, pos, posx, lines_size, pad, false, inner_pad);
                draw_statusbar(&buf, font, font_size);
            } else if (state == STATE_OPEN) {
                draw_buffer(&open_buffer, font, font_size, pos, posx, lines_size, pad, true, inner_pad);
                draw_statusbar(&open_buffer, font, font_size);
            } else if (state == STATE_SAVE) {
                draw_buffer(&save_buffer, font, font_size, pos, posx, lines_size, pad, true, inner_pad);
                draw_statusbar(&save_buffer, font, font_size);
            } else if (state == STATE_HELP) {
                draw_buffer(&help_buffer, font, font_size, pos, posx, lines_size, pad, false, inner_pad);
                draw_statusbar(&help_buffer, font, font_size);
            }
        EndDrawing();
    }

    deinit_buf(&open_buffer);
    deinit_buf(&save_buffer);
    deinit_buf(&buf);
    UnloadImage(icon);
    CloseWindow();

    return 0;
}
