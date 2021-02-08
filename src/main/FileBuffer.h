#pragma once

#include <QByteArray>

namespace cmle {

class FileBuffer
{
public:
    virtual ~FileBuffer() {}

    virtual QString fileName() const = 0;

    virtual QByteArray content() const = 0;
    virtual void setContent(const QByteArray& content) = 0;
};

} // namespace cmle
