#pragma once

#include <QDialog>

class HelpDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit HelpDialog(QWidget *parent = nullptr);
};
