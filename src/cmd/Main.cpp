/* CMakeListsEdit
 *
 * Copyright 2021 Daniel Volk <mail@volkarts.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 */

#include <iostream>

#include "main/CMakeListsFile.h"
#include "main/StandardFileBuffer.h"
#include <QCommandLineParser>

namespace {

struct Options
{
    QString command;
    QString target;
    QString cmlFile;
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
    parser.addOptions({
                {QStringLiteral("add"), QStringLiteral("Add a file name to cmake target (command).")},
                {QStringLiteral("del"), QStringLiteral("Delete a file name from cmake target (command).")},
                {QStringLiteral("ren"), QStringLiteral("Rename a file name from cmake target (command).")},

                {{QStringLiteral("t"), QStringLiteral("target")},
                 QStringLiteral("The cmake target name for the file operations (required)."),
                 QStringLiteral("target")},

                {{QStringLiteral("f"), QStringLiteral("file")},
                 QStringLiteral("Path to the CMakeLists.txt file (required)."),
                 QStringLiteral("file")},

                {{QStringLiteral("s"), QStringLiteral("sort")},
                 QStringLiteral("Sort section after adding/removing/renaming file.")},
                });

    parser.addPositionalArgument(QStringLiteral("file-names"), QStringLiteral("File names to add/remove/rename"),
                                 QStringLiteral("<files-name>..."));

    // parse
    if (!parser.parse(QCoreApplication::arguments()))
    {
        errorMessage = parser.errorText();
        return CommandLineParseResult::Error;
    }

    for (const auto& cmd : QStringList{QStringLiteral("add"), QStringLiteral("del"), QStringLiteral("ren")})
    {
        if (parser.isSet(cmd))
        {
            if (!options.command.isEmpty())
            {
                errorMessage = QStringLiteral("Only one command can be specified");
                return CommandLineParseResult::Error;
            }

            options.command = cmd;
        }
    }

    if (options.command.isEmpty())
    {
        errorMessage = QStringLiteral("No command specified");
        return CommandLineParseResult::Error;
    }

    options.target = parser.value(QStringLiteral("target"));
    if (options.target.isEmpty())
    {
        errorMessage = QStringLiteral("No target specified");
        return CommandLineParseResult::Error;
    }

    options.cmlFile = parser.value(QStringLiteral("file"));
    if (options.cmlFile.isEmpty())
    {
        errorMessage = QStringLiteral("No CMakeLists.txt file specified");
        return CommandLineParseResult::Error;
    }

    const QStringList positionalArguments = parser.positionalArguments();

    if (options.command == QLatin1String("add") || options.command == QLatin1String("del"))
    {
        if (positionalArguments.size() < 1)
        {
            errorMessage = QStringLiteral("No file names specified");
            return CommandLineParseResult::Error;
        }

        options.fileNames = positionalArguments;
    }
    else if (options.command == QLatin1String("ren"))
    {
        if (positionalArguments.size() != 2)
        {
            errorMessage = QStringLiteral("Specify a source and a target file name");
            return CommandLineParseResult::Error;
        }

        options.fileNames = positionalArguments;
    }
    else
    {
        errorMessage = QStringLiteral("Invalid command");
        return CommandLineParseResult::Error;
    }

    options.sort = parser.isSet(QStringLiteral("sort"));

    return CommandLineParseResult::Ok;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));
    QCoreApplication::setApplicationName(QStringLiteral("CMakeFileEdit sample command"));

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

    cmle::StandardFileBuffer fileBuffer(options.cmlFile);
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
            cmakeListsFile.addSourceFile(options.target, f);
        }
    }
    else if (options.command == QLatin1String("ren"))
    {
        cmakeListsFile.renameSourceFile(options.target, options.fileNames[0], options.fileNames[1]);
    }
    if (options.command == QLatin1String("del"))
    {
        for (const auto& f : qAsConst(options.fileNames))
        {
            cmakeListsFile.removeSourceFile(options.target, f);
        }
    }

    cmakeListsFile.save();

    std::cout.write(fileBuffer.content().constData(), fileBuffer.content().size());

    return 0;
}
