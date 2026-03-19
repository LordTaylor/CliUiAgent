#include <catch2/catch_test_macros.hpp>
#include <QCoreApplication>
#include <QDeadlineTimer>
#include "../src/cli/CliRunner.h"
#include "../src/cli/CliProfile.h"

using namespace CodeHex;

// A trivial echo profile for testing
class EchoProfile : public CliProfile {
public:
    QString name() const override { return "echo"; }
    QString displayName() const override { return "Echo"; }
    QString executable() const override {
#ifdef Q_OS_WIN
        return "cmd.exe";
#else
        return "echo";
#endif
    }
    QString defaultModel() const override { return "echo"; }
    QStringList buildArguments(const QString& prompt, const QString&) const override {
#ifdef Q_OS_WIN
        return {"/c", "echo", prompt};
#else
        return {prompt};
#endif
    }
    QString parseStreamChunk(const QByteArray& raw) const override {
        return QString::fromUtf8(raw);
    }
};

TEST_CASE("CliRunner sends and receives output via echo", "[cli]") {
    CliRunner runner;
    runner.setProfile(std::make_unique<EchoProfile>());

    QStringList chunks;
    QObject::connect(&runner, &CliRunner::outputChunk,
                     [&chunks](const QString& c) { chunks << c; });

    int finishedCode = -99;
    QObject::connect(&runner, &CliRunner::finished,
                     [&finishedCode](int code) { finishedCode = code; });

    runner.send("hello");

    // Process events until finished
    QDeadlineTimer deadline(3000);
    while (finishedCode == -99 && !deadline.hasExpired()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }

    CHECK(finishedCode == 0);
    CHECK_FALSE(chunks.isEmpty());
    const QString combined = chunks.join("");
    CHECK(combined.contains("hello", Qt::CaseInsensitive));
}
