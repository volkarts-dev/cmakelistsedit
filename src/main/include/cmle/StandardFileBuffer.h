// Copyright 2021-2023, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cmle/FileBuffer.h>
#include <QScopedPointer>
#include <QString>

namespace cmle {

class StandardFileBufferPrivate;

class StandardFileBuffer : public cmle::FileBuffer
{
public:
    StandardFileBuffer();
    StandardFileBuffer(const QString& fileName);
    ~StandardFileBuffer() override;

    bool isDirty() const;

    QString fileName() const override;
    void setFileName(const QString& fileName);
    bool load();
    bool save();

    QByteArray content() const override;
    void setContent(const QByteArray& content) override;

private:
    QScopedPointer<StandardFileBufferPrivate> d_ptr;
    Q_DECLARE_PRIVATE(StandardFileBuffer)

    Q_DISABLE_COPY_MOVE(StandardFileBuffer)
};

} // namespace cmle
