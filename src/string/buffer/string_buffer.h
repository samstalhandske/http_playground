#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} String_Buffer;

static inline void string_buffer_init(String_Buffer *buf, size_t initial_capacity) {
    buf->data = malloc(initial_capacity);
    assert(buf->data != NULL);
    buf->length = 0;
    buf->capacity = initial_capacity;
    memset(buf->data, 0, initial_capacity);
}

static inline void string_buffer_free(String_Buffer *buf) {
    assert(buf != NULL);
    free(buf->data);
}

static inline void string_buffer_resize(String_Buffer *buf, size_t required_capacity) {
    if (required_capacity > buf->capacity) {
        size_t new_capacity = buf->capacity;
        while (new_capacity < required_capacity) {
            new_capacity *= 2;
        }
        char *new_data = realloc(buf->data, new_capacity);
        assert(new_data != NULL);
        buf->data = new_data;
        memset(&buf->data[buf->capacity], 0, new_capacity - buf->capacity);
        buf->capacity = new_capacity;
    }
}

static inline void string_buffer_append_buf(String_Buffer *buf, const char *char_buffer, uint32_t length) {
    assert(buf != NULL);
    assert(char_buffer != NULL);
    assert(length > 0);
    string_buffer_resize(buf, buf->length + length);

    memcpy(&buf->data[buf->length], char_buffer, length);
    buf->length += length;
}

#endif
