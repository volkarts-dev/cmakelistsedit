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

#pragma once

#include <QMetaType>
#include <QSharedDataPointer>
#include <QVector>
#include <limits>

namespace cmle { namespace parser {

class CMakeFunctionArgument
{
public:
    CMakeFunctionArgument();
    explicit CMakeFunctionArgument(const QString& value);
    CMakeFunctionArgument(const QString& value, bool quoted, const QString& separator = {});

    CMakeFunctionArgument(const CMakeFunctionArgument& other);
    CMakeFunctionArgument& operator=(const CMakeFunctionArgument& other);

    inline bool operator==(const CMakeFunctionArgument& rhs) const
    {
        return (d_->value == rhs.d_->value) && (d_->quoted == rhs.d_->quoted);
    }

    inline bool operator!=(const CMakeFunctionArgument& rhs) const
    {
        return (d_->value != rhs.d_->value) || (d_->quoted != rhs.d_->quoted);
    }

    inline bool operator==(const QString& rhs) const
    {
        return d_->value == rhs;
    }

    QString value() const { return d_->value; }
    void setValue(const QString& value) { d_->value = value; }

    bool isQuoted() const { return d_->quoted; }
    void setQuoted(bool quoted) { d_->quoted = quoted; }

    QString separator() const { return d_->separator; }
    void setSeparator(const QString& separator) { d_->separator = separator; }

    operator bool() const { return !d_->value.isEmpty(); }

private:
    class Data : public QSharedData
    {
    public:
        QString value;
        bool quoted = false;
        QString separator;
    };

private:
    static QString unescapeValue(const QString& value);

private:
    static const QMap<QChar, QChar> scapings;

    QSharedDataPointer<Data> d_;
};

// *********************************************************************************************************************

class CMakeFunctionDesc
{
public:
    CMakeFunctionDesc();
    CMakeFunctionDesc(const QString& name);

    CMakeFunctionDesc(const CMakeFunctionDesc& other);
    CMakeFunctionDesc& operator=(const CMakeFunctionDesc& other);

    QString toString() const;

    QString name() const { return d_->name; }
    void setName(const QString& name) { d_->name = name; }

    const QList<CMakeFunctionArgument> arguments() const { return d_->arguments; }
    void setArguments(const QList<CMakeFunctionArgument>& args);
    void addArguments(const QList<CMakeFunctionArgument>& args);
    void addArgument(const CMakeFunctionArgument& arg);

    int startLine() const { return d_->startLine; }
    void setStartLine(int startLine) { d_->startLine = startLine; };

    int startColumn() const { return d_->startColumn; }
    void setStartColumn(int startColumn) { d_->startColumn = startColumn; }

    int endLine() const { return d_->endLine; }
    void setEndLine(int endLine) { d_->endLine = endLine; }

    int endColumn() const { return d_->endColumn; }
    void setEndColumn(int endColumn) { d_->endColumn = endColumn; }

    QString leadingSpace() const { return d_->leadingSpace; }
    void setLeadingSpace(const QString& leadingSpace) { d_->leadingSpace = leadingSpace; }

    QString trailingSpace() const { return d_->trailingSpace; }
    void setTrailingSpace(const QString& trailingSpace) { d_->trailingSpace = trailingSpace; }

private:
    class Data : public QSharedData
    {
    public:
        QString name;
        QList<CMakeFunctionArgument> arguments;
        int startLine = std::numeric_limits<int>::max();
        int startColumn = std::numeric_limits<int>::max();
        int endLine = std::numeric_limits<int>::max();
        int endColumn = std::numeric_limits<int>::max();
        QString leadingSpace;
        QString trailingSpace;
        void setValue(const QString& value);
    };

private:
    QSharedDataPointer<Data> d_;
};

using CMakeFileContent = QVector<CMakeFunctionDesc>;

}} // namespace cmle::parser

Q_DECLARE_METATYPE(cmle::parser::CMakeFunctionArgument)

Q_DECLARE_METATYPE(cmle::parser::CMakeFunctionDesc)
