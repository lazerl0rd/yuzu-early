// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;
class QHBoxLayout;
class QLabel;
class QListWidget;
class QVBoxLayout;

class InstallDialog : public QDialog {
    Q_OBJECT

public:
    explicit InstallDialog(QWidget* parent, const QStringList& files);
    ~InstallDialog() override;

    QStringList GetFilenames() const;
    bool ShouldOverwriteFiles() const;

private:
    QListWidget* file_list;

    QVBoxLayout* vbox_layout;
    QHBoxLayout* hbox_layout;

    QLabel* description;
    QCheckBox* overwrite_files;
    QDialogButtonBox* buttons;
};