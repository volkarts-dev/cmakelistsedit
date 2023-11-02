// Copyright 2021-2023, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QMap>
#include <QObject>

namespace cmle {

class FileBuffer;

enum class SortSectionPolicy
{
    NoSort,
    Sort,
};

enum class SectionType
{
    Invalid,
    Private,
    Public,
    Interface,
};

enum class BlockCreationPolicy
{
    Create,
    NoCreate,
};

class CMakeListsFilePrivate;

class CMakeListsFile : public QObject
{
    Q_OBJECT

public:
    explicit CMakeListsFile(FileBuffer* fileBuffer, QObject* parent = nullptr);
    ~CMakeListsFile() override;


    FileBuffer* fileBuffer() const;

    void setDefaultSectionType(SectionType type);
    void setSortSectionPolicy(SortSectionPolicy sortSectionPolicy);
    void setBlockCreationPolicy(BlockCreationPolicy blockCreationPolicy);

    bool isLoaded() const;
    bool isDirty() const;

    bool reload();
    bool save();

    bool addSourceFile(const QString& target, const QString& fileName);
    bool renameSourceFile(const QString& target, const QString& oldFileName, const QString& newFileName);
    bool removeSourceFile(const QString& target, const QString& fileName);

signals:
    void changed();

private:
    QScopedPointer<CMakeListsFilePrivate> d_ptr;
    Q_DECLARE_PRIVATE(CMakeListsFile)

    friend class CMakeListsFileTest;
};

} // namespace cmle
