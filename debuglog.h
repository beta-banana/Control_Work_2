#pragma once
#include <QStringList>

// qDebug остаётся в консоли; на время сценария также собираем его для окна.
class DebugLog {
public:
    DebugLog();
    ~DebugLog();
    DebugLog(const DebugLog&) = delete;
    DebugLog& operator=(const DebugLog&) = delete;
    QString text() const;
    static void install();
private:
    QStringList lines_;
    DebugLog* previous_ = nullptr;
    static thread_local DebugLog* current_;
};
