// Copyright 2021-2023, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "CMakeListsFile_p.h"

#include "parser/CMakeListsParser.h"
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <algorithm>
#include <iostream>

namespace cmle {

namespace {

const QLoggingCategory CMAKE{"com.va.cmakelistsedit"};

 // TODO make this configurable or copy from common separators
const QString kDefaultSeparator = QStringLiteral("\n    "); // clazy:exclude=non-pod-global-static

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
        static const QRegularExpression hasSlashExpr(QStringLiteral("[/\\\\]"));
        bool lhsHasSlash = lhs.value().contains(hasSlashExpr);
        bool rhsHasSlash = rhs.value().contains(hasSlashExpr);
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

QString extractPath(const QString& fileName)
{
    const QFileInfo fi{fileName};
    const auto path = fi.path();
    if (path == QLatin1String("."))
        return QLatin1String{""};
    return path;
}

} // namespace

// *********************************************************************************************************************

CMakeListsFilePrivate::Section::Section()
{
}

CMakeListsFilePrivate::Section::Section(parser::CMakeFunctionArgument nameArgument) :
    nameArgument_{std::move(nameArgument)},
    name_{nameArgument_.value()}
{
}

void CMakeListsFilePrivate::Section::finalize()
{
    for (const auto& fileName : fileNames_)
    {
        commonPrefixes_.insert(extractPath(fileName.value()));
    }
}

void CMakeListsFilePrivate::Section::addFileArgument(parser::CMakeFunctionArgument fileArgument)
{
    fileNames_ << std::move(fileArgument);
}

void CMakeListsFilePrivate::Section::addFileName(const QString& fileName)
{
    const QString separator = !fileNames_.isEmpty() ? fileNames_.last().separator() : kDefaultSeparator;

    fileNames_ << parser::CMakeFunctionArgument{fileName, needsQuotation(fileName), separator};
}

bool CMakeListsFilePrivate::Section::renameFile(const QString& oldFileName, const QString& newFileName)
{
    for (auto& fileName : fileNames_)
    {
        if (fileName == oldFileName)
        {
            fileName.setValue(newFileName);
            return true;
        }
    }
    return false;
}

bool CMakeListsFilePrivate::Section::removeFile(const QString& fileName)
{
    for (qsizetype i = 0; i < fileNames_.size(); ++i)
    {
        if (fileNames_[i] == fileName)
        {
            fileNames_.removeAt(i);
            return true;
        }
    }
    return false;
}

void CMakeListsFilePrivate::Section::sortFileNames()
{
    std::sort(fileNames_.begin(), fileNames_.end(), FileNameCompare());
}

// *********************************************************************************************************************

CMakeListsFilePrivate::SourcesFunction::SourcesFunction()
{
}

CMakeListsFilePrivate::SourcesFunction::SourcesFunction(parser::CMakeFunction cmakeFunction, QString target) :
    cmakeFunction_{std::move(cmakeFunction)},
    target_{std::move(target)}
{
}

void CMakeListsFilePrivate::SourcesFunction::finalize()
{
    for (auto it = sections_.begin(); it != sections_.end(); ++it)
    {
        it->finalize();
    }
}

void CMakeListsFilePrivate::SourcesFunction::setCmakeFunction(parser::CMakeFunction cmakeFunction)
{
    cmakeFunction_ = std::move(cmakeFunction);
}

void CMakeListsFilePrivate::SourcesFunction::setTarget(QString target)
{
    target_ = std::move(target);
}

void CMakeListsFilePrivate::SourcesFunction::addArgument(parser::CMakeFunctionArgument argument)
{
    arguments_ << Argument{-1, std::move(argument)};
}

CMakeListsFilePrivate::Section* CMakeListsFilePrivate::SourcesFunction::addSection()
{
    arguments_ << Argument{sections_.size(), {}};
    sections_ << Section{};
    return &sections_.last();
}

CMakeListsFilePrivate::Section* CMakeListsFilePrivate::SourcesFunction::addSection(parser::CMakeFunctionArgument argument)
{
    arguments_ << Argument{sections_.size(), {}};
    sections_ << Section{std::move(argument)};
    return &sections_.last();
}

void CMakeListsFilePrivate::SourcesFunction::setDefaultInsertSection(QString sectionName)
{
    defaultInsertSection_ = std::move(sectionName);
}

void CMakeListsFilePrivate::SourcesFunction::setPreferred(bool preferred)
{
    preferred_ = preferred;
}

// *********************************************************************************************************************

CMakeListsFilePrivate::CMakeListsFilePrivate(CMakeListsFile* q, const QByteArray& fileBuffer) :
    q_ptr{q},
    originalFileContent{fileBuffer},
    loaded{false},
    sortSectionPolicy{SortSectionPolicy::NoSort}
{
    read();
}

QString CMakeListsFilePrivate::sectionName(const parser::CMakeFunctionArgument& arg)
{
    if (arg.value().compare(QLatin1String("PRIVATE"), Qt::CaseInsensitive) == 0)
        return QLatin1String("PRIVATE");
    else if (arg.value().compare(QLatin1String("PUBLIC"), Qt::CaseInsensitive) == 0)
        return QLatin1String("PUBLIC");
    else if (arg.value().compare(QLatin1String("INTERFACE"), Qt::CaseInsensitive) == 0)
        return QLatin1String("INTERFACE");
    else
        return {};
}

parser::CMakeFunctionArgument CMakeListsFilePrivate::sectionTypeArgument(const QString& sectionName)
{
    if (sectionName.isEmpty())
        return {};
    return {sectionName, false, kDefaultSeparator};
}

bool CMakeListsFilePrivate::read()
{
    loaded = [this]() {
        bool error{};
        auto contents = parser::readCMakeFile(originalFileContent, &error);
        if (error)
            return false;

        // TODO read target definition blocks

        if (!readInFunctions(contents))
            return false;

        return true;
    }();

    return loaded;
}

QByteArray CMakeListsFilePrivate::write()
{
    RawDataReader reader(originalFileContent);
    QByteArray line;

    QByteArray output;

    for (auto& sourcesFunction : sourcesFunctions)
    {
        auto function = [&sourcesFunction]() {
            if (!sourcesFunction.isDirty())
                return sourcesFunction.cmakeFunction();

            QList<parser::CMakeFunctionArgument> newArguments;
            for (const auto& argument : qAsConst(sourcesFunction.arguments()))
            {
                if (argument.sectionIndex == -1)
                {
                    newArguments << argument.argument;
                }
                else
                {
                    const auto& section = sourcesFunction.sections()[argument.sectionIndex];

                    if (section.nameArgument())
                        newArguments << section.nameArgument();

                    for (const auto& fileName : section.fileNames())
                    {
                        newArguments << fileName;
                    }
                }
            }
            parser::CMakeFunction newFunction{sourcesFunction.cmakeFunction()};
            newFunction.setArguments(newArguments);

            return newFunction;
        }();

        // advance to source block start line
        while (!reader.eof() && (reader.currentLine() < function.startLine()))
        {
            line = reader.readLine();
            output.append(line);
        }

        // advance to source block start column
        if (function.startColumn() > 1 && !reader.eof())
        {
            line = reader.readLine();
            output.append(line.data(), function.startColumn() - 1);
        }

        // output actual sources block
        const auto funcOutput = function.toString();
        output.append(funcOutput.toLocal8Bit());

        // skip over original source block
        while (!reader.eof() && (reader.currentLine() <= function.endLine()))
        {
            line  = reader.readLine();
        }

        // output remainder of last original line
        if (int lineRest = static_cast<int>(line.size()) - function.endColumn(); lineRest > 0)
        {
            output.append(line.data() + line.size() - lineRest, lineRest);
        }
    }

    // output reamainder of file
    while (!reader.eof())
    {
        line = reader.readLine();
        output.append(line);
    }

    return output;
}

void CMakeListsFilePrivate::addFunctionIndex(const QString& target, qsizetype index)
{
    auto pos = sourcesFunctionsIndex.find(target);
    if (pos == sourcesFunctionsIndex.end())
        pos = sourcesFunctionsIndex.insert(target, {});
    pos->append(index);
}

bool CMakeListsFilePrivate::readInFunctions(const parser::CMakeFileContent& cmakeFileContent)
{
    for (const auto& func : cmakeFileContent)
    {
        auto function = readFunction(func);
        if (function.target().isEmpty())
            continue;

        sourcesFunctions << function;

        addFunctionIndex(function.target(), sourcesFunctions.size() - 1);
    }

    return true;
}

CMakeListsFilePrivate::SectionSearchResult CMakeListsFilePrivate::findBestInsertSection(
        const QString& target, const QString& fileName, const QMimeType& mimeType)
{
    // get sources functions for target
    const auto pos = sourcesFunctionsIndex.find(target);
    if (pos == sourcesFunctionsIndex.end())
    {
        qCWarning(CMAKE) << "Could not find sources function for target" << target;
        return {};
    }

    const auto parentPath = extractPath(fileName);

    auto selectBestSection = [this, &parentPath](qsizetype bestScore,
            SourcesFunction* function, const QString& sectionName)
    {
        Section* bestSection{};

        for (auto& section : function->sections())
        {
            if (!sectionName.isEmpty() && section.name().compare(sectionName, Qt::CaseInsensitive) != 0)
                continue;

            const auto score = commonPrefixScore(parentPath, section);

            if (score > bestScore)
            {
                bestSection = &section;
                bestScore = score;
            }
        }

        return std::pair{bestScore, bestSection};
    };

    SourcesFunction* selectedFunction{};
    Section* selectedSection{};

    // find priority function
    for (qsizetype idx : *pos)
    {
        if (sourcesFunctions[idx].isPreferred())
        {
            selectedFunction = &sourcesFunctions[idx];

            const QString sectionName = preferedSectionName(selectedFunction->cmakeFunction().name(), fileName, mimeType);

            auto [bestScore, bestSection] = selectBestSection(std::numeric_limits<qsizetype>::min(),
                                                              selectedFunction, sectionName);
            if (!bestSection)
            {
                const auto sn = !sectionName.isEmpty() ? sectionName : selectedFunction->defaultInsertSection();

                selectedSection = selectedFunction->addSection({sn, false, kDefaultSeparator});
            }
            else
            {
                selectedSection = bestSection;
            }

            break;
        }
    }

    // find best matching function/section
    if (!selectedSection)
    {
        SourcesFunction* bestFunction{};
        Section* bestSection{};
        qsizetype bestSectionScore = std::numeric_limits<qsizetype>::min();

        for (qsizetype idx : *pos)
        {
            auto function = &sourcesFunctions[idx];

            const QString sectionName = preferedSectionName(function->cmakeFunction().name(), fileName, mimeType);

            auto [score, section] = selectBestSection(bestSectionScore, function, sectionName);

            if (score > bestSectionScore)
            {
                bestFunction = function;
                bestSection = section;
                bestSectionScore = score;
            }
        }

        if (bestSection)
        {
            selectedFunction = bestFunction;
            selectedSection = bestSection;
        }
    }

    // get or create a fallback section
    if (!selectedSection)
    {
        selectedFunction = &sourcesFunctions[pos->first()];
        selectedSection = selectedFunction->addSection(
                    {selectedFunction->defaultInsertSection(), false, kDefaultSeparator});
    }

    return {selectedFunction, selectedSection};
}

CMakeListsFilePrivate::SourcesFunction CMakeListsFilePrivate::readTargetSourcesFunction(const parser::CMakeFunction& function)
{
    SourcesFunction info;

    const auto args = function.arguments();
    auto it = args.begin();
    if (it != args.cend())
    {
        info.setCmakeFunction(function);
        info.setTarget(it->value());
        info.addArgument(*it);

        Section* currentSection{};

        for (++it ; it != args.end(); ++it)
        {
            if (!it->isQuoted() && (compareStrings(it->value(), "INTERFACE", "PUBLIC", "PRIVATE")))
            {
                currentSection = info.addSection({*it});
            }
            else if (currentSection)
            {
                currentSection->addFileArgument(*it);
            }
        }
    }

    info.setDefaultInsertSection(QStringLiteral("PRIVATE"));
    info.finalize();

    return info;
}

CMakeListsFilePrivate::SourcesFunction CMakeListsFilePrivate::readAddTargetFunction(const parser::CMakeFunction& function)
{
    SourcesFunction info;

    const auto args = function.arguments();
    auto it = args.begin();
    if (it != args.cend())
    {
        info.setCmakeFunction(function);
        info.setTarget(it->value());
        info.addArgument(*it);

        Section* filesSection{};

        // MAYBE: Ignore IMPORTED and ALIAS definitions

        for (++it ; it != args.end(); ++it)
        {
            if (!filesSection)
            {
                if (!it->isQuoted() &&
                        compareStrings(it->value(),
                                       "WIN32", "MACOSX_BUNDLE", "EXCLUDE_FROM_ALL",
                                       "STATIC", "SHARED", "MODULE", "INTERFACE", "OBJECT",
                                       "MANUAL_FINALIZATION"))
                {
                    info.addArgument(*it);
                    continue;
                }

                if (!it->isQuoted() &&
                        compareStrings(it->value(),
                                       "CLASS_NAME", "OUTPUT_TARGETS"))
                {
                    info.addArgument(*it);
                    ++it;
                    info.addArgument(*it);
                    continue;
                }

                filesSection = info.addSection();
            }

            if (!it->value().isEmpty())
                filesSection->addFileArgument(*it);
        }
    }

    info.setDefaultInsertSection(QLatin1String(""));
    info.finalize();

    return info;
}

CMakeListsFilePrivate::SourcesFunction CMakeListsFilePrivate::readAddQmlTargetFunction(const parser::CMakeFunction& function)
{
    qCWarning(CMAKE) << "CMake function" << function.name() << "not implemented";

    SourcesFunction info;

    //info.setPreferred(true);
    //info.finalize();

    return info;
}

CMakeListsFilePrivate::SourcesFunction CMakeListsFilePrivate::readFunction(const parser::CMakeFunction& function)
{
    SourcesFunction info;

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
                            "qt6_add_library"))
    {
        info = readAddTargetFunction(function);
    }
    else if (compareStrings(function.name(),
                            "qt_add_qml_module",
                            "qt6_add_qml_module"))
    {
        info = readAddQmlTargetFunction(function);
    }

    return info;
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
    if (section.commonPrefixes().isEmpty())
        return -1;

    qsizetype bestScore = 0;
    for (const auto& path : section.commonPrefixes())
    {
        auto cpl = commonPrefixLength(prefix, path);

        if (cpl == prefix.length() && cpl == path.length()) // perfect match
            return std::numeric_limits<qsizetype>::max();

        if (cpl > bestScore)
            bestScore = cpl;
    }
    return bestScore;
}

QString CMakeListsFilePrivate::preferedSectionName(const QString& functionName, const QString& fileName,
                                                   const QMimeType& mimeType)
{
    auto mt = mimeType.isValid() ? mimeType : QMimeDatabase().mimeTypeForFile(fileName);
    Q_UNUSED(mt)

    if (compareStrings(functionName,
                       "qt_add_qml_module",
                       "qt6_add_qml_module"))
    {
        Q_UNIMPLEMENTED();
    }

    return {};
}

// *********************************************************************************************************************

CMakeListsFile::CMakeListsFile(const QByteArray& fileBuffer, QObject* parent) :
    QObject{parent},
    d_ptr{new CMakeListsFilePrivate{this, fileBuffer}}
{
}

CMakeListsFile::~CMakeListsFile()
{
}

void CMakeListsFile::setSortSectionPolicy(SortSectionPolicy sortSectionPolicy)
{
    Q_D(CMakeListsFile);
    d->sortSectionPolicy = sortSectionPolicy;
}

bool CMakeListsFile::isLoaded() const
{
    Q_D(const CMakeListsFile);
    return d->loaded;
}

bool CMakeListsFile::hasChangedBlocks() const
{
    Q_D(const CMakeListsFile);
    for (const auto& func : d->sourcesFunctions)
    {
        if (func.isDirty())
            return true;
    }
    return false;
}

bool CMakeListsFile::addSourceFile(const QString& target, const QString& fileName, const QMimeType& mimeType)
{
    Q_D(CMakeListsFile);

    auto [function, section] = d->findBestInsertSection(target, fileName, mimeType);
    if (!section)
    {
        qCWarning(CMAKE) << "Target" << target << "has no suitable source block";
        return false;
    }

    section->addFileName(fileName);

    if (d->sortSectionPolicy == SortSectionPolicy::Sort)
        section->sortFileNames();

    function->setDirty();

    return true;
}

bool CMakeListsFile::renameSourceFile(const QString& target, const QString& oldFileName, const QString& newFileName)
{
    Q_D(CMakeListsFile);

    if (!d->sourcesFunctionsIndex.contains(target))
    {
        qCWarning(CMAKE) << "Target" << target << "not found in CMakeLists file";
        return false;
    }

    CMakeListsFilePrivate::SourcesFunction* changedFunction{};
    CMakeListsFilePrivate::Section* changedSection{};

    for (const auto& idx : qAsConst(d->sourcesFunctionsIndex[target]))
    {
        auto& function = d->sourcesFunctions[idx];

        for (auto& section : function.sections())
        {
            if (section.renameFile(oldFileName, newFileName))
            {
                changedFunction = &function;
                changedSection = &section;
                break;
            }
        }
    }

    if (changedSection)
    {
        if (d->sortSectionPolicy == SortSectionPolicy::Sort)
            changedSection->sortFileNames();

        changedFunction->setDirty();
    }

    return changedSection;
}

bool CMakeListsFile::removeSourceFile(const QString& target, const QString& fileName)
{
    Q_D(CMakeListsFile);

    if (!d->sourcesFunctionsIndex.contains(target))
    {
        qCWarning(CMAKE) << "Target" << target << "not found in CMakeLists file";
        return false;
    }

    CMakeListsFilePrivate::SourcesFunction* changedFunction{};
    CMakeListsFilePrivate::Section* changedSection{};

    for (const auto& idx : qAsConst(d->sourcesFunctionsIndex[target]))
    {
        auto& function = d->sourcesFunctions[idx];

        for (auto& section : function.sections())
        {
            if (section.removeFile(fileName))
            {
                changedFunction = &function;
                changedSection = &section;
                break;
            }
        }
    }

    if (changedSection)
    {
        if (d->sortSectionPolicy == SortSectionPolicy::Sort)
            changedSection->sortFileNames();

        changedFunction->setDirty();
    }

    return changedSection;
}

QByteArray CMakeListsFile::write()
{
    Q_D(CMakeListsFile);
    return d->write();
}

} // namespace cmle
