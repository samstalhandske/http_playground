#include "http.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "string/buffer/string_buffer.h"

bool http_parser_init(HTTP_Parser *parser, uint64_t body_buffer_capacity) {
    string_buffer_init(&parser->http.body.string_buffer, body_buffer_capacity);
    return true;
}

bool http_parser_dispose(HTTP_Parser *parser) {
    string_buffer_free(&parser->http.body.string_buffer);
    return true;
}

bool http_try_parse(HTTP_Parser *parser, const char *buf, const uint64_t buf_len, HTTP *out_http) {
    assert(parser != NULL);

    parser->buffer = buf;
    parser->buffer_length = buf_len;

    if(parser->state == HTTP_Parse_Status_Parsing_Status) {
        // printf("Trying to read HTTP status ...\n");
        if(http_try_parse_status(&parser->buffer[parser->bytes_parsed_offset], parser->buffer_length - parser->bytes_parsed_offset, &parser->http.status, &parser->bytes_parsed_offset)) {
            parser->state = HTTP_Parse_Status_Parsing_Headers;
        }
    }

    if(parser->state == HTTP_Parse_Status_Parsing_Headers) {
        // printf("Trying to read HTTP headers ...\n");
        if(http_try_parse_headers(&parser->buffer[parser->bytes_parsed_offset], parser->buffer_length - parser->bytes_parsed_offset, &parser->http.headers, &parser->bytes_parsed_offset)) {
            parser->state = HTTP_Parse_Status_Parsing_Body;
        }
    }

    if(parser->state == HTTP_Parse_Status_Parsing_Body) {
        // printf("Trying to read HTTP body ...\n");
        if(http_try_parse_body(&parser->http.headers, &parser->buffer[parser->bytes_parsed_offset], parser->buffer_length - parser->bytes_parsed_offset, &parser->http.body)) {
            parser->state = HTTP_Parse_Status_Parsing_Done;
        }
    }

    if(parser->state == HTTP_Parse_Status_Parsing_Done) {
        *out_http = parser->http;
        return true;
    }

    return false;
}

bool http_try_parse_status(const char *buf, const uint64_t buf_len, HTTP_Status *out_status, uint64_t *out_consumed_bytes) {
    if (buf == NULL || buf_len == 0) {
        return false;
    }

    uint64_t line_length = 0;
    bool found_line_end = false;
    for (uint64_t i = 0; i < buf_len; i++) {
        if (buf[i] == '\n') {
            line_length = i;
            found_line_end = true;
            break;
        }
    }

    if (!found_line_end) {
        return false;
    }

    char line[256] = {0};
    if (line_length >= sizeof(line)) {
        return false;
    }
    memcpy(line, buf, line_length);
    line[line_length] = '\0';

    int version_major = 0;
    int version_minor = 0;
    int code = 0;
    char text[64] = {0};

    int n = sscanf(line, "HTTP/%d.%d %d %[^\r\n]", &version_major, &version_minor, &code, text);
    if (n < 3) {
        return false;
    }

    snprintf(out_status->http_version, sizeof(out_status->http_version), "HTTP/%d.%d", version_major, version_minor);
    out_status->status_code = code;
    snprintf(out_status->status_text, sizeof(out_status->status_text), "%s", text);

    *out_consumed_bytes = line_length + 1;
    return true;
}


bool http_try_parse_headers(const char *buf, const uint64_t buf_len, HTTP_Headers *out_headers, uint64_t *out_consumed_bytes) {
    assert(buf_len > 0);
    uint64_t row = 0;
    uint64_t column = 0;
    uint64_t index_in_buffer = 0;

    uint64_t row_start = 0;

    bool found_r = false;

    memset(&out_headers->headers, 0, HTTP_MAX_HEADERS * sizeof(HTTP_Header));
    out_headers->header_count = 0;

    bool found_end_of_headers = false;

    while (index_in_buffer < buf_len) {
        char c = buf[index_in_buffer];

        if (c == '\r') {
            index_in_buffer++;
            found_r = true;
            continue;
        }

        if (c == '\n') {
            uint64_t row_end = index_in_buffer;
            assert(row_end >= row_start);
            uint64_t string_length = row_end - row_start;
            if(found_r) {
                string_length -= 1;
                found_r = false;
            }

            if (string_length > 0) {
                // TODO: SS - Use sscanf here instead.
                
                assert(out_headers->header_count < HTTP_MAX_HEADERS);
                HTTP_Header *header = &out_headers->headers[out_headers->header_count];

                int32_t seperator_relative_index = -1;
                // Find out where the seperator (colon) is.
                for(uint32_t i = 0; i < string_length; i++) {
                    char v = buf[row_start + i];

                    if(v == ':') {
                        seperator_relative_index = i;
                        break;
                    }
                }
                assert(seperator_relative_index >= 0);

                int32_t value_start = seperator_relative_index + 1;

                while (value_start < (int32_t)string_length && buf[row_start + value_start] == ' ') {
                    value_start++;
                }

                int32_t value_length = (int32_t)string_length - value_start;

                snprintf(&header->key[0], HTTP_MAX_HEADER_KEY_LENGTH, "%.*s", (int)seperator_relative_index, buf + row_start);
                snprintf(&header->value[0], HTTP_MAX_HEADER_VALUE_LENGTH, "%.*s", value_length, buf + row_start + value_start);

                // printf("Found header! Index: %i. Key: '%s'.", out_headers->header_count, header->key);

                out_headers->header_count += 1;
            }
            else if (string_length == 0) {
                // printf("Row %lu is empty. End of headers. Breaking out of the loop.\n\n", row);
                found_end_of_headers = true;
                break;
            }

            row++;
            column = 0;
            row_start = index_in_buffer + 1;

            index_in_buffer++;
            continue;
        }

        column++;
        index_in_buffer++;
    }

    if(found_end_of_headers) {
        *out_consumed_bytes += index_in_buffer + 1;
    }

    return found_end_of_headers;
}

// NOTE: SS - Returns true when a chunk has been fully read. Has an out-parameter for the size of the read chunk and for offsetting.
static inline bool get_chunk_start_and_length(const char *buf, const uint64_t buf_len, uint32_t *out_chunk_start, uint32_t *out_chunk_length) {
    assert(buf != NULL);

    *out_chunk_start = 0;
    *out_chunk_length = 0;

    if(buf_len == 0) {
        return true;
    }

    uint32_t expected_chunk_length = 0;
    uint32_t start_of_chunk_after_size = 0;

    if (buf_len < 2) {
        return false;
    }

    char size_text[32];
    memset(&size_text[0], 0, sizeof(size_text));

    uint32_t limit = 32;
    uint32_t i = 0;
    while (i < buf_len && buf[i] != '\r' && i < limit) {
        size_text[i] = buf[i];
        i++;
    }

    if (i == limit || buf[i] != '\r' || buf[i+1] != '\n') {
        printf("Chunk size format is incorrect.\n");
        assert(false);
        return false;
    }

    size_text[i] = '\0';

    if(strlen(size_text) > 0) {
        int result = sscanf(size_text, "%x", &expected_chunk_length);
        assert(result == 1);

        // printf("EXPECTED CHUNK LENGTH: %u.\n", expected_chunk_length);
    }

    if (expected_chunk_length == 0) {
        return true;
    }

    start_of_chunk_after_size = i + 2; // Skip \r\n

    if ((buf_len - start_of_chunk_after_size) >= expected_chunk_length) {
        // printf("Enough! :)\n");
        *out_chunk_start = start_of_chunk_after_size;
        *out_chunk_length = expected_chunk_length;
        return true;
    }

    // printf("Not enough! :( I have %lu but I expect at least %u. Waiting for more bytes ...\n", buf_len, expected_chunk_length);
    return false;
}

bool http_try_parse_body(const HTTP_Headers *headers, const char *buf, const uint64_t buf_len, HTTP_Body *out_body) {
    assert(out_body != NULL);

    if(!out_body->has_encoding_set) {
        const char *encoding = NULL;
        if(!http_try_get_key_from_header(headers, "Transfer-Encoding", &encoding)) {
            encoding = "identity"; // The encoding is 'identity' if no encoding is specified.
        }

        if(strcmp(encoding, "identity") == 0) {
            out_body->encoding = HTTP_Transfer_Encoding_Identity;
        }
        else if(strcmp(encoding, "chunked") == 0) {
            out_body->encoding = HTTP_Transfer_Encoding_Chunked;
        }
        else if(strcmp(encoding, "compress") == 0) {
            out_body->encoding = HTTP_Transfer_Encoding_Compress;
        }
        else if(strcmp(encoding, "deflate") == 0) {
            out_body->encoding = HTTP_Transfer_Encoding_Deflate;
        }
        else if(strcmp(encoding, "gzip") == 0) {
            out_body->encoding = HTTP_Transfer_Encoding_Gzip;
        }
        else {
            // TODO: SS - Multiple encodings may be listed, for example: 'Transfer-Encoding: gzip, chunked'. Implement that.
            printf("Unimplemented HTTP encoding '%s'.\n", encoding);
            assert(false);
        }

        out_body->has_encoding_set = true;
    }

    switch(out_body->encoding) {
        case HTTP_Transfer_Encoding_Chunked: {
            uint32_t chunk_start = 0;
            uint32_t chunk_length = 0;

            const char *content_encoding_text = NULL;
            http_try_get_key_from_header(headers, "Content-Encoding", &content_encoding_text);

            while(true) {
                bool success = get_chunk_start_and_length(&buf[out_body->offset], buf_len - out_body->offset, &chunk_start, &chunk_length);
                if(success) {
                    if(chunk_length == 0) {
                        // Done!
                        return true; // Return 'true' because we're now done parsing the body. :)
                    }
                    else {
                        // printf("Chunk start: %u, chunk length: %u.\n", chunk_start, chunk_length);

                        string_buffer_append_buf(&out_body->string_buffer, &buf[chunk_start], chunk_length);
                        out_body->offset = chunk_start + chunk_length;
                    }
                }
            }

            break;
        }
        case HTTP_Transfer_Encoding_Identity: {
            // TODO: SS - Check for the 'Content-Length' header. Expect that many bytes in the body.
            // Apparently, 'Content-Length' is not required.. So we also need to check if the socket is closed.
            const char *content_length_text = NULL;
            http_try_get_key_from_header(headers, "Content-Length", &content_length_text);

            if(content_length_text == NULL) {
                // We don't have a 'Content-Length'. Wait for the socket to close.
                printf("TODO: SS - (Sadly) Support lack of 'Content-Length'.\n");
                assert(false);
            }
            else {
                // We have a 'Content-Length'. Wait for 'Content-Length' bytes.
                uint64_t content_length = atoi(content_length_text);
                if(content_length > 0) {
                    if(buf_len < content_length) {
                        return false;
                    }
                    
                    string_buffer_append_buf(&out_body->string_buffer, &buf[0], content_length);
                }
                return true;
            }

            break;
        }
        default: {
            printf("Unhandled Transfer-Encoding %i!\n", out_body->encoding);
            assert(false);
            break;
        }
    }

    return false;
}

bool http_try_get_key_from_header(const HTTP_Headers *headers, const char *key, const char **out_value) {
    for(uint32_t i = 0; i < headers->header_count; i++) {
        const HTTP_Header *header = &headers->headers[i];
        // printf("Comparing '%s' with '%s' ...\n", header->key, key);
        if(strcmp(header->key, key) == 0) {
            *out_value = &header->value[0];
            return true;
        }
    }

    *out_value = NULL;
    return false;
}

void http_dispose(HTTP *http) {
    assert(http != NULL);
    // TODO: SS - Do something here?
}