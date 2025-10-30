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

void function_to_show_that_i_understand_double_pointers(char **greeting) {
    *greeting = "Bye!";
}

int main() {
    char *hello_text = "Hello there!";
    printf("%s\n", hello_text);

    Worker worker = {0};
    worker.name = "Main Worker";

    http_client_request(
        &worker,
        HTTP_Request_Method_POST,
        "httpbin.org", "post",
        "{\r\n"
        "    \"elite_value\": 1337,\r\n"
        "    \"device\": \"UUID\",\r\n"
        "    \"time\": \"<time>\",\r\n"
        "    \"temperature\": \"<temperature>°C\"\r\n"
        "}\r\n",
        http_client_request_callback
    );

    http_client_request(
        &worker,
        HTTP_Request_Method_GET,
        "api.open-meteo.com", "v1/forecast?latitude=52.52&longitude=13.41&hourly=temperature_2m",
        NULL,
        http_client_request_callback
    );


    http_client_request(
        &worker,
        HTTP_Request_Method_GET,
        "chasacademy.instructure.com", "courses/589",
        NULL,
        http_client_request_callback
    );

    while(worker_work(&worker) > 0);

    function_to_show_that_i_understand_double_pointers(&hello_text);
    printf("%s\n", hello_text);

    return 0;
}