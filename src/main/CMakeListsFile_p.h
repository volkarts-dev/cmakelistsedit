// Copyright 2023, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once

#include "include/cmle/CMakeListsFile.h"
#include "parser/CMakeFileContent.h"
#include <QSet>

namespace cmle {

class CMakeListsFilePrivate
{
private:
    struct Section
    {
        SectionType type;
        parser::CMakeFunctionArgument typeArgument;
        QList<parser::CMakeFunctionArgument> fileNames;
        QSet<QString> commonPrefixes;

        Section(SectionType t, parser::CMakeFunctionArgument a);
    };

    struct SourcesBlock
    {
        parser::CMakeFunctionDesc functionDesc;
        parser::CMakeFunctionArgument target;
        QList<parser::CMakeFunctionArgument> modifiers;
        QList<Section> sections;
        Section* defaultInsertSection = nullptr;
        bool dirty = false;

        SourcesBlock();
        SourcesBlock(parser::CMakeFunctionDesc d, parser::CMakeFunctionArgument t);
    };

    using SectionSearchResult = std::tuple<SourcesBlock*, Section*>;

public:
    CMakeListsFilePrivate(CMakeListsFile* q, FileBuffer* _fileBuffer);

    static SectionType sectionType(const parser::CMakeFunctionArgument& arg);

    static parser::CMakeFunctionArgument sectionTypeArgument(SectionType type);

    void setDirty();

    bool read();

    bool write();

    void addSourcesBlockIndex(const QString& target, qsizetype index);

    void resortSection(Section& section);

    bool readInSourcesBlocks(const parser::CMakeFileContent& cmakeFileContent);

    void writeBackSourcesBlock(SourcesBlock& sourcesBlock);

    SourcesBlock& createSourcesBlock(const QString& target);

    SectionSearchResult findBestInsertSection(const QString& target, const QString& fileName);

private:
    SourcesBlock readAddTargetFunction(const parser::CMakeFunctionDesc& function);

    SourcesBlock readTargetSourcesFunction(const parser::CMakeFunctionDesc& function);

    SourcesBlock readFunction(const parser::CMakeFunctionDesc& function);

    void collectSourcesBlockInfo(SourcesBlock& sourcesBlock);

    QString extractPath(const QString& fileName);

    qsizetype commonPrefixLength(const QString& path1, const QString& path2);

    qsizetype commonPrefixScore(const QString& filePath, const Section& section);

private:
    CMakeListsFile* q_ptr;
    Q_DECLARE_PUBLIC(CMakeListsFile)

public:
    FileBuffer* fileBuffer;
    bool loaded;
    bool dirty;
    QList<SourcesBlock> sourcesBlocks;
    QMap<QString, QList<qsizetype>> sourcesBlocksIndex;
    SectionType defaultSectionType;
    SortSectionPolicy sortSectionPolicy;
    BlockCreationPolicy blockCreationPolicy;
};

} // namespace cmle
