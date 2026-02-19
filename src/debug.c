
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "debug.h"


void log_message(const char* file, int line, const char* func, const char* format, ...) {
    FILE* log_file = fopen(LOG_FILE_NAME, "a"); // Open file in append mode
    if (log_file == NULL) {
        // Fallback to stderr if file cannot be opened
        fprintf(stderr, "Error: Could not open log file %s\n", LOG_FILE_NAME);
        return;
    }

    // Optional: Add a timestamp
    time_t now = time(NULL);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(log_file, "[%s] ", time_str);

    // Print the file, line, and function information
    fprintf(log_file, "[%s:%d %s] ", file, line, func);

    // Use vfprintf to handle the variadic arguments from the macro
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    fprintf(log_file, "\n"); // Add a newline
    fclose(log_file); // Close the file after each log message
}

