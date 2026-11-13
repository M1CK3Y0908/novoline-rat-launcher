#ifndef NOVOLINE_JSON_H
#define NOVOLINE_JSON_H

#include <windows.h>

typedef struct Json Json;

Json *json_parse_file(const WCHAR *path);
Json *json_parse_utf8(const char *data, int length);
void json_free(Json *v);

int json_is_object(const Json *v);
int json_is_array(const Json *v);
int json_is_string(const Json *v);
int json_is_number(const Json *v);
int json_is_bool(const Json *v);

Json *json_get(const Json *obj, const char *key);
Json *json_at(const Json *arr, int index);
int json_count(const Json *v);

const WCHAR *json_string(const Json *v);
int json_int(const Json *v, int fallback);
int json_bool(const Json *v, int fallback);

const WCHAR *json_get_string(const Json *obj, const char *key, const WCHAR *fallback);
int json_get_int(const Json *obj, const char *key, int fallback);
int json_get_bool(const Json *obj, const char *key, int fallback);

#endif
