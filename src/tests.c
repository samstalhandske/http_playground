#include <stdio.h>
#include <string.h>
#include "http/client/http_client.h"

// TODO: SS - Run tests.
// TODO: SS - Create an application for each test and run them for it be more correct.

void test_http_parse_response() { // Verifies that the HTTP parser works as expected.
    HTTP http;
    memset(&http, 0, sizeof(HTTP));

    HTTP_Parser parser;
    memset(&parser, 0, sizeof(HTTP_Parser));
    http_parser_init(&parser, 1024);

    char json_data[] = "{\"latitude\":52.52,\"longitude\":13.419998,\"generationtime_ms\":0.041484832763671875,\"utc_offset_seconds\":0,\"timezone\":\"GMT\",\"timezone_abbreviation\":\"GMT\",\"elevation\":38.0,\"hourly_units\":{\"time\":\"iso8601\",\"temperature_2m\":\"°C\"},\"hourly\":{\"time\":[\"2025-10-29T00:00\",\"2025-10-29T01:00\",\"2025-10-29T02:00\",\"2025-10-29T03:00\",\"2025-10-29T04:00\",\"2025-10-29T05:00\",\"2025-10-29T06:00\",\"2025-10-29T07:00\",\"2025-10-29T08:00\",\"2025-10-29T09:00\",\"2025-10-29T10:00\",\"2025-10-29T11:00\",\"2025-10-29T12:00\",\"2025-10-29T13:00\",\"2025-10-29T14:00\",\"2025-10-29T15:00\",\"2025-10-29T16:00\",\"2025-10-29T17:00\",\"2025-10-29T18:00\",\"2025-10-29T19:00\",\"2025-10-29T20:00\",\"2025-10-29T21:00\",\"2025-10-29T22:00\",\"2025-10-29T23:00\"],\"temperature_2m\":[9.5,9.0,8.8,8.1,8.1,7.8,7.5,7.6,8.4,9.8,11.5,12.8,13.6,14.1,14.1,14.0,13.3,12.5,11.7,11.4,11.1,10.8,10.4,10.4]}}";

    // TODO: SS - Add a utility function for building a response to minimize risk for bugs when creating testing.
    char response_buffer[1024];
    memset(&response_buffer[0], 0, 1024);
    snprintf(&response_buffer[0], 1024,
        "HTTP/1.1 200 OK\r\n"
        "Date: Wed, 29 Oct 2025 09:02:16 GMT\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: keep-alive\r\n"
        "\r\n"
        "%X\r\n"
        "%s\r\n"
        "0\r\n",
        (uint32_t)strlen(json_data),
        json_data
    );

    assert(http_try_parse(&parser, &response_buffer[0], strlen(response_buffer), &http));

    // printf("Text: '%s'\n", http.body.string_buffer.data);
    if(strcmp(http.body.string_buffer.data, json_data) != 0) {
        printf("Test failed! Expected '%s' but got '%s'.\n", json_data, http.body.string_buffer.data);
        assert(false);
    }
}

void run_all_tests() {
    test_http_parse_response();
}