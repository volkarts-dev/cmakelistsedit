// Copyright 2021-2023, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <cmle/StandardFileBuffer.h>
#include <QtTest>

QString resourceFile(const char* name)
{
    return QLatin1String(RESOURCE_DIR) + QLatin1Char('/') + QLatin1String(name);
}

QByteArray checksum(const QByteArray& data)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(data);
    return hash.result();
}

QByteArray fileChecksum(const QString& fileName)
{
    QFile f(fileName);
    if (!f.open(QFile::ReadOnly))
        return {};

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&f))
        return {};
    return hash.result();
}

class StandardFileBufferTest : public QObject
{
    Q_OBJECT

private slots:
    void openGood()
    {
        auto filePath = resourceFile("empty_source_block.cmake");

        cmle::StandardFileBuffer fileBuffer{filePath};
        QVERIFY(fileBuffer.load());
        QVERIFY(!fileBuffer.isDirty());

        QCOMPARE(fileBuffer.content().size(), 309);

        QCOMPARE(checksum(fileBuffer.content()), fileChecksum(filePath));
    }

    void openNonExisting()
    {
        QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(QStringLiteral("^Could not open.*for reading$")));

        cmle::StandardFileBuffer fileBuffer{resourceFile("non_existing.cmake")};
        QVERIFY(!fileBuffer.load());
        QVERIFY(!fileBuffer.isDirty());
    }

    void setValue()
    {
        cmle::StandardFileBuffer fileBuffer{resourceFile("empty_source_block.cmake")};
        QByteArray data{"1234567890"};
        fileBuffer.setContent(data);
        QVERIFY(fileBuffer.isDirty());
        QCOMPARE(checksum(fileBuffer.content()), checksum(data));
    }

    void setFileName()
    {
        cmle::StandardFileBuffer fileBuffer{};
        fileBuffer.setFileName(resourceFile("empty_source_block.cmake"));
        QVERIFY(fileBuffer.load());
    }

    void saveData()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        auto filePath = dir.filePath(QStringLiteral("test.cmake"));

        QByteArray data{"1234567890"};

        cmle::StandardFileBuffer fileBuffer{filePath};
        fileBuffer.setContent(data);
        QVERIFY(fileBuffer.save());
        QVERIFY(!fileBuffer.isDirty());

        QCOMPARE(checksum(fileBuffer.content()), fileChecksum(filePath));
    }
};

#include "test_StandardFileBuffer.moc"
QTEST_MAIN(StandardFileBufferTest)
