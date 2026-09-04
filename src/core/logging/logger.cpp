#include "logger.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>
#include <QThread>

namespace {

QFile q_logFile;
QMutex q_logMutex;

constexpr quint64 MAX_LOG_SIZE = 5 * 1024 *1025; // 5MB

QString levelName(QtMsgType type)
{
    switch(type) {
    case QtDebugMsg:
        return QStringLiteral ("DEBUG");
    case QtWarningMsg:
        return QStringLiteral ("WARNING");
    case QtCriticalMsg:
        return QStringLiteral ("CRITICAL");
    case QtFatalMsg:
        return QStringLiteral ("FATAL");
    case QtInfoMsg:
        return QStringLiteral ("INFO");
        break;
    }
}

void rotateLogIfNeeded()
{
    if (!q_logFile.isOpen ()) {
        return;
    }
    if (q_logFile.size () < MAX_LOG_SIZE) {
        return;
    }

    const QString file_name = q_logFile.fileName ();

    q_logFile.close ();

    const QString backup = file_name + QStringLiteral (".old");

    QFile::remove (backup);
    QFile::rename (file_name, backup);

    q_logFile.setFileName (file_name);
    bool is_open = q_logFile.open (
        QIODevice::WriteOnly
        | QIODevice::Append
        | QIODevice::Text
        );
    if (!is_open) {
        qWarning() << "q_logFile is not open";
    }

}
QString colorForType(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return QStringLiteral("\033[90m"); // Gray

    case QtInfoMsg:
        return QStringLiteral("\033[32m"); // Green

    case QtWarningMsg:
        return QStringLiteral("\033[33m"); // Yellow

    case QtCriticalMsg:
        return QStringLiteral("\033[31m"); // Red

    case QtFatalMsg:
        return QStringLiteral("\033[1;31m"); // Bold red
    }

    return {};
}

} // namespace

namespace core {
namespace logging {

void Logger::initialize()
{
    QMutexLocker locker(&q_logMutex);

    const QString log_directory = QStandardPaths::writableLocation (QStandardPaths::AppConfigLocation) + QStringLiteral ("/logs");

    QDir().mkpath (log_directory);

    const QString log_path = log_directory + QStringLiteral ("/app.log");

    q_logFile.setFileName (log_path);

    if (!q_logFile.open (
            QIODevice::WriteOnly
            | QIODevice::Append
            | QIODevice::Text)) {
        qWarning() << "Failed to open log file:"
                   << log_path;
        return;
    }
    qInfo() << "Logger initialized:"
            << log_path;
}

void Logger::shutdown()
{
    QMutexLocker locker(&q_logMutex);

    if(q_logFile.isOpen ()) {
        q_logFile.flush ();
        q_logFile.close ();
    }
}

void Logger::debug(
    const QString message,
    const char *file,
    int line,
    const char *function)
{
    log(
        QtDebugMsg,
        message,
        file,
        line,
        function
        );
}

void Logger::info(
    const QString message,
    const char *file,
    int line,
    const char *function)
{
    log(
        QtInfoMsg,
        message,
        file,
        line,
        function
        );
}

void Logger::warning(
    const QString message,
    const char *file,
    int line,
    const char *function)
{
    log(
        QtWarningMsg,
        message,
        file,
        line,
        function
        );
}

void Logger::critical(
    const QString message,
    const char *file,
    int line,
    const char *function)
{
    log(
        QtCriticalMsg,
        message,
        file,
        line,
        function
        );
}

void Logger::log(
    QtMsgType type,
    const QString message,
    const char *file,
    int line,
    const char *function)
{
#ifndef QT_DEBUG
    if (type == QtDebugMsg) {
        return;
    }
#endif

    QMutexLocker locker(&q_logMutex);
    rotateLogIfNeeded ();

    const QString timestamp = QDateTime::currentDateTime ()
                                  .toString (QStringLiteral ("yyyy-MM-dd HH::mm::ss.zzz"));

    const QString thread_id = QString::number (reinterpret_cast<quintptr>(QThread::currentThread ()), 16);

    const QString file_name = QFileInfo(QString::fromUtf8 (file)).fileName ();


    const QString output = QStringLiteral("[%1] [%2] [Thread:%3] [%4:%5] %6 | %7")
                               .arg(
                                   timestamp,
                                   levelName(type),
                                   thread_id,
                                   file_name,
                                   QString::number(line),
                                   QString::fromUtf8(function),
                                   message);

    constexpr auto RESET = "\033[0m";
    const QByteArray outputUtf8 = output.toUtf8();

    fprintf(
        stderr,
        "%s%s%s\n",
        colorForType(type).toUtf8().constData(),
        outputUtf8.constData(),
        RESET
        );

    // File
    if (q_logFile.isOpen ()) {
        QTextStream stream(&q_logFile);
        stream << output << '\n';
        stream.flush ();
    }

}



} // namespace logging
} // namespace core