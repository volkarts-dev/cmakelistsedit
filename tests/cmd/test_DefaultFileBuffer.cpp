/* CMakeListsEdit
 *
 * Copyright 2021 Daniel Volk <mail@volkarts.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 */

#include "cmd/DefaultFileBuffer.h"
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

class DefaultFileBufferTest : public QObject
{
    Q_OBJECT

private slots:
    void openGood()
    {
        auto filePath = resourceFile("empty_source_block.cmake");

        DefaultFileBuffer fileBuffer{filePath};
        QVERIFY(fileBuffer.load());
        QVERIFY(!fileBuffer.isDirty());

        QCOMPARE(fileBuffer.content().size(), 309);

        QCOMPARE(checksum(fileBuffer.content()), fileChecksum(filePath));
    }

    void openNonExisting()
    {
        QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(QStringLiteral("^Could not open.*for reading$")));

        DefaultFileBuffer fileBuffer{resourceFile("non_existing.cmake")};
        QVERIFY(!fileBuffer.load());
        QVERIFY(!fileBuffer.isDirty());
    }

    void setValue()
    {
        DefaultFileBuffer fileBuffer{resourceFile("empty_source_block.cmake")};
        QByteArray data{"1234567890"};
        fileBuffer.setContent(data);
        QVERIFY(fileBuffer.isDirty());
        QCOMPARE(checksum(fileBuffer.content()), checksum(data));
    }

    void setFileName()
    {
        DefaultFileBuffer fileBuffer{};
        fileBuffer.setFileName(resourceFile("empty_source_block.cmake"));
        QVERIFY(fileBuffer.load());
    }

    void saveData()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        auto filePath = dir.filePath(QStringLiteral("test.cmake"));

        QByteArray data{"1234567890"};

        DefaultFileBuffer fileBuffer{filePath};
        fileBuffer.setContent(data);
        QVERIFY(fileBuffer.save());
        QVERIFY(!fileBuffer.isDirty());

        QCOMPARE(checksum(fileBuffer.content()), fileChecksum(filePath));
    }
};

#include "test_DefaultFileBuffer.moc"
QTEST_MAIN(DefaultFileBufferTest)
