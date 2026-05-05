#include <cJSON.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct pkg_json_reader {
    const char *text;
    const char *pos;
    const char *end;
    const char *error;
    unsigned int depth;
} pkg_json_reader;

static const char *pkg_cjson_error;

static int pkg_json_is_ws(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static int pkg_json_hex(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return -1;
}

static void pkg_json_skip_ws(pkg_json_reader *reader) {
    while (reader->pos < reader->end && pkg_json_is_ws(*reader->pos) != 0) {
        reader->pos++;
    }
}

static cJSON *pkg_cjson_new(int type) {
    cJSON *item = (cJSON *)calloc(1U, sizeof(cJSON));
    if (item != (cJSON *)0) {
        item->type = type;
        item->valuedouble = 0.0;
    }
    return item;
}

static char *pkg_cjson_strdup_range(const char *start, size_t len) {
    char *out = (char *)malloc(len + 1U);
    if (out == (char *)0) {
        return (char *)0;
    }
    if (len > 0U) {
        memcpy(out, start, len);
    }
    out[len] = '\0';
    return out;
}

static char *pkg_cjson_strdup(const char *text) {
    if (text == (const char *)0) {
        return (char *)0;
    }
    return pkg_cjson_strdup_range(text, strlen(text));
}

static void pkg_cjson_append_child(cJSON *parent, cJSON *child) {
    cJSON *tail;

    if (parent == (cJSON *)0 || child == (cJSON *)0) {
        return;
    }
    if (parent->child == (cJSON *)0) {
        parent->child = child;
        return;
    }
    tail = parent->child;
    while (tail->next != (cJSON *)0) {
        tail = tail->next;
    }
    tail->next = child;
    child->prev = tail;
}

static int pkg_json_emit_utf8(char *out, size_t out_cap, size_t *used, unsigned int codepoint) {
    unsigned char bytes[4];
    size_t count = 0U;
    size_t i;

    if (codepoint <= 0x7FU) {
        bytes[count++] = (unsigned char)codepoint;
    } else if (codepoint <= 0x7FFU) {
        bytes[count++] = (unsigned char)(0xC0U | (codepoint >> 6U));
        bytes[count++] = (unsigned char)(0x80U | (codepoint & 0x3FU));
    } else if (codepoint <= 0xFFFFU) {
        bytes[count++] = (unsigned char)(0xE0U | (codepoint >> 12U));
        bytes[count++] = (unsigned char)(0x80U | ((codepoint >> 6U) & 0x3FU));
        bytes[count++] = (unsigned char)(0x80U | (codepoint & 0x3FU));
    } else if (codepoint <= 0x10FFFFU) {
        bytes[count++] = (unsigned char)(0xF0U | (codepoint >> 18U));
        bytes[count++] = (unsigned char)(0x80U | ((codepoint >> 12U) & 0x3FU));
        bytes[count++] = (unsigned char)(0x80U | ((codepoint >> 6U) & 0x3FU));
        bytes[count++] = (unsigned char)(0x80U | (codepoint & 0x3FU));
    } else {
        return 0;
    }

    if (*used + count >= out_cap) {
        return 0;
    }
    for (i = 0U; i < count; i++) {
        out[*used] = (char)bytes[i];
        *used += 1U;
    }
    return 1;
}

static char *pkg_json_parse_string_raw(pkg_json_reader *reader) {
    const char *scan;
    char *out;
    size_t cap;
    size_t used = 0U;

    if (reader->pos >= reader->end || *reader->pos != '"') {
        reader->error = reader->pos;
        return (char *)0;
    }
    reader->pos++;
    scan = reader->pos;
    cap = (size_t)(reader->end - reader->pos) + 1U;
    out = (char *)malloc(cap);
    if (out == (char *)0) {
        reader->error = reader->pos;
        return (char *)0;
    }

    while (scan < reader->end) {
        char ch = *scan++;
        if (ch == '"') {
            out[used] = '\0';
            reader->pos = scan;
            return out;
        }
        if ((unsigned char)ch < 0x20U) {
            break;
        }
        if (ch == '\\') {
            if (scan >= reader->end) {
                break;
            }
            ch = *scan++;
            if (ch == '"' || ch == '\\' || ch == '/') {
                out[used++] = ch;
            } else if (ch == 'b') {
                out[used++] = '\b';
            } else if (ch == 'f') {
                out[used++] = '\f';
            } else if (ch == 'n') {
                out[used++] = '\n';
            } else if (ch == 'r') {
                out[used++] = '\r';
            } else if (ch == 't') {
                out[used++] = '\t';
            } else if (ch == 'u') {
                int h0;
                int h1;
                int h2;
                int h3;
                unsigned int codepoint;
                if (reader->end - scan < 4) {
                    break;
                }
                h0 = pkg_json_hex(scan[0]);
                h1 = pkg_json_hex(scan[1]);
                h2 = pkg_json_hex(scan[2]);
                h3 = pkg_json_hex(scan[3]);
                if (h0 < 0 || h1 < 0 || h2 < 0 || h3 < 0) {
                    break;
                }
                codepoint = ((unsigned int)h0 << 12U) | ((unsigned int)h1 << 8U) |
                            ((unsigned int)h2 << 4U) | (unsigned int)h3;
                scan += 4;
                if (codepoint >= 0xD800U && codepoint <= 0xDBFFU && reader->end - scan >= 6 &&
                    scan[0] == '\\' && scan[1] == 'u') {
                    int l0 = pkg_json_hex(scan[2]);
                    int l1 = pkg_json_hex(scan[3]);
                    int l2 = pkg_json_hex(scan[4]);
                    int l3 = pkg_json_hex(scan[5]);
                    unsigned int low = ((unsigned int)l0 << 12U) | ((unsigned int)l1 << 8U) |
                                       ((unsigned int)l2 << 4U) | (unsigned int)l3;
                    if (l0 >= 0 && l1 >= 0 && l2 >= 0 && l3 >= 0 && low >= 0xDC00U && low <= 0xDFFFU) {
                        codepoint = 0x10000U + (((codepoint - 0xD800U) << 10U) | (low - 0xDC00U));
                        scan += 6;
                    }
                }
                if (pkg_json_emit_utf8(out, cap, &used, codepoint) == 0) {
                    break;
                }
            } else {
                break;
            }
        } else {
            out[used++] = ch;
        }
    }

    free(out);
    reader->error = scan;
    return (char *)0;
}

static cJSON *pkg_json_parse_value(pkg_json_reader *reader);

static cJSON *pkg_json_parse_number(pkg_json_reader *reader) {
    const char *start = reader->pos;
    int negative = 0;
    int any = 0;
    int value = 0;
    cJSON *item;

    if (reader->pos < reader->end && *reader->pos == '-') {
        negative = 1;
        reader->pos++;
    }
    while (reader->pos < reader->end && *reader->pos >= '0' && *reader->pos <= '9') {
        int digit = *reader->pos - '0';
        if (value <= 214748364) {
            value = (value * 10) + digit;
        }
        any = 1;
        reader->pos++;
    }
    if (reader->pos < reader->end && *reader->pos == '.') {
        reader->pos++;
        while (reader->pos < reader->end && *reader->pos >= '0' && *reader->pos <= '9') {
            any = 1;
            reader->pos++;
        }
    }
    if (reader->pos < reader->end && (*reader->pos == 'e' || *reader->pos == 'E')) {
        reader->pos++;
        if (reader->pos < reader->end && (*reader->pos == '+' || *reader->pos == '-')) {
            reader->pos++;
        }
        while (reader->pos < reader->end && *reader->pos >= '0' && *reader->pos <= '9') {
            reader->pos++;
        }
    }
    if (any == 0) {
        reader->error = start;
        return (cJSON *)0;
    }

    item = pkg_cjson_new(cJSON_Number);
    if (item == (cJSON *)0) {
        reader->error = start;
        return (cJSON *)0;
    }
    item->valueint = negative != 0 ? -value : value;
    return item;
}

static cJSON *pkg_json_parse_array(pkg_json_reader *reader) {
    cJSON *array;

    if (reader->depth >= CJSON_NESTING_LIMIT) {
        reader->error = reader->pos;
        return (cJSON *)0;
    }
    reader->depth++;
    reader->pos++;
    array = pkg_cjson_new(cJSON_Array);
    if (array == (cJSON *)0) {
        reader->depth--;
        return (cJSON *)0;
    }
    pkg_json_skip_ws(reader);
    if (reader->pos < reader->end && *reader->pos == ']') {
        reader->pos++;
        reader->depth--;
        return array;
    }
    while (reader->pos < reader->end) {
        cJSON *child;
        pkg_json_skip_ws(reader);
        child = pkg_json_parse_value(reader);
        if (child == (cJSON *)0) {
            cJSON_Delete(array);
            reader->depth--;
            return (cJSON *)0;
        }
        pkg_cjson_append_child(array, child);
        pkg_json_skip_ws(reader);
        if (reader->pos < reader->end && *reader->pos == ',') {
            reader->pos++;
            continue;
        }
        if (reader->pos < reader->end && *reader->pos == ']') {
            reader->pos++;
            reader->depth--;
            return array;
        }
        break;
    }
    reader->error = reader->pos;
    cJSON_Delete(array);
    reader->depth--;
    return (cJSON *)0;
}

static cJSON *pkg_json_parse_object(pkg_json_reader *reader) {
    cJSON *object;

    if (reader->depth >= CJSON_NESTING_LIMIT) {
        reader->error = reader->pos;
        return (cJSON *)0;
    }
    reader->depth++;
    reader->pos++;
    object = pkg_cjson_new(cJSON_Object);
    if (object == (cJSON *)0) {
        reader->depth--;
        return (cJSON *)0;
    }
    pkg_json_skip_ws(reader);
    if (reader->pos < reader->end && *reader->pos == '}') {
        reader->pos++;
        reader->depth--;
        return object;
    }
    while (reader->pos < reader->end) {
        char *key;
        cJSON *child;

        pkg_json_skip_ws(reader);
        key = pkg_json_parse_string_raw(reader);
        if (key == (char *)0) {
            cJSON_Delete(object);
            reader->depth--;
            return (cJSON *)0;
        }
        pkg_json_skip_ws(reader);
        if (reader->pos >= reader->end || *reader->pos != ':') {
            free(key);
            cJSON_Delete(object);
            reader->depth--;
            reader->error = reader->pos;
            return (cJSON *)0;
        }
        reader->pos++;
        pkg_json_skip_ws(reader);
        child = pkg_json_parse_value(reader);
        if (child == (cJSON *)0) {
            free(key);
            cJSON_Delete(object);
            reader->depth--;
            return (cJSON *)0;
        }
        child->string = key;
        pkg_cjson_append_child(object, child);
        pkg_json_skip_ws(reader);
        if (reader->pos < reader->end && *reader->pos == ',') {
            reader->pos++;
            continue;
        }
        if (reader->pos < reader->end && *reader->pos == '}') {
            reader->pos++;
            reader->depth--;
            return object;
        }
        break;
    }
    reader->error = reader->pos;
    cJSON_Delete(object);
    reader->depth--;
    return (cJSON *)0;
}

static cJSON *pkg_json_parse_value(pkg_json_reader *reader) {
    cJSON *item;

    pkg_json_skip_ws(reader);
    if (reader->pos >= reader->end) {
        reader->error = reader->pos;
        return (cJSON *)0;
    }
    if (*reader->pos == '{') {
        return pkg_json_parse_object(reader);
    }
    if (*reader->pos == '[') {
        return pkg_json_parse_array(reader);
    }
    if (*reader->pos == '"') {
        char *text = pkg_json_parse_string_raw(reader);
        if (text == (char *)0) {
            return (cJSON *)0;
        }
        item = pkg_cjson_new(cJSON_String);
        if (item == (cJSON *)0) {
            free(text);
            return (cJSON *)0;
        }
        item->valuestring = text;
        return item;
    }
    if (reader->end - reader->pos >= 4 && strncmp(reader->pos, "true", 4U) == 0) {
        reader->pos += 4;
        return pkg_cjson_new(cJSON_True);
    }
    if (reader->end - reader->pos >= 5 && strncmp(reader->pos, "false", 5U) == 0) {
        reader->pos += 5;
        return pkg_cjson_new(cJSON_False);
    }
    if (reader->end - reader->pos >= 4 && strncmp(reader->pos, "null", 4U) == 0) {
        reader->pos += 4;
        return pkg_cjson_new(cJSON_NULL);
    }
    if (*reader->pos == '-' || (*reader->pos >= '0' && *reader->pos <= '9')) {
        return pkg_json_parse_number(reader);
    }
    reader->error = reader->pos;
    return (cJSON *)0;
}

CJSON_PUBLIC(const char *) cJSON_Version(void) {
    return "1.7.19-xiaobaios-pkg";
}

CJSON_PUBLIC(void) cJSON_InitHooks(cJSON_Hooks *hooks) {
    (void)hooks;
}

CJSON_PUBLIC(cJSON *) cJSON_Parse(const char *value) {
    return cJSON_ParseWithLengthOpts(value, value != (const char *)0 ? strlen(value) : 0U, (const char **)0, 0);
}

CJSON_PUBLIC(cJSON *) cJSON_ParseWithLength(const char *value, size_t buffer_length) {
    return cJSON_ParseWithLengthOpts(value, buffer_length, (const char **)0, 0);
}

CJSON_PUBLIC(cJSON *) cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated) {
    return cJSON_ParseWithLengthOpts(value, value != (const char *)0 ? strlen(value) : 0U, return_parse_end, require_null_terminated);
}

CJSON_PUBLIC(cJSON *) cJSON_ParseWithLengthOpts(const char *value, size_t buffer_length, const char **return_parse_end, cJSON_bool require_null_terminated) {
    pkg_json_reader reader;
    cJSON *root;

    if (value == (const char *)0) {
        pkg_cjson_error = (const char *)0;
        return (cJSON *)0;
    }
    reader.text = value;
    reader.pos = value;
    reader.end = value + buffer_length;
    reader.error = (const char *)0;
    reader.depth = 0U;
    root = pkg_json_parse_value(&reader);
    if (root == (cJSON *)0) {
        pkg_cjson_error = reader.error != (const char *)0 ? reader.error : reader.pos;
        if (return_parse_end != (const char **)0) {
            *return_parse_end = pkg_cjson_error;
        }
        return (cJSON *)0;
    }
    pkg_json_skip_ws(&reader);
    if ((require_null_terminated != 0 && reader.pos != reader.end) ||
        (require_null_terminated == 0 && reader.pos < reader.end && *reader.pos != '\0')) {
        pkg_cjson_error = reader.pos;
        if (return_parse_end != (const char **)0) {
            *return_parse_end = reader.pos;
        }
        cJSON_Delete(root);
        return (cJSON *)0;
    }
    pkg_cjson_error = (const char *)0;
    if (return_parse_end != (const char **)0) {
        *return_parse_end = reader.pos;
    }
    return root;
}

CJSON_PUBLIC(void) cJSON_Delete(cJSON *item) {
    while (item != (cJSON *)0) {
        cJSON *next = item->next;
        if ((item->type & cJSON_IsReference) == 0) {
            cJSON_Delete(item->child);
            free(item->valuestring);
        }
        if ((item->type & cJSON_StringIsConst) == 0) {
            free(item->string);
        }
        free(item);
        item = next;
    }
}

CJSON_PUBLIC(int) cJSON_GetArraySize(const cJSON *array) {
    const cJSON *item;
    int count = 0;

    if (array == (const cJSON *)0) {
        return 0;
    }
    for (item = array->child; item != (const cJSON *)0; item = item->next) {
        count++;
    }
    return count;
}

CJSON_PUBLIC(cJSON *) cJSON_GetArrayItem(const cJSON *array, int index) {
    cJSON *item;
    int i = 0;

    if (array == (const cJSON *)0 || index < 0) {
        return (cJSON *)0;
    }
    for (item = array->child; item != (cJSON *)0; item = item->next) {
        if (i == index) {
            return item;
        }
        i++;
    }
    return (cJSON *)0;
}

CJSON_PUBLIC(cJSON *) cJSON_GetObjectItem(const cJSON * const object, const char * const string) {
    return cJSON_GetObjectItemCaseSensitive(object, string);
}

CJSON_PUBLIC(cJSON *) cJSON_GetObjectItemCaseSensitive(const cJSON * const object, const char * const string) {
    cJSON *item;

    if (object == (const cJSON *)0 || string == (const char *)0 || (object->type & 0xFF) != cJSON_Object) {
        return (cJSON *)0;
    }
    for (item = object->child; item != (cJSON *)0; item = item->next) {
        if (item->string != (char *)0 && strcmp(item->string, string) == 0) {
            return item;
        }
    }
    return (cJSON *)0;
}

CJSON_PUBLIC(cJSON_bool) cJSON_HasObjectItem(const cJSON *object, const char *string) {
    return cJSON_GetObjectItemCaseSensitive(object, string) != (cJSON *)0;
}

CJSON_PUBLIC(const char *) cJSON_GetErrorPtr(void) {
    return pkg_cjson_error;
}

CJSON_PUBLIC(char *) cJSON_GetStringValue(const cJSON * const item) {
    return cJSON_IsString(item) != 0 ? item->valuestring : (char *)0;
}

CJSON_PUBLIC(double) cJSON_GetNumberValue(const cJSON * const item) {
    return cJSON_IsNumber(item) != 0 ? item->valuedouble : 0.0;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsInvalid(const cJSON * const item) {
    return item == (const cJSON *)0 || (item->type & 0xFF) == cJSON_Invalid;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsFalse(const cJSON * const item) {
    return item != (const cJSON *)0 && (item->type & 0xFF) == cJSON_False;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsTrue(const cJSON * const item) {
    return item != (const cJSON *)0 && (item->type & 0xFF) == cJSON_True;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsBool(const cJSON * const item) {
    return cJSON_IsFalse(item) != 0 || cJSON_IsTrue(item) != 0;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsNull(const cJSON * const item) {
    return item != (const cJSON *)0 && (item->type & 0xFF) == cJSON_NULL;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsNumber(const cJSON * const item) {
    return item != (const cJSON *)0 && (item->type & 0xFF) == cJSON_Number;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsString(const cJSON * const item) {
    return item != (const cJSON *)0 && (item->type & 0xFF) == cJSON_String;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsArray(const cJSON * const item) {
    return item != (const cJSON *)0 && (item->type & 0xFF) == cJSON_Array;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsObject(const cJSON * const item) {
    return item != (const cJSON *)0 && (item->type & 0xFF) == cJSON_Object;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsRaw(const cJSON * const item) {
    return item != (const cJSON *)0 && (item->type & 0xFF) == cJSON_Raw;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateNull(void) {
    return pkg_cjson_new(cJSON_NULL);
}

CJSON_PUBLIC(cJSON *) cJSON_CreateTrue(void) {
    return pkg_cjson_new(cJSON_True);
}

CJSON_PUBLIC(cJSON *) cJSON_CreateFalse(void) {
    return pkg_cjson_new(cJSON_False);
}

CJSON_PUBLIC(cJSON *) cJSON_CreateBool(cJSON_bool boolean) {
    return pkg_cjson_new(boolean != 0 ? cJSON_True : cJSON_False);
}

CJSON_PUBLIC(cJSON *) cJSON_CreateNumber(double num) {
    cJSON *item = pkg_cjson_new(cJSON_Number);
    (void)num;
    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateString(const char *string) {
    cJSON *item = pkg_cjson_new(cJSON_String);
    if (item == (cJSON *)0) {
        return (cJSON *)0;
    }
    item->valuestring = pkg_cjson_strdup(string);
    if (item->valuestring == (char *)0) {
        cJSON_Delete(item);
        return (cJSON *)0;
    }
    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateRaw(const char *raw) {
    cJSON *item = cJSON_CreateString(raw);
    if (item != (cJSON *)0) {
        item->type = cJSON_Raw;
    }
    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateArray(void) {
    return pkg_cjson_new(cJSON_Array);
}

CJSON_PUBLIC(cJSON *) cJSON_CreateObject(void) {
    return pkg_cjson_new(cJSON_Object);
}

CJSON_PUBLIC(char *) cJSON_Print(const cJSON *item) {
    (void)item;
    return (char *)0;
}

CJSON_PUBLIC(char *) cJSON_PrintUnformatted(const cJSON *item) {
    (void)item;
    return (char *)0;
}

CJSON_PUBLIC(char *) cJSON_PrintBuffered(const cJSON *item, int prebuffer, cJSON_bool fmt) {
    (void)item;
    (void)prebuffer;
    (void)fmt;
    return (char *)0;
}

CJSON_PUBLIC(cJSON_bool) cJSON_PrintPreallocated(cJSON *item, char *buffer, const int length, const cJSON_bool format) {
    (void)item;
    (void)buffer;
    (void)length;
    (void)format;
    return 0;
}

CJSON_PUBLIC(double) cJSON_SetNumberHelper(cJSON *object, double number) {
    (void)number;
    if (object != (cJSON *)0) {
        object->type = cJSON_Number;
        object->valueint = 0;
        object->valuedouble = 0.0;
    }
    return 0.0;
}

CJSON_PUBLIC(char *) cJSON_SetValuestring(cJSON *object, const char *valuestring) {
    char *copy;

    if (object == (cJSON *)0 || cJSON_IsString(object) == 0 || valuestring == (const char *)0) {
        return (char *)0;
    }
    copy = pkg_cjson_strdup(valuestring);
    if (copy == (char *)0) {
        return (char *)0;
    }
    free(object->valuestring);
    object->valuestring = copy;
    return object->valuestring;
}

CJSON_PUBLIC(void *) cJSON_malloc(size_t size) {
    return malloc(size);
}

CJSON_PUBLIC(void) cJSON_free(void *object) {
    free(object);
}
