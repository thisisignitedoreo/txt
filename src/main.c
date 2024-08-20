
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
#define ERROR        (Color){255, 162, 162, 255}
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
    int* filename;
    size_t filenamel;
    int* content;
    Line* lines;
    size_t cursor;
    Token* tokens;
    bool changed;
    bool readonly;
    int selection_origin;
} Buffer;

Buffer* buffers = 0;
size_t buffers_length = 0;
size_t current_buffer = 0;

void buf_get_cursor_pos(Buffer* buf, Font font, int font_size, size_t* lp, size_t* cp) {
    int y = 0, x = 0;
    Vector2 lsize;
    for (size_t l = 0; l < da_length(buf->lines); ++l) {
        Line line = buf->lines[l];
        
        if (line.start <= buf->cursor && buf->cursor <= line.end) {
            int str[buf->cursor - line.start];
            memcpy(str, buf->content + line.start, sizeof(int)*(buf->cursor - line.start));
            char* utf8_string = LoadUTF8(str, buf->cursor - line.start);
            lsize = MeasureTextEx(font, utf8_string, font_size, 0);
            UnloadUTF8(utf8_string);
            x = lsize.x;
            break;
        }

        y += font_size;
    }
    *lp = y;
    *cp = x;
}

bool buf_get_selection_cursor(Buffer* buf, size_t* lp, size_t* cp) {
    if (buf->selection_origin < 0) return false;
    Line res_line = {0};
    size_t res_l = 0;
    for (size_t l = 0; l < da_length(buf->lines); ++l) {
        Line line = buf->lines[l];
        if ((int) line.start <= buf->selection_origin && buf->selection_origin <= (int) line.end) {
            res_line = line;
            res_l = l;
            break;
        }
    }
    *lp = res_l;
    *cp = buf->selection_origin - res_line.start;
    return true;
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
    size_t cl, cc, sl, sc;
    bool selection = buf_get_selection_cursor(buf, &sl, &sc);
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
            int str[line.end-line.start];
            memcpy(str, buf->content + line.start, (line.end-line.start)*sizeof(int));
            char* utf8_string = LoadUTF8(str, line.end - line.start);
            Vector2 size = {0};
            if (line.end-line.start == 0) size = (Vector2) {.x = 0, .y = font_size};
            else size = MeasureTextEx(font, utf8_string, font_size, 0);
            UnloadUTF8(utf8_string);
            DrawRectangle(pad + line_size + posx, y, size.x, size.y, FAINT_FG);
        }

        if (selection && buf->selection_origin > (int) buf->cursor) {
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
        } else if (selection && buf->selection_origin < (int) buf->cursor) {
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

        if (cl == i && (!selection || buf->cursor == (size_t) buf->selection_origin)) {
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

const char *py_keywords[] = {
    "and",    "as",    "assert", "break",  "class",   "continue", "def",    "del",
    "elif",   "else",  "except", "False",  "finally", "from",     "global", "if",
    "import", "in",    "is",     "lambda", "None",    "nonlocal", "not",    "or",
    "pass",   "raise", "return", "True",   "try",     "while",    "with",   "yield",
};
int py_keywords_length = 32;

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

void color_highlight_basic(Buffer* buf, const char** keywords, int keywords_length, int comment_type) {
    bool comment = false;
    for (size_t i = 0; i < da_length(buf->lines); i++) {
        bool preprocessor_line = false;
        bool done_preprocessor = false;
        Line line = buf->lines[i];
        int line_length = line.end-line.start;

        for (int j = 0; j < line_length;) {
            char ch = buf->content[line.start + j];
            if (isalpha(ch) && !comment) {
                size_t length = 1;
                j++; while ((isalnum(buf->content[line.start + j]) || buf->content[line.start + j] == '_') && j < line_length) { length++; j++; }

                int* b = malloc(sizeof(int)*length);
                memcpy(b, buf->content + line.start + j - length, sizeof(int)*length);
                char* bu = LoadUTF8(b, length);

                bool keyword = needlehaystack_string(bu, keywords, keywords_length);
                free(b);
                UnloadUTF8(bu);

                Token token = {i, j-length, j, preprocessor_line ? (!done_preprocessor ? PREPROCESSOR : (keyword ? KWORD : DEFAULT)) : (keyword ? KWORD : DEFAULT)};
                done_preprocessor = true;
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
                if (comment && comment_type == 0) {
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
                if (ch == '#' && comment_type == 0) {
                    color = PREPROCESSOR;
                    preprocessor_line = true;
                } else if (ch == '/' && j < line_length-1 && buf->content[line.start + j + 1] == '/' && comment_type == 0) {
                    Token token = {i, j, line_length, COMMENT};
                    da_push(buf->tokens, token);
                    j = line_length;
                } else if (ch == '/' && j < line_length-1 && buf->content[line.start + j + 1] == '*' && comment_type == 0) {
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
                } else if (comment_type == 1 && ch == '#') {
                    Token token = {i, j, line_length, COMMENT};
                    da_push(buf->tokens, token);
                    j = line_length;
                }
                else color = DEFAULT;
                Token token = {i, j, j+1, color};
                da_push(buf->tokens, token);
                j++;
            }
        }
    }
}

void color_highlight_c(Buffer* buf) {
    color_highlight_basic(buf, keywords, keywords_length, 0);
}

void color_highlight_py(Buffer* buf) {
    color_highlight_basic(buf, py_keywords, py_keywords_length, 1);
}

void color_highlight_openfile(Buffer* buf) {
    for (size_t i = 0; i < da_length(buf->lines); i++) {
        Color color = {0};

        if (i == 0) {
            color = KWORD;
        } else if (i == 1) {
            color = COMMENT;
        } else if (buf->content[buf->lines[i].end-1] == '/' || i == 2) {
            color = NUMBER;
        } else {
            color = DEFAULT;
        }

        int line_length = buf->lines[i].end - buf->lines[i].start;
        Token token = {i, 0, line_length, color};
        da_push(buf->tokens, token);
    }
}

void color_highlight_commiteditmsg(Buffer* buf) {
    for (size_t i = 0; i < da_length(buf->lines); i++) {
        Line line = buf->lines[i];
        size_t line_length = line.end-line.start;
        int comment = -1;

        for (size_t j = 0; j < line_length; ++j) {
            if (buf->content[line.start+j] == '#') {
                comment = j;
                break;
            }
        }

        Color color = {0};
        if (i == 0) {
            color = KWORD;
        } else if (i == 1) {
            color = ERROR;
        } else {
            color = DEFAULT;
        }
        
        if (comment >= 0) {
            Token token = {i, 0, comment, color};
            da_push(buf->tokens, token);
            Token token2 = {i, comment, line_length, COMMENT};
            da_push(buf->tokens, token2);
        } else {
            Token token = {i, 0, line_length, color};
            da_push(buf->tokens, token);
        }
    }
}

void color_highlight(Buffer* buf) {
    da_free(buf->tokens);
    buf->tokens = da_new(Token);
    if (buf->filename == 0) { color_highlight_simple(buf); return; }
    char* utf8_string = LoadUTF8(buf->filename, buf->filenamel);
    if (endswith(utf8_string, ".c") || endswith(utf8_string, ".h"))
        color_highlight_c(buf);
    else if (endswith(utf8_string, ".py") || endswith(utf8_string, ".py"))
        color_highlight_py(buf);
    else if (strcmp(utf8_string, "Open a file...") == 0 || strcmp(utf8_string, "Save a file...") == 0)
        color_highlight_openfile(buf);
    else if (endswith(utf8_string, "COMMIT_EDITMSG"))
        color_highlight_commiteditmsg(buf);
    else color_highlight_simple(buf);
    UnloadUTF8(utf8_string);
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
    buf->content = da_new(int);
    buf->tokens = da_new(Token);
    buf->selection_origin = -1;
    int ustrl = 0;
    int* ustr = LoadCodepoints("Help Text", &ustrl);
    buf->filename = ustr;
    buf->filenamel = ustrl;

    const char* hstr = "Ctrl-S:       Save a file (select location if didnt save before)\n"
                       "Ctrl-Shift-S: Save a file to a new location\n"
                       "Ctrl-O:       Open a new file\n"
                       "Ctrl-'-':     Decrease font size\n"
                       "Ctrl-'+':     Increase font size\n"
                       "Ctrl-L:       Enable/Disable line counter\n"
                       "Hold Shift:   Create a selection\n"
                       "Ctrl-C:       Copy a selection to system clipboard\n"
                       "Ctrl-X:       Copy a selection to system clipboard and remove\n"
                       "              it from a buffer\n"
                       "Ctrl-P:       Paste system clipboard content into a buffer\n"
                       "ESC:          Escape to Text mode from any other mode\n"
                       "              (save/open/help)";
    ustr = LoadCodepoints(hstr, &ustrl);
    for (int i = 0; i < ustrl; ++i) da_push(buf->content, ustr[i]);

    update_newlines(buf);
    color_highlight(buf);
}

void init_buf(Buffer* buf) {
    buf->selection_origin = -1;
    buf->lines = da_new(Line);
    buf->content = da_new(int);
    buf->tokens = da_new(Token);
    update_newlines(buf);
    color_highlight(buf);
}

void deinit_buf(Buffer* buf) {
    buf->selection_origin = -1;
    da_free(buf->lines);
    da_free(buf->content);
    da_free(buf->tokens);
    buf->changed = false;
    if (buf->filename != 0) free(buf->filename);
}

void print_sb(char* sb) {
    for (size_t i = 0; i < da_length(sb); ++i) printf("%c", sb[i]);
    printf("\n");
}

void init_open_buffer(Buffer* buf) {
    buf->selection_origin = -1;
    buf->lines = da_new(Line);
    buf->content = da_new(int);
    buf->tokens = da_new(Token);
    int ustrl = 0;
    int* ustr = LoadCodepoints("Open a file...", &ustrl);
    buf->filename = ustr;
    buf->filenamel = ustrl;

    const char* path = GetWorkingDirectory();
    FilePathList files = LoadDirectoryFiles(path);
    
    int ucwdl;
    int* ucwd = LoadCodepoints(path, &ucwdl);
    for (int j = 0; j < ucwdl; ++j) {
        da_push(buf->content, ucwd[j]);
    }
    UnloadCodepoints(ucwd);
    da_push(buf->content, '\n');
    da_push(buf->content, '\n');
    
    da_push(buf->content, '.');
    da_push(buf->content, '.');
    da_push(buf->content, '\n');
    
    for (size_t i = 0; i < files.count; ++i) {
        if (DirectoryExists(files.paths[i])) {
            const char* str = basename(files.paths[i]);
            int fl;
            int* fs = LoadCodepoints(str, &fl);
            for (int j = 0; j < fl; ++j) {
                da_push(buf->content, fs[j]);
            }
            da_push(buf->content, '/');
            da_push(buf->content, '\n');
        }
    }

    for (size_t i = 0; i < files.count; ++i) {
        if (!DirectoryExists(files.paths[i])) {
            const char* str = basename(files.paths[i]);
            int fl;
            int* fs = LoadCodepoints(str, &fl);
            for (int j = 0; j < fl; ++j) {
                da_push(buf->content, fs[j]);
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
    int ustrl = 0;
    int* ustr = LoadCodepoints("Save a file...", &ustrl);
    buf->filename = ustr;
    buf->filenamel = ustrl;
    
    update_newlines(buf);
    color_highlight(buf);
}

void push_at_cursor(Buffer* buf, int charachter) {
    da_push(buf->content, 0);
    memcpy(buf->content + buf->cursor + 1, buf->content + buf->cursor, (da_length(buf->content) - 1 - buf->cursor)*sizeof(int));
    buf->content[buf->cursor] = charachter;
    if ((size_t) buf->selection_origin > buf->cursor && buf->selection_origin != -1) buf->selection_origin++;
    buf->cursor++;
}

void push_cstr_at_cursor(Buffer* buf, char* string) {
    size_t length = strlen(string);
    for (size_t i = 0; i < length; ++i) {
        push_at_cursor(buf, string[i]);
    }
}

void save_file(Buffer* buf) {
    buf->changed = false;
    char* utf8_string = LoadUTF8(buf->content, da_length(buf->content));
    char* ufilename = LoadUTF8(buf->filename, buf->filenamel);
    FILE* f = fopen(ufilename, "w");
    size_t utf8l = strlen(utf8_string);
    fwrite(utf8_string, 1, utf8l, f);
    UnloadUTF8(utf8_string);
    fclose(f);
    UnloadUTF8(ufilename);
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

void remove_selection(Buffer* buf) {
    size_t start = buf->cursor;
    size_t end = buf->selection_origin;
    if (start > end) {
        start = buf->selection_origin;
        end = buf->cursor;
    }
    size_t len = da_length(buf->content)-end;
    int temp[len];
    memcpy(temp, buf->content + end, sizeof(int)*len);
    memcpy(buf->content + start, temp, sizeof(int)*len);
    for (size_t i = 0; i < end-start; ++i) da_pop(buf->content, 0);
    buf->selection_origin = -1;
    buf->cursor = start;
}

void update_buf_ro(Buffer* buf) {
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        if (buf->selection_origin == -1) buf->selection_origin = buf->cursor;
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
        if (IsKeyUp(KEY_LEFT_SHIFT)) buf->selection_origin = -1;
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
        if (IsKeyUp(KEY_LEFT_SHIFT)) buf->selection_origin = -1;
    }

    update_newlines(buf);
    color_highlight(buf);
}

void update_buf(Buffer* buf, bool change_lines) {
    int key_char = GetCharPressed();
    while (key_char != 0) {
        if (change_lines) {
            if (key_char == '/') {
                key_char = GetCharPressed();
                continue;
            }
        }
        if (buf->selection_origin != -1) remove_selection(buf);
        push_at_cursor(buf, key_char);
        buf->changed = true;
        if (IsKeyUp(KEY_LEFT_SHIFT)) buf->selection_origin = -1;
        key_char = GetCharPressed();
    }

    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        if (buf->selection_origin == -1) buf->selection_origin = buf->cursor;
    }
    if (key_pressed(KEY_ENTER) && !change_lines) {
        if (buf->selection_origin != -1) remove_selection(buf);
        push_at_cursor(buf, '\n');
        buf->changed = true;
    } else if (key_pressed(KEY_DELETE)) {
        if (buf->selection_origin != -1) remove_selection(buf);
        else if (buf->cursor < da_length(buf->content) && (change_lines ? buf->content[buf->cursor] != '\n' : true)) {
            int temp[da_length(buf->content) - buf->cursor - 1];
            memcpy(temp, buf->content + buf->cursor + 1, (da_length(buf->content) - buf->cursor - 1)*sizeof(int));
            memcpy(buf->content + buf->cursor, temp, (da_length(buf->content) - buf->cursor - 1)*sizeof(int));
            da_pop(buf->content, 0);
            buf->changed = true;
            if (IsKeyUp(KEY_LEFT_SHIFT)) buf->selection_origin = -1;
        }
    } else if (key_pressed(KEY_BACKSPACE)) {
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
    } else if (key_pressed(KEY_TAB)) {
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
        if (IsKeyUp(KEY_LEFT_SHIFT)) buf->selection_origin = -1;
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

    const char* lstatus = TextFormat("%s%s", buf->filename == 0 ? "<new file>" : basename(str), buf->changed ? "*" : "");
    Vector2 lssize = MeasureTextEx(font, lstatus, font_size, 0);
    DrawTextEx(font, lstatus, (Vector2) {pad, wh - lssize.y - pad}, font_size, 0, FOREGROUND);
    
    const char* rstatus = TextFormat("%ld:%ld", l+1, c+1);
    Vector2 rssize = MeasureTextEx(font, rstatus, font_size, 0);
    DrawTextEx(font, rstatus, (Vector2) {ww - rssize.x - pad, wh - lssize.y - pad}, font_size, 0, FOREGROUND);
}

void init_buf_from_file(Buffer* buf, char* fname) {
    int ufnl;
    buf->filename = LoadCodepoints(fname, &ufnl);
    buf->filenamel = ufnl;
    buf->selection_origin = -1;
    FILE* f = fopen(fname, "r");
    buf->lines = da_new(Line);
    buf->content = da_new(int);
    buf->tokens = da_new(Token);
    size_t size = GetFileLength(fname);
    char* file = malloc(size+1);
    file[size] = '\0';
    fread(file, 1, size, f);
    int fs;
    int* uf = LoadCodepoints(file, &fs);
    free(file);
    for (int i = 0; i < fs; ++i) {
        da_push(buf->content, uf[i]);
    }
    if (fs != 0) UnloadCodepoints(uf);
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
                    int utfl;
                    int* utfs = LoadCodepoints(cwd, &utfl);
                    int strl = cwdl + 1 + line.end - line.start;
                    int* str = malloc(strl*sizeof(int));
                    memcpy(str + utfl + 1, save_buffer.content + line.start, sizeof(int)*(line.end-line.start));
                    memcpy(str, utfs, utfl*sizeof(int));
                    str[utfl] = '/';
                    
                    free(buf.filename);
                    buf.filename = str;
                    buf.filenamel = strl;
                    save_file(&buf);
                    state = STATE_TEXT;
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
                update_buf(&save_buffer, true);
            } else update_buf_ro(&save_buffer);
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

