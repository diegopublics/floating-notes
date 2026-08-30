#pragma once

#include <QString>

#include <functional>
#include <memory>

class GlobalHotkeys
{
public:
    enum class Action {
        NewNote,
        AllNotes,
        ArchiveCurrent,
    };

    using Callback = std::function<void(Action)>;

    explicit GlobalHotkeys(Callback callback);
    ~GlobalHotkeys();

    bool registerHotkeys(QString *errorMessage = nullptr);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
