/* CMakeListsEdit
 *
 * Copyright 2021 Daniel Volk <mail@volkarts.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 */

#include "CMakeListsFile_p.h"

#include "parser/CMakeListsParser.h"
#include <cmle/FileBuffer.h>
#include <QFile>
#include <QFileInfo>
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
        data_(std::move(data))
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

    bool eof() const { return bytePos_ >= data_.size(); }
    int currentLine() const { return currentLine_; }

private:
    QByteArray data_;
    int bytePos_{0};
    int currentLine_{1};
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

template<typename... T>
inline bool compareStrings(const QString& string, T... strings)
{
    return ((string.compare(QLatin1String(strings), Qt::CaseInsensitive) == 0) || ...);
}

} // namespace

// *********************************************************************************************************************

CMakeListsFilePrivate::Section::Section(SectionType t, parser::CMakeFunctionArgument a) :
    type{t},
    typeArgument{std::move(a)}
{
}

CMakeListsFilePrivate::SourcesBlock::SourcesBlock()
{
}

CMakeListsFilePrivate::SourcesBlock::SourcesBlock(parser::CMakeFunctionDesc d, parser::CMakeFunctionArgument t) :
    functionDesc{std::move(d)},
    target{std::move(t)}
{
}


// *********************************************************************************************************************

CMakeListsFilePrivate::CMakeListsFilePrivate(CMakeListsFile* q, FileBuffer* _fileBuffer) :
    q_ptr{q},
    fileBuffer{_fileBuffer},
    loaded{false},
    dirty{false},
    defaultSectionType{SectionType::Private},
    sortSectionPolicy{SortSectionPolicy::NoSort},
    blockCreationPolicy{BlockCreationPolicy::Create}
{
    read();
}

SectionType CMakeListsFilePrivate::sectionType(const parser::CMakeFunctionArgument& arg)
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

parser::CMakeFunctionArgument CMakeListsFilePrivate::sectionTypeArgument(SectionType type)
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

void CMakeListsFilePrivate::setDirty()
{
    Q_Q(CMakeListsFile);
    dirty = true;
    emit q->changed();
}

bool CMakeListsFilePrivate::read()
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

bool CMakeListsFilePrivate::write()
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

void CMakeListsFilePrivate::addSourcesBlockIndex(const QString& target, qsizetype index)
{
    auto pos = sourcesBlocksIndex.find(target);
    if (pos == sourcesBlocksIndex.end())
        pos = sourcesBlocksIndex.insert(target, {});
    pos->append(index);
}

void CMakeListsFilePrivate::resortSection(Section& section)
{
    std::sort(section.fileNames.begin(), section.fileNames.end(), FileNameCompare());
}

bool CMakeListsFilePrivate::readInSourcesBlocks(const parser::CMakeFileContent& cmakeFileContent)
{
    for (const auto& func : cmakeFileContent)
    {
        auto block = readFunction(func);
        if (block.target.value().isEmpty())
            continue;

        collectSourcesBlockInfo(block);

        sourcesBlocks << block;

        addSourcesBlockIndex(block.target.value(), sourcesBlocks.size() - 1);
    }

    return true;
}

void CMakeListsFilePrivate::writeBackSourcesBlock(SourcesBlock& sourcesBlock)
{
    if (!sourcesBlock.dirty)
        return;

    QList<parser::CMakeFunctionArgument> newArguments;
    newArguments << sourcesBlock.target;
    for (const auto& modifier : qAsConst(sourcesBlock.modifiers))
    {
        newArguments << modifier;
    }
    for (const auto& section : qAsConst(sourcesBlock.sections))
    {
        if (section.typeArgument)
            newArguments << section.typeArgument;

        for (const auto& fileName : section.fileNames)
        {
            newArguments << fileName;
        }
    }
    sourcesBlock.functionDesc.setArguments(newArguments);

    sourcesBlock.dirty = false;
}

CMakeListsFilePrivate::SourcesBlock& CMakeListsFilePrivate::createSourcesBlock(const QString& target)
{
    parser::CMakeFunctionArgument targetArg{target, false};
    parser::CMakeFunctionDesc func{QStringLiteral("target_sources")};
    func.addArguments({targetArg});
    sourcesBlocks << SourcesBlock{func, targetArg};
    qsizetype newSourcesBlockIndex = sourcesBlocks.size() - 1;

    addSourcesBlockIndex(target, newSourcesBlockIndex);

    auto& sourcesBlock = sourcesBlocks[newSourcesBlockIndex];
    sourcesBlock.sections.append({defaultSectionType, sectionTypeArgument(defaultSectionType)});
    sourcesBlock.defaultInsertSection = &sourcesBlock.sections.last();

    return sourcesBlock;
}

CMakeListsFilePrivate::SectionSearchResult CMakeListsFilePrivate::findBestInsertSection(
        const QString& target, const QString& fileName)
{
    const auto pos = sourcesBlocksIndex.find(target);
    if (pos == sourcesBlocksIndex.end())
    {
        if (blockCreationPolicy == BlockCreationPolicy::NoCreate)
            return {};

        auto& sourcesBlock = createSourcesBlock(target);
        return {&sourcesBlock, sourcesBlock.defaultInsertSection};
    }

    SourcesBlock* bestBlock{};
    Section* bestSection{};
    qsizetype bestSectionScore = std::numeric_limits<qsizetype>::min();

    const auto parentPath = extractPath(fileName);

    for (qsizetype idx : *pos)
    {
        auto& block = sourcesBlocks[idx];

        for (auto& section : block.sections)
        {
            auto score = commonPrefixScore(parentPath, section);
            if (score > bestSectionScore)
            {
                bestBlock = &block;
                bestSection = &section;
                bestSectionScore = score;
            }
        }
    }

    return {bestBlock, bestSection};
}

CMakeListsFilePrivate::SourcesBlock CMakeListsFilePrivate::readAddTargetFunction(const parser::CMakeFunctionDesc& function)
{
    SourcesBlock info;

    const auto args = function.arguments();
    auto it = args.begin();
    if (it != args.cend())
    {
        info.functionDesc = function;
        info.target = *it;
        info.sections << Section{SectionType::Invalid, {}};

        bool optionsDone{false};

        for (++it ; it != args.end(); ++it)
        {
            if (!optionsDone &&
                    compareStrings(it->value(),
                                   "WIN32", "MACOSX_BUNDLE", "EXCLUDE_FROM_ALL",
                                   "STATIC", "SHARED", "MODULE", "INTERFACE", "OBJECT",
                                   "MANUAL_FINALIZATION"))
            {
                info.modifiers << *it;
                continue;
            }

            if (!optionsDone &&
                    compareStrings(it->value(),
                                   "CLASS_NAME", "OUTPUT_TARGETS"))
            {
                info.modifiers << *it;
                ++it;
                info.modifiers << *it;
                continue;
            }

            optionsDone = true;

            const auto fileName = *it;
            if (!fileName.value().isEmpty())
                info.sections.last().fileNames << fileName;
        }
    }

    return info;
}

CMakeListsFilePrivate::SourcesBlock CMakeListsFilePrivate::readTargetSourcesFunction(const parser::CMakeFunctionDesc& function)
{
    SourcesBlock info;

    const auto args = function.arguments();
    auto it = args.begin();
    if (it != args.cend())
    {
        info.functionDesc = function;
        info.target = *it;

        for (++it ; it != args.end(); ++it)
        {
            SectionType type = sectionType(*it);

            if (!it->isQuoted() && (type != SectionType::Invalid))
            {
                info.sections << Section{type, *it};
            }
            else if (!info.sections.isEmpty())
            {
                info.sections.last().fileNames << *it;
            }
        }
    }

    return info;
}

CMakeListsFilePrivate::SourcesBlock CMakeListsFilePrivate::readFunction(const parser::CMakeFunctionDesc& function)
{
    SourcesBlock info;

    if (compareStrings(function.name(), "target_sources"))
    {
        info = readTargetSourcesFunction(function);
    }
    else if (compareStrings(function.name(),
                            "add_executable",
                            "add_library",
                            "qt_add_executable",
                            "qt_add_library",
                            "qt6_add_executable",
                            "qt6_add_library",
                            "qt_add_plugin",
                            "qt6_add_plugin",
                            "qt_add_qml_module",
                            "qt6_add_qml_module"))
    {
        info = readAddTargetFunction(function);
    }

    return info;
}

void CMakeListsFilePrivate::collectSourcesBlockInfo(SourcesBlock& sourcesBlock)
{
    Section* defaultInsertSection = nullptr;
    for (auto it = sourcesBlock.sections.rbegin(); it != sourcesBlock.sections.rend(); ++it)
    {
        if (it->type == defaultSectionType)
        {
            defaultInsertSection = &*it;
        }

        for (const auto& fileName : it->fileNames)
        {
            it->commonPrefixes.insert(extractPath(fileName.value()));
        }
    }
    if (!defaultInsertSection && !sourcesBlock.sections.isEmpty())
        defaultInsertSection = &sourcesBlock.sections.last();
    sourcesBlock.defaultInsertSection = defaultInsertSection;
}

QString CMakeListsFilePrivate::extractPath(const QString& fileName)
{
    const QFileInfo fi{fileName};
    const auto path = fi.path();
    if (path == QLatin1String("."))
        return QLatin1String{""};
    return path;
}

qsizetype CMakeListsFilePrivate::commonPrefixLength(const QString& path1, const QString& path2)
{
    qsizetype i = 0;
    for ( ; i < qMin(path1.length(), path2.length()); ++i)
    {
        if (path1.at(i) != path2.at(i))
            break;
    }
    return i;
}

qsizetype CMakeListsFilePrivate::commonPrefixScore(const QString& prefix, const Section& section)
{
    if (section.commonPrefixes.isEmpty())
        return -1;

    qsizetype bestScore = 0;
    for (const auto& path : section.commonPrefixes)
    {
        auto cpl = commonPrefixLength(prefix, path);

        if (cpl == prefix.length() && cpl == path.length()) // perfect match
            return std::numeric_limits<qsizetype>::max();

        if (cpl > bestScore)
            bestScore = cpl;
    }
    return bestScore;
}

// *********************************************************************************************************************

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

    auto [sourcesBlock, section] = d->findBestInsertSection(target, fileName);
    if (!sourcesBlock)
    {
        qCWarning(CMAKE) << "Target" << target << "has no suitable source block";
        return false;
    }

    QString separator;
    if (section->fileNames.isEmpty())
        separator = kDefaultSeparator;
    else
        separator = section->fileNames.last().separator();

    section->fileNames << parser::CMakeFunctionArgument{fileName, needsQuotation(fileName), separator};

    if (d->sortSectionPolicy == SortSectionPolicy::Sort)
        d->resortSection(*section);

    sourcesBlock->dirty = true;

    d->setDirty();

    return true;
}

bool CMakeListsFile::renameSourceFile(const QString& target, const QString& oldFileName, const QString& newFileName)
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
