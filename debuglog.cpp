#include "debuglog.h"
#include <QtLogging>
#include <cstdio>

thread_local DebugLog* DebugLog::current_ = nullptr;

DebugLog::DebugLog() : previous_(current_) { current_ = this; }
DebugLog::~DebugLog() { current_ = previous_; }
QString DebugLog::text() const { return lines_.join('\n'); }

void DebugLog::install()
{
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& context, const QString& text) {
        if (type == QtDebugMsg && current_)
            current_->lines_.append(text);
        const auto bytes = qFormatLogMessage(type, context, text).toLocal8Bit();
        std::fprintf(stderr, "%s\n", bytes.constData());
    });
}
