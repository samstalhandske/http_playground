#ifndef HTTP_H
#define HTTP_H

#include <stdint.h>
#include <stdbool.h>
#include "string/buffer/string_buffer.h"

#ifndef HTTP_MAX_HEADERS
#define HTTP_MAX_HEADERS 16
#endif

#ifndef HTTP_MAX_HEADER_KEY_LENGTH
#define HTTP_MAX_HEADER_KEY_LENGTH 64
#endif

#ifndef HTTP_MAX_HEADER_VALUE_LENGTH
#define HTTP_MAX_HEADER_VALUE_LENGTH 256
#endif

typedef enum {
    HTTP_Transfer_Encoding_Chunked,
    HTTP_Transfer_Encoding_Compress,
    HTTP_Transfer_Encoding_Deflate,
    HTTP_Transfer_Encoding_Gzip,
    HTTP_Transfer_Encoding_Identity
} HTTP_Transfer_Encoding;

typedef struct {
    char http_version[16];
    int  status_code;
    char status_text[64];
} HTTP_Status;

typedef struct {
    char key[HTTP_MAX_HEADER_KEY_LENGTH];
    char value[HTTP_MAX_HEADER_VALUE_LENGTH];
} HTTP_Header;

typedef struct {
    HTTP_Header headers[HTTP_MAX_HEADERS];
    uint32_t header_count;
} HTTP_Headers;

typedef struct {
    bool has_encoding_set;
    HTTP_Transfer_Encoding encoding;

    String_Buffer string_buffer;

    // Chunked.
    uint32_t offset;
} HTTP_Body;

typedef enum {
    HTTP_Parse_Status_Parsing_Status,
    HTTP_Parse_Status_Parsing_Headers,
    HTTP_Parse_Status_Parsing_Body,
    HTTP_Parse_Status_Parsing_Done
} HTTP_Parse_Status;

typedef struct {
    HTTP_Status status;
    HTTP_Headers headers;
    HTTP_Body body;
} HTTP;

typedef struct {
    HTTP http;
    HTTP_Parse_Status state;

    const char *buffer;
    uint64_t buffer_length;

    uint64_t bytes_parsed_offset;
} HTTP_Parser;

bool http_parser_init(HTTP_Parser *parser, uint64_t body_buffer_capacity);
bool http_parser_dispose(HTTP_Parser *parser);

bool http_try_parse(HTTP_Parser *parser, const char *buf, const uint64_t buf_len, HTTP *out_http);

bool http_try_parse_status(const char *buf, const uint64_t buf_len, HTTP_Status *out_status, uint64_t *out_consumed_bytes);
bool http_try_parse_headers(const char *buf, const uint64_t buf_len, HTTP_Headers *out_headers, uint64_t *out_consumed_bytes);
bool http_try_parse_body(const HTTP_Headers *headers, const char *buf, const uint64_t buf_len, HTTP_Body *out_body);

bool http_try_get_key_from_header(const HTTP_Headers *headers, const char *key, const char **out_value);

void http_dispose(HTTP *http);

#endif