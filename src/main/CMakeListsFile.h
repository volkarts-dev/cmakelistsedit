#pragma once

#include "parser/CMakeFileContent.h"
#include <QMap>
#include <QObject>

namespace cmle {

class FileBuffer;

class CMakeListsFile : public QObject
{
    Q_OBJECT

public:
    enum class InsertBlockPolicy
    {
        First,
        Last,
    };

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

public:
    CMakeListsFile(FileBuffer* fileBuffer, QObject* parent = nullptr);
    ~CMakeListsFile() override;

    FileBuffer* fileBuffer() const { return fileBuffer_; }

    void setInsertBlockPolicy(InsertBlockPolicy policy) { insertBlockPolicy_ = policy; }
    void setDefaultSectionType(SectionType type)
    {
        Q_ASSERT(type != SectionType::Invalid);
        defaultSectionType_ = type;
    }
    void setSortSectionPolicy(SortSectionPolicy sortSectionPolicy) { sortSectionPolicy_ = sortSectionPolicy; }

    bool isLoaded() const { return loaded_; }
    bool isDirty() const { return dirty_; }

    bool reload();
    bool save();

    bool addSourceFile(const QString& target, const QString& fileName);
    bool renameSourceFile(const QString& target, const QString& oldFileName, const QString& newFileName);
    bool removeSourceFile(const QString& target, const QString& fileName);

signals:
    void changed();

private:
    struct Section
    {
        SectionType type;
        parser::CMakeFunctionArgument typeArgument;
        QList<parser::CMakeFunctionArgument> fileNames;
    };

    struct SourcesBlock
    {
        parser::CMakeFunctionDesc functionDesc;
        parser::CMakeFunctionArgument target;
        QList<Section> sections;
        Section* defaultInsertSection = nullptr;
        bool dirty = false;
    };

private:
    static SectionType sectionType(const parser::CMakeFunctionArgument& arg);
    static parser::CMakeFunctionArgument sectionTypeArgument(SectionType type);

    void setDirty();
    bool read();
    bool write();
    void resortSection(Section& section);
    SourcesBlock& ensureSourcesBlock(const QString& target);
    bool readInSourcesBlocks(const parser::CMakeFileContent& cmakeFileContent);
    void writeBackSourcesBlock(SourcesBlock& sourcesBlock);

private:
    FileBuffer* fileBuffer_;
    bool loaded_;
    bool dirty_;
    QList<SourcesBlock> sourcesBlocks_;
    QMap<QString, QList<int>> sourcesBlocksIndex_;
    InsertBlockPolicy insertBlockPolicy_;
    SectionType defaultSectionType_;
    SortSectionPolicy sortSectionPolicy_;

    friend class CMakeListsFileTest;
};

} // namespace cmle
