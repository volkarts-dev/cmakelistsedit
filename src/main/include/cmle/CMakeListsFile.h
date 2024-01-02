// Copyright 2021-2023, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QMimeType>
#include <QObject>
#include <QPoint>

namespace cmle {

enum class SortSectionPolicy
{
    NoSort,
    Sort
};

class FileBuffer;
class CMakeListsFilePrivate;

class CMakeListsFile : public QObject
{
    Q_OBJECT

public:
    struct ChangedBlock
    {
        QPoint origStart;
        QPoint origEnd;
        QString newContent;
    };

public:
    CMakeListsFile(const QByteArray& fileBuffer, QObject* parent = nullptr);
    ~CMakeListsFile() override;

    void setSortSectionPolicy(SortSectionPolicy sortSectionPolicy);

    bool isLoaded() const;
    bool hasChangedBlocks() const;

    bool addSourceFile(const QString& target, const QString& fileName, const QMimeType& mimeType = {});
    bool renameSourceFile(const QString& target, const QString& oldFileName, const QString& newFileName);
    bool removeSourceFile(const QString& target, const QString& fileName);

    QByteArray write();

private:
    QScopedPointer<CMakeListsFilePrivate> d_ptr;
    Q_DECLARE_PRIVATE(CMakeListsFile)

    friend class CMakeListsFileTest;

    Q_DISABLE_COPY_MOVE(CMakeListsFile)
};

} // namespace cmle
