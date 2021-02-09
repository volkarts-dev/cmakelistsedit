/* CMakeListsEdit
 *
 * Copyright 2021 Daniel Volk <mail@volkarts.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 */

#include "CMakeListsFile.h"

#include "FileBuffer.h"
#include "parser/CMakeListsParser.h"
#include <QFile>
#include <QLoggingCategory>
#include <algorithm>
#include <iostream>

namespace cmle {

namespace {

const QLoggingCategory CMAKE{"CMAKE"};

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
        bool lhsHasSlash = lhs.value().contains(QRegExp(QStringLiteral("/\\")));
        bool rhsHasSlash = rhs.value().contains(QRegExp(QStringLiteral("/\\")));
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

// *********************************************************************************************************************

CMakeListsFile::CMakeListsFile(FileBuffer* fileBuffer, QObject* parent) :
    QObject(parent),
    fileBuffer_(fileBuffer),
    loaded_{false},
    dirty_{false},
    insertBlockPolicy_(InsertBlockPolicy::First),
    defaultSectionType_(SectionType::Private),
    sortSectionPolicy_{SortSectionPolicy::NoSort}
{
    loaded_ = read();
}

CMakeListsFile::~CMakeListsFile()
{
}

CMakeListsFile::SectionType CMakeListsFile::sectionType(const parser::CMakeFunctionArgument& arg)
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

parser::CMakeFunctionArgument CMakeListsFile::sectionTypeArgument(CMakeListsFile::SectionType type)
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

void CMakeListsFile::setDirty()
{
    dirty_ = true;
    emit changed();
}

bool CMakeListsFile::reload()
{
    sourcesBlocks_.clear();
    return loaded_ = read();
}

bool CMakeListsFile::save()
{
    return write();
}

bool CMakeListsFile::addSourceFile(const QString& target, const QString& fileName)
{
    auto& sourcesBlock = ensureSourcesBlock(target);
    auto section = sourcesBlock.defaultInsertSection;

    QString separator;
    if (section->fileNames.isEmpty())
        separator = kDefaultSeparator;
    else
        separator = section->fileNames.last().separator();

    section->fileNames << parser::CMakeFunctionArgument{fileName, needsQuotation(fileName), separator};

    if (sortSectionPolicy_ == SortSectionPolicy::Sort)
        resortSection(*section);

    sourcesBlock.dirty = true;

    setDirty();

    return true;
}

bool CMakeListsFile::renameSourceFile(const QString& target, const QString& oldFileName, const QString& newFileName)
{
    if (!sourcesBlocksIndex_.contains(target))
    {
        // TODO handle missing source block for defined target (depends on 'read target blocks')
        qCWarning(CMAKE) << "Target" << target << "not found in CMakeLists file" << fileBuffer_->fileName();
        return false;
    }

    Section* foundSection = nullptr;

    for (const auto& idx : qAsConst(sourcesBlocksIndex_[target]))
    {
        auto& sourcesBlock = sourcesBlocks_[idx];

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
        if (sortSectionPolicy_ == SortSectionPolicy::Sort)
            resortSection(*foundSection);

        setDirty();
    }

    return foundSection;
}

bool CMakeListsFile::removeSourceFile(const QString& target, const QString& fileName)
{
    if (!sourcesBlocksIndex_.contains(target))
    {
        qCWarning(CMAKE) << "Target" << target << "not found in CMakeLists file" << fileBuffer_->fileName();
        return false;
    }

    Section* foundSection = nullptr;

    for (const auto& idx : qAsConst(sourcesBlocksIndex_[target]))
    {
        auto& sourcesBlock = sourcesBlocks_[idx];

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
        if (sortSectionPolicy_ == SortSectionPolicy::Sort)
            resortSection(*foundSection);

        setDirty();
    }

    return foundSection;
}

bool CMakeListsFile::read()
{
    bool error;
    auto contents = parser::readCMakeFile(fileBuffer_->content(), &error);
    if (error)
        return false;

    // TODO read target definition blocks

    return readInSourcesBlocks(contents);
}

bool CMakeListsFile::write()
{
    RawDataReader reader(fileBuffer_->content());
    QByteArray line;

    int lineOffset = 0;

    QByteArray output;

    for (auto& sourcesBlock : sourcesBlocks_)
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
        if (int lineRest = line.size() - static_cast<int>(func.endColumn()); lineRest > 0)
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
    fileBuffer_->setContent(output);

    dirty_ = false;

    return true;
}

void CMakeListsFile::resortSection(Section& section)
{
    std::sort(section.fileNames.begin(), section.fileNames.end(), FileNameCompare());
}

CMakeListsFile::SourcesBlock& CMakeListsFile::ensureSourcesBlock(const QString& target)
{
    auto pos = sourcesBlocksIndex_.find(target);
    if (pos == sourcesBlocksIndex_.end())
    {
        parser::CMakeFunctionArgument targetArg{target, false};
        parser::CMakeFunctionDesc func{QStringLiteral("target_sources")};
        func.addArguments({targetArg});
        sourcesBlocks_ << SourcesBlock{func, targetArg, {}, nullptr};
        pos = sourcesBlocksIndex_.insert(target, {sourcesBlocks_.size() - 1});
    }

    int blockIndex;
    switch (insertBlockPolicy_)
    {
    case InsertBlockPolicy::Last:
        blockIndex = pos->size() - 1;
        break;
    case InsertBlockPolicy::First:
    default:
        blockIndex = 0;
    }

    auto& sourcesBlock = sourcesBlocks_[(*pos)[blockIndex]];

    if (!sourcesBlock.defaultInsertSection)
    {
        sourcesBlock.sections.append({defaultSectionType_, sectionTypeArgument(defaultSectionType_), {}});
        sourcesBlock.defaultInsertSection = &sourcesBlock.sections.last();
    }

    return sourcesBlock;
}

bool CMakeListsFile::readInSourcesBlocks(const parser::CMakeFileContent& cmakeFileContent)
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
            if (it->type == defaultSectionType_)
            {
                defaultInsertSection = &*it;
                break;
            }
        }

        if (!defaultInsertSection && !sections.isEmpty())
            defaultInsertSection = &sections.last();

        sourcesBlocks_ << SourcesBlock{func, target, sections, defaultInsertSection};

        auto pos = sourcesBlocksIndex_.find(target.value());
        if (pos == sourcesBlocksIndex_.end())
            pos = sourcesBlocksIndex_.insert(target.value(), {});
        pos->append(sourcesBlocks_.size() - 1);
    }

    return true;
}

void CMakeListsFile::writeBackSourcesBlock(SourcesBlock& sourcesBlock)
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

} // namespace cmle
