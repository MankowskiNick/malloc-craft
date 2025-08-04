#include <asset.h>
#include <stdio.h>
#include <stdlib.h>

char* read_file_to_string(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the contents (+1 for null terminator)
    char* buffer = malloc(length + 1);
    if (!buffer) {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    // Read file contents
    size_t bytesRead = fread(buffer, 1, length, file);
    buffer[bytesRead] = '\0'; // Null-terminate the string

    fclose(file);
    return buffer;
}