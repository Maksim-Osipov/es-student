#pragma once
#ifndef LOG_LEVEL
#define LOG_LEVEL 2
#endif
void log_write(const char *level, const char *function, unsigned line, const char *format, ...);
#define LOG_ERR(...) do { if (LOG_LEVEL>=1) log_write("err", __func__, __LINE__, __VA_ARGS__); } while (0)
#define LOG_INF(...) do { if (LOG_LEVEL>=2) log_write("inf", __func__, __LINE__, __VA_ARGS__); } while (0)
#define LOG_DBG(...) do { if (LOG_LEVEL>=3) log_write("dbg", __func__, __LINE__, __VA_ARGS__); } while (0)
