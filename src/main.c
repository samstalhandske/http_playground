#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "string/buffer/string_buffer.h"
// #include "http/client/http_client.h"
#include "http/http.h"

typedef enum {
    HTTP_Fuzz_Result_OK,
    HTTP_Fuzz_Result_Failed_To_Open_Input_File,
    HTTP_Fuzz_Result_Failed_When_Reading_Input_File,
    HTTP_Fuzz_Result_Input_File_Empty,
    HTTP_Fuzz_Result_Failed_To_Parse,
    HTTP_Fuzz_Result_Count,
} HTTP_Fuzz_Result;

const char *fuzz_result_descriptions[HTTP_Fuzz_Result_Count] = {
    "OK",
    "Failed to open input-file",
    "Failed when reading input-file",
    "Input-file empty",
    "Failed to parse"
};

HTTP_Fuzz_Result http_fuzz(const char *input_file_path) {
    FILE *input_file = fopen(input_file_path, "rb");
    if(input_file == NULL) {
        return HTTP_Fuzz_Result_Failed_To_Open_Input_File;
    }

    fseek(input_file, 0, SEEK_END);
    long amount_of_bytes_in_file = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);

    if(amount_of_bytes_in_file == 0) {
        fclose(input_file);
        return HTTP_Fuzz_Result_Input_File_Empty;
    }
    if(amount_of_bytes_in_file < 0) {
        fclose(input_file);
        return HTTP_Fuzz_Result_Failed_When_Reading_Input_File;
    }

    char *input_buffer = (char *)malloc(amount_of_bytes_in_file * sizeof(char) + 1);
    assert(input_buffer != NULL);
    size_t bytes_read_into_buffer = fread(&input_buffer[0], sizeof(char), amount_of_bytes_in_file, input_file);
    if(bytes_read_into_buffer != (size_t)amount_of_bytes_in_file) {
        fclose(input_file);
        free(input_buffer);
        return HTTP_Fuzz_Result_Failed_When_Reading_Input_File;
    }

    input_buffer[amount_of_bytes_in_file] = '\0';

    fclose(input_file);

    HTTP http;
    memset(&http, 0, sizeof(HTTP));

    HTTP_Parser parser;
    memset(&parser, 0, sizeof(HTTP_Parser));
    http_parser_init(&parser, 1024);

    HTTP_Parse_Result parse_result = http_try_parse(&parser, &input_buffer[0], amount_of_bytes_in_file, &http);
    switch(parse_result) {
        case HTTP_Parse_Result_Done: {
            break;
        }
        case HTTP_Parse_Result_Invalid_Data:
        case HTTP_Parse_Result_Needs_More_Data:
        case HTTP_Parse_Result_TODO: {
            free(input_buffer);
            return HTTP_Fuzz_Result_Failed_To_Parse;
        }
    }

    free(input_buffer);
    return HTTP_Fuzz_Result_OK;
}

void http_client_request_callback(const char *hostname, const char *path, HTTP *http) {
    HTTP_Status *status = &http->status;
    HTTP_Headers *headers = &http->headers;
    HTTP_Body *body = &http->body;

    printf("Got HTTP response from '%s/%s'!\n", hostname, path);
    printf("   Status code: %i (%s).\n", status->status_code, http_get_status_text_for_status_code(status->status_code));

    printf("   Headers:\n");
    for(uint32_t i = 0; i < headers->header_count; i++) {
        HTTP_Header *header = &headers->headers[i];
        printf("    - %i. Key: '%s', Value: '%s'.\n", i, header->key, header->value);
    }

    printf("Body (%lu):\n%s\n", body->string_buffer.length, body->string_buffer.data);
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Usage: %s <http-file> ..\n", argv[0]);
        return 0;
    }

    printf("Fuzzing HTTP-parser with %i files ...\n", argc - 1);

    int amount_ok = 0;
    
    uint32_t i;
    for(i = 1; i < (uint32_t)argc; i++) {
        printf("%i. '%s' ...\n", i, argv[i]);
        HTTP_Fuzz_Result fuzz_result = http_fuzz(argv[i]);

        printf("\nResult: ");
        if(fuzz_result != HTTP_Fuzz_Result_OK) {
            printf("\033[31;1mFail!\033[0m (%s)\n\n", fuzz_result_descriptions[fuzz_result]); // TODO: SS - fuzz_result_to_string(fuzz_result)
            continue;
        }

        printf("\033[32;1mOK!\033[0m\n\n");
        amount_ok += 1;
    }

    printf("Result: %i/%i OK.\n", amount_ok, argc - 1);


    /////////////////////////////////////

    // Worker worker = {0};
    // worker.name = "Main Worker";

    // http_client_request(
    //     &worker,
    //     HTTP_Method_POST,
    //     "httpbin.org", "post",
    //     "{\r\n"
    //     "    \"elite_value\": 1337,\r\n"
    //     "    \"device\": \"UUID\",\r\n"
    //     "    \"time\": \"<time>\",\r\n"
    //     "    \"temperature\": \"<temperature>°C\"\r\n"
    //     "}\r\n",
    //     http_client_request_callback
    // );

    // http_client_request(
    //     &worker,
    //     HTTP_Method_GET,
    //     "api.open-meteo.com", "v1/forecast?latitude=52.52&longitude=13.41&hourly=temperature_2m",
    //     NULL,
    //     http_client_request_callback
    // );


    // http_client_request(
    //     &worker,
    //     HTTP_Method_GET,
    //     "chasacademy.instructure.com", "courses/589",
    //     NULL,
    //     http_client_request_callback
    // );

    // while(worker_work(&worker) > 0);
    
    return 0;
}