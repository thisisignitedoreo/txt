
#include <raylib.h>

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
    int* search_buffer;
    int is_searching;
} Buffer;

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
}bool needlehaystack(char needle, char* haystack) {
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
    buf->search_buffer = da_new(int);
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
                       "Ctrl-Q:       Select a line\n"
                       "Ctrl-A:       Select whole file\n"
                       "Ctrl-G:       Goto a line\n"
                       "Ctrl-F:       Find a string\n"
                       "Ctrl-C:       Copy a selection to system clipboard\n"
                       "Ctrl-X:       Copy a selection to system clipboard and remove\n"
                       "              it from a buffer\n"
                       "Ctrl-P:       Paste system clipboard content into a buffer\n"
                       "ESC:          Escape to Text mode from any other mode\n"
                       "              (save/open/help)\n\n"
                       "Open %localappdata%\\txt\\config.txt or ~/.config/txt/config.txt to\n"
                       "change color scheme";
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
    buf->search_buffer = da_new(int);
    update_newlines(buf);
    color_highlight(buf);
}

void deinit_buf(Buffer* buf) {
    buf->selection_origin = -1;
    da_free(buf->lines);
    da_free(buf->content);
    da_free(buf->tokens);
    da_free(buf->search_buffer);
    buf->changed = false;
    if (buf->filename != 0) free(buf->filename);
}

void init_open_buffer(Buffer* buf) {
    buf->selection_origin = -1;
    buf->lines = da_new(Line);
    buf->content = da_new(int);
    buf->tokens = da_new(Token);
    buf->search_buffer = da_new(int);
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
    char* utf8_string = LoadUTF8(buf->content, da_length(buf->content));
    char* ufilename = LoadUTF8(buf->filename, buf->filenamel);
    FILE* f = fopen(ufilename, "w");
    if (f == NULL) {
        error = strerror(errno);
        return;
    }
    size_t utf8l = strlen(utf8_string);
    fwrite(utf8_string, 1, utf8l, f);
    UnloadUTF8(utf8_string);
    fclose(f);
    UnloadUTF8(ufilename);
    buf->changed = false;
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

void init_buf_from_file(Buffer* buf, char* fname) {
    int ufnl;
    buf->filename = LoadCodepoints(fname, &ufnl);
    buf->filenamel = ufnl;
    buf->selection_origin = -1;
    buf->lines = da_new(Line);
    buf->content = da_new(int);
    buf->search_buffer = da_new(int);
    buf->tokens = da_new(Token);
    FILE* f = fopen(fname, "r");
    if (f == NULL) {
        error = strerror(errno);
        return;
    }
    size_t size = GetFileLength(fname);
    char* file = malloc(size+1);
    file[size] = '\0';
    fread(file, 1, size, f);
    int fs;
    int* uf = LoadCodepoints(file, &fs);
    free(file);
    for (int i = 0; i < fs; ++i) {
        if (uf[i] == '\t') {
            da_push(buf->content, ' ');
            da_push(buf->content, ' ');
            da_push(buf->content, ' ');
            da_push(buf->content, ' ');
        } else da_push(buf->content, uf[i]);
    }
    if (fs != 0) UnloadCodepoints(uf);
    fclose(f);
    update_newlines(buf);
    color_highlight(buf);
}
