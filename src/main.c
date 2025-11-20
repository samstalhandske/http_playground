#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "http/client/http_client.h"

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

    // char body_data[] = "{\"latitude\":52.52,\"longitude\":13.419998,\"generationtime_ms\":0.041484832763671875,\"utc_offset_seconds\":0,\"timezone\":\"GMT\",\"timezone_abbreviation\":\"GMT\",\"elevation\":38.0,\"hourly_units\":{\"time\":\"iso8601\",\"temperature_2m\":\"°C\"},\"hourly\":{\"time\":[\"2025-10-29T00:00\",\"2025-10-29T01:00\",\"2025-10-29T02:00\",\"2025-10-29T03:00\",\"2025-10-29T04:00\",\"2025-10-29T05:00\",\"2025-10-29T06:00\",\"2025-10-29T07:00\",\"2025-10-29T08:00\",\"2025-10-29T09:00\",\"2025-10-29T10:00\",\"2025-10-29T11:00\",\"2025-10-29T12:00\",\"2025-10-29T13:00\",\"2025-10-29T14:00\",\"2025-10-29T15:00\",\"2025-10-29T16:00\",\"2025-10-29T17:00\",\"2025-10-29T18:00\",\"2025-10-29T19:00\",\"2025-10-29T20:00\",\"2025-10-29T21:00\",\"2025-10-29T22:00\",\"2025-10-29T23:00\"],\"temperature_2m\":[9.5,9.0,8.8,8.1,8.1,7.8,7.5,7.6,8.4,9.8,11.5,12.8,13.6,14.1,14.1,14.0,13.3,12.5,11.7,11.4,11.1,10.8,10.4,10.4]}}";

    if(!http_try_parse(&parser, &input_buffer[0], amount_of_bytes_in_file, &http)) {
        free(input_buffer);
        return HTTP_Fuzz_Result_Failed_To_Parse;
    }

    free(input_buffer);
    return HTTP_Fuzz_Result_OK;
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
    
    return 0;
}