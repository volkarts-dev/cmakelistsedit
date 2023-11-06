// Copyright 2021-2023, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "include/cmle/ByteArrayFileBuffer.h"

#include <QLoggingCategory>

namespace cmle {

class ByteArrayFileBufferPrivate
{
public:
    ByteArrayFileBufferPrivate(ByteArrayFileBuffer* q) :
        q_ptr{q}
    {
    }

    QByteArray fileContent{};

private:
    ByteArrayFileBuffer* q_ptr;
    Q_DECLARE_PUBLIC(ByteArrayFileBuffer)
};

// *********************************************************************************************************************

ByteArrayFileBuffer::~ByteArrayFileBuffer() = default;

ByteArrayFileBuffer::ByteArrayFileBuffer() :
    ByteArrayFileBuffer(QByteArray{})
{
}

ByteArrayFileBuffer::ByteArrayFileBuffer(const QByteArray& content) :
    d_ptr{new ByteArrayFileBufferPrivate{this}}
{
    Q_D(ByteArrayFileBuffer);
    d->fileContent = content;
}

QString ByteArrayFileBuffer::fileName() const
{
    return QStringLiteral("[GenericByteArray]");
}

QByteArray ByteArrayFileBuffer::content() const
{
    Q_D(const ByteArrayFileBuffer);
    return d->fileContent;
}

void ByteArrayFileBuffer::setContent(const QByteArray& content)
{
    Q_D(ByteArrayFileBuffer);
    d->fileContent = content;
}

} // namespace cmle
