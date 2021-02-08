#include <iostream>

#include "main/CMakeListsFile.h"
#include "DefaultFileBuffer.h"
#include <QCommandLineParser>

namespace {

struct Options
{
    QString command;
    QStringList fileNames;
    bool sort = false;
};

enum class CommandLineParseResult
{
    Ok,
    Error,
    VersionRequested,
    HelpRequested
};

CommandLineParseResult parseCommandLine(QCommandLineParser& parser, Options& options, QString& errorMessage)
{
    // config
    parser.addPositionalArgument(QStringLiteral("command"), QStringLiteral("subcommand"));

    parser.addPositionalArgument(QStringLiteral("files"), QStringLiteral("files"), QStringLiteral("files..."));

    QCommandLineOption sortOption{{QStringLiteral("sort")}, QStringLiteral("Sort section after adding/renaming file")};
    parser.addOption(sortOption);

    // parse
    if (!parser.parse(QCoreApplication::arguments()))
    {
        errorMessage = parser.errorText();
        return CommandLineParseResult::Error;
    }

    const QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty()) {
        errorMessage = QStringLiteral("No Command specified");
        return CommandLineParseResult::Error;
    }
    options.command = positionalArguments.first();

    if (options.command == QLatin1String("add") || options.command == QLatin1String("del"))
    {
        if (positionalArguments.size() <= 1)
        {
            errorMessage = QStringLiteral("No files specified");
        }
        for (int i = 1; i < positionalArguments.size(); ++i)
        {
            options.fileNames << positionalArguments[i];
        }
    }
    else if (options.command == QLatin1String("ren"))
    {
        if (positionalArguments.size() != 3)
        {
            errorMessage = QStringLiteral("Specify a source and a target file");
        }
        for (int i = 1; i < positionalArguments.size(); ++i)
        {
            options.fileNames << positionalArguments[i];
        }
    }
    else
    {
        errorMessage = QStringLiteral("Invalid command");
        return CommandLineParseResult::Error;
    }

    options.sort = parser.isSet(sortOption);

    return CommandLineParseResult::Ok;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));
    QCoreApplication::setApplicationName(QStringLiteral("CMakeFileEdit sandbox"));

    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    Options options;
    QString errorMessage;
    switch (parseCommandLine(parser, options, errorMessage))
    {
        case CommandLineParseResult::Ok:
            break;
        case CommandLineParseResult::Error:
            std::cerr << qPrintable(errorMessage) << "\n\n" << qPrintable(parser.helpText()) << std::endl;
            return 1;
        case CommandLineParseResult::VersionRequested:
            std::cout << qPrintable(QCoreApplication::applicationName()) << " " <<
                qPrintable(QCoreApplication::applicationVersion()) << std::endl;
            return 0;
        case CommandLineParseResult::HelpRequested:
            parser.showHelp();
            Q_UNREACHABLE();
    }

    DefaultFileBuffer fileBuffer(QStringLiteral("src/parser/CMakeLists.txt"));
    if (!fileBuffer.load())
    {
        std::cerr << "Could not open CMakeLists file" << std::endl;
        return 1;
    }

    cmle::CMakeListsFile cmakeListsFile(&fileBuffer);
    if (!cmakeListsFile.isLoaded())
    {
        std::cerr << "Could parse CMakeLists file" << std::endl;
        return 1;
    }

    cmakeListsFile.setSortSectionPolicy(options.sort ? cmle::CMakeListsFile::SortSectionPolicy::Sort :
                                                       cmle::CMakeListsFile::SortSectionPolicy::NoSort);

    if (options.command == QLatin1String("add"))
    {
        for (const auto& f : qAsConst(options.fileNames))
        {
            cmakeListsFile.addSourceFile(QStringLiteral("parser"), f);
        }
    }
    else if (options.command == QLatin1String("ren"))
    {
        cmakeListsFile.renameSourceFile(QStringLiteral("parser"), options.fileNames[0], options.fileNames[1]);
    }
    if (options.command == QLatin1String("del"))
    {
        for (const auto& f : qAsConst(options.fileNames))
        {
            cmakeListsFile.removeSourceFile(QStringLiteral("parser"), f);
        }
    }

    cmakeListsFile.save();

    std::cout.write(fileBuffer.content().constData(), fileBuffer.content().size());

    return 0;
}
