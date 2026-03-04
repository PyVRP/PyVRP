#ifndef PYVRP_LOGGING_H
#define PYVRP_LOGGING_H

#include <spdlog/spdlog.h>

/**
 * Logging macro definitions, mostly as simple wrappers around spdlog macros.
 * Use these instead of relying on spdlog directly.
 */

#define PYVRP_LEVEL_DEBUG SPDLOG_LEVEL_DEBUG
#define PYVRP_LEVEL_INFO SPDLOG_LEVEL_INFO
#define PYVRP_LEVEL_WARN SPDLOG_LEVEL_WARN
#define PYVRP_LEVEL_ERROR SPDLOG_LEVEL_ERROR
#define PYVRP_LEVEL_CRITICAL SPDLOG_LEVEL_CRITICAL

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
