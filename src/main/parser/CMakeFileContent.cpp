// KDevelop CMake Support
// Copyright 2006 Matt Rogers <mattr@kde.org>
// Copyright 2008 Aleix Pol <aleixpol@gmail.com>
// Copyright 2021-2023, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CMakeFileContent.h"

#include <QMap>

namespace cmle::parser {

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

CMakeFunctionArgument::~CMakeFunctionArgument() = default;

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

CMakeFunctionArgument::CMakeFunctionArgument(CMakeFunctionArgument&& other) :
    d_(std::move(other.d_))
{
}

CMakeFunctionArgument& CMakeFunctionArgument::operator=(CMakeFunctionArgument&& other)
{
    d_ = std::move(other.d_);
    return *this;
}

QString CMakeFunctionArgument::unescapeValue(const QString& value)
{
    auto firstScape = value.indexOf(scapingChar);
    if (firstScape < 0)
    {
        return value;
    }

    QString newValue;
    qsizetype last = 0;
    QMap<QChar, QChar>::const_iterator itEnd = scapings.constEnd();
    for(qsizetype i = firstScape; i < value.size() - 1 && i >= 0; i = value.indexOf(scapingChar, i + 2))
    {
        newValue += value.mid(last, i - last);
        const QChar current = value[i + 1];
        QMap<QChar, QChar>::const_iterator it = scapings.constFind(current);

        if (it != itEnd)
            newValue += *it;
        else
            newValue += current;

        last = i + 2;
    }
    newValue += value.mid(last, value.size());
    return newValue;
}

// *********************************************************************************************************************

CMakeFunction::~CMakeFunction() = default;

CMakeFunction::CMakeFunction() :
    d_(new Data())
{
}

CMakeFunction::CMakeFunction(const QString& name) :
    CMakeFunction()
{
    d_->name = name;
}

CMakeFunction::CMakeFunction(const CMakeFunction& other) :
    d_(other.d_)
{
}

CMakeFunction& CMakeFunction::operator=(const CMakeFunction& other)
{
    d_ = other.d_;
    return *this;
}

CMakeFunction::CMakeFunction(CMakeFunction&& other) :
    d_(std::move(other.d_))
{
}

CMakeFunction& CMakeFunction::operator=(CMakeFunction&& other)
{
    d_ = std::move(other.d_);
    return *this;
}

void CMakeFunction::setArguments(const QList<CMakeFunctionArgument>& args)
{
    d_->arguments = args;
}

void CMakeFunction::addArguments(const QList<CMakeFunctionArgument>& args)
{
    d_->arguments.append(args);
}

void CMakeFunction::addArgument(const CMakeFunctionArgument& arg)
{
    d_->arguments << arg;
}

QString CMakeFunction::toString() const
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

} // namespace cmle::parser
