/* CMakeListsEdit
 *
 * Copyright 2021 Daniel Volk <mail@volkarts.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 */

#pragma once

#include <QByteArray>

namespace cmle {

class FileBuffer
{
public:
    virtual ~FileBuffer() = default;

    virtual QString fileName() const = 0;

    virtual QByteArray content() const = 0;
    virtual void setContent(const QByteArray& content) = 0;
};

} // namespace cmle
