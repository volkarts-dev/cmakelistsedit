// Copyright 2023, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once

#include <cmle/FileBuffer.h>
#include <QScopedPointer>

namespace cmle {

class ByteArrayFileBufferPrivate;

class ByteArrayFileBuffer : public cmle::FileBuffer
{
public:
    ByteArrayFileBuffer();
    ByteArrayFileBuffer(const QByteArray& content);
    ~ByteArrayFileBuffer() override;

    QString fileName() const override;

    QByteArray content() const override;
    void setContent(const QByteArray& content) override;

private:
    QScopedPointer<ByteArrayFileBufferPrivate> d_ptr;
    Q_DECLARE_PRIVATE(ByteArrayFileBuffer)

    Q_DISABLE_COPY_MOVE(ByteArrayFileBuffer)
};

} // namespace cmle
