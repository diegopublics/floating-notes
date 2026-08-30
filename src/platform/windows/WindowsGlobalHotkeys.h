#pragma once

#include "platform/GlobalHotkeys.h"

#include <QAbstractNativeEventFilter>

class WindowsGlobalHotkeys final : public QAbstractNativeEventFilter
{
public:
    explicit WindowsGlobalHotkeys(GlobalHotkeys::Callback callback);
    ~WindowsGlobalHotkeys() override;

    bool registerHotkeys(QString *errorMessage = nullptr);
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    GlobalHotkeys::Callback m_callback;
    bool m_registered = false;
};
