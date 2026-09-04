#ifndef CORE_CONSTANTS_LOGGER_HPP
#define CORE_CONSTANTS_LOGGER_HPP

#include <QLoggingCategory>
#include <QString>

namespace core {
namespace logging {

class Logger
{
public:
    Logger() = delete;

    static void initialize();
    static void shutdown();

    static void debug(
        const QString message,
        const char *file,
        int line,
        const char *function);

    static void info(
        const QString message,
        const char *file,
        int line,
        const char *function);

    static void warning(
        const QString message,
        const char *file,
        int line,
        const char *function);

    static void critical(
        const QString message,
        const char *file,
        int line,
        const char *function);

private:
    static void log(
        QtMsgType type,
        const QString message,
        const char *file,
        int line,
        const char *function);
};


} // namespace logging
} // namespace core


#define LOG_DEBUG(message) \
::core::logging::Logger::debug( \
                                (message), __FILE__, __LINE__, Q_FUNC_INFO)

#define LOG_INFO(message) \
    ::core::logging::Logger::info( \
                                    (message), __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_WARNING(message) \
    ::core::logging::Logger::warning( \
                                    (message), __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_CRITICAL(message) \
    ::core::logging::Logger::critical( \
                                    (message), __FILE__, __LINE__, Q_FUNC_INFO)


#endif // CORE_CONSTANTS_LOGGER_HPP
