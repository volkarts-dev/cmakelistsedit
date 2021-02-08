/* KDevelop CMake Support
 *
 * Copyright 2006 Matt Rogers <mattr@kde.org>
 * Copyright 2008 Aleix Pol <aleixpol@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 */

#include "CMakeFileContent.h"

#include <QMap>

namespace cmle { namespace parser {

namespace {

QMap<QChar, QChar> whatToScape()
{
    //Only add those where we're not scaping the next character
    QMap<QChar, QChar> ret{
        {QLatin1Char('n'), QLatin1Char('\n')},
        {QLatin1Char('r'), QLatin1Char('\r')},
        {QLatin1Char('t'), QLatin1Char('\t')},
    };
    return ret;
}

constexpr QChar scapingChar = QLatin1Char('\\');

} // namespace

// *********************************************************************************************************************

const QMap<QChar, QChar> CMakeFunctionArgument::scapings = whatToScape();

CMakeFunctionArgument::CMakeFunctionArgument() :
    d_(new Data())
{
}

CMakeFunctionArgument::CMakeFunctionArgument(const QString& value) :
    d_(new Data())
{
    d_->value = value;
}

CMakeFunctionArgument::CMakeFunctionArgument(const QString& v, bool quoted, const QString& separator) :
    d_(new Data())
{
    d_->value = unescapeValue(v);
    d_->quoted = quoted;
    d_->separator = separator;
}

CMakeFunctionArgument::CMakeFunctionArgument(const CMakeFunctionArgument& other) :
    d_(other.d_)
{
}

CMakeFunctionArgument& CMakeFunctionArgument::operator=(const CMakeFunctionArgument& other)
{
    d_ = other.d_;
    return *this;
}

QString CMakeFunctionArgument::unescapeValue(const QString& value)
{
    int firstScape = value.indexOf(scapingChar);
    if (firstScape<0)
    {
        return value;
    }

    QString newValue;
    int last = 0;
    QMap<QChar, QChar>::const_iterator itEnd = scapings.constEnd();
    for(int i = firstScape; i < value.size() - 1 && i >= 0; i = value.indexOf(scapingChar, i + 2))
    {
        newValue += value.midRef(last, i - last);
        const QChar current = value[i + 1];
        QMap<QChar, QChar>::const_iterator it = scapings.constFind(current);

        if (it != itEnd)
            newValue += *it;
        else
            newValue += current;

        last = i + 2;
    }
    newValue += value.midRef(last, value.size());
    return newValue;
}

// *********************************************************************************************************************

CMakeFunctionDesc::CMakeFunctionDesc() :
    d_(new Data())
{
}

CMakeFunctionDesc::CMakeFunctionDesc(const QString& name) :
    CMakeFunctionDesc()
{
    d_->name = name;
}

CMakeFunctionDesc::CMakeFunctionDesc(const CMakeFunctionDesc& other) :
    d_(other.d_)
{
}

CMakeFunctionDesc& CMakeFunctionDesc::operator=(const CMakeFunctionDesc& other)
{
    d_ = other.d_;
    return *this;
}

void CMakeFunctionDesc::setArguments(const QList<CMakeFunctionArgument>& args)
{
    d_->arguments = args;
}

void CMakeFunctionDesc::addArguments(const QList<CMakeFunctionArgument>& args)
{
    d_->arguments.append(args);
}

void CMakeFunctionDesc::addArgument(const CMakeFunctionArgument& arg)
{
    d_->arguments << arg;
}

QString CMakeFunctionDesc::toString() const
{
    QStringList args;
    args.reserve(d_->arguments.size());
    for (const auto& arg : d_->arguments)
    {
        args.append(arg.separator());

        if (arg.isQuoted())
        {
            args.append(QLatin1Char('"') + arg.value() + QLatin1Char('"'));
        }
        else
        {
            args.append(arg.value());
        }
    }
    return d_->name + d_->leadingSpace + QLatin1String("(") + args.join(QLatin1String("")) + d_->trailingSpace +
            QLatin1String(")");
}

}} // namespace cmle::parser
