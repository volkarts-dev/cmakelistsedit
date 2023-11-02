// Copyright 2021-2023, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: GPL-3.0-only

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
