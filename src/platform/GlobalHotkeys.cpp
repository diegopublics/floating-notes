#include "GlobalHotkeys.h"

#include <QString>

#if defined(Q_OS_WIN)
#include "windows/WindowsGlobalHotkeys.h"
#endif

class GlobalHotkeys::Impl
{
public:
    explicit Impl(Callback callback)
#if defined(Q_OS_WIN)
        : windowsHotkeys(std::move(callback))
#else
        : callback(std::move(callback))
#endif
    {
    }

    bool registerHotkeys(QString *errorMessage)
    {
#if defined(Q_OS_WIN)
        return windowsHotkeys.registerHotkeys(errorMessage);
#else
        Q_UNUSED(errorMessage)
        return false;
#endif
    }

#if defined(Q_OS_WIN)
    WindowsGlobalHotkeys windowsHotkeys;
#else
    Callback callback;
#endif
};

GlobalHotkeys::GlobalHotkeys(Callback callback)
    : m_impl(std::make_unique<Impl>(std::move(callback)))
{
}

GlobalHotkeys::~GlobalHotkeys() = default;

bool GlobalHotkeys::registerHotkeys(QString *errorMessage)
{
    return m_impl->registerHotkeys(errorMessage);
}
