#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "worker/worker.h"
#include "http/client/http_client.h"

void http_client_request_callback(const char *hostname, const char *path, HTTP *http) {
    HTTP_Status *status = &http->status;
    HTTP_Headers *headers = &http->headers;
    HTTP_Body *body = &http->body;

    printf("Got HTTP response from '%s/%s'!\n", hostname, path);
    printf("   Status code: %i (%s).\n", status->status_code, status->status_text);
    
    printf("   Headers:\n");
    for(uint32_t i = 0; i < headers->header_count; i++) {
        HTTP_Header *header = &headers->headers[i];
        printf("    - %i. Key: '%s', Value: '%s'.\n", i, header->key, header->value);
    }

    printf("Body (%lu):\n%s\n", body->string_buffer.length, body->string_buffer.data);
}

int main() {
    Worker worker = {0};
    worker.name = "Main Worker";

    http_client_request(&worker, HTTP_Request_Method_GET, "api.open-meteo.com", "v1/forecast?latitude=52.52&longitude=13.41&hourly=temperature_2m&forecast_days=1", NULL, http_client_request_callback);
    http_client_request(&worker, HTTP_Request_Method_GET, "www.youtube.com", "", NULL, http_client_request_callback);
    http_client_request(&worker, HTTP_Request_Method_GET, "www.google.com", "", NULL, http_client_request_callback);
    http_client_request(&worker, HTTP_Request_Method_GET, "www.facebook.com", "", NULL, http_client_request_callback);
    http_client_request(&worker, HTTP_Request_Method_GET, "www.example.com", "", NULL, http_client_request_callback);
    http_client_request(&worker, HTTP_Request_Method_GET, "www.iana.org", "help/example-domains", NULL, http_client_request_callback);

    // { // A simple test. Tests the HTTP parser on different responses.
    //     HTTP http;
    //     memset(&http, 0, sizeof(HTTP));
    
    //     HTTP_Parser parser;
    //     memset(&parser, 0, sizeof(HTTP_Parser));
    //     http_parser_init(&parser, 1024);
    
    //     // char response_buffer[] = // TODO: SS - Add a utility function for building a response to minimize risk for bugs.
    //     //     "HTTP/1.1 200 OK\r\n"
    //     //     "Date: Wed, 29 Oct 2025 09:02:16 GMT\r\n"
    //     //     "Content-Type: application/json; charset=utf-8\r\n"
    //     //     "Transfer-Encoding: chunked\r\n"
    //     //     "Connection: keep-alive\r\n"
    //     //     "\r\n"
    //     //     "19\r\n"
    //     //     "ABCDEFGHIJKLMNOPQRSTUVXYZ\r\n"
    //     //     "0\r\n"
    //     // ;

    //     // char response_buffer[] =
    //     //     "HTTP/1.1 200 OK\r\n"
    //     //     "Date: Wed, 29 Oct 2025 09:02:16 GMT\r\n"
    //     //     "Content-Type: application/json; charset=utf-8\r\n"
    //     //     "Transfer-Encoding: chunked\r\n"
    //     //     "Connection: keep-alive\r\n"
    //     //     "\r\n"
    //     //     "33e\r\n"
    //     //     "{\"latitude\":52.52,\"longitude\":13.419998,\"generationtime_ms\":0.041484832763671875,\"utc_offset_seconds\":0,\"timezone\":\"GMT\",\"timezone_abbreviation\":\"GMT\",\"elevation\":38.0,\"hourly_units\":{\"time\":\"iso8601\",\"temperature_2m\":\"°C\"},\"hourly\":{\"time\":[\"2025-10-29T00:00\",\"2025-10-29T01:00\",\"2025-10-29T02:00\",\"2025-10-29T03:00\",\"2025-10-29T04:00\",\"2025-10-29T05:00\",\"2025-10-29T06:00\",\"2025-10-29T07:00\",\"2025-10-29T08:00\",\"2025-10-29T09:00\",\"2025-10-29T10:00\",\"2025-10-29T11:00\",\"2025-10-29T12:00\",\"2025-10-29T13:00\",\"2025-10-29T14:00\",\"2025-10-29T15:00\",\"2025-10-29T16:00\",\"2025-10-29T17:00\",\"2025-10-29T18:00\",\"2025-10-29T19:00\",\"2025-10-29T20:00\",\"2025-10-29T21:00\",\"2025-10-29T22:00\",\"2025-10-29T23:00\"],\"temperature_2m\":[9.5,9.0,8.8,8.1,8.1,7.8,7.5,7.6,8.4,9.8,11.5,12.8,13.6,14.1,14.1,14.0,13.3,12.5,11.7,11.4,11.1,10.8,10.4,10.4]}}\r\n"
    //     //     "0\r\n"
    //     // ;

    //     char response_buffer[] =
    //         "HTTP/1.1 200 OK\r\n"
    //         "Date: Wed, 29 Oct 2025 09:02:16 GMT\r\n"
    //         "Content-Type: application/json; charset=utf-8\r\n"
    //         "Transfer-Encoding: chunked\r\n"
    //         "Connection: keep-alive\r\n"
    //         "\r\n"
    //         "33e\r\n"
    //         "{\"latitude\":52.52,\"longitude\":13.419998,\"generationtime_ms\":0.041484832763671875,\"utc_offset_seconds\":0,\"timezone\":\"GMT\",\"timezone_abbreviation\":\"GMT\",\"elevation\":38.0,\"hourly_units\":{\"time\":\"iso8601\",\"temperature_2m\":\"°C\"},\"hourly\":{\"time\":[\"2025-10-29T00:00\",\"2025-10-29T01:00\",\"2025-10-29T02:00\",\"2025-10-29T03:00\",\"2025-10-29T04:00\",\"2025-10-29T05:00\",\"2025-10-29T06:00\",\"2025-10-29T07:00\",\"2025-10-29T08:00\",\"2025-10-29T09:00\",\"2025-10-29T10:00\",\"2025-10-29T11:00\",\"2025-10-29T12:00\",\"2025-10-29T13:00\",\"2025-10-29T14:00\",\"2025-10-29T15:00\",\"2025-10-29T16:00\",\"2025-10-29T17:00\",\"2025-10-29T18:00\",\"2025-10-29T19:00\",\"2025-10-29T20:00\",\"2025-10-29T21:00\",\"2025-10-29T22:00\",\"2025-10-29T23:00\"],\"temperature_2m\":[9.5,9.0,8.8,8.1,8.1,7.8,7.5,7.6,8.4,9.8,11.5,12.8,13.6,14.1,14.1,14.0,13.3,12.5,11.7,11.4,11.1,10.8,10.4,10.4]}}\r\n"
    //         "0\r\n"
    //     ;
    
    //     assert(http_try_parse(&parser, &response_buffer[0], strlen(response_buffer), &http));
    
    //     printf("Text: '%s'\n", http.body.string_buffer.data);
    
    //     http_dispose(&http);
    // }

    while(worker_work(&worker) > 0);

    return 0;
}