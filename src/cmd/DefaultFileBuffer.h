#pragma once

#include "main/FileBuffer.h"
#include <QString>
#include <QObject>

class DefaultFileBuffer : public QObject, public cmle::FileBuffer
{
    Q_OBJECT

public:
    explicit DefaultFileBuffer(const QString& fileName = {});
    ~DefaultFileBuffer() override;

    bool isDirty() const { return dirty_; }

    QString fileName() const override { return fileName_; }
    void setFileName(const QString& fileName) { fileName_ = fileName; }

    bool load();
    bool save();

    QByteArray content() const override { return fileContent_; }
    void setContent(const QByteArray& content) override;

signals:
    void changed();

private:
    QString fileName_;
    QByteArray fileContent_;
    bool dirty_;
};
