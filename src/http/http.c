#include "http.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "string/buffer/string_buffer.h"

const char *http_status_codes[] = { // https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
    // 1xx informational response – the request was received, continuing process
    [100] = "Continue",
    [101] = "Switching Protocols",
    [102] = "Processing",
    [103] = "Early Hints",

    // 2xx successful – the request was successfully received, understood, and accepted
    [200] = "OK",
    [201] = "Created",
    [202] = "Accepted",
    [203] = "Non-Authoritative Information",
    [204] = "No Content",
    [205] = "Reset Content",
    [206] = "Partial Content",
    [207] = "Multi-Status",
    [208] = "Already Reported",
    [226] = "IM Used",

    // 3xx redirection – further action needs to be taken in order to complete the request
    [300] = "Multiple Choices",
    [301] = "Moved Permanently",
    [302] = "Found",
    [303] = "See Other",
    [304] = "Not Modified",
    [305] = "Use Proxy",
    [306] = "Switch Proxy",
    [307] = "Temporary Redirect",
    [308] = "Permanent Redirect",

    // 4xx client error – the request contains bad syntax or cannot be fulfilled
    [400] = "Bad Request",
    [401] = "Unauthorized",
    [402] = "Payment Required",
    [403] = "Forbidden",
    [404] = "Not Found",
    [405] = "Method Not Allowed",
    [406] = "Not Acceptable",
    [407] = "Proxy Authentication Required",
    [408] = "Request Timeout",
    [409] = "Conflict",
    [410] = "Gone",
    [411] = "Length Required",
    [412] = "Precondition Failed",
    [413] = "Payload Too Large",
    [414] = "URI Too Long",
    [415] = "Unsupported Media Type",
    [416] = "Range Not Satisfiable",
    [417] = "Expectation Failed",
    [418] = "I'm a teapot",
    [421] = "Misdirected Request",
    [422] = "Unprocessable Content",
    [423] = "Locked",
    [424] = "Failed Dependency",
    [425] = "Too Early",
    [426] = "Upgrade Required",
    [428] = "Precondition Required",
    [429] = "Too Many Requests",
    [431] = "Request Header Fields Too Large",
    [451] = "Unavailable For Legal Reasons",

    // 5xx server error – the server failed to fulfil an apparently valid request
    [500] = "Internal Server Error",
    [501] = "Not Implemented",
    [502] = "Bad Gateway",
    [503] = "Service Unavailable",
    [504] = "Gateway Timeout",
    [505] = "HTTP Version Not Supported",
    [506] = "Variant Also Negotiates",
    [507] = "Insufficient Storage",
    [508] = "Loop Detected",
    [510] = "Not Extended",
    [511] = "Network Authentication Required"
};

bool http_parser_init(HTTP_Parser *parser, uint64_t body_buffer_capacity) {
    string_buffer_init(&parser->http.body.string_buffer, body_buffer_capacity);
    return true;
}

bool http_parser_dispose(HTTP_Parser *parser) {
    string_buffer_free(&parser->http.body.string_buffer);
    return true;
}

bool http_string_to_method_type(const char *text, HTTP_Method *out_method) {
    assert(text != NULL);

    if(strcmp(text, "GET") == 0) {
        *out_method = HTTP_Method_GET;
        return true;
    }
    else if(strcmp(text, "POST") == 0) {
        *out_method = HTTP_Method_POST;
        return true;
    }
    else if(strcmp(text, "PUT") == 0) {
        *out_method = HTTP_Method_PUT;
        return true;
    }

    return false;
}

HTTP_Parse_Result http_try_parse(HTTP_Parser *parser, const char *buf, const uint64_t buf_len, HTTP *out_http) {
    assert(parser != NULL);

    parser->buffer = buf;
    parser->buffer_length = buf_len;

    if(parser->state == HTTP_Parse_Status_Parsing_Status) {
        // printf("Trying to read HTTP status ...\n");
        HTTP_Parse_Result result = http_try_parse_status(
            &parser->buffer[parser->bytes_parsed_offset],
            parser->buffer_length - parser->bytes_parsed_offset,
            &parser->http.status,
            &parser->bytes_parsed_offset
        );

        if(result != HTTP_Parse_Result_Done) {
            // printf("**A** result: %i.\n", result);
            return result;
        }

        parser->state = HTTP_Parse_Status_Parsing_Headers;
    }

    if(parser->state == HTTP_Parse_Status_Parsing_Headers) {
        // printf("Trying to read HTTP headers ...\n");
        HTTP_Parse_Result result = http_try_parse_headers(
            &parser->buffer[parser->bytes_parsed_offset],
            parser->buffer_length - parser->bytes_parsed_offset,
            &parser->http.headers,
            &parser->bytes_parsed_offset
        );

        if(result != HTTP_Parse_Result_Done) {
            // printf("**B** result: %i.\n", result);
            return result;
        }

        parser->state = HTTP_Parse_Status_Parsing_Body;
    }

    if(parser->state == HTTP_Parse_Status_Parsing_Body) {
        // printf("Trying to read HTTP body ...\n");
        HTTP_Parse_Result result = http_try_parse_body(
            &parser->http.status,
            &parser->http.headers,
            &parser->buffer[parser->bytes_parsed_offset],
            parser->buffer_length - parser->bytes_parsed_offset,
            &parser->http.body
        );

        if(result != HTTP_Parse_Result_Done) {
            // printf("**C** result: %i\n", result);
            return result;
        }

        parser->state = HTTP_Parse_Status_Parsing_Done;
    }

    if(parser->state == HTTP_Parse_Status_Parsing_Done) {
        *out_http = parser->http;
        return HTTP_Parse_Result_Done;
    }

    return HTTP_Parse_Result_Needs_More_Data; // NOTE: SS - Default is to assume that we're missing data. Not sure if correct or not, yet.
}

HTTP_Parse_Result http_try_parse_status(const char *buf, const uint64_t buf_len, HTTP_Status *out_status, uint64_t *out_consumed_bytes) {
    if (buf == NULL || buf_len == 0) {
        return false;
    }

    bool got_status_type = false;
    { // Check if the status-line is a request or a response.
        { // Check if it's a HTTP-request ...
            char method_str[64];
            memset(&method_str[0], 0, sizeof(method_str));

            char path_str[2048];
            memset(&path_str[0], 0, sizeof(path_str));

            int http_version_major = 0;
            int http_version_minor = 0;

            int n = sscanf(buf,
                "%s" // Method.
                " "
                "%s" // Path.
                " "
                "HTTP/%d.%d" // Version.
                ,
                &method_str[0],
                &path_str[0],
                &http_version_major,
                &http_version_minor
            );

            if(n == 4) {
                got_status_type = true;

                out_status->type = HTTP_Status_Type_Request;
                if(!http_string_to_method_type(method_str, &out_status->method)) {
                    return HTTP_Parse_Result_Invalid_Data;
                }

                out_status->http_version_major = (uint8_t)http_version_major;
                out_status->http_version_minor = (uint8_t)http_version_minor;
                
                out_status->status_code = 0;
                
                printf("Request! Method: '%s', Path: '%s'. HTTP-version is %d.%d.\n", method_str, path_str, http_version_major, http_version_minor);
            }
        }

        if(!got_status_type) {
            // Okay, so far we've not successfully parsed a HTTP-request. Let's check if it's a HTTP-response ...

            int http_version_major = 0;
            int http_version_minor = 0;

            int status_code = 0;

            char status_str[256];
            memset(&status_str[0], 0, sizeof(status_str));

            int n = sscanf(buf,
                "HTTP/%d.%d" // Version.
                " "
                "%d" // Status-code.
                " "
                "%s" // Status-description.
                ,
                &http_version_major, &http_version_minor,
                &status_code,
                &status_str[0]
            );

            if(n == 4) {
                got_status_type = true;

                out_status->type = HTTP_Status_Type_Response;
                out_status->status_code = status_code;
                out_status->http_version_major = (uint8_t)http_version_major;
                out_status->http_version_minor = (uint8_t)http_version_minor;
                
                printf("Response! HTTP-version is %d.%d, status code: %d (%s).\n", http_version_major, http_version_minor, status_code, status_str);
            }
        }
    }

    if(!got_status_type) {
        return HTTP_Parse_Result_Invalid_Data;
    }

    // Now that we've successfully parsed the status-line, count the amount of bytes to consume (offset our buffer).
    uint32_t i;
    uint32_t consumed = 0;
    for(i = 1; i < buf_len; i++) {
        const char *prev = &buf[i - 1];
        const char *current = &buf[i];
        assert(prev != NULL);
        assert(current != NULL);
        
        consumed = i;
        if(*prev == '\r' && *current == '\n') {
            break;
        }
    }

    consumed += 1;

    assert(buf[consumed] != '\n');
    *out_consumed_bytes = consumed;
    
    return HTTP_Parse_Result_Done;
}

HTTP_Parse_Result http_try_parse_header(const char *buf, HTTP_Header *out_header) {
    assert(buf != NULL);
    
    // TODO: SS - Verify that we have a newline/eof, otherwise we can't be sure that all the data is here.
    
    char fmt[64];
    snprintf(fmt, sizeof(fmt),
        "%%%d[^:]:" // Key
        " %%%d[^\r\n]", // Value
        HTTP_MAX_HEADER_KEY_LENGTH - 1,
        HTTP_MAX_HEADER_VALUE_LENGTH - 1
    );

    int n = sscanf(buf, fmt, out_header->key, out_header->value);
    if(n != 2) {
        return HTTP_Parse_Result_Invalid_Data;
    }

    return HTTP_Parse_Result_Done;
}

HTTP_Parse_Result http_try_parse_headers(const char *buf, const uint64_t buf_len, HTTP_Headers *out_headers, uint64_t *out_consumed_bytes) {
    const char *line_start = buf;

    (void)buf_len;
    (void)out_consumed_bytes;

    uint64_t headers_end_index = 0;
    for(uint64_t i = 0; ; i++) {
        if(buf[i] == '\n' || buf[i] == '\0') {
            uint64_t line_len = &buf[i] - line_start;

            char line[HTTP_MAX_HEADER_KEY_LENGTH + HTTP_MAX_HEADER_VALUE_LENGTH + 32];
            if(line_len >= sizeof(line)) {
                line_len = sizeof(line) - 1;
            }

            memcpy(line, line_start, line_len);
            line[line_len] = '\0';

            if(line_len > 0 && line[line_len - 1] == '\r') {
                line[line_len - 1] = '\0';
            }

            if(strlen(line) == 0) {
                headers_end_index = i;
                break;
            }
        
            // printf("%s\n", line);

            if(out_headers->header_count >= HTTP_MAX_HEADERS) {
                break;
            }

            HTTP_Header *header = &out_headers->headers[out_headers->header_count];
            HTTP_Parse_Result parse_header_result = http_try_parse_header(&line[0], header);
            switch(parse_header_result) {
                case HTTP_Parse_Result_Done: {
                    printf("Found header! Index: %i. Key: '%s', value: '%s'.\n", out_headers->header_count, header->key, header->value);
                    out_headers->header_count += 1;
                    break;
                }
                case HTTP_Parse_Result_Needs_More_Data:
                case HTTP_Parse_Result_TODO:
                case HTTP_Parse_Result_Invalid_Data: {
                    return parse_header_result;
                }
            }

            if (buf[i] == '\0') break;
            line_start = buf + i + 1;
        }
    }

    headers_end_index += 1;
    
    // printf("Done with headers.\n");
    *out_consumed_bytes += headers_end_index;

    return HTTP_Parse_Result_Done;
}

// NOTE: SS - Returns true when a chunk has been fully read. Has an out-parameter for the size of the read chunk and for offsetting.
static inline bool get_chunk_start_and_length(const char *buf, const uint64_t buf_len, uint32_t *out_chunk_start, uint32_t *out_chunk_length) {
    assert(buf != NULL);

    *out_chunk_start = 0;
    *out_chunk_length = 0;

    if(buf_len == 0) {
        return false;
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

    assert(strlen(size_text) > 0);


    // printf("Size text: '%s'\n", size_text);
    int result = sscanf(size_text, "%x", &expected_chunk_length);
    assert(result == 1);

    // printf("EXPECTED CHUNK LENGTH: %u.\n", expected_chunk_length);

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

HTTP_Parse_Result http_try_parse_body(const HTTP_Status *status, const HTTP_Headers *headers, const char *buf, const uint64_t buf_len, HTTP_Body *out_body) {
    assert(out_body != NULL);

    if(status->type == HTTP_Status_Type_Request) {
        bool should_parse_body = false;
        switch(status->method) {
            case HTTP_Method_GET: {
                should_parse_body = false;
                break;
            }
            case HTTP_Method_POST: {
                should_parse_body = true;
                break;
            }
            default: {
                printf("Unimplemented HTTP-method %i when trying to parse the body. Assuming that no body should be parsed.", status->method);
                should_parse_body = false;
                break;
            }
        }
        
        if(!should_parse_body) {
            return HTTP_Parse_Result_Done;
        }
    }

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
            return HTTP_Parse_Result_TODO;
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
                        // printf("Chunk length: %u\n", chunk_length);
                        return HTTP_Parse_Result_Done; // Return 'done' because we're now done parsing the body. :)
                    }

                    string_buffer_append_buf(&out_body->string_buffer, &buf[out_body->offset + chunk_start], chunk_length);
                    out_body->offset += chunk_start + chunk_length + 2;
                }
                else {
                    // Wait for more data.
                    return HTTP_Parse_Result_Needs_More_Data;
                }
            }

            break;
        }
        case HTTP_Transfer_Encoding_Identity: {
            const char *content_length_text = NULL;
            http_try_get_key_from_header(headers, "Content-Length", &content_length_text);

            if(content_length_text == NULL) {
                // We don't have a 'Content-Length'. Wait for the socket to close.
                printf("TODO: SS - Missing 'Content-Length'. Support this.\n");
                return HTTP_Parse_Result_TODO;
            }
            else {
                // We have a 'Content-Length'. Wait for 'Content-Length' bytes.
                uint64_t content_length = atoi(content_length_text);
                if(content_length > 0) {
                    // printf("I have %lu, need %lu.\n", buf_len, content_length);
                    if(buf_len < content_length) {
                        return HTTP_Parse_Result_Needs_More_Data;
                    }

                    string_buffer_append_buf(&out_body->string_buffer, &buf[0], content_length);
                }
            }

            break;
        }
        default: {
            printf("Unhandled Transfer-Encoding %i!\n", out_body->encoding);
            return HTTP_Parse_Result_TODO;
        }
    }

    return HTTP_Parse_Result_Done;
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

const char *http_get_status_text_for_status_code(int status_code) {
    return http_status_codes[status_code];
}

void http_dispose(HTTP *http) {
    assert(http != NULL);
    // TODO: SS - Do something here?
}