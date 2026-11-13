#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include "win_extra.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include "json.h"

typedef enum JsonType {
    JSON_NULL = 0,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JsonType;

struct Json {
    JsonType type;
    WCHAR *text;
    double number;
    int boolean;
    Json **items;
    char **keys;
    int count;
    int capacity;
};

typedef struct Parser {
    const char *p;
    const char *end;
    int depth;
    int error;
} Parser;

static void *xalloc(size_t size) {
    void *p = malloc(size);
    if (p) ZeroMemory(p, size);
    return p;
}

static void *xrealloc(void *ptr, size_t size) {
    return realloc(ptr, size);
}

static Json *json_new(JsonType type) {
    Json *v = (Json *)xalloc(sizeof(Json));
    if (v) v->type = type;
    return v;
}

void json_free(Json *v) {
    int i;
    if (!v) return;
    if (v->text) free(v->text);
    if (v->items) {
        for (i = 0; i < v->count; i++) json_free(v->items[i]);
        free(v->items);
    }
    if (v->keys) {
        for (i = 0; i < v->count; i++) free(v->keys[i]);
        free(v->keys);
    }
    free(v);
}

static void skip_ws(Parser *ps) {
    while (ps->p < ps->end) {
        char c = *ps->p;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') ps->p++;
        else break;
    }
}

static int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int read_hex4(Parser *ps, unsigned *out) {
    unsigned v = 0;
    int i;
    if (ps->end - ps->p < 4) return 0;
    for (i = 0; i < 4; i++) {
        int h = hex_value(ps->p[i]);
        if (h < 0) return 0;
        v = (v << 4) | (unsigned)h;
    }
    ps->p += 4;
    *out = v;
    return 1;
}

static void utf8_append(char *buf, int *len, int cap, unsigned cp) {
    if (cp <= 0x7F) {
        if (*len + 1 < cap) buf[(*len)++] = (char)cp;
    } else if (cp <= 0x7FF) {
        if (*len + 2 < cap) {
            buf[(*len)++] = (char)(0xC0 | (cp >> 6));
            buf[(*len)++] = (char)(0x80 | (cp & 0x3F));
        }
    } else if (cp <= 0xFFFF) {
        if (*len + 3 < cap) {
            buf[(*len)++] = (char)(0xE0 | (cp >> 12));
            buf[(*len)++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            buf[(*len)++] = (char)(0x80 | (cp & 0x3F));
        }
    } else if (cp <= 0x10FFFF) {
        if (*len + 4 < cap) {
            buf[(*len)++] = (char)(0xF0 | (cp >> 18));
            buf[(*len)++] = (char)(0x80 | ((cp >> 12) & 0x3F));
            buf[(*len)++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            buf[(*len)++] = (char)(0x80 | (cp & 0x3F));
        }
    }
}

static char *parse_raw_string(Parser *ps) {
    int cap = 64;
    int len = 0;
    char *buf;
    if (ps->p >= ps->end || *ps->p != '"') { ps->error = 1; return 0; }
    ps->p++;
    buf = (char *)xalloc(cap);
    if (!buf) { ps->error = 1; return 0; }
    while (ps->p < ps->end) {
        unsigned char c = (unsigned char)*ps->p++;
        if (c == '"') {
            buf[len] = 0;
            return buf;
        }
        if (c == '\\') {
            char esc;
            if (ps->p >= ps->end) break;
            esc = *ps->p++;
            switch (esc) {
                case '"': c = '"'; break;
                case '\\': c = '\\'; break;
                case '/': c = '/'; break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case 'u': {
                    unsigned cp;
                    if (!read_hex4(ps, &cp)) { ps->error = 1; free(buf); return 0; }
                    if (cp >= 0xD800 && cp <= 0xDBFF && ps->end - ps->p >= 6 && ps->p[0] == '\\' && ps->p[1] == 'u') {
                        unsigned lo;
                        const char *save = ps->p;
                        ps->p += 2;
                        if (read_hex4(ps, &lo) && lo >= 0xDC00 && lo <= 0xDFFF) {
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        } else {
                            ps->p = save;
                        }
                    }
                    if (len + 5 >= cap) {
                        char *n;
                        cap *= 2;
                        n = (char *)xrealloc(buf, cap);
                        if (!n) { ps->error = 1; free(buf); return 0; }
                        buf = n;
                    }
                    utf8_append(buf, &len, cap, cp);
                    continue;
                }
                default:
                    c = (unsigned char)esc;
                    break;
            }
        }
        if (len + 2 >= cap) {
            char *n;
            cap *= 2;
            n = (char *)xrealloc(buf, cap);
            if (!n) { ps->error = 1; free(buf); return 0; }
            buf = n;
        }
        buf[len++] = (char)c;
    }
    ps->error = 1;
    free(buf);
    return 0;
}

static WCHAR *utf8_to_wide(const char *s) {
    int n;
    WCHAR *w;
    if (!s) return 0;
    n = MultiByteToWideChar(CP_UTF8, 0, s, -1, 0, 0);
    if (n <= 0) return 0;
    w = (WCHAR *)xalloc(sizeof(WCHAR) * (size_t)n);
    if (!w) return 0;
    MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n);
    return w;
}

static Json *parse_value(Parser *ps);

static Json *parse_object(Parser *ps) {
    Json *v = json_new(JSON_OBJECT);
    if (!v) { ps->error = 1; return 0; }
    ps->p++; /* { */
    skip_ws(ps);
    if (ps->p < ps->end && *ps->p == '}') { ps->p++; return v; }
    while (ps->p < ps->end) {
        char *key;
        Json *item;
        skip_ws(ps);
        key = parse_raw_string(ps);
        if (!key) { json_free(v); return 0; }
        skip_ws(ps);
        if (ps->p >= ps->end || *ps->p != ':') { free(key); ps->error = 1; json_free(v); return 0; }
        ps->p++;
        item = parse_value(ps);
        if (!item) { free(key); json_free(v); return 0; }
        if (v->count == v->capacity) {
            int nc = v->capacity ? v->capacity * 2 : 8;
            Json **ni = (Json **)xrealloc(v->items, sizeof(Json *) * (size_t)nc);
            char **nk = (char **)xrealloc(v->keys, sizeof(char *) * (size_t)nc);
            if (!ni || !nk) { free(key); json_free(item); json_free(v); ps->error = 1; return 0; }
            v->items = ni;
            v->keys = nk;
            v->capacity = nc;
        }
        v->keys[v->count] = key;
        v->items[v->count] = item;
        v->count++;
        skip_ws(ps);
        if (ps->p < ps->end && *ps->p == ',') { ps->p++; continue; }
        if (ps->p < ps->end && *ps->p == '}') { ps->p++; return v; }
        ps->error = 1;
        json_free(v);
        return 0;
    }
    ps->error = 1;
    json_free(v);
    return 0;
}

static Json *parse_array(Parser *ps) {
    Json *v = json_new(JSON_ARRAY);
    if (!v) { ps->error = 1; return 0; }
    ps->p++; /* [ */
    skip_ws(ps);
    if (ps->p < ps->end && *ps->p == ']') { ps->p++; return v; }
    while (ps->p < ps->end) {
        Json *item = parse_value(ps);
        if (!item) { json_free(v); return 0; }
        if (v->count == v->capacity) {
            int nc = v->capacity ? v->capacity * 2 : 8;
            Json **ni = (Json **)xrealloc(v->items, sizeof(Json *) * (size_t)nc);
            if (!ni) { json_free(item); json_free(v); ps->error = 1; return 0; }
            v->items = ni;
            v->capacity = nc;
        }
        v->items[v->count++] = item;
        skip_ws(ps);
        if (ps->p < ps->end && *ps->p == ',') { ps->p++; continue; }
        if (ps->p < ps->end && *ps->p == ']') { ps->p++; return v; }
        ps->error = 1;
        json_free(v);
        return 0;
    }
    ps->error = 1;
    json_free(v);
    return 0;
}

static Json *parse_value(Parser *ps) {
    Json *v;
    skip_ws(ps);
    if (ps->p >= ps->end || ps->depth > 80) { ps->error = 1; return 0; }
    ps->depth++;
    if (*ps->p == '{') v = parse_object(ps);
    else if (*ps->p == '[') v = parse_array(ps);
    else if (*ps->p == '"') {
        char *s = parse_raw_string(ps);
        if (!s) { ps->depth--; return 0; }
        v = json_new(JSON_STRING);
        if (v) {
            v->text = utf8_to_wide(s);
            if (!v->text) { json_free(v); v = 0; ps->error = 1; }
        }
        free(s);
    } else if (ps->end - ps->p >= 4 && !strncmp(ps->p, "true", 4)) {
        ps->p += 4;
        v = json_new(JSON_BOOL);
        if (v) v->boolean = 1;
    } else if (ps->end - ps->p >= 5 && !strncmp(ps->p, "false", 5)) {
        ps->p += 5;
        v = json_new(JSON_BOOL);
    } else if (ps->end - ps->p >= 4 && !strncmp(ps->p, "null", 4)) {
        ps->p += 4;
        v = json_new(JSON_NULL);
    } else {
        char *ep = 0;
        double d;
        const char *start = ps->p;
        while (ps->p < ps->end) {
            char c = *ps->p;
            if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E') ps->p++;
            else break;
        }
        if (ps->p == start) { ps->error = 1; ps->depth--; return 0; }
        {
            int len = (int)(ps->p - start);
            char tmp[64];
            if (len >= (int)sizeof(tmp)) { ps->error = 1; ps->depth--; return 0; }
            memcpy(tmp, start, (size_t)len);
            tmp[len] = 0;
            d = strtod(tmp, &ep);
        }
        v = json_new(JSON_NUMBER);
        if (v) v->number = d;
    }
    ps->depth--;
    if (!v) ps->error = 1;
    return v;
}

Json *json_parse_utf8(const char *data, int length) {
    Parser ps;
    Json *v;
    if (!data || length < 0) return 0;
    if (length >= 3 && (unsigned char)data[0] == 0xEF && (unsigned char)data[1] == 0xBB && (unsigned char)data[2] == 0xBF) {
        data += 3;
        length -= 3;
    }
    ps.p = data;
    ps.end = data + length;
    ps.depth = 0;
    ps.error = 0;
    v = parse_value(&ps);
    if (!v || ps.error) {
        json_free(v);
        return 0;
    }
    return v;
}

Json *json_parse_file(const WCHAR *path) {
    HANDLE h;
    LARGE_INTEGER size;
    DWORD got = 0;
    char *buf;
    Json *v;
    h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (h == INVALID_HANDLE_VALUE) return 0;
    if (!GetFileSizeEx(h, &size) || size.QuadPart <= 0 || size.QuadPart > 64 * 1024 * 1024) {
        CloseHandle(h);
        return 0;
    }
    buf = (char *)xalloc((size_t)size.QuadPart + 1);
    if (!buf) { CloseHandle(h); return 0; }
    if (!ReadFile(h, buf, (DWORD)size.QuadPart, &got, 0)) {
        free(buf);
        CloseHandle(h);
        return 0;
    }
    CloseHandle(h);
    buf[got] = 0;
    v = json_parse_utf8(buf, (int)got);
    free(buf);
    return v;
}

int json_is_object(const Json *v) { return v && v->type == JSON_OBJECT; }
int json_is_array(const Json *v) { return v && v->type == JSON_ARRAY; }
int json_is_string(const Json *v) { return v && v->type == JSON_STRING; }
int json_is_number(const Json *v) { return v && v->type == JSON_NUMBER; }
int json_is_bool(const Json *v) { return v && v->type == JSON_BOOL; }
int json_count(const Json *v) { return v ? v->count : 0; }

Json *json_at(const Json *arr, int index) {
    if (!arr || index < 0 || index >= arr->count) return 0;
    return arr->items[index];
}

Json *json_get(const Json *obj, const char *key) {
    int i;
    if (!obj || obj->type != JSON_OBJECT || !key) return 0;
    for (i = 0; i < obj->count; i++) {
        if (obj->keys[i] && !strcmp(obj->keys[i], key)) return obj->items[i];
    }
    return 0;
}

const WCHAR *json_string(const Json *v) {
    return (v && v->type == JSON_STRING) ? v->text : 0;
}

int json_int(const Json *v, int fallback) {
    if (v && v->type == JSON_NUMBER) return (int)v->number;
    return fallback;
}

int json_bool(const Json *v, int fallback) {
    if (v && v->type == JSON_BOOL) return v->boolean;
    return fallback;
}

const WCHAR *json_get_string(const Json *obj, const char *key, const WCHAR *fallback) {
    Json *v = json_get(obj, key);
    return json_string(v) ? json_string(v) : fallback;
}

int json_get_int(const Json *obj, const char *key, int fallback) {
    return json_int(json_get(obj, key), fallback);
}

int json_get_bool(const Json *obj, const char *key, int fallback) {
    return json_bool(json_get(obj, key), fallback);
}
