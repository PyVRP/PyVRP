#ifndef PYVRP_LOGGING_H
#define PYVRP_LOGGING_H

#include <spdlog/spdlog.h>

#define PYVRP_LEVEL_DEBUG 1
#define PYVRP_LEVEL_INFO 2
#define PYVRP_LEVEL_WARN 3
#define PYVRP_LEVEL_ERROR 4
#define PYVRP_LEVEL_CRITICAL 5

#if PYVRP_LOG_LEVEL <= PYVRP_LEVEL_DEBUG
#define PYVRP_DEBUG(name, ...)                                                 \
    SPDLOG_LOGGER_CALL(spdlog::get(name), spdlog::level::debug, __VA_ARGS__)
#else
#define PYVRP_DEBUG(name, ...) (void)0
#endif

#if PYVRP_LOG_LEVEL <= PYVRP_LEVEL_INFO
#define PYVRP_INFO(name, ...)                                                  \
    SPDLOG_LOGGER_CALL(spdlog::get(name), spdlog::level::info, __VA_ARGS__)
#else
#define PYVRP_INFO(name, ...) (void)0
#endif

#if PYVRP_LOG_LEVEL <= PYVRP_LEVEL_WARN
#define PYVRP_WARN(name, ...)                                                  \
    SPDLOG_LOGGER_CALL(spdlog::get(name), spdlog::level::warn, __VA_ARGS__)
#else
#define PYVRP_WARN(name, ...) (void)0
#endif

#if PYVRP_LOG_LEVEL <= PYVRP_LEVEL_ERROR
#define PYVRP_ERROR(name, ...)                                                 \
    SPDLOG_LOGGER_CALL(spdlog::get(name), spdlog::level::err, __VA_ARGS__)
#else
#define PYVRP_ERROR(name, ...) (void)0
#endif

#if PYVRP_LOG_LEVEL <= PYVRP_LEVEL_CRITICAL
#define PYVRP_CRITICAL(name, ...)                                              \
    SPDLOG_LOGGER_CALL(spdlog::get(name), spdlog::level::critical, __VA_ARGS__)
#else
#define PYVRP_CRITICAL(name, ...) (void)0
#endif

#endif  // PYVRP_LOGGING_H
