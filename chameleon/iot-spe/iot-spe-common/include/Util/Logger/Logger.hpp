/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        https://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef x_COMMON_INCLUDE_UTIL_LOGGER_LOGGER_HPP_
#define x_COMMON_INCLUDE_UTIL_LOGGER_LOGGER_HPP_
#include <Exceptions/NotImplementedException.hpp>
#include <Exceptions/SignalHandling.hpp>
#include <Util/Logger/LogLevel.hpp>
#include <Util/Logger/impl/xLogger.hpp>
#include <Util/StacktraceLoader.hpp>
#include <iostream>
#include <memory>
#include <sstream>
namespace x {

// In the following we define the x_COMPILE_TIME_LOG_LEVEL macro.
// This macro indicates the log level, which was chooses at compilation time and enables the complete
// elimination of log messages.
#if defined(x_LOGLEVEL_TRACE)
#define x_COMPILE_TIME_LOG_LEVEL 7
#elif defined(x_LOGLEVEL_DEBUG)
#define x_COMPILE_TIME_LOG_LEVEL 6
#elif defined(x_LOGLEVEL_INFO)
#define x_COMPILE_TIME_LOG_LEVEL 5
#elif defined(x_LOGLEVEL_WARN)
#define x_COMPILE_TIME_LOG_LEVEL 4
#elif defined(x_LOGLEVEL_ERROR)
#define x_COMPILE_TIME_LOG_LEVEL 3
#elif defined(x_LOGLEVEL_FATAL_ERROR)
#define x_COMPILE_TIME_LOG_LEVEL 2
#elif defined(x_LOGLEVEL_NONE)
#define x_COMPILE_TIME_LOG_LEVEL 1
#endif

/**
 * @brief LogCaller is our compile-time trampoline to invoke the Logger method for the desired level of logging L
 * @tparam L the level of logging
 */
template<LogLevel L>
struct LogCaller {
    template<typename... arguments>
    constexpr static void do_call(spdlog::source_loc&&, fmt::format_string<arguments...>, arguments&&...) {
        // nop
    }
};

template<>
struct LogCaller<LogLevel::LOG_INFO> {
    template<typename... arguments>
    constexpr static void do_call(spdlog::source_loc&& loc, fmt::format_string<arguments...> format, arguments&&... args) {
        x::Logger::getInstance()->info(std::move(loc), std::move(format), std::forward<arguments>(args)...);
    }
};

template<>
struct LogCaller<LogLevel::LOG_TRACE> {
    template<typename... arguments>
    constexpr static void do_call(spdlog::source_loc&& loc, fmt::format_string<arguments...> format, arguments&&... args) {
        x::Logger::getInstance()->trace(std::move(loc), std::move(format), std::forward<arguments>(args)...);
    }
};

template<>
struct LogCaller<LogLevel::LOG_DEBUG> {
    template<typename... arguments>
    constexpr static void do_call(spdlog::source_loc&& loc, fmt::format_string<arguments...> format, arguments&&... args) {
        x::Logger::getInstance()->debug(std::move(loc), std::move(format), std::forward<arguments>(args)...);
    }
};

template<>
struct LogCaller<LogLevel::LOG_ERROR> {
    template<typename... arguments>
    constexpr static void do_call(spdlog::source_loc&& loc, fmt::format_string<arguments...> format, arguments&&... args) {
        x::Logger::getInstance()->error(std::move(loc), std::move(format), std::forward<arguments>(args)...);
    }
};

template<>
struct LogCaller<LogLevel::LOG_FATAL_ERROR> {
    template<typename... arguments>
    constexpr static void do_call(spdlog::source_loc&& loc, fmt::format_string<arguments...> format, arguments&&... args) {
        x::Logger::getInstance()->fatal(std::move(loc), std::move(format), std::forward<arguments>(args)...);
    }
};

template<>
struct LogCaller<LogLevel::LOG_WARNING> {
    template<typename... arguments>
    constexpr static void do_call(spdlog::source_loc&& loc, fmt::format_string<arguments...> format, arguments&&... args) {
        x::Logger::getInstance()->warn(std::move(loc), std::move(format), std::forward<arguments>(args)...);
    }
};

/// @brief this is the new logging macro that is the entry point for logging calls
#define x_LOG(LEVEL, ...)                                                                                                      \
    do {                                                                                                                         \
        auto constexpr __level = x::getLogLevel(LEVEL);                                                                        \
        if constexpr (x_COMPILE_TIME_LOG_LEVEL >= __level) {                                                                   \
            x::LogCaller<LEVEL>::do_call(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__);                \
        }                                                                                                                        \
    } while (0)

// Creates a log message with log level trace.
#define x_TRACE(...) x_LOG(x::LogLevel::LOG_TRACE, __VA_ARGS__);
// Creates a log message with log level info.
#define x_INFO(...) x_LOG(x::LogLevel::LOG_INFO, __VA_ARGS__);
// Creates a log message with log level debug.
#define x_DEBUG(...) x_LOG(x::LogLevel::LOG_DEBUG, __VA_ARGS__);
// Creates a log message with log level warning.
#define x_WARNING(...) x_LOG(x::LogLevel::LOG_WARNING, __VA_ARGS__);
// Creates a log message with log level error.
#define x_ERROR(...) x_LOG(x::LogLevel::LOG_ERROR, __VA_ARGS__);
// Creates a log message with log level fatal error.
#define x_FATAL_ERROR(...) x_LOG(x::LogLevel::LOG_FATAL_ERROR, __VA_ARGS__);

/// I am aware that we do not like __ before variable names but here we need them
/// to avoid name collions, e.g., __buffer, __stacktrace
/// that should not be a problem because of the scope, however, better be safe than sorry :P
#ifdef x_DEBUG_MODE
//Note Verify is only evaluated in Debug but not in Release
#define x_VERIFY(CONDITION, TEXT)                                                                                              \
    do {                                                                                                                         \
        if (!(CONDITION)) {                                                                                                      \
            std::stringstream textString;                                                                                        \
            textString << TEXT;                                                                                                  \
            x_ERROR("x Fatal Error on {} message: {}", #CONDITION, textString.str());                                        \
            {                                                                                                                    \
                auto __stacktrace = x::collectAndPrintStacktrace();                                                            \
                std::stringbuf __buffer;                                                                                         \
                std::ostream __os(&__buffer);                                                                                    \
                __os << "Failed assertion on " #CONDITION;                                                                       \
                __os << " error message: " << TEXT;                                                                              \
                x::Exceptions::invokeErrorHandlers(__buffer.str(), std::move(__stacktrace));                                   \
            }                                                                                                                    \
        }                                                                                                                        \
    } while (0)
#else
#define x_VERIFY(CONDITION, TEXT) ((void) 0)
#endif

#define x_ASSERT(CONDITION, TEXT)                                                                                              \
    do {                                                                                                                         \
        if (!(CONDITION)) {                                                                                                      \
            std::stringstream textString;                                                                                        \
            textString << TEXT;                                                                                                  \
            x_ERROR("x Fatal Error on {} message: {}", #CONDITION, textString.str());                                        \
            {                                                                                                                    \
                auto __stacktrace = x::collectAndPrintStacktrace();                                                            \
                std::stringbuf __buffer;                                                                                         \
                std::ostream __os(&__buffer);                                                                                    \
                __os << "Failed assertion on " #CONDITION;                                                                       \
                __os << " error message: " << TEXT;                                                                              \
                x::Exceptions::invokeErrorHandlers(__buffer.str(), std::move(__stacktrace));                                   \
            }                                                                                                                    \
        }                                                                                                                        \
    } while (0)

#define x_ASSERT_THROW_EXCEPTION(CONDITION, EXCEPTION_TYPE, ...)                                                               \
    do {                                                                                                                         \
        if (!(CONDITION)) {                                                                                                      \
            std::stringstream args;                                                                                              \
            args << __VA_ARGS__;                                                                                                 \
            x_ERROR("x Fatal Error on {} message: {}", #CONDITION, args.str());                                              \
            {                                                                                                                    \
                auto __stacktrace = x::collectAndPrintStacktrace();                                                            \
                std::stringbuf __buffer;                                                                                         \
                std::ostream __os(&__buffer);                                                                                    \
                __os << "Failed assertion on " #CONDITION;                                                                       \
                __os << " error message: " << __VA_ARGS__;                                                                       \
                throw EXCEPTION_TYPE(__buffer.str());                                                                            \
            }                                                                                                                    \
        }                                                                                                                        \
    } while (0)

#define x_ASSERT2_FMT(CONDITION, ...)                                                                                          \
    do {                                                                                                                         \
        if (!(CONDITION)) {                                                                                                      \
            std::stringstream args;                                                                                              \
            args << __VA_ARGS__;                                                                                                 \
            x_ERROR("x Fatal Error on {} message: {}", #CONDITION, args.str());                                              \
            {                                                                                                                    \
                auto __stacktrace = x::collectAndPrintStacktrace();                                                            \
                std::stringbuf __buffer;                                                                                         \
                std::ostream __os(&__buffer);                                                                                    \
                __os << "Failed assertion on " #CONDITION;                                                                       \
                __os << " error message: " << __VA_ARGS__;                                                                       \
                x::Exceptions::invokeErrorHandlers(__buffer.str(), std::move(__stacktrace));                                   \
            }                                                                                                                    \
        }                                                                                                                        \
    } while (0)

#define x_THROW_RUNTIME_ERROR(...)                                                                                             \
    do {                                                                                                                         \
        auto __stacktrace = x::collectAndPrintStacktrace();                                                                    \
        std::stringbuf __buffer;                                                                                                 \
        std::ostream __os(&__buffer);                                                                                            \
        __os << __VA_ARGS__;                                                                                                     \
        const std::source_location __location = std::source_location::current();                                                 \
        throw x::Exceptions::RuntimeException(__buffer.str(), std::move(__stacktrace), std::move(__location));                 \
    } while (0)

#define x_NOT_IMPLEMENTED()                                                                                                    \
    do {                                                                                                                         \
        throw Exceptions::NotImplementedException("not implemented");                                                            \
    } while (0)

#define x_ERROR_OR_THROW_RUNTIME(THROW_EXCEPTION, ...)                                                                         \
    do {                                                                                                                         \
        if ((THROW_EXCEPTION)) {                                                                                                 \
            x_THROW_RUNTIME_ERROR(__VA_ARGS__);                                                                                \
        } else {                                                                                                                 \
            std::stringbuf __buffer;                                                                                             \
            std::ostream __os(&__buffer);                                                                                        \
            __os << __VA_ARGS__;                                                                                                 \
            x_ERROR("{}", __buffer.str());                                                                                     \
        }                                                                                                                        \
    } while (0)

}// namespace x

#endif// x_COMMON_INCLUDE_UTIL_LOGGER_LOGGER_HPP_
