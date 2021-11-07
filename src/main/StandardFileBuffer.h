/* CMakeListsEdit
 *
 * Copyright 2021 Daniel Volk <mail@volkarts.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 */

#pragma once

#include "cmakelistsedit/FileBuffer.h"
#include <QString>
#include <QObject>

namespace cmle {

class StandardFileBuffer : public QObject, public cmle::FileBuffer
{
    Q_OBJECT

public:
    explicit StandardFileBuffer(QObject* parent = nullptr);
    StandardFileBuffer(const QString& fileName, QObject* parent = nullptr);
    ~StandardFileBuffer() override;

    bool isDirty() const { return dirty_; }

    QString fileName() const override { return fileName_; }
    void setFileName(const QString& fileName) { fileName_ = fileName; }

    bool load();
    bool save();

    QByteArray content() const override { return fileContent_; }
    void setContent(const QByteArray& content) override;

signals:
    void changed();

private:
    QString fileName_;
    QByteArray fileContent_;
    bool dirty_;
};

} // namespace cmle
