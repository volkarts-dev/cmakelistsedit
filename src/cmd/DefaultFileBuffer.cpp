#include "DefaultFileBuffer.h"

#include <QFile>
#include <QLoggingCategory>

namespace {

const QLoggingCategory CMAKE{"CMAKE"};

} // namespace

DefaultFileBuffer::DefaultFileBuffer(const QString& fileName) :
    dirty_{false}
{
    setFileName(fileName);
}

DefaultFileBuffer::~DefaultFileBuffer()
{
}

bool DefaultFileBuffer::load()
{
    Q_ASSERT(!fileName_.isEmpty());

    dirty_ = false;

    QFile file(fileName_);
    if (!file.open(QFile::ReadOnly))
    {
        qCCritical(CMAKE) << "Could not open" << fileName_ << "for reading";
        return false;
    }

    constexpr qint64 readChunkSize = 0x10000;
    qint64 readBytes = 0;
    qint64 readResult;
    do
    {
        if (readBytes + readChunkSize >= std::numeric_limits<int>::max())
            break;
        fileContent_.resize(readBytes + readChunkSize);
        readResult = file.read(fileContent_.data() + readBytes, readChunkSize);
        if (readResult > 0 || readBytes == 0)
        {
            readBytes += readResult;
        }
    }
    while (readResult > 0);

    if (readResult < 0)
    {
        qCCritical(CMAKE) << "Error while reading file" << fileName_;
        return false;
    }

    fileContent_.resize(static_cast<int>(readBytes));

    return true;
}

bool DefaultFileBuffer::save()
{
    QFile file(fileName_);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        qCCritical(CMAKE) << "Could not open" << fileName_ << "for writing";
        return false;
    }

    if (file.write(fileContent_) != fileContent_.size())
    {
        qCCritical(CMAKE) << "Could not write all data to file" << fileName_;
        return false;
    }

    dirty_ = false;

    return true;
}

void DefaultFileBuffer::setContent(const QByteArray& content)
{
    fileContent_ = content;
    dirty_ = true;
    emit changed();
}
