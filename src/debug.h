
#ifndef UNO_CLI_DEBUG_H
#define UNO_CLI_DEBUG_H


#define LOG_FILE_NAME "application.log"

// A variadic macro that automatically passes file name, line number, and function name
#define LOG_TO_FILE(format, ...) \
    log_message(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

void log_message(const char* file, int line, const char* func, const char* format, ...);

#endif //UNO_CLI_DEBUG_H
