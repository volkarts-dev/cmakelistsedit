/* CMakeListsEdit
 *
 * Copyright 2021 Daniel Volk <mail@volkarts.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 */

#include "include/cmle/CMakeListsFile.h"

#include "parser/CMakeListsParser.h"
#include <cmle/FileBuffer.h>
#include <QFile>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <algorithm>
#include <iostream>

namespace cmle {

namespace {

const QLoggingCategory CMAKE{"com.va.cmakelistsedit"};

 // TODO make this configurable or copy from common separators
const QString kDefaultSeparator = QStringLiteral("\n    ");

class RawDataReader
{
public:
    RawDataReader(QByteArray data) :
        data_(std::move(data)),
        bytePos_(0),
        currentLine_(1)
    {
    }

    QByteArray readLine()
    {
        QByteArray output;
        output.reserve(1024);
        while (!eof())
        {
            char ch = data_.at(bytePos_++);
            output.append(ch);
            if (ch == '\n')
            {
                ++currentLine_;
                break;
            }
        }
        return output;
    }

    [[nodiscard]] inline bool eof() const { return bytePos_ >= data_.size(); }
    [[nodiscard]] inline int currentLine() const { return currentLine_; }

private:
    QByteArray data_;
    int bytePos_;
    int currentLine_;
};

struct FileNameCompare
{
    inline bool operator()(const parser::CMakeFunctionArgument& lhs, const parser::CMakeFunctionArgument& rhs)
    {
        bool lhsHasSlash = lhs.value().contains(QRegularExpression(QStringLiteral("[/\\\\]")));
        bool rhsHasSlash = rhs.value().contains(QRegularExpression(QStringLiteral("[/\\\\]")));
        if (lhsHasSlash && !rhsHasSlash)
            return true;
        else if (!lhsHasSlash && rhsHasSlash)
            return false;
        return QString::localeAwareCompare(lhs.value(), rhs.value()) < 0;
    }
};

bool needsQuotation(const QString& argument)
{
    return argument.contains(QLatin1Char(' ')); // TODO find more reasons for quoting
}

auto countRows(const QString& string)
{
    std::tuple<int, int> output{1, 0};

    for (auto i : string)
    {
        if (i == QLatin1Char('\n'))
        {
            std::get<0>(output) += 1;
            std::get<1>(output) = 0;
        }
        else
        {
            std::get<1>(output) += 1;
        }
    }

    return output;
}

} // namespace

// ********************************************************

class CMakeListsFilePrivate
{
public:
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
    CMakeListsFile* q_ptr;
    Q_DECLARE_PUBLIC(CMakeListsFile)

public:
    CMakeListsFilePrivate(CMakeListsFile* q, FileBuffer* _fileBuffer) :
        q_ptr{q},
        fileBuffer{_fileBuffer},
        loaded{false},
        dirty{false},
        insertBlockPolicy{InsertBlockPolicy::First},
        defaultSectionType{SectionType::Private},
        sortSectionPolicy{SortSectionPolicy::NoSort},
        blockCreationPolicy{BlockCreationPolicy::Create}
    {
        read();
    }

    static SectionType sectionType(const parser::CMakeFunctionArgument& arg)
    {
        if (arg.value().compare(QLatin1String("PRIVATE"), Qt::CaseInsensitive) == 0)
            return SectionType::Private;
        else if (arg.value().compare(QLatin1String("PUBLIC"), Qt::CaseInsensitive) == 0)
            return SectionType::Public;
        else if (arg.value().compare(QLatin1String("INTERFACE"), Qt::CaseInsensitive) == 0)
            return SectionType::Interface;
        else
            return SectionType::Invalid;
    }

    static parser::CMakeFunctionArgument sectionTypeArgument(SectionType type)
    {
        switch (type)
        {
        case SectionType::Private:
            return {QStringLiteral("PRIVATE"), false, kDefaultSeparator};
        case SectionType::Public:
            return {QStringLiteral("PUBLIC"), false, kDefaultSeparator};
        case SectionType::Interface:
            return {QStringLiteral("INTERFACE"), false, kDefaultSeparator};
        case SectionType::Invalid:
        default:
            return {};
        }
    }

    void setDirty()
    {
        Q_Q(CMakeListsFile);
        dirty = true;
        emit q->changed();
    }

    bool read()
    {
        loaded = [this]() {
            bool error{};
            auto contents = parser::readCMakeFile(fileBuffer->content(), &error);
            if (error)
                return false;

            // TODO read target definition blocks

            if (!readInSourcesBlocks(contents))
                return false;

            return true;
        }();

        return loaded;
    }

    bool write()
    {
        RawDataReader reader(fileBuffer->content());
        QByteArray line;

        int lineOffset = 0;

        QByteArray output;

        for (auto& sourcesBlock : sourcesBlocks)
        {
            // convert section to argument list
            writeBackSourcesBlock(sourcesBlock);

            auto& func = sourcesBlock.functionDesc;

            // advance to source block start line
            while (!reader.eof() && (reader.currentLine() < func.startLine()))
            {
                line = reader.readLine();
                output.append(line);
            }

            // advance to source block start column
            if (func.startColumn() > 1 && !reader.eof())
            {
                line = reader.readLine();
                output.append(line.data(), func.startColumn() - 1);
            }

            // output actual sources block
            auto funcOutput = sourcesBlock.functionDesc.toString();
            output.append(funcOutput.toLocal8Bit());

            // skip over original source block
            while (!reader.eof() && (reader.currentLine() <= func.endLine()))
            {
                line  = reader.readLine();
            }

            // output remainder of last original line
            if (int lineRest = static_cast<int>(line.size()) - func.endColumn(); lineRest > 0)
            {
                output.append(line.data() + line.size() - lineRest, lineRest);
            }

            // remember new block position
            auto [newRowCount, newEndColumn] = countRows(funcOutput);
            int oldRowCount = func.endLine() - func.startLine() + 1;
            func.setStartLine(func.startLine() + lineOffset);
            func.setEndLine(func.startLine() + newRowCount - 1);
            func.setEndColumn(newEndColumn);
            lineOffset += newRowCount - oldRowCount;
        }

        // output reamainder of file
        while (!reader.eof())
        {
            line = reader.readLine();
            output.append(line);
        }

        // write back to file
        fileBuffer->setContent(output);

        dirty = false;

        return true;
    }

    void resortSection(Section& section)
    {
        std::sort(section.fileNames.begin(), section.fileNames.end(), FileNameCompare());
    }

    SourcesBlock& ensureSourcesBlock(const QString& target)
    {
        auto pos = sourcesBlocksIndex.find(target);
        if (pos == sourcesBlocksIndex.end())
        {
            parser::CMakeFunctionArgument targetArg{target, false};
            parser::CMakeFunctionDesc func{QStringLiteral("target_sources")};
            func.addArguments({targetArg});
            sourcesBlocks << SourcesBlock{func, targetArg, {}, nullptr};
            pos = sourcesBlocksIndex.insert(target, {static_cast<int>(sourcesBlocks.size()) - 1});
        }

        int blockIndex{};
        switch (insertBlockPolicy)
        {
        case InsertBlockPolicy::Last:
            blockIndex = static_cast<int>(pos->size()) - 1;
            break;
        case InsertBlockPolicy::First:
        default:
            blockIndex = 0;
        }

        auto& sourcesBlock = sourcesBlocks[(*pos)[blockIndex]];

        if (!sourcesBlock.defaultInsertSection)
        {
            sourcesBlock.sections.append({defaultSectionType, sectionTypeArgument(defaultSectionType), {}});
            sourcesBlock.defaultInsertSection = &sourcesBlock.sections.last();
        }

        return sourcesBlock;
    }

    bool readInSourcesBlocks(const parser::CMakeFileContent& cmakeFileContent)
    {
        for (const auto& func : cmakeFileContent)
        {
            if (func.name().compare(QLatin1String("target_sources"), Qt::CaseInsensitive) != 0)
                continue;

            parser::CMakeFunctionArgument target;
            QList<Section> sections;
            for (const auto& arg : func.arguments())
            {
                SectionType type = sectionType(arg);

                if (!arg.isQuoted() && (type != SectionType::Invalid))
                {
                    sections << Section{type, arg, {}};
                }
                else if (!sections.isEmpty())
                {
                    sections.last().fileNames << arg;
                }
                else
                {
                    target = arg;
                }
            }
            if (target.value().isEmpty())
                continue;

            Section* defaultInsertSection = nullptr;
            for (auto it = sections.rbegin(); it != sections.rend(); ++it)
            {
                if (it->type == defaultSectionType)
                {
                    defaultInsertSection = &*it;
                    break;
                }
            }

            if (!defaultInsertSection && !sections.isEmpty())
                defaultInsertSection = &sections.last();

            sourcesBlocks << SourcesBlock{func, target, sections, defaultInsertSection};

            auto pos = sourcesBlocksIndex.find(target.value());
            if (pos == sourcesBlocksIndex.end())
                pos = sourcesBlocksIndex.insert(target.value(), {});
            pos->append(static_cast<int>(sourcesBlocks.size()) - 1);
        }

        return true;
    }

    void writeBackSourcesBlock(SourcesBlock& sourcesBlock)
    {
        if (!sourcesBlock.dirty)
            return;

        QList<parser::CMakeFunctionArgument> newArguments;
        newArguments << sourcesBlock.target;
        for (const auto& section : qAsConst(sourcesBlock.sections))
        {
            newArguments << section.typeArgument;

            for (const auto& fileName : section.fileNames)
            {
                newArguments << fileName;
            }
        }
        sourcesBlock.functionDesc.setArguments(newArguments);

        sourcesBlock.dirty = false;
    }

    FileBuffer* fileBuffer;
    bool loaded;
    bool dirty;
    QList<SourcesBlock> sourcesBlocks;
    QMap<QString, QList<int>> sourcesBlocksIndex;
    InsertBlockPolicy insertBlockPolicy;
    SectionType defaultSectionType;
    SortSectionPolicy sortSectionPolicy;
    BlockCreationPolicy blockCreationPolicy;
};

// ********************************************************

CMakeListsFile::CMakeListsFile(FileBuffer* fileBuffer, QObject* parent) :
    QObject{parent},
    d_ptr{new CMakeListsFilePrivate{this, fileBuffer}}
{
}

CMakeListsFile::~CMakeListsFile()
{
}

FileBuffer* CMakeListsFile::fileBuffer() const
{
    Q_D(const CMakeListsFile);
    return d->fileBuffer;
}

void CMakeListsFile::setInsertBlockPolicy(InsertBlockPolicy policy)
{
    Q_D(CMakeListsFile);
    d->insertBlockPolicy = policy;
}

void CMakeListsFile::setDefaultSectionType(SectionType type)
{
    Q_D(CMakeListsFile);
    Q_ASSERT(type != SectionType::Invalid);
    d->defaultSectionType = type;
}

void CMakeListsFile::setSortSectionPolicy(SortSectionPolicy sortSectionPolicy)
{
    Q_D(CMakeListsFile);
    d->sortSectionPolicy = sortSectionPolicy;
}

void CMakeListsFile::setBlockCreationPolicy(BlockCreationPolicy blockCreationPolicy)
{
    Q_D(CMakeListsFile);
    d->blockCreationPolicy = blockCreationPolicy;
}

bool CMakeListsFile::isLoaded() const
{
    Q_D(const CMakeListsFile);
    return d->loaded;
}

bool CMakeListsFile::isDirty() const
{
    Q_D(const CMakeListsFile);
    return d->dirty;
}

bool CMakeListsFile::reload()
{
    Q_D(CMakeListsFile);
    d->sourcesBlocks.clear();
    return d->read();
}

bool CMakeListsFile::save()
{
    Q_D(CMakeListsFile);
    return d->write();
}

bool CMakeListsFile::addSourceFile(const QString& target, const QString& fileName)
{
    Q_D(CMakeListsFile);

    if (!d->sourcesBlocksIndex.contains(target))
    {
        if (d->blockCreationPolicy == BlockCreationPolicy::NoCreate)
            return false;
    }

    auto& sourcesBlock = d->ensureSourcesBlock(target);
    auto section = sourcesBlock.defaultInsertSection;

    QString separator;
    if (section->fileNames.isEmpty())
        separator = kDefaultSeparator;
    else
        separator = section->fileNames.last().separator();

    section->fileNames << parser::CMakeFunctionArgument{fileName, needsQuotation(fileName), separator};

    if (d->sortSectionPolicy == SortSectionPolicy::Sort)
        d->resortSection(*section);

    sourcesBlock.dirty = true;

    d->setDirty();

    return true;
}

bool CMakeListsFile::renameSourceFile(const QString& target, const QString& oldFileName, const QString& newFileName)
{
    Q_D(CMakeListsFile);

    if (!d->sourcesBlocksIndex.contains(target))
    {
        // TODO handle missing source block for defined target (depends on 'read target blocks')
        qCWarning(CMAKE) << "Target" << target << "not found in CMakeLists file" << d->fileBuffer->fileName();
        return false;
    }

    CMakeListsFilePrivate::Section* foundSection = nullptr;

    for (const auto& idx : qAsConst(d->sourcesBlocksIndex[target]))
    {
        auto& sourcesBlock = d->sourcesBlocks[idx];

        for (auto& section : sourcesBlock.sections)
        {
            for (auto& fileName : section.fileNames)
            {
                if (fileName == oldFileName)
                {
                    fileName.setValue(newFileName);
                    sourcesBlock.dirty = true;
                    foundSection = &section;
                    break;
                }
            }

            if (foundSection) break;
        }

        if (foundSection) break;
    }

    if (foundSection)
    {
        if (d->sortSectionPolicy == SortSectionPolicy::Sort)
            d->resortSection(*foundSection);

        d->setDirty();
    }

    return foundSection;
}

bool CMakeListsFile::removeSourceFile(const QString& target, const QString& fileName)
{
    Q_D(CMakeListsFile);

    if (!d->sourcesBlocksIndex.contains(target))
    {
        qCWarning(CMAKE) << "Target" << target << "not found in CMakeLists file" << d->fileBuffer->fileName();
        return false;
    }

    CMakeListsFilePrivate::Section* foundSection = nullptr;

    for (const auto& idx : qAsConst(d->sourcesBlocksIndex[target]))
    {
        auto& sourcesBlock = d->sourcesBlocks[idx];

        for (auto& section : sourcesBlock.sections)
        {
            for (int i = 0; i < section.fileNames.size(); ++i)
            {
                if (section.fileNames[i] == fileName)
                {
                    section.fileNames.removeAt(i);
                    sourcesBlock.dirty = true;
                    foundSection = &section;
                    break;
                }
            }

            if (foundSection) break;
        }

        if (foundSection) break;
    }

    if (foundSection)
    {
        if (d->sortSectionPolicy == SortSectionPolicy::Sort)
            d->resortSection(*foundSection);

        d->setDirty();
    }

    return foundSection;
}

} // namespace cmle
