/* CMakeListsEdit
 *
 * Copyright 2021 Daniel Volk <mail@volkarts.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 */

#include "include/cmle/StandardFileBuffer.h"

#include <QFile>
#include <QLoggingCategory>

namespace cmle {

namespace {

const QLoggingCategory CMAKE{"CMAKE"};

} // namespace

// ********************************************************

class StandardFileBufferPrivate
{
public:
    StandardFileBufferPrivate(StandardFileBuffer* q) :
        q_ptr{q}
    {
    }

    QString fileName{};
    QByteArray fileContent{};
    bool dirty{};

private:
    StandardFileBuffer* q_ptr;
};

// ********************************************************

StandardFileBuffer::StandardFileBuffer(QObject* parent) :
    StandardFileBuffer({}, parent)
{
}

StandardFileBuffer::StandardFileBuffer(const QString& fileName, QObject* parent) :
    QObject(parent),
    d_ptr{new StandardFileBufferPrivate{this}}
{
    setFileName(fileName);
}

StandardFileBuffer::~StandardFileBuffer()
{
}

bool StandardFileBuffer::isDirty() const
{
    Q_D(const StandardFileBuffer);
    return d->dirty;
}

QString StandardFileBuffer::fileName() const
{
    Q_D(const StandardFileBuffer);
    return d->fileName;
}


void StandardFileBuffer::setFileName(const QString& fileName)
{
    Q_D(StandardFileBuffer);
    d->fileName = fileName;
}

bool StandardFileBuffer::load()
{
    Q_D(StandardFileBuffer);

    Q_ASSERT(!d->fileName.isEmpty());

    d->dirty = false;

    QFile file(d->fileName);
    if (!file.open(QFile::ReadOnly))
    {
        qCCritical(CMAKE) << "Could not open" << d->fileName << "for reading";
        return false;
    }

    constexpr qint64 readChunkSize = 0x10000;
    qint64 readBytes = 0;
    qint64 readResult;
    do
    {
        if (readBytes + readChunkSize >= qint64{std::numeric_limits<int>::max()})
            break;
        d->fileContent.resize(static_cast<int>(readBytes + readChunkSize));
        readResult = file.read(d->fileContent.data() + readBytes, readChunkSize);
        if (readResult > 0 || readBytes == 0)
        {
            readBytes += readResult;
        }
    }
    while (readResult > 0);

    if (readResult < 0)
    {
        qCCritical(CMAKE) << "Error while reading file" << d->fileName;
        return false;
    }

    d->fileContent.resize(static_cast<int>(readBytes));

    return true;
}

bool StandardFileBuffer::save()
{
    Q_D(StandardFileBuffer);

    QFile file(d->fileName);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        qCCritical(CMAKE) << "Could not open" << d->fileName << "for writing";
        return false;
    }

    if (file.write(d->fileContent) != d->fileContent.size())
    {
        qCCritical(CMAKE) << "Could not write all data to file" << d->fileName;
        return false;
    }

    d->dirty = false;

    return true;
}

QByteArray StandardFileBuffer::content() const
{
    Q_D(const StandardFileBuffer);
    return d->fileContent;
}


void StandardFileBuffer::setContent(const QByteArray& content)
{
    Q_D(StandardFileBuffer);

    d->fileContent = content;
    d->dirty = true;
    emit changed();
}

} // namespace cmle
