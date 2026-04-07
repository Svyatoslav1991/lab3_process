#pragma once

#include <QSharedMemory>
#include <QString>
#include <QDataStream>

class SharedMemoryChannel final
{
public:
    static constexpr int kDefaultSegmentSize = 4096;

    static QString defaultKey();

    explicit SharedMemoryChannel(const QString& key = defaultKey(),
                                 int segmentSize = kDefaultSegmentSize);

    bool create(QString* errorMessage = nullptr);
    bool attach(QString* errorMessage = nullptr);
    bool detach(QString* errorMessage = nullptr);

    bool isAttached() const;
    QString errorString() const;
    int segmentSize() const;

    bool writeString(const QString& value, QString* errorMessage = nullptr);
    bool readString(QString* value, QString* errorMessage = nullptr);

private:
    struct Header
    {
        quint32 magic = 0;
        quint32 version = 0;
        quint32 payloadSize = 0;
    };

    class LockGuard final
    {
    public:
        explicit LockGuard(QSharedMemory& memory);
        ~LockGuard();

        bool locked() const;

    private:
        QSharedMemory& memory_;
        bool locked_ = false;
    };

private:
    static constexpr quint32 kMagic = 0x4C334D51; // "L3MQ"
    static constexpr quint32 kProtocolVersion = 1;
    static constexpr int kStreamVersion = QDataStream::Qt_5_12;

    QByteArray serializeString(const QString& value, QString* errorMessage) const;
    bool deserializeString(const QByteArray& payload, QString* value, QString* errorMessage) const;
    bool setError(QString* errorMessage, const QString& text) const;

private:
    QSharedMemory memory_;
    int segmentSize_ = kDefaultSegmentSize;
};
