#include "sharedmemorychannel.h"

#include <QBuffer>
#include <QDataStream>

#include <algorithm>
#include <cstring>

QString SharedMemoryChannel::defaultKey()
{
    return QStringLiteral("lab3_qt20_shared_memory");
}

SharedMemoryChannel::SharedMemoryChannel(const QString& key, const int segmentSize)
    : memory_(key)
    , segmentSize_(std::max(segmentSize, static_cast<int>(sizeof(Header) + 1)))
{
}

SharedMemoryChannel::LockGuard::LockGuard(QSharedMemory& memory)
    : memory_(memory)
    , locked_(memory_.lock())
{
}

SharedMemoryChannel::LockGuard::~LockGuard()
{
    if (locked_) {
        memory_.unlock();
    }
}

bool SharedMemoryChannel::LockGuard::locked() const
{
    return locked_;
}

bool SharedMemoryChannel::setError(QString* errorMessage, const QString& text) const
{
    if (errorMessage != nullptr) {
        *errorMessage = text;
    }

    return false;
}

bool SharedMemoryChannel::create(QString* errorMessage)
{
    if (memory_.isAttached()) {
        return true;
    }

    if (memory_.create(segmentSize_)) {
        LockGuard guard(memory_);

        if (!guard.locked()) {
            return setError(
                errorMessage,
                QStringLiteral("Не удалось заблокировать разделяемую память после создания: %1")
                    .arg(memory_.errorString())
            );
        }

        std::memset(memory_.data(), 0, static_cast<std::size_t>(memory_.size()));

        Header header;
        header.magic = kMagic;
        header.version = kProtocolVersion;
        header.payloadSize = 0;

        std::memcpy(memory_.data(), &header, sizeof(Header));
        return true;
    }

    if (memory_.error() == QSharedMemory::AlreadyExists && memory_.attach()) {
        return true;
    }

    return setError(
        errorMessage,
        QStringLiteral("Не удалось создать сегмент разделяемой памяти: %1")
            .arg(memory_.errorString())
    );
}

bool SharedMemoryChannel::attach(QString* errorMessage)
{
    if (memory_.isAttached()) {
        return true;
    }

    if (memory_.attach()) {
        return true;
    }

    return setError(
        errorMessage,
        QStringLiteral("Не удалось подключиться к сегменту разделяемой памяти: %1")
            .arg(memory_.errorString())
    );
}

bool SharedMemoryChannel::detach(QString* errorMessage)
{
    if (!memory_.isAttached()) {
        return true;
    }

    if (memory_.detach()) {
        return true;
    }

    return setError(
        errorMessage,
        QStringLiteral("Не удалось отключиться от сегмента разделяемой памяти: %1")
            .arg(memory_.errorString())
    );
}

bool SharedMemoryChannel::isAttached() const
{
    return memory_.isAttached();
}

QString SharedMemoryChannel::errorString() const
{
    return memory_.errorString();
}

int SharedMemoryChannel::segmentSize() const
{
    return segmentSize_;
}

QByteArray SharedMemoryChannel::serializeString(const QString& value, QString* errorMessage) const
{
    QByteArray payload;
    QBuffer buffer(&payload);

    if (!buffer.open(QIODevice::WriteOnly)) {
        setError(errorMessage, QStringLiteral("Не удалось открыть буфер для сериализации."));
        return {};
    }

    QDataStream stream(&buffer);
    stream.setVersion(kStreamVersion);
    stream << value;

    if (stream.status() != QDataStream::Ok) {
        setError(errorMessage, QStringLiteral("Ошибка сериализации строки в буфер."));
        return {};
    }

    return payload;
}

bool SharedMemoryChannel::deserializeString(const QByteArray& payload,
                                            QString* value,
                                            QString* errorMessage) const
{
    QByteArray localCopy = payload;
    QBuffer buffer(&localCopy);

    if (!buffer.open(QIODevice::ReadOnly)) {
        return setError(errorMessage, QStringLiteral("Не удалось открыть буфер для чтения."));
    }

    QDataStream stream(&buffer);
    stream.setVersion(kStreamVersion);

    QString result;
    stream >> result;

    if (stream.status() != QDataStream::Ok) {
        return setError(errorMessage, QStringLiteral("Ошибка чтения строки из буфера."));
    }

    if (value != nullptr) {
        *value = result;
    }

    return true;
}

bool SharedMemoryChannel::writeString(const QString& value, QString* errorMessage)
{
    if (!memory_.isAttached()) {
        return setError(errorMessage, QStringLiteral("Разделяемая память не подключена."));
    }

    const QByteArray payload = serializeString(value, errorMessage);

    if (!errorMessage || errorMessage->isEmpty()) {
        const int totalSize = static_cast<int>(sizeof(Header)) + payload.size();

        if (totalSize > memory_.size()) {
            return setError(
                errorMessage,
                QStringLiteral("Размер данных (%1 байт) превышает размер сегмента (%2 байт).")
                    .arg(totalSize)
                    .arg(memory_.size())
            );
        }

        LockGuard guard(memory_);

        if (!guard.locked()) {
            return setError(
                errorMessage,
                QStringLiteral("Не удалось заблокировать разделяемую память для записи: %1")
                    .arg(memory_.errorString())
            );
        }

        auto* destination = static_cast<char*>(memory_.data());
        std::memset(destination, 0, static_cast<std::size_t>(memory_.size()));

        Header header;
        header.magic = kMagic;
        header.version = kProtocolVersion;
        header.payloadSize = static_cast<quint32>(payload.size());

        std::memcpy(destination, &header, sizeof(Header));

        if (!payload.isEmpty()) {
            std::memcpy(destination + sizeof(Header),
                        payload.constData(),
                        static_cast<std::size_t>(payload.size()));
        }

        return true;
    }

    return false;
}

bool SharedMemoryChannel::readString(QString* value, QString* errorMessage)
{
    if (!memory_.isAttached()) {
        return setError(errorMessage, QStringLiteral("Разделяемая память не подключена."));
    }

    LockGuard guard(memory_);

    if (!guard.locked()) {
        return setError(
            errorMessage,
            QStringLiteral("Не удалось заблокировать разделяемую память для чтения: %1")
                .arg(memory_.errorString())
        );
    }

    const auto* source = static_cast<const char*>(memory_.constData());

    Header header;
    std::memcpy(&header, source, sizeof(Header));

    if (header.magic == 0 && header.payloadSize == 0) {
        if (value != nullptr) {
            value->clear();
        }
        return true;
    }

    if (header.magic != kMagic) {
        return setError(errorMessage, QStringLiteral("Формат данных в разделяемой памяти не распознан."));
    }

    if (header.version != kProtocolVersion) {
        return setError(errorMessage, QStringLiteral("Версия формата данных не поддерживается."));
    }

    const int maxPayloadSize = memory_.size() - static_cast<int>(sizeof(Header));

    if (static_cast<int>(header.payloadSize) < 0 || static_cast<int>(header.payloadSize) > maxPayloadSize) {
        return setError(errorMessage, QStringLiteral("Повреждён размер полезной нагрузки в разделяемой памяти."));
    }

    const QByteArray payload(source + sizeof(Header), static_cast<int>(header.payloadSize));
    return deserializeString(payload, value, errorMessage);
}