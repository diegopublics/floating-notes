#pragma once

#include <QString>

struct Note
{
    int id = 0;
    QString title;
    QString body;
    QString color;
    QString tags;
    QString updatedAt;
    bool archived = false;
};
