// KDevelop CMake Support
// Copyright 2006 Matt Rogers <mattr@kde.org>
// Copyright 2008 Aleix Pol <aleixpol@gmail.com>
// Copyright 2021-2023, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CMakeListsParser.h"

#include "cmake/cmListFileLexer.h"
#include <QDir>
#include <QLoggingCategory>
#include <QString>

namespace cmle { namespace parser {

namespace {

const QLoggingCategory CMAKE{"CMAKE"};

bool readCMakeFunction(cmListFileLexer* lexer, CMakeFunction& func)
{
    // Command name has already been parsed.
    cmListFileLexer_Token* token{};

    QString lastSeparator = QStringLiteral("");

    // eat spaces and left paren.
    while ((token = cmListFileLexer_Scan(lexer)))
    {
        if (token->type == cmListFileLexer_Token_Space)
        {
            lastSeparator += QString::fromLocal8Bit(token->text, token->length);
        }
        else if (token->type == cmListFileLexer_Token_ParenLeft)
        {
            break;
        }
        else
        {
            return false;
        }
    }

    func.setLeadingSpace(lastSeparator);

    // Arguments.
    int parenthesis = 1;
    bool wasSpace{};
    while ((token = cmListFileLexer_Scan(lexer)))
    {
        wasSpace = false;

        switch (token->type)
        {
            case cmListFileLexer_Token_ParenRight:
                parenthesis--;
                if (parenthesis == 0)
                {
                    func.setTrailingSpace(lastSeparator);
                    func.setEndLine(token->line);
                    func.setEndColumn(token->column);
                    return true;
                }
                else if (parenthesis < 0)
                {
                    return false;
                }
                else
                {
                    func.addArgument(CMakeFunctionArgument(QString::fromLocal8Bit(token->text, token->length),
                                                           false,
                                                           lastSeparator));
                }
                break;

            case cmListFileLexer_Token_ParenLeft:
                parenthesis++;
                func.addArgument(CMakeFunctionArgument(QString::fromLocal8Bit(token->text, token->length),
                                                       false,
                                                       lastSeparator));
                break;

            case cmListFileLexer_Token_Identifier:
            case cmListFileLexer_Token_ArgumentBracket:
            case cmListFileLexer_Token_ArgumentUnquoted:
                func.addArgument(CMakeFunctionArgument(QString::fromLocal8Bit(token->text, token->length),
                                                       false,
                                                       lastSeparator));
                break;

            case cmListFileLexer_Token_ArgumentQuoted:
                func.addArgument(CMakeFunctionArgument(QString::fromLocal8Bit(token->text, token->length),
                                                       true,
                                                       lastSeparator));
                break;

            case cmListFileLexer_Token_Space:
            case cmListFileLexer_Token_Newline:
                lastSeparator += QString::fromLocal8Bit(token->text, token->length);
                wasSpace = true;
                break;

            default:
                return false;
        }

        if (!wasSpace)
            lastSeparator = QStringLiteral("");
    }

    return false;
}

} // namespace

CMakeFileContent readCMakeFile(const QByteArray& fileContent, bool* error)
{
    *error = false;

    cmListFileLexer* lexer = cmListFileLexer_New();
    if (!lexer)
    {
        qCCritical(CMAKE) << "failed to create lexer";
        *error = true;
        return {};
    }

    if (!cmListFileLexer_SetString(lexer, fileContent.data(), static_cast<int>(fileContent.size())))
    {
        qCCritical(CMAKE) << "cmake read error.";
        cmListFileLexer_Delete(lexer);
        *error = true;
        return {};
    }

    CMakeFileContent ret;

    bool readError = false, haveNewline = true;
    cmListFileLexer_Token* token{};

    while (!readError && (token = cmListFileLexer_Scan(lexer)))
    {
        readError = false;

        if (token->type == cmListFileLexer_Token_Newline)
        {
            readError = false;
            haveNewline = true;
        }
        else if (token->type == cmListFileLexer_Token_Identifier && haveNewline)
        {
            haveNewline = false;
            CMakeFunction function;
            function.setName(QString::fromLocal8Bit(token->text, token->length).toLower());
            function.setStartLine(token->line);
            function.setStartColumn(token->column);

            readError = !readCMakeFunction(lexer, function);

            if (readError)
            {
                *error = true;
                qCCritical(CMAKE) << "Error while parsing:" << function.name() << "at line" <<
                    cmListFileLexer_GetCurrentLine(lexer);
                return {};
            }
            else
                ret.append(function);
        }
    }
    cmListFileLexer_Delete(lexer);

    return ret;
}

}} // namespace cmle::parser
