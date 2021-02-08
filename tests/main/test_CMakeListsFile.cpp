#include "main/FileBuffer.h"
#include "main/CMakeListsFile.h"
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


class TestFileBuffer : public cmle::FileBuffer
{
public:
    explicit TestFileBuffer(const QString& fileName = {})
    {
        fileName_ = fileName;
    }

    ~TestFileBuffer() override
    {
    }

    QString fileName() const override
    {
        return {};
    }

    bool load()
    {
        Q_ASSERT(!fileName_.isEmpty());

        QFile file(fileName_);
        if (!file.open(QFile::ReadOnly))
        {
            qCritical() << "Could not open" << fileName_ << "for reading";
            return false;
        }

        fileContent_ = file.readAll();
        if (fileContent_.isEmpty())
        {
            qCritical() << "File" << fileName_ << "is empty or error while reading";
            return false;
        }

        return true;
    }

    QByteArray content() const override { return fileContent_; }

    void setContent(const QByteArray& content) override
    {
        fileContent_ = content;
    }

private:
    QString fileName_;
    QByteArray fileContent_;
};

} // namespace

#define FILE_BUFFER(fileName) \
    QScopedPointer<TestFileBuffer> fileBuffer{new TestFileBuffer{resourceFile(fileName)}}; \
    QVERIFY(fileBuffer->load())

#define CMAKE_FILE(fileName) \
    FILE_BUFFER(fileName); \
    cmle::CMakeListsFile file{fileBuffer.data()}; \
    QVERIFY(file.isLoaded())

#define COMPARE_FILE(fileName) \
    QCOMPARE(fileBuffer->content(), fileData(resourceFile(fileName)))

class CMakeListsFileTest : public QObject
{
    Q_OBJECT

private slots:
    void openGood()
    {
        FILE_BUFFER("no_source_block.cmake");
        cmle::CMakeListsFile file{fileBuffer.data()};
        QVERIFY(file.isLoaded());
        QVERIFY(!file.isDirty());
    }

    void openParseError()
    {
        FILE_BUFFER("invalid_listsfile.cmake");
        QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(QStringLiteral("^Error while parsing")));
        cmle::CMakeListsFile file{fileBuffer.data()};
        QVERIFY(!file.isLoaded());
        QVERIFY(!file.isDirty());
    }

    void addToTop()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.setInsertBlockPolicy(cmle::CMakeListsFile::InsertBlockPolicy::First);
        file.addSourceFile(QStringLiteral("main"), QStringLiteral("Atest1.cpp"));
        COMPARE_FILE("two_source_blocks.cmake");
        file.save();
        COMPARE_FILE("two_source_blocks-add_top.cmake");
    }

    void addToTopSorted()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.setInsertBlockPolicy(cmle::CMakeListsFile::InsertBlockPolicy::First);
        file.setSortSectionPolicy(cmle::CMakeListsFile::SortSectionPolicy::Sort);
        file.addSourceFile(QStringLiteral("main"), QStringLiteral("Atest1.cpp"));
        file.save();
        COMPARE_FILE("two_source_blocks-add_top_sorted.cmake");
    }

    void addToBottom()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.setInsertBlockPolicy(cmle::CMakeListsFile::InsertBlockPolicy::Last);
        file.addSourceFile(QStringLiteral("main"), QStringLiteral("Atest1.cpp"));
        file.save();
        COMPARE_FILE("two_source_blocks-add_bottom.cmake");
    }

    void removeFromTop()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.removeSourceFile(QStringLiteral("main"), QStringLiteral("CMakeListsFile.cpp"));
        file.save();
        COMPARE_FILE("two_source_blocks-remove_top.cmake");
    }

    void removeFromBottom()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.removeSourceFile(QStringLiteral("main"), QStringLiteral("DefaultFileBuffer.cpp"));
        file.save();
        COMPARE_FILE("two_source_blocks-remove_bottom.cmake");
    }

    void removeFromBottomSorted()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.setSortSectionPolicy(cmle::CMakeListsFile::SortSectionPolicy::Sort);
        file.removeSourceFile(QStringLiteral("main"), QStringLiteral("DefaultFileBuffer.cpp"));
        file.save();
        COMPARE_FILE("two_source_blocks-remove_bottom_sorted.cmake");
    }

    void renameInTop()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.renameSourceFile(QStringLiteral("main"), QStringLiteral("CMakeListsFile.cpp"),
                              QStringLiteral("Atest1.cpp"));
        file.save();
        COMPARE_FILE("two_source_blocks-rename_top.cmake");
    }

    void renameInBottom()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.renameSourceFile(QStringLiteral("main"), QStringLiteral("DefaultFileBuffer.cpp"),
                              QStringLiteral("Atest1.cpp"));
        file.save();
        COMPARE_FILE("two_source_blocks-rename_bottom.cmake");
    }

    void renameInBottomSorted()
    {
        CMAKE_FILE("two_source_blocks.cmake");
        file.setSortSectionPolicy(cmle::CMakeListsFile::SortSectionPolicy::Sort);
        file.renameSourceFile(QStringLiteral("main"), QStringLiteral("DefaultFileBuffer.cpp"),
                              QStringLiteral("Atest1.cpp"));
        file.save();
        COMPARE_FILE("two_source_blocks-rename_bottom_sorted.cmake");
    }

    void addToNoSourceBlock()
    {
        QSKIP("Creating source block is not implemented yet in CMakeListsFile");
    }

    void addToEmptySourceBlock()
    {
        QSKIP("Creating source block is not implemented yet in CMakeListsFile");
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
