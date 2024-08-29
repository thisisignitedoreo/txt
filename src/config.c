
#include <raylib.h>
#include <stdio.h>

// #define BACKGROUND   (Color){18,  18,  18,  255}
// #define FOREGROUND   (Color){255, 255, 255, 255}
// #define MIDDLEGROUND (Color){150, 150, 150, 255}
// #define FAINT_FG     (Color){50,  50,  50,  255}

// #define DEFAULT      (Color){255, 255, 255, 255}
// #define COMMENT      (Color){190, 255, 181, 255}
// #define NUMBER       (Color){181, 219, 255, 255}
// #define KWORD        (Color){252, 237, 162, 255}
// #define STRING       (Color){252, 167, 162, 255}
// #define ERROR        (Color){255, 0,   0,   255}
// #define PREPROCESSOR (Color){218, 181, 255, 255}

Color BACKGROUND = {0};
Color FOREGROUND = {0};
Color MIDDLEGROUND = {0};
Color FAINT_FG = {0};
Color DEFAULT = {0};
Color COMMENT = {0};
Color NUMBER = {0};
Color KWORD = {0};
Color STRING = {0};
Color ERROR = {0};
Color PREPROCESSOR = {0};

Color BACKGROUND_A(int alpha) {return (Color) {BACKGROUND.r, BACKGROUND.g, BACKGROUND.b, alpha};};
Color FOREGROUND_A(int alpha) {return (Color) {FOREGROUND.r, FOREGROUND.g, FOREGROUND.b, alpha};};
Color MIDDLEGROUND_A(int alpha) {return (Color) {MIDDLEGROUND.r, MIDDLEGROUND.g, MIDDLEGROUND.b, alpha};};
Color FAINT_FG_A(int alpha) {return (Color) {FAINT_FG.r, FAINT_FG.g, FAINT_FG.b, alpha};};
Color DEFAULT_A(int alpha) {return (Color) {DEFAULT.r, DEFAULT.g, DEFAULT.b, alpha};};
Color COMMENT_A(int alpha) {return (Color) {COMMENT.r, COMMENT.g, COMMENT.b, alpha};};
Color NUMBER_A(int alpha) {return (Color) {NUMBER.r, NUMBER.g, NUMBER.b, alpha};};
Color KWORD_A(int alpha) {return (Color) {KWORD.r, KWORD.g, KWORD.b, alpha};};
Color STRING_A(int alpha) {return (Color) {STRING.r, STRING.g, STRING.b, alpha};};
Color ERROR_A(int alpha) {return (Color) {ERROR.r, ERROR.g, ERROR.b, alpha};};
Color PREPROCESSOR_A(int alpha) {return (Color) {PREPROCESSOR.r, PREPROCESSOR.g, PREPROCESSOR.b, alpha};};

char config_path[2048];

#ifdef _WIN32

#include <fileapi.h>

void load_cfg_path() {
    char* str = getenv("localappdata");
    memcpy(config_path, str, strlen(str));
    const char* suffix = "\\txt\\config.txt";
    memcpy(config_path+strlen(str), suffix, 15);
    config_path[strlen(str)+strlen(suffix)] = '\0';
}

void r_mkdir(const char *dir) {
    char tmp[strlen(dir)+1];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '\\')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '\\') {
            *p = 0;
            CreateDirectoryA(tmp, NULL);
            *p = '\\';
        }
    CreateDirectoryA(tmp, NULL);
}

#else // posix stuff

#include <sys/stat.h>

void load_cfg_path() {
    char* str = getenv("home");
    memcpy(config_path, str, strlen(str));
    const char* suffix = "/.config/txt/config.txt";
    memcpy(config_path+strlen(str), suffix, 15);
    config_path[strlen(str)+strlen(suffix)] = '\0';
}

void r_mkdir(const char *dir) {
    char tmp[strlen(dir)+1];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    mkdir(tmp, S_IRWXU);
}

#endif

size_t getsize(FILE* f) {
    size_t old = ftell(f);
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, old, SEEK_SET);
    return size;
}

void load_config() {
    BACKGROUND   = (Color){18,  18,  18,  255};
    FOREGROUND   = (Color){255, 255, 255, 255};
    MIDDLEGROUND = (Color){150, 150, 150, 255};
    FAINT_FG     = (Color){50,  50,  50,  255};
    DEFAULT      = (Color){255, 255, 255, 255};
    COMMENT      = (Color){190, 255, 181, 255};
    NUMBER       = (Color){181, 219, 255, 255};
    KWORD        = (Color){252, 237, 162, 255};
    STRING       = (Color){252, 167, 162, 255};
    ERROR        = (Color){255, 0,   0,   255};
    PREPROCESSOR = (Color){218, 181, 255, 255};
    load_cfg_path();
    if (!FileExists(config_path)) {
        char dname[strlen(config_path)+1];
        memcpy(dname, config_path, strlen(config_path));
        dname[strlen(config_path)] = '\0';
        r_mkdir(dirname(dname));
        FILE* fw = fopen(config_path, "w");
        const char* defconf = "BACKGROUND   18  18  18\n"
                              "FOREGROUND   255 255 255\n"
                              "MIDDLEGROUND 150 150 150\n"
                              "FAINT_FG     50  50  50\n"
                              "DEFAULT      255 255 255\n"
                              "COMMENT      190 255 181\n"
                              "NUMBER       181 219 255\n"
                              "KWORD        252 237 162\n"
                              "STRING       252 167 162\n"
                              "ERROR        255 0   0\n"
                              "PREPROCESSOR 218 181 255";
        fwrite(defconf, 1, strlen(defconf), fw);
        fclose(fw);
    }
    FILE* f = fopen(config_path, "rb");
    if (f == NULL) {
        error = strerror(errno);
        return;
    }
    int fsize = getsize(f);
    char conf[fsize+1];
    printf("%d\n", fsize);
    conf[fsize] = '\0';
    fread(conf, 1, fsize, f);
    fclose(f);
    printf("%s: %s", config_path, conf);
    int start = 0;
    for (int end = 0; end < fsize; end++) {
        if (conf[end] == '\n') {
            if (end+1 < fsize)
                if (conf[end+1] == '\n') { end++; continue; }
            int i = start;

            char name[16], r[16], g[16], b[16];
            int state = 0;
            while (state < 4) {
                while (isspace(conf[i])) {
                    i++;
                }
                int length = 0;
                while (!isspace(conf[i+length])) {
                    length++;
                }

                char* buf;
                if (state == 0) buf = name;
                if (state == 1) buf = r;
                if (state == 2) buf = g;
                if (state == 3) buf = b;

                memcpy(buf, conf+i, length);
                buf[length] = '\0';
                i += length;

                state++;
            }

            // conv
            int rn = atoi(r);
            int gn = atoi(g);
            int bn = atoi(b);

            printf("%s\n", name);

            if (strncmp(name, "BACKGROUND", 16) == 0) BACKGROUND = (Color) {rn, gn, bn, 255};
            if (strncmp(name, "FOREGROUND", 16) == 0) FOREGROUND = (Color) {rn, gn, bn, 255};
            if (strncmp(name, "MIDDLEGROUND", 16) == 0) MIDDLEGROUND = (Color) {rn, gn, bn, 255};
            if (strncmp(name, "FAINT_FG", 16) == 0) FAINT_FG = (Color) {rn, gn, bn, 255};
            if (strncmp(name, "DEFAULT", 16) == 0) DEFAULT = (Color) {rn, gn, bn, 255};
            if (strncmp(name, "COMMENT", 16) == 0) COMMENT = (Color) {rn, gn, bn, 255};
            if (strncmp(name, "NUMBER", 16) == 0) NUMBER = (Color) {rn, gn, bn, 255};
            if (strncmp(name, "KWORD", 16) == 0) KWORD = (Color) {rn, gn, bn, 255};
            if (strncmp(name, "STRING", 16) == 0) STRING = (Color) {rn, gn, bn, 255};
            if (strncmp(name, "ERROR", 16) == 0) ERROR = (Color) {rn, gn, bn, 255};
            if (strncmp(name, "PREPROCESSOR", 16) == 0) PREPROCESSOR = (Color) {rn, gn, bn, 255};

            start = end + 1;
        }
    }
}
