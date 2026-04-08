/**
 * @file test_mainwindow.cpp
 * @brief Юнит-тесты для класса MainWindow родительского приложения.
 *
 * @details
 * Тесты покрывают:
 * - начальную инициализацию пользовательского интерфейса;
 * - вспомогательные методы normalizeLine(), exitStatusToString(),
 *   processErrorToString();
 * - чтение текста программы и разбор аргументов командной строки;
 * - разрешение относительного и абсолютного пути к исполняемому файлу;
 * - обновление полей завершения процесса;
 * - переключение доступности элементов при старте/завершении процесса;
 * - построчную обработку stdout дочернего процесса;
 * - отображение полученной строки от дочернего процесса;
 * - создание, запись, чтение и отключение shared memory через UI.
 *
 * Для тестирования приватных методов и полей используется test seam:
 * директива `#define private public` временно раскрывает приватную часть
 * класса MainWindow.
 *
 * Это позволяет:
 * - вызывать приватные слоты и вспомогательные методы напрямую;
 * - подменять внутренний SharedMemoryChannel на экземпляр с уникальным ключом;
 * - проверять состояние внутреннего буфера stdout.
 */

#define private public
#include "mainwindow.h"
#undef private

#include "sharedmemorychannel.h"

#include <QtTest/QtTest>

#include <QCoreApplication>
#include <QDir>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QStatusBar>
#include <QUuid>

#include <memory>

/**
 * @brief Генерирует уникальный ключ сегмента разделяемой памяти для теста.
 * @return Уникальный строковый ключ.
 */
static QString makeUniqueSharedMemoryKey()
{
    return QStringLiteral("test_parent_mainwindow_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

/**
 * @class TestParentMainWindow
 * @brief Набор unit-тестов для MainWindow родительского приложения.
 */
class TestParentMainWindow final : public QObject
{
    Q_OBJECT

private slots:
    /**
     * @brief Проверяет начальную инициализацию интерфейса окна.
     *
     * @details
     * Проверяется:
     * - заголовок окна;
     * - начальная доступность кнопок start/kill;
     * - режим read-only для полей вывода;
     * - начальное состояние секции shared memory;
     * - пустые поля exit code / exit status.
     */
    void constructor_initializesUiState();

    /**
     * @brief Проверяет корректность normalizeLine().
     */
    void normalizeLine_trimsTrailingLineEndings();

    /**
     * @brief Проверяет преобразование статуса завершения процесса в строку.
     */
    void exitStatusToString_returnsExpectedText();

    /**
     * @brief Проверяет преобразование кода ошибки процесса в пользовательский текст.
     */
    void processErrorToString_returnsExpectedText();

    /**
     * @brief Проверяет получение текста программы и разбор аргументов.
     */
    void programTextAndArguments_readFromUi();

    /**
     * @brief Проверяет разрешение абсолютного пути к программе.
     */
    void resolvedProgramPath_returnsAbsolutePathAsIs();

    /**
     * @brief Проверяет разрешение относительного пути относительно каталога приложения.
     */
    void resolvedProgramPath_resolvesRelativePathAgainstApplicationDir();

    /**
     * @brief Проверяет очистку полей завершения процесса.
     */
    void clearExitInfo_clearsExitFields();

    /**
     * @brief Проверяет обновление полей завершения процесса.
     */
    void updateExitInfo_updatesExitFields();

    /**
     * @brief Проверяет переключение кнопок start/kill через setProcessControlsRunning().
     */
    void setProcessControlsRunning_updatesButtons();

    /**
     * @brief Проверяет обработку события успешного старта дочернего процесса.
     */
    void onChildStarted_updatesControlsAndStatusMessage();

    /**
     * @brief Проверяет обработку события завершения дочернего процесса.
     */
    void onChildFinished_updatesControlsExitInfoAndStatusMessage();

    /**
     * @brief Проверяет отображение строки, полученной от дочернего процесса.
     */
    void displayReceivedFromChild_updatesUi();

    /**
     * @brief Проверяет разбор stdout по строкам, включая неполный хвост.
     */
    void handleStdoutChunk_processesCompleteLinesAndKeepsTail();

    /**
     * @brief Проверяет поведение kill-кнопки, когда процесс не запущен.
     */
    void killButton_whenProcessNotRunning_showsSafeStatusMessage();

    /**
     * @brief Проверяет начальную инициализацию секции shared memory.
     */
    void sharedMemoryUi_initialStateIsCorrect();

    /**
     * @brief Проверяет создание/подключение к shared memory через UI.
     */
    void creationMemory_connectsAndUpdatesControls();

    /**
     * @brief Проверяет запись строки в shared memory через UI.
     */
    void recordButton_writesValueToSharedMemory();

    /**
     * @brief Проверяет чтение строки из shared memory через UI.
     */
    void readButton_readsValueFromSharedMemory();

    /**
     * @brief Проверяет отключение от shared memory и очистку связанных полей.
     */
    void memoryShutdown_detachesClearsFieldsAndUpdatesControls();

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

void TestParentMainWindow::constructor_initializesUiState()
{
    MainWindow window;

    QCOMPARE(window.windowTitle(), QStringLiteral("Parent"));

    auto* killButton = button(window, "kill_button");
    auto* startButton = button(window, "start_button");

    auto* exitCodeLineEdit = lineEdit(window, "exitCode_lineEdit");
    auto* exitStatusLineEdit = lineEdit(window, "exitStatus_lineEdit");
    auto* dataReceptionLineEdit = lineEdit(window, "dataReception_lineEdit");
    auto* readLineEdit = lineEdit(window, "read_lineEdit");
    auto* recordLineEdit = lineEdit(window, "record_lineEdit");

    auto* creationMemoryButton = button(window, "creationMemory_button");
    auto* recordButton = button(window, "record_button");
    auto* readButton = button(window, "read_button");
    auto* memoryShutdownButton = button(window, "memoryShutdown_button");

    QVERIFY(startButton->isEnabled());
    QVERIFY(!killButton->isEnabled());

    QVERIFY(exitCodeLineEdit->isReadOnly());
    QVERIFY(exitStatusLineEdit->isReadOnly());
    QVERIFY(dataReceptionLineEdit->isReadOnly());
    QVERIFY(readLineEdit->isReadOnly());

    QCOMPARE(exitCodeLineEdit->text(), QStringLiteral(""));
    QCOMPARE(exitStatusLineEdit->text(), QStringLiteral(""));

    QVERIFY(creationMemoryButton->isEnabled());
    QVERIFY(!recordButton->isEnabled());
    QVERIFY(!readButton->isEnabled());
    QVERIFY(!memoryShutdownButton->isEnabled());
    QVERIFY(!recordLineEdit->isEnabled());

    QVERIFY(window.statusBar() != nullptr);
    QVERIFY(!window.statusBar()->currentMessage().isEmpty());
}

void TestParentMainWindow::normalizeLine_trimsTrailingLineEndings()
{
    QCOMPARE(MainWindow::normalizeLine(QStringLiteral("abc")), QStringLiteral("abc"));
    QCOMPARE(MainWindow::normalizeLine(QStringLiteral("abc\n")), QStringLiteral("abc"));
    QCOMPARE(MainWindow::normalizeLine(QStringLiteral("abc\r")), QStringLiteral("abc"));
    QCOMPARE(MainWindow::normalizeLine(QStringLiteral("abc\r\n")), QStringLiteral("abc"));
    QCOMPARE(MainWindow::normalizeLine(QStringLiteral("")), QStringLiteral(""));
    QCOMPARE(MainWindow::normalizeLine(QStringLiteral("a\nb")), QStringLiteral("a\nb"));
}

void TestParentMainWindow::exitStatusToString_returnsExpectedText()
{
    QCOMPARE(MainWindow::exitStatusToString(QProcess::NormalExit), QStringLiteral("NormalExit"));
    QCOMPARE(MainWindow::exitStatusToString(QProcess::CrashExit), QStringLiteral("CrashExit"));
}

void TestParentMainWindow::processErrorToString_returnsExpectedText()
{
    QCOMPARE(MainWindow::processErrorToString(QProcess::FailedToStart),
             QStringLiteral("Не удалось запустить процесс"));
    QCOMPARE(MainWindow::processErrorToString(QProcess::Crashed),
             QStringLiteral("Процесс аварийно завершился"));
    QCOMPARE(MainWindow::processErrorToString(QProcess::Timedout),
             QStringLiteral("Операция завершилась по таймауту"));
    QCOMPARE(MainWindow::processErrorToString(QProcess::WriteError),
             QStringLiteral("Ошибка записи в процесс"));
    QCOMPARE(MainWindow::processErrorToString(QProcess::ReadError),
             QStringLiteral("Ошибка чтения из процесса"));
    QCOMPARE(MainWindow::processErrorToString(QProcess::UnknownError),
             QStringLiteral("Неизвестная ошибка процесса"));
}

void TestParentMainWindow::programTextAndArguments_readFromUi()
{
    MainWindow window;

    lineEdit(window, "name_lineEdit")->setText(QStringLiteral("  child.exe  "));
    lineEdit(window, "arguments_lineEdit")->setText(QStringLiteral("  arg1   arg2  arg3  "));

    QCOMPARE(window.programText(), QStringLiteral("child.exe"));
    QCOMPARE(window.programArguments(),
             QStringList({QStringLiteral("arg1"), QStringLiteral("arg2"), QStringLiteral("arg3")}));

    lineEdit(window, "arguments_lineEdit")->setText(QStringLiteral("   "));
    QCOMPARE(window.programArguments(), QStringList{});
}

void TestParentMainWindow::resolvedProgramPath_returnsAbsolutePathAsIs()
{
    MainWindow window;

    const QString absolutePath =
        QDir::cleanPath(QDir::temp().filePath(QStringLiteral("app.exe")));

    QVERIFY(QFileInfo(absolutePath).isAbsolute());

    lineEdit(window, "name_lineEdit")->setText(absolutePath);

    QCOMPARE(window.resolvedProgramPath(), QFileInfo(absolutePath).absoluteFilePath());
}

void TestParentMainWindow::resolvedProgramPath_resolvesRelativePathAgainstApplicationDir()
{
    MainWindow window;

    lineEdit(window, "name_lineEdit")->setText(QStringLiteral("child.exe"));

    const QString expected =
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("child.exe"));

    QCOMPARE(window.resolvedProgramPath(), expected);
}

void TestParentMainWindow::clearExitInfo_clearsExitFields()
{
    MainWindow window;

    lineEdit(window, "exitCode_lineEdit")->setText(QStringLiteral("42"));
    lineEdit(window, "exitStatus_lineEdit")->setText(QStringLiteral("CrashExit"));

    window.clearExitInfo();

    QCOMPARE(lineEdit(window, "exitCode_lineEdit")->text(), QStringLiteral(""));
    QCOMPARE(lineEdit(window, "exitStatus_lineEdit")->text(), QStringLiteral(""));
}

void TestParentMainWindow::updateExitInfo_updatesExitFields()
{
    MainWindow window;

    window.updateExitInfo(17, QProcess::CrashExit);

    QCOMPARE(lineEdit(window, "exitCode_lineEdit")->text(), QStringLiteral("17"));
    QCOMPARE(lineEdit(window, "exitStatus_lineEdit")->text(), QStringLiteral("CrashExit"));
}

void TestParentMainWindow::setProcessControlsRunning_updatesButtons()
{
    MainWindow window;

    window.setProcessControlsRunning(true);
    QVERIFY(!button(window, "start_button")->isEnabled());
    QVERIFY(button(window, "kill_button")->isEnabled());

    window.setProcessControlsRunning(false);
    QVERIFY(button(window, "start_button")->isEnabled());
    QVERIFY(!button(window, "kill_button")->isEnabled());
}

void TestParentMainWindow::onChildStarted_updatesControlsAndStatusMessage()
{
    MainWindow window;

    window.onChildStarted();

    QVERIFY(!button(window, "start_button")->isEnabled());
    QVERIFY(button(window, "kill_button")->isEnabled());
    QVERIFY(!window.statusBar()->currentMessage().isEmpty());
}

void TestParentMainWindow::onChildFinished_updatesControlsExitInfoAndStatusMessage()
{
    MainWindow window;

    window.onChildFinished(5, QProcess::NormalExit);

    QVERIFY(button(window, "start_button")->isEnabled());
    QVERIFY(!button(window, "kill_button")->isEnabled());

    QCOMPARE(lineEdit(window, "exitCode_lineEdit")->text(), QStringLiteral("5"));
    QCOMPARE(lineEdit(window, "exitStatus_lineEdit")->text(), QStringLiteral("NormalExit"));

    QVERIFY(!window.statusBar()->currentMessage().isEmpty());
}

void TestParentMainWindow::displayReceivedFromChild_updatesUi()
{
    MainWindow window;

    const QString expected = QStringLiteral("message from child");
    window.displayReceivedFromChild(expected);

    QCOMPARE(lineEdit(window, "dataReception_lineEdit")->text(), expected);
    QVERIFY(!window.statusBar()->currentMessage().isEmpty());
}

void TestParentMainWindow::handleStdoutChunk_processesCompleteLinesAndKeepsTail()
{
    MainWindow window;

    window.handleStdoutChunk("first line\nsecond");
    QCOMPARE(lineEdit(window, "dataReception_lineEdit")->text(), QStringLiteral("first line"));
    QCOMPARE(window.childStdoutBuffer_, QByteArray("second"));

    window.handleStdoutChunk(" line\r\nthird line\n");
    QCOMPARE(lineEdit(window, "dataReception_lineEdit")->text(), QStringLiteral("third line"));
    QCOMPARE(window.childStdoutBuffer_, QByteArray());
}

void TestParentMainWindow::killButton_whenProcessNotRunning_showsSafeStatusMessage()
{
    MainWindow window;

    QVERIFY(window.process_->state() == QProcess::NotRunning);

    window.on_kill_button_clicked();

    QVERIFY(button(window, "start_button")->isEnabled());
    QVERIFY(!button(window, "kill_button")->isEnabled());
    QVERIFY(!window.statusBar()->currentMessage().isEmpty());
}

void TestParentMainWindow::sharedMemoryUi_initialStateIsCorrect()
{
    MainWindow window;

    QVERIFY(button(window, "creationMemory_button")->isEnabled());
    QVERIFY(!button(window, "record_button")->isEnabled());
    QVERIFY(!button(window, "read_button")->isEnabled());
    QVERIFY(!button(window, "memoryShutdown_button")->isEnabled());
    QVERIFY(lineEdit(window, "read_lineEdit")->isReadOnly());
    QVERIFY(!lineEdit(window, "record_lineEdit")->isEnabled());
}

void TestParentMainWindow::creationMemory_connectsAndUpdatesControls()
{
    const QString key = makeUniqueSharedMemoryKey();

    MainWindow window;
    replaceSharedMemoryChannel(window, key);

    window.on_creationMemory_button_clicked();

    QVERIFY(!button(window, "creationMemory_button")->isEnabled());
    QVERIFY(button(window, "record_button")->isEnabled());
    QVERIFY(button(window, "read_button")->isEnabled());
    QVERIFY(button(window, "memoryShutdown_button")->isEnabled());
    QVERIFY(lineEdit(window, "record_lineEdit")->isEnabled());

    QVERIFY(!window.statusBar()->currentMessage().isEmpty());
}

void TestParentMainWindow::recordButton_writesValueToSharedMemory()
{
    const QString key = makeUniqueSharedMemoryKey();

    MainWindow window;
    replaceSharedMemoryChannel(window, key);
    window.on_creationMemory_button_clicked();

    const QString expected = QStringLiteral("Parent пишет в shared memory\nс кириллицей");
    lineEdit(window, "record_lineEdit")->setText(expected);

    window.on_record_button_clicked();

    SharedMemoryChannel reader(key);
    QString errorMessage;
    QVERIFY2(reader.attach(&errorMessage), qPrintable(errorMessage));

    QString actual;
    QVERIFY2(reader.readString(&actual, &errorMessage), qPrintable(errorMessage));
    QCOMPARE(actual, expected);

    QVERIFY2(reader.detach(&errorMessage), qPrintable(errorMessage));
}

void TestParentMainWindow::readButton_readsValueFromSharedMemory()
{
    const QString key = makeUniqueSharedMemoryKey();

    SharedMemoryChannel writer(key);
    QString errorMessage;
    QVERIFY2(writer.create(&errorMessage), qPrintable(errorMessage));

    const QString expected = QStringLiteral("Value prepared externally");
    QVERIFY2(writer.writeString(expected, &errorMessage), qPrintable(errorMessage));

    MainWindow window;
    replaceSharedMemoryChannel(window, key);
    window.on_creationMemory_button_clicked();

    window.on_read_button_clicked();

    QCOMPARE(lineEdit(window, "read_lineEdit")->text(), expected);

    QVERIFY2(writer.detach(&errorMessage), qPrintable(errorMessage));
}

void TestParentMainWindow::memoryShutdown_detachesClearsFieldsAndUpdatesControls()
{
    const QString key = makeUniqueSharedMemoryKey();

    MainWindow window;
    replaceSharedMemoryChannel(window, key);
    window.on_creationMemory_button_clicked();

    lineEdit(window, "record_lineEdit")->setText(QStringLiteral("something to record"));
    lineEdit(window, "read_lineEdit")->setText(QStringLiteral("something read"));

    window.on_memoryShutdown_button_clicked();

    QVERIFY(button(window, "creationMemory_button")->isEnabled());
    QVERIFY(!button(window, "record_button")->isEnabled());
    QVERIFY(!button(window, "read_button")->isEnabled());
    QVERIFY(!button(window, "memoryShutdown_button")->isEnabled());
    QVERIFY(!lineEdit(window, "record_lineEdit")->isEnabled());

    QCOMPARE(lineEdit(window, "record_lineEdit")->text(), QStringLiteral(""));
    QCOMPARE(lineEdit(window, "read_lineEdit")->text(), QStringLiteral(""));

    QVERIFY(!window.statusBar()->currentMessage().isEmpty());
}

QTEST_MAIN(TestParentMainWindow)
#include "test_mainwindow.moc"
