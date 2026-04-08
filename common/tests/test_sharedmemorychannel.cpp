/**
 * @file test_sharedmemorychannel.cpp
 * @brief Юнит-тесты для класса SharedMemoryChannel.
 *
 * @details
 * Тесты покрывают:
 * - базовые свойства нового объекта;
 * - создание и подключение к сегменту разделяемой памяти;
 * - безопасное повторное создание и повторное отключение;
 * - запись и чтение строк через один и через два экземпляра канала;
 * - работу с пустой строкой, кириллицей и переводами строк;
 * - ошибки чтения/записи без подключения к памяти;
 * - ошибку подключения к несуществующему сегменту;
 * - ошибку переполнения сегмента;
 * - защиту внутреннего протокола через проверки magic/version/payloadSize.
 *
 * Для проверки внутреннего протокола хранения используется тестовый seam:
 * в данном файле `private` временно раскрывается в `public`, чтобы получить
 * доступ к служебному заголовку Header и внутреннему объекту QSharedMemory.
 * Это оправдано для legacy/low-level тестов, где требуется проверка именно
 * формата данных в памяти, а не только публичного API.
 */

#define private public
#include "sharedmemorychannel.h"
#undef private

#include <QtTest/QtTest>

#include <QUuid>
#include <cstring>

/**
 * @brief Генерирует уникальный ключ сегмента разделяемой памяти для теста.
 * @return Уникальный строковый ключ.
 *
 * @details
 * Уникальные ключи позволяют тестам не конфликтовать друг с другом и не
 * зависеть от сегментов памяти, оставшихся после предыдущих запусков.
 */
static QString makeUniqueKey()
{
    return QStringLiteral("test_sharedmemorychannel_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

/**
 * @class TestSharedMemoryChannel
 * @brief Набор unit-тестов для SharedMemoryChannel.
 */
class TestSharedMemoryChannel final : public QObject
{
    Q_OBJECT

private slots:
    /**
     * @brief Проверяет базовые свойства нового объекта.
     *
     * @details
     * Проверяется:
     * - что defaultKey() не пустой;
     * - что объект после создания ещё не подключён к сегменту;
     * - что слишком маленький размер сегмента автоматически нормализуется
     *   до минимально допустимого.
     */
    void constructor_setsExpectedDefaults();

    /**
     * @brief Проверяет, что create() создаёт сегмент и инициализирует заголовок.
     *
     * @details
     * После создания вручную считывается Header из разделяемой памяти
     * и проверяются magic, version и payloadSize.
     */
    void create_initializesProtocolHeader();

    /**
     * @brief Проверяет идемпотентность create() для одного и того же объекта.
     */
    void create_calledTwice_keepsValidState();

    /**
     * @brief Проверяет, что второй объект может подключиться к уже созданному сегменту.
     */
    void attach_toExistingSegment_succeeds();

    /**
     * @brief Проверяет ошибку подключения к несуществующему сегменту.
     */
    void attach_toMissingSegment_fails();

    /**
     * @brief Проверяет запись и чтение строки одним и тем же объектом.
     */
    void writeAndRead_sameInstance_roundtripWorks();

    /**
     * @brief Проверяет запись одним объектом и чтение другим объектом.
     */
    void writeAndRead_twoInstances_roundtripWorks();

    /**
     * @brief Проверяет корректную запись и чтение пустой строки.
     */
    void writeAndRead_emptyString_roundtripWorks();

    /**
     * @brief Проверяет корректную запись и чтение Unicode-строки.
     *
     * @details
     * Тест важен, потому что класс использует сериализацию QString
     * через QDataStream и должен корректно сохранять кириллицу и переводы строк.
     */
    void writeAndRead_unicodeString_roundtripWorks();

    /**
     * @brief Проверяет, что writeString() без подключения к памяти возвращает ошибку.
     */
    void writeString_withoutAttach_fails();

    /**
     * @brief Проверяет, что readString() без подключения к памяти возвращает ошибку.
     */
    void readString_withoutAttach_fails();

    /**
     * @brief Проверяет, что слишком большая строка не помещается в сегмент.
     */
    void writeString_tooLargePayload_fails();

    /**
     * @brief Проверяет, что detach() безопасно вызывать повторно.
     */
    void detach_calledTwice_isSafe();

    /**
     * @brief Проверяет отказ чтения при повреждённой сигнатуре формата.
     */
    void readString_withInvalidMagic_fails();

    /**
     * @brief Проверяет отказ чтения при неподдерживаемой версии протокола.
     */
    void readString_withInvalidVersion_fails();

    /**
     * @brief Проверяет отказ чтения при повреждённом размере полезной нагрузки.
     */
    void readString_withCorruptedPayloadSize_fails();
};

void TestSharedMemoryChannel::constructor_setsExpectedDefaults()
{
    QVERIFY(!SharedMemoryChannel::defaultKey().isEmpty());

    SharedMemoryChannel channel(makeUniqueKey(), 1);

    QCOMPARE(channel.isAttached(), false);
    QVERIFY(channel.segmentSize() >= static_cast<int>(sizeof(SharedMemoryChannel::Header)) + 1);
}

void TestSharedMemoryChannel::create_initializesProtocolHeader()
{
    const QString key = makeUniqueKey();
    SharedMemoryChannel channel(key);

    QString errorMessage;
    QVERIFY2(channel.create(&errorMessage), qPrintable(errorMessage));
    QVERIFY(channel.isAttached());

    QVERIFY(channel.memory_.lock());

    SharedMemoryChannel::Header header;
    std::memcpy(&header, channel.memory_.constData(), sizeof(header));

    channel.memory_.unlock();

    QCOMPARE(header.magic, SharedMemoryChannel::kMagic);
    QCOMPARE(header.version, SharedMemoryChannel::kProtocolVersion);
    QCOMPARE(header.payloadSize, quint32(0));

    QVERIFY2(channel.detach(&errorMessage), qPrintable(errorMessage));
}

void TestSharedMemoryChannel::create_calledTwice_keepsValidState()
{
    const QString key = makeUniqueKey();
    SharedMemoryChannel channel(key);

    QString errorMessage;
    QVERIFY2(channel.create(&errorMessage), qPrintable(errorMessage));
    QVERIFY2(channel.create(&errorMessage), qPrintable(errorMessage));
    QVERIFY(channel.isAttached());

    QVERIFY2(channel.detach(&errorMessage), qPrintable(errorMessage));
}

void TestSharedMemoryChannel::attach_toExistingSegment_succeeds()
{
    const QString key = makeUniqueKey();

    SharedMemoryChannel creator(key);
    SharedMemoryChannel consumer(key);

    QString errorMessage;
    QVERIFY2(creator.create(&errorMessage), qPrintable(errorMessage));
    QVERIFY2(consumer.attach(&errorMessage), qPrintable(errorMessage));

    QVERIFY(creator.isAttached());
    QVERIFY(consumer.isAttached());

    QVERIFY2(consumer.detach(&errorMessage), qPrintable(errorMessage));
    QVERIFY2(creator.detach(&errorMessage), qPrintable(errorMessage));
}

void TestSharedMemoryChannel::attach_toMissingSegment_fails()
{
    SharedMemoryChannel channel(makeUniqueKey());

    QString errorMessage;
    QVERIFY(!channel.attach(&errorMessage));
    QVERIFY(!errorMessage.isEmpty());
    QCOMPARE(channel.isAttached(), false);
}

void TestSharedMemoryChannel::writeAndRead_sameInstance_roundtripWorks()
{
    const QString key = makeUniqueKey();
    SharedMemoryChannel channel(key);

    QString errorMessage;
    QVERIFY2(channel.create(&errorMessage), qPrintable(errorMessage));

    const QString expected = QStringLiteral("hello shared memory");
    QVERIFY2(channel.writeString(expected, &errorMessage), qPrintable(errorMessage));

    QString actual;
    QVERIFY2(channel.readString(&actual, &errorMessage), qPrintable(errorMessage));
    QCOMPARE(actual, expected);

    QVERIFY2(channel.detach(&errorMessage), qPrintable(errorMessage));
}

void TestSharedMemoryChannel::writeAndRead_twoInstances_roundtripWorks()
{
    const QString key = makeUniqueKey();

    SharedMemoryChannel writer(key);
    SharedMemoryChannel reader(key);

    QString errorMessage;
    QVERIFY2(writer.create(&errorMessage), qPrintable(errorMessage));
    QVERIFY2(reader.attach(&errorMessage), qPrintable(errorMessage));

    const QString expected = QStringLiteral("parent -> child -> parent");
    QVERIFY2(writer.writeString(expected, &errorMessage), qPrintable(errorMessage));

    QString actual;
    QVERIFY2(reader.readString(&actual, &errorMessage), qPrintable(errorMessage));
    QCOMPARE(actual, expected);

    QVERIFY2(reader.detach(&errorMessage), qPrintable(errorMessage));
    QVERIFY2(writer.detach(&errorMessage), qPrintable(errorMessage));
}

void TestSharedMemoryChannel::writeAndRead_emptyString_roundtripWorks()
{
    const QString key = makeUniqueKey();
    SharedMemoryChannel channel(key);

    QString errorMessage;
    QVERIFY2(channel.create(&errorMessage), qPrintable(errorMessage));

    const QString expected;
    QVERIFY2(channel.writeString(expected, &errorMessage), qPrintable(errorMessage));

    QString actual = QStringLiteral("stub");
    QVERIFY2(channel.readString(&actual, &errorMessage), qPrintable(errorMessage));
    QCOMPARE(actual, expected);

    QVERIFY2(channel.detach(&errorMessage), qPrintable(errorMessage));
}

void TestSharedMemoryChannel::writeAndRead_unicodeString_roundtripWorks()
{
    const QString key = makeUniqueKey();
    SharedMemoryChannel channel(key);

    QString errorMessage;
    QVERIFY2(channel.create(&errorMessage), qPrintable(errorMessage));

    const QString expected =
        QStringLiteral("Привет, Qt!\nСтрока 2\nСпецсимволы: ёжик, € и №42");

    QVERIFY2(channel.writeString(expected, &errorMessage), qPrintable(errorMessage));

    QString actual;
    QVERIFY2(channel.readString(&actual, &errorMessage), qPrintable(errorMessage));
    QCOMPARE(actual, expected);

    QVERIFY2(channel.detach(&errorMessage), qPrintable(errorMessage));
}

void TestSharedMemoryChannel::writeString_withoutAttach_fails()
{
    SharedMemoryChannel channel(makeUniqueKey());

    QString errorMessage;
    QVERIFY(!channel.writeString(QStringLiteral("data"), &errorMessage));
    QVERIFY(!errorMessage.isEmpty());
}

void TestSharedMemoryChannel::readString_withoutAttach_fails()
{
    SharedMemoryChannel channel(makeUniqueKey());

    QString errorMessage;
    QString value = QStringLiteral("stub");

    QVERIFY(!channel.readString(&value, &errorMessage));
    QVERIFY(!errorMessage.isEmpty());
}

void TestSharedMemoryChannel::writeString_tooLargePayload_fails()
{
    const QString key = makeUniqueKey();
    SharedMemoryChannel channel(key, 64);

    QString errorMessage;
    QVERIFY2(channel.create(&errorMessage), qPrintable(errorMessage));

    const QString hugeString(10'000, QLatin1Char('A'));

    QVERIFY(!channel.writeString(hugeString, &errorMessage));
    QVERIFY(!errorMessage.isEmpty());

    QVERIFY2(channel.detach(&errorMessage), qPrintable(errorMessage));
}

void TestSharedMemoryChannel::detach_calledTwice_isSafe()
{
    const QString key = makeUniqueKey();
    SharedMemoryChannel channel(key);

    QString errorMessage;
    QVERIFY2(channel.create(&errorMessage), qPrintable(errorMessage));
    QVERIFY2(channel.detach(&errorMessage), qPrintable(errorMessage));
    QVERIFY2(channel.detach(&errorMessage), qPrintable(errorMessage));

    QCOMPARE(channel.isAttached(), false);
}

void TestSharedMemoryChannel::readString_withInvalidMagic_fails()
{
    const QString key = makeUniqueKey();
    SharedMemoryChannel channel(key);

    QString errorMessage;
    QVERIFY2(channel.create(&errorMessage), qPrintable(errorMessage));
    QVERIFY2(channel.writeString(QStringLiteral("abc"), &errorMessage), qPrintable(errorMessage));

    QVERIFY(channel.memory_.lock());

    auto* raw = static_cast<char*>(channel.memory_.data());
    SharedMemoryChannel::Header header;
    std::memcpy(&header, raw, sizeof(header));
    header.magic = 0xDEADBEEF;
    std::memcpy(raw, &header, sizeof(header));

    channel.memory_.unlock();

    QString value;
    QVERIFY(!channel.readString(&value, &errorMessage));
    QVERIFY(!errorMessage.isEmpty());

    QVERIFY2(channel.detach(&errorMessage), qPrintable(errorMessage));
}

void TestSharedMemoryChannel::readString_withInvalidVersion_fails()
{
    const QString key = makeUniqueKey();
    SharedMemoryChannel channel(key);

    QString errorMessage;
    QVERIFY2(channel.create(&errorMessage), qPrintable(errorMessage));
    QVERIFY2(channel.writeString(QStringLiteral("abc"), &errorMessage), qPrintable(errorMessage));

    QVERIFY(channel.memory_.lock());

    auto* raw = static_cast<char*>(channel.memory_.data());
    SharedMemoryChannel::Header header;
    std::memcpy(&header, raw, sizeof(header));
    header.version = SharedMemoryChannel::kProtocolVersion + 1;
    std::memcpy(raw, &header, sizeof(header));

    channel.memory_.unlock();

    QString value;
    QVERIFY(!channel.readString(&value, &errorMessage));
    QVERIFY(!errorMessage.isEmpty());

    QVERIFY2(channel.detach(&errorMessage), qPrintable(errorMessage));
}

void TestSharedMemoryChannel::readString_withCorruptedPayloadSize_fails()
{
    const QString key = makeUniqueKey();
    SharedMemoryChannel channel(key);

    QString errorMessage;
    QVERIFY2(channel.create(&errorMessage), qPrintable(errorMessage));
    QVERIFY2(channel.writeString(QStringLiteral("abc"), &errorMessage), qPrintable(errorMessage));

    QVERIFY(channel.memory_.lock());

    auto* raw = static_cast<char*>(channel.memory_.data());
    SharedMemoryChannel::Header header;
    std::memcpy(&header, raw, sizeof(header));
    header.payloadSize = static_cast<quint32>(channel.memory_.size());
    std::memcpy(raw, &header, sizeof(header));

    channel.memory_.unlock();

    QString value;
    QVERIFY(!channel.readString(&value, &errorMessage));
    QVERIFY(!errorMessage.isEmpty());

    QVERIFY2(channel.detach(&errorMessage), qPrintable(errorMessage));
}

QTEST_GUILESS_MAIN(TestSharedMemoryChannel)

#include "test_sharedmemorychannel.moc"
