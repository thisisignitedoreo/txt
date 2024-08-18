
#include "raylib.h"
#include <libgen.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include "font.c"

#define DA_IMPL
#include "da.h"

#define BACKGROUND   (Color){18,  18,  18,  255}
#define FOREGROUND   (Color){255, 255, 255, 255}
#define MIDDLEGROUND (Color){150, 150, 150, 255}
#define FAINT_FG     (Color){50,  50,  50,  255}

#define DEFAULT      (Color){255, 255, 255, 255}
#define COMMENT      (Color){190, 255, 181, 255}
#define NUMBER       (Color){181, 219, 255, 255}
#define KWORD        (Color){252, 237, 162, 255}
#define STRING       (Color){252, 167, 162, 255}
#define PREPROCESSOR (Color){218, 181, 255, 255}

typedef struct {
    size_t start;
    size_t end;
} Line;

typedef struct {
    size_t line;
    size_t start;
    size_t end;
    Color color;
} Token;

typedef struct {
    const char* filename;
    char* content;
    Line* lines;
    size_t cursor;
    Token* tokens;
    bool changed;
    bool readonly;
} Buffer;

Buffer* buffers = 0;
size_t buffers_length = 0;
size_t current_buffer = 0;

char* sized_str_to_cstr(char* str, size_t length) {
    char* res = malloc(length + 1);
    memcpy(res, str, length);
    res[length] = '\0';
    return res;
}

void buf_get_cursor_pos(Buffer* buf, Font font, int font_size, size_t* lp, size_t* cp) {
    int y = 0, x = 0;
    Vector2 lsize;
    for (size_t l = 0; l < da_length(buf->lines); ++l) {
        Line line = buf->lines[l];
        
        if (line.start <= buf->cursor && buf->cursor <= line.end) {
            char str[buf->cursor - line.start + 1];
            memcpy(str, buf->content + line.start, buf->cursor - line.start);
            str[buf->cursor - line.start] = '\0';
            lsize = MeasureTextEx(font, str, font_size, 0);
            x = lsize.x;
            break;
        }

        y += font_size;
    }
    *lp = y;
    *cp = x;
}

void buf_get_cursor(Buffer* buf, size_t* lp, size_t* cp) {
    Line res_line = {0};
    size_t res_l = 0;
    for (size_t l = 0; l < da_length(buf->lines); ++l) {
        Line line = buf->lines[l];
        if (line.start <= buf->cursor && buf->cursor <= line.end) {
            res_line = line;
            res_l = l;
            break;
        }
    }
    *lp = res_l;
    *cp = buf->cursor - res_line.start;
}

void draw_text(char* str, Font font, size_t x, size_t y,
               size_t font_size, int posx, size_t line_num, Token* tokens) {
    size_t dx = 0;

    for (size_t i = 0; i < da_length(tokens); ++i) {
        Token token = tokens[i];
        size_t token_length = token.end - token.start;
        if (token.line == line_num && token_length > 0) {
            char dstr[token_length+1];
            dstr[token_length] = '\0';
            memcpy(dstr, str + token.start, token_length);
            DrawTextEx(font, dstr, (Vector2) {(float) dx+x+posx, (float) y}, font_size, 0, token.color);
            Vector2 text_size = MeasureTextEx(font, dstr, font_size, 0);
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

float lerp(float a, float b, float t) {
    return a + ((b - a) * t);
}

Color* lerp_color(Color c1, Color c2, float t) {
    Color* a = malloc(sizeof(Color));
    a->r = lerp(c1.r, c2.r, t);
    a->g = lerp(c1.g, c2.g, t);
    a->b = lerp(c1.b, c2.b, t);
    a->a = lerp(c1.a, c2.a, t);
    return a;
}

void draw_buffer(Buffer* buf, Font font, int font_size, int posy, int posx, int line_size, int pad, bool select_line) {
    int inner_pad = 2;
    int y = pad - posy * font_size - posy * inner_pad;
    int drawn_y = y;
    size_t cl, cc;
    buf_get_cursor(buf, &cl, &cc);
    for (size_t i = 0; i < da_length(buf->lines); ++i) {
        if (drawn_y < -font_size) {
            y += font_size + inner_pad;
            drawn_y += font_size + inner_pad;
            continue;
        }
        Line line = buf->lines[i];
        
        const char* lstr = TextFormat("%d ", i + 1);
        Vector2 lsize = MeasureTextEx(font, lstr, font_size, 0);
        DrawTextEx(font, lstr, (Vector2) {pad + posx - lsize.x + line_size, y}, font_size, 0, MIDDLEGROUND);

        if (cl == i && select_line) {
            char* str = sized_str_to_cstr(buf->content + line.start, line.end-line.start);
            Vector2 size = MeasureTextEx(font, str, font_size, 0);
            free(str);
            DrawRectangle(pad + line_size + posx, y, size.x, size.y, FAINT_FG);
        }

        draw_text(buf->content + line.start,
                  font, pad+line_size, y, font_size, posx, i, buf->tokens);

        if (cl == i) {
            char* str = sized_str_to_cstr(buf->content + line.start, cc);
            Vector2 size = MeasureTextEx(font, str, font_size, 0);
            DrawRectangle(size.x + pad + line_size + posx, y, 2, font_size, FOREGROUND);
            free(str);
        }
    
        y += font_size + inner_pad;
        drawn_y += font_size + inner_pad;
        if (drawn_y > GetScreenHeight()) break;
    }
}

bool needlehaystack(char needle, char* haystack) {
    while (*haystack != 0) {
        if (needle == *haystack) return true;
        haystack++;
    }
    return false;
}

const char *keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do", "double",
    "else", "enum", "extern", "float", "for", "goto", "if", "int", "long", "register",
    "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
    "union", "unsigned", "void", "volatile", "while", "alignas", "alignof", "and",
    "and_eq", "asm", "atomic_cancel", "atomic_commit", "atomic_noexcept", "bitand",
    "bitor", "bool", "catch", "char16_t", "char32_t", "char8_t", "class", "co_await",
    "co_return", "co_yield", "compl", "concept", "const_cast", "consteval", "constexpr",
    "constinit", "decltype", "delete", "dynamic_cast", "explicit", "export", "false",
    "friend", "inline", "mutable", "namespace", "new", "noexcept", "not", "not_eq",
    "nullptr", "operator", "or", "or_eq", "private", "protected", "public", "reflexpr",
    "reinterpret_cast", "requires", "static_assert", "static_cast", "synchronized",
    "template", "this", "thread_local", "throw", "true", "try", "typeid", "typename",
    "using", "virtual", "wchar_t", "xor", "xor_eq",
};
int keywords_length = 97;

bool needlehaystack_string(const char* string, const char** list, size_t list_length) {
    for (size_t i = 0; i < list_length; ++i) {
        if (strcmp(string, list[i]) == 0) return true;
    }
    return false;
}

bool endswith(const char* str, char* end) {
    size_t strl = strlen(str);
    size_t endl = strlen(end);
    if (strl < endl) return false;
    str += strl-endl;
    return strcmp(str, end) == 0;
}

void color_highlight_simple(Buffer* buf) {
    for (size_t i = 0; i < da_length(buf->lines); i++) {
        int line_length = buf->lines[i].end - buf->lines[i].start;
        Token token = {i, 0, line_length, DEFAULT};
        da_push(buf->tokens, token);
    }
}

void color_highlight_c(Buffer* buf) {
    bool comment = false;
    for (size_t i = 0; i < da_length(buf->lines); i++) {
        bool preprocessor_line = false;
        Line line = buf->lines[i];
        int line_length = line.end-line.start;

        for (int j = 0; j < line_length;) {
            char ch = buf->content[line.start + j];
            if (isalpha(ch) && !comment) {
                size_t length = 1;
                j++; while (isalnum(buf->content[line.start + j]) && j < line_length) { length++; j++; }
                char* b = malloc(length + 1);
                b[length] = '\0';

                memcpy(b, buf->content + line.start + j - length, length);
                bool keyword = needlehaystack_string(b, keywords, keywords_length);
                free(b);

                Token token = {i, j-length, j, preprocessor_line ? PREPROCESSOR : keyword ? KWORD : DEFAULT};
                da_push(buf->tokens, token);
            } else if (isdigit(ch) && !comment) {
                size_t length = 1;
                j++; while ((isdigit(buf->content[line.start + j]) || needlehaystack(buf->content[line.start + j], "xlL.abcdefABCDEF")) && j < line_length) { length++; j++; }
                Token token = {i, j-length, j, NUMBER};
                da_push(buf->tokens, token);
            } else if ((ch == '"' || ch == '\'') && !comment) {
                size_t length = 0;
                if (j < line_length) { j++; length++; }
                while (buf->content[line.start + j] != ch && j < line_length) { length++; j++; }
                if (j < line_length) { j++; length++; }
                Token token = {i, j-length, j, STRING};
                da_push(buf->tokens, token);
            } else if (ch == '<' && preprocessor_line && !comment) {
                size_t length = 0;
                if (j < line_length) { j++; length++; }
                while (buf->content[line.start + j] != '>' && j < line_length) { length++; j++; }
                if (j < line_length) { j++; length++; }
                Token token = {i, j-length, j, STRING};
                da_push(buf->tokens, token);
            } else {
                if (comment) {
                    bool comment_end = false;
                    for (int c = 0; c < line_length; c++) {
                        if (buf->content[line.start + j + c] == '*' && j < line_length-1 && buf->content[line.start + j + c + 1] == '/') {
                            Token token = {i, 0, j+c+2, COMMENT};
                            da_push(buf->tokens, token);
                            j += c + 2;
                            comment = false;
                            comment_end = true;
                            break;
                        }
                    }
                    if (!comment_end) {
                        Token token = {i, 0, line_length, COMMENT};
                        da_push(buf->tokens, token);
                        j = line_length;
                    }
                    continue;
                }
                Color color = {0};
                if (ch == '#') {
                    color = PREPROCESSOR;
                    preprocessor_line = true;
                } else if (ch == '/' && j < line_length-1 && buf->content[line.start + j + 1] == '/') {
                    Token token = {i, j, line_length, COMMENT};
                    da_push(buf->tokens, token);
                    j = line_length;
                } else if (ch == '/' && j < line_length-1 && buf->content[line.start + j + 1] == '*') {
                    comment = true;
                    for (int c = j; c < line_length; c++) {
                        if (buf->content[line.start + c] == '*' && j < line_length-1 && buf->content[line.start + c + 1] == '/') {
                            comment = false;
                            Token token = {i, j, c + 2, COMMENT};
                            da_push(buf->tokens, token);
                            j = c+2;
                            break;
                        }
                    }
                    if (comment) {
                        Token token = {i, j, line_length, COMMENT};
                        da_push(buf->tokens, token);
                        j = line_length;
                    }
                    continue;
                } else color = DEFAULT;
                Token token = {i, j, j+1, color};
                da_push(buf->tokens, token);
                j++;
            }
        }
    }
}

void color_highlight_openfile(Buffer* buf) {
    for (size_t i = 0; i < da_length(buf->lines); i++) {
        Color color = {0};

        if (i == 0) {
            color = KWORD;
        } else if (i == 1) {
            color = COMMENT;
        } else if (buf->content[buf->lines[i].end-1] == '/' || memcmp(buf->content + buf->lines[i].start, "..", buf->lines[i].end-buf->lines[i].start) == 0) {
            color = NUMBER;
        } else {
            color = DEFAULT;
        }

        int line_length = buf->lines[i].end - buf->lines[i].start;
        Token token = {i, 0, line_length, color};
        da_push(buf->tokens, token);
    }
}

void color_highlight(Buffer* buf) {
    da_free(buf->tokens);
    buf->tokens = da_new(Token);
    if (buf->filename == 0) color_highlight_simple(buf);
    else if (endswith(buf->filename, ".c") || endswith(buf->filename, ".h"))
        color_highlight_c(buf);
    else if (strcmp(buf->filename, "Open a file...") == 0 || strcmp(buf->filename, "Save a file...") == 0)
        color_highlight_openfile(buf);
    else color_highlight_simple(buf);
}

void update_newlines(Buffer* buf) {
    da_free(buf->lines);
    buf->lines = da_new(Line);
    int start = 0;
    for (size_t i = 0; i < da_length(buf->content); ++i) {
        if (buf->content[i] == '\n') {
            Line line = {start, i};
            da_push(buf->lines, line);
            start = i + 1;
        }
    }
    Line line = {start, da_length(buf->content)};
    da_push(buf->lines, line);
}

void init_help_buffer(Buffer* buf) {
    buf->lines = da_new(Line);
    buf->content = da_new(char);
    buf->tokens = da_new(Token);
    const char* str = "Help Text";
    char* strm = malloc(10);
    strm[9] = '\0';
    memcpy(strm, str, 9);
    buf->filename = strm;

    const char* hstr = "Ctrl-S:       Save a file (select location if didnt save before)\n"
                       "Ctrl-Shift-S: Save a file to a new location\n"
                       "Ctrl-O:       Open a new file\n"
                       "Ctrl-'-':     Decrease font size\n"
                       "Ctrl-'+':     Increase font size\n"
                       "Ctrl-L:       Enable/Disable line counter\n"
                       "ESC:          Escape to Text mode from any other mode (save/open/help)";
    size_t hstrl = strlen(hstr);
    for (size_t i = 0; i < hstrl; ++i) da_push(buf->content, hstr[i]);

    update_newlines(buf);
    color_highlight(buf);
}

void init_buf(Buffer* buf) {
    buf->lines = da_new(Line);
    buf->content = da_new(char);
    buf->tokens = da_new(Token);
    update_newlines(buf);
    color_highlight(buf);
}

void deinit_buf(Buffer* buf) {
    da_free(buf->lines);
    da_free(buf->content);
    da_free(buf->tokens);
    buf->changed = false;
    if (buf->filename != 0) free((char*) buf->filename);
}


void print_sb(char* sb) {
    for (size_t i = 0; i < da_length(sb); ++i) printf("%c", sb[i]);
    printf("\n");
}

void init_open_buffer(Buffer* buf) {
    buf->lines = da_new(Line);
    buf->content = da_new(char);
    buf->tokens = da_new(Token);
    const char* str = "Open a file...";
    char* strm = malloc(15);
    strm[14] = '\0';
    memcpy(strm, str, 14);
    buf->filename = strm;

    const char* path = GetWorkingDirectory();
    FilePathList files = LoadDirectoryFiles(path);
    
    size_t length = strlen(path);
    for (size_t j = 0; j < length; ++j) {
        da_push(buf->content, path[j]);
    }
    da_push(buf->content, '\n');
    da_push(buf->content, '\n');
    
    da_push(buf->content, '.');
    da_push(buf->content, '.');
    da_push(buf->content, '\n');
    
    for (size_t i = 0; i < files.count; ++i) {
        if (DirectoryExists(files.paths[i])) {
            const char* str = basename(files.paths[i]);
            size_t length = strlen(str);
            for (size_t j = 0; j < length; ++j) {
                da_push(buf->content, str[j]);
            }
            da_push(buf->content, '/');
            da_push(buf->content, '\n');
        }
    }

    for (size_t i = 0; i < files.count; ++i) {
        if (!DirectoryExists(files.paths[i])) {
            const char* str = basename(files.paths[i]);
            size_t length = strlen(str);
            for (size_t j = 0; j < length; ++j) {
                da_push(buf->content, str[j]);
            }
            da_push(buf->content, '\n');
        }
    }

    da_pop(buf->content, 0);

    UnloadDirectoryFiles(files);

    update_newlines(buf);
    color_highlight(buf);
}

void init_save_buffer(Buffer* buf) {
    init_open_buffer(buf);
    free((char*) buf->filename);
    const char* str = "Save a file...";
    char* strm = malloc(15);
    strm[14] = '\0';
    memcpy(strm, str, 14);
    buf->filename = strm;
    
    update_newlines(buf);
    color_highlight(buf);
}

void push_at_cursor(Buffer* buf, char charachter) {
    da_push(buf->content, '\0');
    memcpy(buf->content + buf->cursor + 1, buf->content + buf->cursor, da_length(buf->content) - 1 - buf->cursor);
    buf->content[buf->cursor] = charachter;
    buf->cursor += 1;
}

void push_cstr_at_cursor(Buffer* buf, char* string) {
    size_t length = strlen(string);
    for (size_t i = 0; i < length; ++i) {
        push_at_cursor(buf, string[i]);
    }
}

void save_file(Buffer* buf) {
    buf->changed = false;
    FILE* f = fopen(buf->filename, "w");
    fwrite(buf->content, 1, da_length(buf->content), f);
    fclose(f);
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

void update_buf_ro(Buffer* buf) {
    if (key_pressed(KEY_LEFT)) {
        if (buf->cursor > 0) {
            buf->cursor -= 1;
        }
    } else if (key_pressed(KEY_RIGHT)) {
        if (buf->cursor < da_length(buf->content)) {
            buf->cursor += 1;
        }
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
    }

    size_t mouse_wheeled = 3;
    
    float wheel = GetMouseWheelMove();
    if (wheel > 0) {
        size_t l, c;
        buf_get_cursor(buf, &l, &c);
        if (l > mouse_wheeled-1) {
            Line next_line = buf->lines[l - mouse_wheeled];
            if (next_line.end - next_line.start < c) {
                buf->cursor = next_line.end;
            } else {
                buf->cursor = next_line.start + c;
            }
        } else {
            buf->cursor = buf->lines[0].start;
        }
    } else if (wheel < 0) {
        size_t l, c;
        buf_get_cursor(buf, &l, &c);
        if (l < da_length(buf->lines)-mouse_wheeled) {
            Line next_line = buf->lines[l + mouse_wheeled];
            if (next_line.end - next_line.start < c) {
                buf->cursor = next_line.end;
            } else {
                buf->cursor = next_line.start + c;
            }
        } else {
            buf->cursor = buf->lines[da_length(buf->lines)-1].end;
        }
    }

    update_newlines(buf);
    color_highlight(buf);
}

void update_buf(Buffer* buf, bool change_lines) {
    char key_char = GetCharPressed();
    while (key_char != 0) {
        if (change_lines) {
            if (key_char == '/') {
                key_char = GetCharPressed();
                continue;
            }
        }
        push_at_cursor(buf, key_char);
        buf->changed = true;
        key_char = GetCharPressed();
    }

    if (key_pressed(KEY_ENTER) && !change_lines) {
        push_at_cursor(buf, '\n');
        buf->changed = true;
    } else if (key_pressed(KEY_DELETE)) {
        if (buf->cursor < da_length(buf->content) && (change_lines ? buf->content[buf->cursor] != '\n' : true)) {
            char* temp = malloc(da_length(buf->content) - buf->cursor - 1);
            memcpy(temp, buf->content + buf->cursor + 1, da_length(buf->content) - buf->cursor - 1);
            memcpy(buf->content + buf->cursor, temp, da_length(buf->content) - buf->cursor - 1);
            free(temp);
            da_pop(buf->content, 0);
            buf->changed = true;
        }
    } else if (key_pressed(KEY_BACKSPACE)) {
        if (buf->cursor > 0 && (change_lines ? buf->content[buf->cursor-1] != '\n' : true)) {
            char* temp = malloc(da_length(buf->content) - buf->cursor);
            memcpy(temp, buf->content + buf->cursor, da_length(buf->content) - buf->cursor);
            memcpy(buf->content + buf->cursor - 1, temp, da_length(buf->content) - buf->cursor);
            free(temp);
            da_pop(buf->content, 0);
            buf->cursor -= 1;
            buf->changed = true;
        }
    } else if (key_pressed(KEY_TAB)) {
        push_at_cursor(buf, ' ');
        push_at_cursor(buf, ' ');
        push_at_cursor(buf, ' ');
        push_at_cursor(buf, ' ');
        buf->changed = true;
    } else if (key_pressed(KEY_LEFT)) {
        if (buf->cursor > 0) {
            buf->cursor -= 1;
        }
    } else if (key_pressed(KEY_RIGHT)) {
        if (buf->cursor < da_length(buf->content)) {
            buf->cursor += 1;
        }
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
    }

    size_t mouse_wheeled = 3;
    
    float wheel = GetMouseWheelMove();
    if (wheel > 0) {
        size_t l, c;
        buf_get_cursor(buf, &l, &c);
        if (l > mouse_wheeled-1) {
            Line next_line = buf->lines[l - mouse_wheeled];
            if (next_line.end - next_line.start < c) {
                buf->cursor = next_line.end;
            } else {
                buf->cursor = next_line.start + c;
            }
        } else {
            buf->cursor = buf->lines[0].start;
        }
    } else if (wheel < 0) {
        size_t l, c;
        buf_get_cursor(buf, &l, &c);
        if (l < da_length(buf->lines)-mouse_wheeled) {
            Line next_line = buf->lines[l + mouse_wheeled];
            if (next_line.end - next_line.start < c) {
                buf->cursor = next_line.end;
            } else {
                buf->cursor = next_line.start + c;
            }
        } else {
            buf->cursor = buf->lines[da_length(buf->lines)-1].end;
        }
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
    
    const char* lstatus = TextFormat("%s%s", buf->filename == 0 ? "<new file>" : basename((char*) buf->filename), buf->changed ? "*" : "");
    Vector2 lssize = MeasureTextEx(font, lstatus, font_size, 0);
    DrawTextEx(font, lstatus, (Vector2) {pad, wh - lssize.y - pad}, font_size, 0, FOREGROUND);
    
    const char* rstatus = TextFormat("%ld:%ld", l+1, c+1);
    Vector2 rssize = MeasureTextEx(font, rstatus, font_size, 0);
    DrawTextEx(font, rstatus, (Vector2) {ww - rssize.x - pad, wh - lssize.y - pad}, font_size, 0, FOREGROUND);
}

void init_buf_from_file(Buffer* buf, char* fname) {
    buf->filename = fname;
    FILE* f = fopen(fname, "r");
    buf->lines = da_new(Line);
    buf->content = da_new(char);
    buf->tokens = da_new(char);
    size_t size = GetFileLength(fname);
    char* file = malloc(size);
    fread(file, 1, size, f);
    bool good = true;
    char ch = 0;
    for (size_t i = 0; i < size; ++i) {
        if (file[i] < 0x20 || file[i] > 0x7e ) {
            if (file[i] != '\n' && file[i] != '\r' && file[i] != '\t') {
                good = false;
                ch = file[i];
                break;
            }
        }
        da_push(buf->content, file[i]);
    }
    if (!good) {
        da_free(buf->content);
        buf->content = da_new(char);
        const char* str = TextFormat("This file is either non-ascii or binary.\n'%c'", ch); //, ch);
        size_t strl = strlen(str);
        buf->readonly = true;
        for (size_t i = 0; i < strl; i++) {
            da_push(buf->content, str[i]);
        }
    }
    free(file);
    fclose(f);
    update_newlines(buf);
    color_highlight(buf);
}

#define STATE_TEXT 0
#define STATE_SAVE 1
#define STATE_OPEN 2
#define STATE_HELP 3

int state = STATE_TEXT;

Font load_font(size_t font_size) {
    if (FileExists("font.ttf") && !DirectoryExists("font.ttf"))
        return LoadFontEx("font.ttf", font_size, 0, 0);
    else return LoadFontFromMemory(".ttf", __FONT_TTF, __FONT_TTF_LENGTH, font_size, 0, 0);
}

int main(int argc, char** argv) {
    Buffer buf = {0};
    if (argc == 2) {
        if (FileExists(argv[1])) {
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

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTraceLogLevel(LOG_NONE);
    InitWindow(800, 600, "txt");
	SetExitKey(0);
    init_open_buffer(&open_buffer);
    init_save_buffer(&save_buffer);
    init_help_buffer(&help_buffer);

    int font_size = 24;
    Font font = load_font(font_size);
        
    int pad = 8, pos = 0, posx = 0, lines_size = 80;

	SetTargetFPS(60);
    while (!WindowShouldClose()) {
        size_t l, c;
        size_t lp, cp;
        Buffer cursorbuf = state == STATE_TEXT ? buf : state == STATE_OPEN ? open_buffer : state == STATE_SAVE ? save_buffer : help_buffer;
        buf_get_cursor(&cursorbuf, &l, &c);
        buf_get_cursor_pos(&cursorbuf, font, font_size, &lp, &cp);
        
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
                }
            } else {
                if (buf.readonly) update_buf_ro(&buf);
                else update_buf(&buf, false);
            }
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
                        char* line = open_buffer.content + open_buffer.lines[l].start;
                        size_t line_length = open_buffer.lines[l].end - open_buffer.lines[l].start;
                        char* dir = malloc(line_length + 1);
                        dir[line_length] = '\0';
                        memcpy(dir, line, line_length);
                        if (dir[line_length-1] == '/') {
                            ChangeDirectory(dir);
                            deinit_buf(&open_buffer);
                            init_open_buffer(&open_buffer);
                        } else {
                            deinit_buf(&buf);
                            init_buf_from_file(&buf, dir);
                            state = STATE_TEXT;
                        }
                    }
                }
            }
            update_buf_ro(&open_buffer);
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
                    char* str = malloc(cwdl + 1 + line.end-line.start + 1);
                    str[cwdl + 1 + line.end-line.start] = 0;
                    memcpy(str + cwdl + 1, save_buffer.content + line.start, line.end-line.start);
                    memcpy(str, cwd, cwdl);
                    str[cwdl] = '/';
                    
                    free((char*) buf.filename);
                    buf.filename = str;
                    save_file(&buf);
                    state = STATE_TEXT;
                } else if (l > 1) {
                    if (l == 2) {
                        ChangeDirectory("..");
                        deinit_buf(&save_buffer);
                        init_save_buffer(&save_buffer);
                    } else if (save_buffer.content[line.end - 1] == '/') {
                        char str[line.end-line.start + 1];
                        str[line.end-line.start] = 0;
                        memcpy(str, save_buffer.content + line.start, line.end-line.start);
                        ChangeDirectory(str);
                        deinit_buf(&save_buffer);
                        init_save_buffer(&save_buffer);
                    }
                }
            }
            if (l == 1) update_buf(&save_buffer, true);
            else update_buf_ro(&save_buffer);
        } else if (state == STATE_HELP) {
            if (key_pressed(KEY_ESCAPE)) state = STATE_TEXT;
            else update_buf_ro(&help_buffer);
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
                draw_buffer(&buf, font, font_size, pos, posx, lines_size, pad, false);
                draw_statusbar(&buf, font, font_size);
            } else if (state == STATE_OPEN) {
                draw_buffer(&open_buffer, font, font_size, pos, posx, lines_size, pad, true);
                draw_statusbar(&open_buffer, font, font_size);
            } else if (state == STATE_SAVE) {
                draw_buffer(&save_buffer, font, font_size, pos, posx, lines_size, pad, true);
                draw_statusbar(&save_buffer, font, font_size);
            } else if (state == STATE_HELP) {
                draw_buffer(&help_buffer, font, font_size, pos, posx, lines_size, pad, false);
                draw_statusbar(&help_buffer, font, font_size);
            }
		EndDrawing();
	}

    deinit_buf(&open_buffer);
    deinit_buf(&save_buffer);
    deinit_buf(&buf);
	CloseWindow();

	return 0;
}

