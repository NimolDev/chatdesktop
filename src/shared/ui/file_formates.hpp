#pragma once

#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <algorithm>

class FileFormates
{
public:
    inline static const QStringList kImages = {
        "jpg", "jpeg", "png", "webp", "gif", "bmp",
        "heic", "heif", "avif", "jfif", "jxl", "pgm", "ppm", "xbm", "xpm"
    };

    inline static const QStringList kVideos = {
        "mp4", "mov", "mkv", "avi", "webm"
    };

    inline static const QStringList kAudios = {
        "mp3", "wav", "m4a", "aac", "ogg", "flac"
    };
    inline static const QStringList kDocuments = {
        "pdf", "doc", "docx", "txt", "xlsx"
    };
    static QString filter(const QStringList &extensions)
    {
        QStringList patterns;

        for (const QString &ext : extensions) {
            patterns.append ("*." + ext);
        }
        return "(" + patterns.join (' ') + ")";
    }
    static bool matches(const QString &file_path,
                        const QStringList &extensions)
    {
        const QString ext = QFileInfo(file_path).suffix ().toLower ();
        return extensions.contains (ext);
    }

    static bool matches(const QStringList &file_paths,
                        const QStringList &extensions)
    {

        if (file_paths.isEmpty()) {
            return false;
        }

        return std::all_of(
            file_paths.cbegin(),
            file_paths.cend(),
            [&extensions](const QString &path) {
                return matches(path, extensions);
            }
            );
    }

    // Return only matching files
    static QStringList filterMatches(const QStringList &file_paths,
                                     const QStringList &extensions)

    {
        QStringList result;
        for (const auto &path : file_paths) {
            if (matches(path, extensions)) {
                result.append(path);
            }
        }
        return result;
    }
};
