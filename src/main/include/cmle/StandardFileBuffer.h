/* CMakeListsEdit
 *
 * Copyright 2021 Daniel Volk <mail@volkarts.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 */

#pragma once

#include <cmle/FileBuffer.h>
#include <QString>
#include <QObject>

namespace cmle {

class StandardFileBufferPrivate;

class StandardFileBuffer : public QObject, public cmle::FileBuffer
{
    Q_OBJECT

public:
    explicit StandardFileBuffer(QObject* parent = nullptr);
    explicit StandardFileBuffer(const QString& fileName, QObject* parent = nullptr);
    ~StandardFileBuffer() override;

    bool isDirty() const;

    QString fileName() const override;
    void setFileName(const QString& fileName);
    bool load();
    bool save();

    QByteArray content() const override;
    void setContent(const QByteArray& content) override;

signals:
    void changed();

private:
    QScopedPointer<StandardFileBufferPrivate> d_ptr;
    Q_DECLARE_PRIVATE(StandardFileBuffer)
};

} // namespace cmle
