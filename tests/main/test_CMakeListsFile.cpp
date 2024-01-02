// Copyright 2021-2023, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <cmle/CMakeListsFile.h>
#include <QtTest>
#include <iostream>

namespace {

QByteArray fileData(const QString& fileName)
{
    QFile f(fileName);
    Q_ASSERT(f.open(QFile::ReadOnly));
    return f.readAll();
}

QString resourceFile(const char* name)
{
    return QLatin1String(RESOURCE_DIR) + QLatin1Char('/') + QLatin1String(name);
}

} // namespace

#define FILE_BUFFER(fileName) \
    QFile listsFile{resourceFile(fileName)}; \
    QVERIFY(listsFile.open(QFile::ReadOnly)); \
    QByteArray fileBuffer{listsFile.readAll()}; \
    QVERIFY(!fileBuffer.isEmpty())

#define CMAKE_FILE(fileName) \
    FILE_BUFFER(fileName); \
    cmle::CMakeListsFile file{fileBuffer.data()}; \
    QVERIFY(file.isLoaded())

#define COMPARE_FILE(fileName) \
    QCOMPARE(fileBuffer, fileData(resourceFile(fileName)))

class CMakeListsFileTest : public QObject
{
    Q_OBJECT

public:
    CMakeListsFileTest()
    {
        cppSrcMimeType = QMimeDatabase().mimeTypeForName(QStringLiteral("text/x-c++src"));
    }

private:
    QMimeType cppSrcMimeType;

private slots:
    void openGood()
    {
        FILE_BUFFER("no_source_block.cmake");
        cmle::CMakeListsFile file{fileBuffer.data()};
        QVERIFY(file.isLoaded());
    }

    void openParseError()
    {
        FILE_BUFFER("invalid_listsfile.cmake");
        QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(QStringLiteral("^Error while parsing")));
        cmle::CMakeListsFile file{fileBuffer.data()};
        QVERIFY(!file.isLoaded());
    }

    void addToBestFitNoPrefix()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.addSourceFile(QStringLiteral("main"), QStringLiteral("Atest1.cpp"), cppSrcMimeType);
        COMPARE_FILE("two_source_blocks.cmake");
        fileBuffer = file.write();
        COMPARE_FILE("two_source_blocks-no_prefix.cmake");
    }

    void addToBestFitSamePrefix()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.addSourceFile(QStringLiteral("main"), QStringLiteral("abc/Atest1.cpp"), cppSrcMimeType);
        COMPARE_FILE("two_source_blocks.cmake");
        fileBuffer = file.write();
        COMPARE_FILE("two_source_blocks-same_prefix.cmake");
    }

    void addToBestFitDifferentPrefix()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.addSourceFile(QStringLiteral("main"), QStringLiteral("xyz/Atest1.cpp"), cppSrcMimeType);
        COMPARE_FILE("two_source_blocks.cmake");
        fileBuffer = file.write();
        COMPARE_FILE("two_source_blocks-different_prefix.cmake");
    }

    void addToBestFitPartialPrefix1()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.addSourceFile(QStringLiteral("main"), QStringLiteral("abc/xyz/Atest1.cpp"), cppSrcMimeType);
        COMPARE_FILE("two_source_blocks.cmake");
        fileBuffer = file.write();
        COMPARE_FILE("two_source_blocks-partial_prefix_1.cmake");
    }

    void addToBestFitPartialPrefix2()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.addSourceFile(QStringLiteral("main"), QStringLiteral("def/Atest1.cpp"), cppSrcMimeType);
        COMPARE_FILE("two_source_blocks.cmake");
        fileBuffer = file.write();
        COMPARE_FILE("two_source_blocks-partial_prefix_2.cmake");
    }

    void addToBestFitNoPrefixSorted()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.setSortSectionPolicy(cmle::SortSectionPolicy::Sort);
        file.addSourceFile(QStringLiteral("main"), QStringLiteral("Atest1.cpp"), cppSrcMimeType);
        fileBuffer = file.write();
        COMPARE_FILE("two_source_blocks-no_prefix_sorted.cmake");
    }

    void addToDefault()
    {
        CMAKE_FILE("no_source_block.cmake");
        file.addSourceFile(QStringLiteral("main"), QStringLiteral("Atest1.cpp"), cppSrcMimeType);
        fileBuffer = file.write();
        COMPARE_FILE("no_source_block-default.cmake");
    }

    void removeFromTop()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.removeSourceFile(QStringLiteral("main"), QStringLiteral("CMakeListsFile.cpp"));
        fileBuffer = file.write();
        COMPARE_FILE("two_source_blocks-remove_top.cmake");
    }

    void removeFromBottom()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.removeSourceFile(QStringLiteral("main"), QStringLiteral("abc/DefaultFileBuffer.cpp"));
        fileBuffer = file.write();
        COMPARE_FILE("two_source_blocks-remove_bottom.cmake");
    }

    void removeFromBottomSorted()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.setSortSectionPolicy(cmle::SortSectionPolicy::Sort);
        file.removeSourceFile(QStringLiteral("main"), QStringLiteral("abc/DefaultFileBuffer.cpp"));
        fileBuffer = file.write();
        COMPARE_FILE("two_source_blocks-remove_bottom_sorted.cmake");
    }

    void renameInTop()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.renameSourceFile(QStringLiteral("main"), QStringLiteral("CMakeListsFile.cpp"),
                              QStringLiteral("Atest1.cpp"));
        fileBuffer = file.write();
        COMPARE_FILE("two_source_blocks-rename_top.cmake");
    }

    void renameInBottom()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.renameSourceFile(QStringLiteral("main"), QStringLiteral("abc/DefaultFileBuffer.cpp"),
                              QStringLiteral("Atest1.cpp"));
        fileBuffer = file.write();
        COMPARE_FILE("two_source_blocks-rename_bottom.cmake");
    }

    void renameInBottomSorted()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.setSortSectionPolicy(cmle::SortSectionPolicy::Sort);
        file.renameSourceFile(QStringLiteral("main"), QStringLiteral("abc/DefaultFileBuffer.cpp"),
                              QStringLiteral("Atest1.cpp"));
        fileBuffer = file.write();
        COMPARE_FILE("two_source_blocks-rename_bottom_sorted.cmake");
    }

    void addToEmptySourceBlock()
    {
        CMAKE_FILE("empty_source_block.cmake");
        file.addSourceFile(QStringLiteral("main"), QStringLiteral("Atest1.cpp"), cppSrcMimeType);
        fileBuffer = file.write();
        COMPARE_FILE("empty_source_block-add.cmake");
    }

    void addInInvalid()
    {
        QSKIP("Source block validation not implemented yet in CMakeListsFile");
    }

    void addToPublicSection()
    {
        QSKIP("Insert into other section is not implemented yet in CMakeListsFile");
    }

    void addToInterfaceSection()
    {
        QSKIP("Insert into other section is not implemented yet in CMakeListsFile");
    }
};

#include "test_CMakeListsFile.moc"
QTEST_MAIN(CMakeListsFileTest)
