/**
 * @file test_mainwindow.cpp
 * @brief Юнит-тесты для класса MainWindow дочернего приложения.
 *
 * @details
 * Тесты покрывают:
 * - начальную инициализацию пользовательского интерфейса;
 * - вспомогательную функцию normalizeLine();
 * - отправку строки в стандартный вывод через слот отправки;
 * - подключение к сегменту разделяемой памяти и переключение состояний UI;
 * - отключение от разделяемой памяти и очистку полей;
 * - запись строки в shared memory через элементы интерфейса;
 * - чтение строки из shared memory через элементы интерфейса.
 *
 * Для удобства тестирования используется test seam:
 * директива `#define private public` временно раскрывает приватные поля
 * и методы класса MainWindow, чтобы:
 * - подменить внутренний SharedMemoryChannel на экземпляр с уникальным ключом;
 * - напрямую вызвать приватные слоты;
 * - протестировать normalizeLine().
 *
 * Это оправдано, поскольку класс привязан к GUI и жёстко создаёт свой канал
 * shared memory внутри конструктора, без возможности внедрения зависимости извне.
 */

#define private public
#include "mainwindow.h"
#undef private

#include "sharedmemorychannel.h"

#include <QtTest/QtTest>

#include <QLineEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QUuid>

#include <memory>
#include <sstream>
#include <streambuf>
#include <iostream>

/**
 * @brief Генерирует уникальный ключ сегмента разделяемой памяти для теста.
 * @return Уникальный строковый ключ.
 *
 * @details
 * Подмена ключа позволяет изолировать тесты друг от друга и избежать
 * конфликтов с реальным приложением, которое использует ключ по умолчанию.
 */
static QString makeUniqueSharedMemoryKey()
{
    return QStringLiteral("test_child_mainwindow_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

/**
 * @class StdoutRedirectGuard
 * @brief RAII-обёртка для временного перенаправления std::cout.
 *
 * @details
 * Используется в тестах слота on_sendData_button_clicked(), чтобы проверить,
 * какие именно байты были записаны в стандартный вывод дочернего процесса.
 */
class StdoutRedirectGuard final
{
public:
    /**
     * @brief Перенаправляет std::cout во внутренний строковый буфер.
     */
    StdoutRedirectGuard()
        : oldBuffer_(std::cout.rdbuf(stream_.rdbuf()))
    {
    }

    /**
     * @brief Восстанавливает исходный буфер std::cout.
     */
    ~StdoutRedirectGuard()
    {
        std::cout.rdbuf(oldBuffer_);
    }

    /**
     * @brief Возвращает всё, что было записано в std::cout во время перехвата.
     * @return Захваченная строка как std::string.
     */
    std::string capturedText() const
    {
        return stream_.str();
    }

private:
    std::streambuf* oldBuffer_ = nullptr;
    std::ostringstream stream_;
};

/**
 * @class TestChildMainWindow
 * @brief Набор unit-тестов для MainWindow дочернего приложения.
 */
class TestChildMainWindow final : public QObject
{
    Q_OBJECT

private slots:
    /**
     * @brief Проверяет начальную инициализацию окна и состояний UI.
     *
     * @details
     * Проверяется:
     * - заголовок окна;
     * - режим read-only для полей приёма и чтения;
     * - начальная доступность кнопок shared memory;
     * - наличие стартового сообщения в status bar.
     */
    void constructor_initializesUiState();

    /**
     * @brief Проверяет корректность удаления завершающих символов конца строки.
     */
    void normalizeLine_trimsTrailingLineEndings();

    /**
     * @brief Проверяет, что слот отправки пишет UTF-8 строку с '\\n' в stdout.
     */
    void sendData_writesUtf8LineToStdout();

    /**
     * @brief Проверяет успешное подключение к уже существующему сегменту памяти.
     *
     * @details
     * Дополнительно проверяется, что после подключения правильно
     * переключаются доступность кнопок и полей shared memory.
     */
    void connection_attachesAndUpdatesControls();

    /**
     * @brief Проверяет отключение от shared memory, очистку полей и обновление UI.
     */
    void disconnection_detachesClearsFieldsAndUpdatesControls();

    /**
     * @brief Проверяет запись строки в shared memory через UI окна.
     *
     * @details
     * Значение записывается через слот MainWindow, а затем считывается
     * отдельным экземпляром SharedMemoryChannel.
     */
    void writeButton_writesValueToSharedMemory();

    /**
     * @brief Проверяет чтение строки из shared memory в поле интерфейса.
     *
     * @details
     * Значение заранее записывается отдельным экземпляром SharedMemoryChannel,
     * после чего считывается слотом MainWindow.
     */
    void readButton_readsValueFromSharedMemory();

private:
    /**
     * @brief Подменяет внутренний канал shared memory на экземпляр с заданным ключом.
     * @param window Тестируемое окно.
     * @param key Уникальный ключ сегмента памяти.
     */
    static void replaceSharedMemoryChannel(MainWindow& window, const QString& key)
    {
        window.sharedMemory_ = std::make_unique<SharedMemoryChannel>(key);
    }

    /**
     * @brief Ищет QLineEdit по objectName и проверяет, что он найден.
     * @param window Тестируемое окно.
     * @param objectName Имя объекта из .ui.
     * @return Указатель на найденный QLineEdit.
     */
    static QLineEdit* lineEdit(MainWindow& window, const char* objectName)
    {
        auto* widget = window.findChild<QLineEdit*>(QString::fromLatin1(objectName));
        Q_ASSERT(widget != nullptr);
        return widget;
    }

    /**
     * @brief Ищет QPushButton по objectName и проверяет, что он найден.
     * @param window Тестируемое окно.
     * @param objectName Имя объекта из .ui.
     * @return Указатель на найденный QPushButton.
     */
    static QPushButton* button(MainWindow& window, const char* objectName)
    {
        auto* widget = window.findChild<QPushButton*>(QString::fromLatin1(objectName));
        Q_ASSERT(widget != nullptr);
        return widget;
    }
};

void TestChildMainWindow::constructor_initializesUiState()
{
    MainWindow window;

    QCOMPARE(window.windowTitle(), QStringLiteral("Child"));

    auto* dataReceptionLineEdit = lineEdit(window, "dataReception_lineEdit");
    auto* readLineEdit = lineEdit(window, "read_lineEdit");
    auto* writeLineEdit = lineEdit(window, "write_lineEdit");

    auto* dataReceptionButton = button(window, "dataReception_button");
    auto* connectionButton = button(window, "connection_button");
    auto* disconnectionButton = button(window, "disconnection_button");
    auto* writeButton = button(window, "write_button");
    auto* readButton = button(window, "read_button");

    QVERIFY(dataReceptionLineEdit->isReadOnly());
    QVERIFY(readLineEdit->isReadOnly());

    QVERIFY(dataReceptionButton->isEnabled());
    QVERIFY(connectionButton->isEnabled());

    QVERIFY(!disconnectionButton->isEnabled());
    QVERIFY(!writeButton->isEnabled());
    QVERIFY(!readButton->isEnabled());
    QVERIFY(!writeLineEdit->isEnabled());

    QVERIFY(window.statusBar() != nullptr);
    QVERIFY(!window.statusBar()->currentMessage().isEmpty());
}

void TestChildMainWindow::normalizeLine_trimsTrailingLineEndings()
{
    QCOMPARE(MainWindow::normalizeLine(QStringLiteral("abc")), QStringLiteral("abc"));
    QCOMPARE(MainWindow::normalizeLine(QStringLiteral("abc\n")), QStringLiteral("abc"));
    QCOMPARE(MainWindow::normalizeLine(QStringLiteral("abc\r")), QStringLiteral("abc"));
    QCOMPARE(MainWindow::normalizeLine(QStringLiteral("abc\r\n")), QStringLiteral("abc"));
    QCOMPARE(MainWindow::normalizeLine(QStringLiteral("")), QStringLiteral(""));
    QCOMPARE(MainWindow::normalizeLine(QStringLiteral("a\nb")), QStringLiteral("a\nb"));
}

void TestChildMainWindow::sendData_writesUtf8LineToStdout()
{
    MainWindow window;

    const QString text = QStringLiteral("Привет, родитель");
    lineEdit(window, "sendingData_lineEdit")->setText(text);

    StdoutRedirectGuard redirect;
    window.on_sendData_button_clicked();

    const std::string expected = text.toUtf8().toStdString() + "\n";
    QCOMPARE(redirect.capturedText(), expected);

    QVERIFY(!window.statusBar()->currentMessage().isEmpty());
}

void TestChildMainWindow::connection_attachesAndUpdatesControls()
{
    const QString key = makeUniqueSharedMemoryKey();

    SharedMemoryChannel creator(key);
    QString errorMessage;
    QVERIFY2(creator.create(&errorMessage), qPrintable(errorMessage));

    MainWindow window;
    replaceSharedMemoryChannel(window, key);

    window.on_connection_button_clicked();

    QVERIFY(!button(window, "connection_button")->isEnabled());
    QVERIFY(button(window, "disconnection_button")->isEnabled());
    QVERIFY(button(window, "write_button")->isEnabled());
    QVERIFY(button(window, "read_button")->isEnabled());
    QVERIFY(lineEdit(window, "write_lineEdit")->isEnabled());

    QVERIFY(!window.statusBar()->currentMessage().isEmpty());

    QVERIFY2(creator.detach(&errorMessage), qPrintable(errorMessage));
}

void TestChildMainWindow::disconnection_detachesClearsFieldsAndUpdatesControls()
{
    const QString key = makeUniqueSharedMemoryKey();

    SharedMemoryChannel creator(key);
    QString errorMessage;
    QVERIFY2(creator.create(&errorMessage), qPrintable(errorMessage));

    MainWindow window;
    replaceSharedMemoryChannel(window, key);

    window.on_connection_button_clicked();

    lineEdit(window, "write_lineEdit")->setText(QStringLiteral("данные на запись"));
    lineEdit(window, "read_lineEdit")->setText(QStringLiteral("старое прочитанное значение"));

    window.on_disconnection_button_clicked();

    QVERIFY(button(window, "connection_button")->isEnabled());
    QVERIFY(!button(window, "disconnection_button")->isEnabled());
    QVERIFY(!button(window, "write_button")->isEnabled());
    QVERIFY(!button(window, "read_button")->isEnabled());
    QVERIFY(!lineEdit(window, "write_lineEdit")->isEnabled());

    QCOMPARE(lineEdit(window, "write_lineEdit")->text(), QStringLiteral(""));
    QCOMPARE(lineEdit(window, "read_lineEdit")->text(), QStringLiteral(""));

    QVERIFY(!window.statusBar()->currentMessage().isEmpty());

    QVERIFY2(creator.detach(&errorMessage), qPrintable(errorMessage));
}

void TestChildMainWindow::writeButton_writesValueToSharedMemory()
{
    const QString key = makeUniqueSharedMemoryKey();

    SharedMemoryChannel creator(key);
    QString errorMessage;
    QVERIFY2(creator.create(&errorMessage), qPrintable(errorMessage));

    MainWindow window;
    replaceSharedMemoryChannel(window, key);
    window.on_connection_button_clicked();

    const QString expected = QStringLiteral("Тестовая строка\nс переводом строки и кириллицей");
    lineEdit(window, "write_lineEdit")->setText(expected);

    window.on_write_button_clicked();

    QString actual;
    QVERIFY2(creator.readString(&actual, &errorMessage), qPrintable(errorMessage));
    QCOMPARE(actual, expected);

    QVERIFY2(creator.detach(&errorMessage), qPrintable(errorMessage));
}

void TestChildMainWindow::readButton_readsValueFromSharedMemory()
{
    const QString key = makeUniqueSharedMemoryKey();

    SharedMemoryChannel creator(key);
    QString errorMessage;
    QVERIFY2(creator.create(&errorMessage), qPrintable(errorMessage));

    const QString expected = QStringLiteral("Значение из shared memory");
    QVERIFY2(creator.writeString(expected, &errorMessage), qPrintable(errorMessage));

    MainWindow window;
    replaceSharedMemoryChannel(window, key);
    window.on_connection_button_clicked();

    window.on_read_button_clicked();

    QCOMPARE(lineEdit(window, "read_lineEdit")->text(), expected);

    QVERIFY2(creator.detach(&errorMessage), qPrintable(errorMessage));
}

QTEST_MAIN(TestChildMainWindow)
#include "test_mainwindow.moc"
