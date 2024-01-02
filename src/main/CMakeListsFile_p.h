// Copyright 2023, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "include/cmle/CMakeListsFile.h"
#include "parser/CMakeFileContent.h"
#include <QMap>
#include <QSet>

namespace cmle {

class CMakeListsFilePrivate
{
private:
    class Section
    {
    public:
        Section();
        Section(parser::CMakeFunctionArgument nameArgument);

        void finalize();

        const parser::CMakeFunctionArgument& nameArgument() const { return nameArgument_; }
        const QString& name() const { return name_; }
        const QList<parser::CMakeFunctionArgument>& fileNames() const { return fileNames_; }
        const QSet<QString>& commonPrefixes() const { return commonPrefixes_; }

        void addFileArgument(parser::CMakeFunctionArgument fileArgument);
        void addFileName(const QString& fileName);
        bool renameFile(const QString& oldFileName, const QString& newFileName);
        bool removeFile(const QString& fileName);

        void sortFileNames();

    private:
        parser::CMakeFunctionArgument nameArgument_{};
        QString name_{};
        QList<parser::CMakeFunctionArgument> fileNames_{};
        QSet<QString> commonPrefixes_{};
    };

    class SourcesFunction
    {
    public:
        struct Argument
        {
            qsizetype sectionIndex{-1};
            parser::CMakeFunctionArgument argument{};

            operator const parser::CMakeFunctionArgument&() const { return argument; }
        };

    public:
        SourcesFunction();
        SourcesFunction(parser::CMakeFunction cmakeFunction, QString target);

        void finalize();

        const parser::CMakeFunction& cmakeFunction() const { return cmakeFunction_; }
        const QString& target() const { return target_; }
        const QList<Argument>& arguments() const { return arguments_; }
        const QList<Section>& sections() const { return sections_; }
        const QString& defaultInsertSection() const { return defaultInsertSection_; }
        bool isPreferred() const { return preferred_; }

        QList<Section>& sections() { return sections_; }

        void setDirty(bool _dirty = true) { dirty_ = _dirty; }
        bool isDirty() const { return dirty_; }

        void setCmakeFunction(parser::CMakeFunction cmakeFunction);
        void setTarget(QString target);
        void addArgument(parser::CMakeFunctionArgument argument);
        Section* addSection();
        Section* addSection(parser::CMakeFunctionArgument argument);
        void setDefaultInsertSection(QString sectionName);
        void setPreferred(bool preferred);

    private:
        parser::CMakeFunction cmakeFunction_{};
        QString target_{};
        QList<Argument> arguments_{};
        QList<Section> sections_{};
        QString defaultInsertSection_{};
        bool preferred_{false};
        bool dirty_{false};
    };

    using SectionSearchResult = std::tuple<SourcesFunction*, Section*>;

public:
    CMakeListsFilePrivate(CMakeListsFile* q, const QByteArray& fileBuffer);

    static QString sectionName(const parser::CMakeFunctionArgument& arg);

    static parser::CMakeFunctionArgument sectionTypeArgument(const QString& sectionName);

    QByteArray write();

    void addFunctionIndex(const QString& target, qsizetype index);

    bool readInFunctions(const parser::CMakeFileContent& cmakeFileContent);

    SectionSearchResult findBestInsertSection(const QString& target, const QString& fileName, const QMimeType& mimeType);

private:
    bool read();

    SourcesFunction readTargetSourcesFunction(const parser::CMakeFunction& function);
    SourcesFunction readAddTargetFunction(const parser::CMakeFunction& function);
    SourcesFunction readAddQmlTargetFunction(const parser::CMakeFunction& function);

    SourcesFunction readFunction(const parser::CMakeFunction& function);

    qsizetype commonPrefixLength(const QString& path1, const QString& path2);

    qsizetype commonPrefixScore(const QString& filePath, const Section& section);

    QString preferedSectionName(const QString& functionName, const QString& fileName, const QMimeType& mimeType);

private:
    CMakeListsFile* q_ptr;
    Q_DECLARE_PUBLIC(CMakeListsFile)

public:
    QByteArray originalFileContent;
    bool loaded;
    QList<SourcesFunction> sourcesFunctions;
    QMap<QString, QList<qsizetype>> sourcesFunctionsIndex;
    SortSectionPolicy sortSectionPolicy;
};

} // namespace cmle
