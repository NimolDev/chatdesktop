#include "storage/message_cache.hpp"
#include "storage/file_name_helper.hpp"
#include "constants/app_constants.hpp"
#include "data/dto/message_payload_dto.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace core {
namespace constants {
QString AppConstants::appSecretKey()
{
    return QStringLiteral("benchmark-only-key");
}
} // namespace constants
} // namespace core

void check(bool ok)
{
    if (!ok) {
        throw std::runtime_error("Benchmark correctness check failed");
    }
}

template <typename F>
double measure(F operation)
{
    QElapsedTimer timer;
    timer.start();
    operation();
    return timer.nsecsElapsed() / 1e6;
}

void report(const char *operation, std::vector<double> samples)
{
    std::sort(samples.begin(), samples.end());
    std::cout << operation << ",median_ms=" << samples[samples.size() / 2]
              << ",p95_ms=" << samples[static_cast<size_t>(std::ceil(samples.size() * .95)) - 1]
              << std::endl;
}

int runBenchmark(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    app.setApplicationName("ChatCacheBenchmark-" + QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QDir appData(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    struct Cleanup {
        QDir directory;
        ~Cleanup() { core::storage::MessageCache::closeAll(); directory.removeRecursively(); }
    } cleanup{appData};
    const QString secret = QStringLiteral("benchmark-secret");
    using core::storage::MessageCache;
    for (int count : {20, 200}) {
        data::dto::MessageDto dto{};
        dto.page = 1;
        dto.limit = count;
        dto.total = count;
        dto.total_pages = 1;
        for (int index = 0; index < count; ++index) {
            data::dto::MessageItemDto item{};
            item.id = QString::number(index);
            item.sender_id = "benchmark-sender";
            item.recipient_id = "benchmark-recipient";
            item.sent_at = "2026-10-09T00:00:00Z";
            item.body.message_id = item.id;
            item.body.sender_id = item.sender_id;
            item.body.receiver_id = item.recipient_id;
            item.body.timestamp = "1791504000000";
            item.body.content.type = domain::entity::MessageType::Text;
            item.body.content.text = QString(256, QLatin1Char('x')) + QString::fromUtf8(" សួស្តី 🌏");
            dto.messages.append(item);
        }
        const QString payload = QString::fromUtf8(QJsonDocument(dto.toJson()).toJson(QJsonDocument::Compact));
        for (int snapshots : {1, 10, 50}) {
            const QString name = QUuid::createUuid().toString(QUuid::WithoutBraces);
            for (int index = 0; index < snapshots; ++index) {
                check(MessageCache::save(payload, name, secret));
            }
            const QString path = appData.filePath("user_data/" + (QStringLiteral("messages-v3-") + core::storage::FileNameHelper::hexNameV2(name, secret)));
            std::cout << "messages=" << count << ",snapshots=" << snapshots
                      << ",payload_bytes=" << payload.toUtf8().size()
                      << ",file_bytes=" << QFileInfo(path).size() << std::endl;
            std::vector<double> reads, decodes, writes;
            // Warm filesystem cache; each public call still reopens/decrypts the store.
            check(MessageCache::read(name, secret).has_value());
            for (int run = 0; run < 20; ++run) {
                std::optional<QStringList> batches;
                reads.push_back(measure([&] { batches = MessageCache::read(name, secret); }));
                check(batches && batches->size() == snapshots && batches->last() == payload);
                decodes.push_back(measure([&] {
                    auto decoded = data::dto::MessageDto::fromJson(QJsonDocument::fromJson(batches->last().toUtf8()).object());
                    check(decoded && decoded->messages.size() == count);
                    check(decoded->toDomain().messages.size() == count);
                }));
            }
            report("read_all_snapshots", reads);
            report("decode_latest_dto_and_domain", decodes);
            // Append-only API: these samples grow the file by 20 snapshots.
            for (int run = 0; run < 20; ++run) {
                writes.push_back(measure([&] { check(MessageCache::save(payload, name, secret)); }));
            }
            report("append_20_snapshots", writes);
            for (int page = 1; page <= snapshots; ++page) {
                check(MessageCache::savePage(payload, name, secret, page));
            }
            std::vector<double> pageReads, pageWrites;
            for (int run = 0; run < 20; ++run) {
                pageReads.push_back(measure([&] {
                    check(MessageCache::readPage(name, secret, 1) == payload);
                }));
                pageWrites.push_back(measure([&] {
                    check(MessageCache::savePage(payload, name, secret, 1));
                }));
            }
            report("read_one_page", pageReads);
            report("replace_one_page", pageWrites);
            std::cout << "conversation_file_bytes_after_20_replacements="
                      << QFileInfo(path).size() << std::endl;

            auto final = MessageCache::read(name, secret);
            check(final && final->size() == snapshots * 2 + 20 && final->last() == payload);
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    try {
        return runBenchmark(argc, argv);
    } catch (const std::exception &error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
