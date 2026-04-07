#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sharedmemorychannel.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QStatusBar>

/**
 * @file mainwindow.cpp
 * @brief Реализация главного окна родительского процесса.
 *
 * @details
 * Файл содержит реализацию логики запуска дочерних процессов, обмена данными
 * через QProcess и работы с разделяемой памятью через SharedMemoryChannel.
 */

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , process_(new QProcess(this))
    , sharedMemory_(std::make_unique<SharedMemoryChannel>())
{
    ui->setupUi(this);

    initializeUi();
    connectProcessSignals();
}

MainWindow::~MainWindow()
{
    if (process_->state() != QProcess::NotRunning) {
        process_->kill();
        process_->waitForFinished(1000);
    }

    delete ui;
}

void MainWindow::initializeUi()
{
    ui->kill_button->setEnabled(false);

    ui->exitCode_lineEdit->setReadOnly(true);
    ui->exitStatus_lineEdit->setReadOnly(true);
    ui->dataReception_lineEdit->setReadOnly(true);

    clearExitInfo();

    statusBar()->showMessage(QStringLiteral("Готово к запуску процессов"), 3000);

    initializeSharedMemoryUi();
}

void MainWindow::connectProcessSignals()
{
    connect(process_, &QProcess::started,
            this, &MainWindow::onChildStarted);

    connect(process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &MainWindow::onChildFinished);

    connect(process_, &QProcess::errorOccurred,
            this, &MainWindow::onChildError);

    connect(process_, &QProcess::readyReadStandardOutput,
            this, &MainWindow::onChildStandardOutputReady);
}

QString MainWindow::programText() const
{
    return ui->name_lineEdit->text().trimmed();
}

QStringList MainWindow::programArguments() const
{
    const QString rawArguments = ui->arguments_lineEdit->text().trimmed();

    if (rawArguments.isEmpty()) {
        return {};
    }

    return rawArguments.split(' ', Qt::SkipEmptyParts);
}

bool MainWindow::validateProgram()
{
    if (!programText().isEmpty()) {
        return true;
    }

    QMessageBox::warning(
        this,
        QStringLiteral("Ошибка"),
        QStringLiteral("Укажите имя или путь к исполняемому файлу.")
    );

    ui->name_lineEdit->setFocus();
    return false;
}

QString MainWindow::resolvedProgramPath() const
{
    const QString program = programText();
    const QFileInfo info(program);

    if (info.isAbsolute()) {
        return info.absoluteFilePath();
    }

    return QDir(QCoreApplication::applicationDirPath()).filePath(program);
}

void MainWindow::clearExitInfo()
{
    ui->exitCode_lineEdit->clear();
    ui->exitStatus_lineEdit->clear();
}

void MainWindow::updateExitInfo(const int exitCode, const QProcess::ExitStatus exitStatus)
{
    ui->exitCode_lineEdit->setText(QString::number(exitCode));
    ui->exitStatus_lineEdit->setText(exitStatusToString(exitStatus));
}

void MainWindow::setProcessControlsRunning(const bool running)
{
    ui->start_button->setEnabled(!running);
    ui->kill_button->setEnabled(running);
}

QString MainWindow::normalizeLine(QString text)
{
    while (!text.isEmpty() && (text.endsWith('\n') || text.endsWith('\r'))) {
        text.chop(1);
    }

    return text;
}

QString MainWindow::exitStatusToString(const QProcess::ExitStatus exitStatus)
{
    switch (exitStatus) {
    case QProcess::NormalExit:
        return QStringLiteral("NormalExit");
    case QProcess::CrashExit:
        return QStringLiteral("CrashExit");
    }

    return QStringLiteral("UnknownExitStatus");
}

QString MainWindow::processErrorToString(const QProcess::ProcessError error)
{
    switch (error) {
    case QProcess::FailedToStart:
        return QStringLiteral("Не удалось запустить процесс");
    case QProcess::Crashed:
        return QStringLiteral("Процесс аварийно завершился");
    case QProcess::Timedout:
        return QStringLiteral("Операция завершилась по таймауту");
    case QProcess::WriteError:
        return QStringLiteral("Ошибка записи в процесс");
    case QProcess::ReadError:
        return QStringLiteral("Ошибка чтения из процесса");
    case QProcess::UnknownError:
        return QStringLiteral("Неизвестная ошибка процесса");
    }

    return QStringLiteral("Неизвестная ошибка процесса");
}

void MainWindow::launchDetached(const QStringList& arguments)
{
    if (!validateProgram()) {
        return;
    }

    qint64 pid = 0;
    const bool started = QProcess::startDetached(resolvedProgramPath(), arguments, QString(), &pid);

    if (!started) {
        QMessageBox::warning(
            this,
            QStringLiteral("Ошибка запуска"),
            QStringLiteral("Не удалось запустить обособленный процесс.")
        );
        return;
    }

    statusBar()->showMessage(
        QStringLiteral("Detached-процесс запущен. PID=%1").arg(pid),
        5000
    );
}

void MainWindow::executeProgram(const QStringList& arguments)
{
    if (!validateProgram()) {
        return;
    }

    const int result = QProcess::execute(resolvedProgramPath(), arguments);

    statusBar()->showMessage(
        QStringLiteral("execute завершён, код возврата: %1").arg(result),
        5000
    );
}

/**
 * @brief Запускает дочерний процесс в управляемом режиме.
 *
 * @details
 * Перед запуском очищает остаточные данные предыдущего сеанса, задаёт
 * рабочий каталог и включает раздельные каналы stdout/stderr.
 */
void MainWindow::startInteractiveProcess()
{
    if (!validateProgram()) {
        return;
    }

    if (process_->state() != QProcess::NotRunning) {
        statusBar()->showMessage(QStringLiteral("Процесс уже запущен или запускается"), 3000);
        return;
    }

    clearExitInfo();
    ui->dataReception_lineEdit->clear();
    childStdoutBuffer_.clear();

    process_->setWorkingDirectory(QCoreApplication::applicationDirPath());
    process_->setProcessChannelMode(QProcess::SeparateChannels);
    process_->start(resolvedProgramPath(), programArguments());

    statusBar()->showMessage(
        QStringLiteral("Запуск дочернего процесса: %1").arg(resolvedProgramPath()),
        4000
    );
}

/**
 * @brief Выполняет построчную обработку stdout дочернего процесса.
 *
 * @details
 * Метод накапливает данные во внутреннем буфере до обнаружения символа '\n'.
 * Это позволяет корректно обрабатывать случаи, когда одна строка приходит
 * из stdout несколькими фрагментами.
 */
void MainWindow::handleStdoutChunk(const QByteArray& chunk)
{
    childStdoutBuffer_.append(chunk);

    while (true) {
        const int lineEndIndex = childStdoutBuffer_.indexOf('\n');

        if (lineEndIndex < 0) {
            break;
        }

        const QByteArray rawLine = childStdoutBuffer_.left(lineEndIndex);
        childStdoutBuffer_.remove(0, lineEndIndex + 1);

        displayReceivedFromChild(normalizeLine(QString::fromUtf8(rawLine)));
    }
}

void MainWindow::displayReceivedFromChild(const QString& text)
{
    ui->dataReception_lineEdit->setText(text);

    statusBar()->showMessage(
        QStringLiteral("Получены данные от дочернего процесса"),
        3000
    );
}

void MainWindow::on_startDetached_button_clicked()
{
    launchDetached({});
}

void MainWindow::on_startDetachedAndParams_button_clicked()
{
    launchDetached(programArguments());
}

void MainWindow::on_execute_button_clicked()
{
    executeProgram({});
}

void MainWindow::on_executeAndParams_button_clicked()
{
    executeProgram(programArguments());
}

void MainWindow::on_start_button_clicked()
{
    startInteractiveProcess();
}

void MainWindow::on_kill_button_clicked()
{
    if (process_->state() == QProcess::NotRunning) {
        statusBar()->showMessage(QStringLiteral("Нет запущенного процесса для завершения"), 3000);
        return;
    }

    process_->kill();
    statusBar()->showMessage(QStringLiteral("Отправлен kill дочернему процессу"), 3000);
}

/**
 * @brief Отправляет пользовательскую строку в stdin дочернего процесса.
 *
 * @details
 * Строка кодируется в UTF-8 и завершается символом перевода строки, чтобы
 * дочерний процесс мог считать её как завершённое текстовое сообщение.
 */
void MainWindow::on_sendingData_button_clicked()
{
    if (process_->state() != QProcess::Running) {
        QMessageBox::warning(
            this,
            QStringLiteral("Ошибка"),
            QStringLiteral("Сначала запустите дочерний процесс.")
        );
        return;
    }

    QByteArray payload = ui->sendingData_lineEdit->text().toUtf8();
    payload.append('\n');

    const qint64 written = process_->write(payload);

    if (written == -1) {
        QMessageBox::warning(
            this,
            QStringLiteral("Ошибка записи"),
            QStringLiteral("Не удалось отправить данные дочернему процессу.\n%1")
                .arg(process_->errorString())
        );
        return;
    }

    if (!process_->waitForBytesWritten(1000)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Ошибка записи"),
            QStringLiteral("Данные не были записаны в дочерний процесс за ожидаемое время.")
        );
        return;
    }

    statusBar()->showMessage(
        QStringLiteral("Отправлено дочернему процессу %1 байт").arg(written),
        3000
    );
}

void MainWindow::onChildStarted()
{
    setProcessControlsRunning(true);

    statusBar()->showMessage(QStringLiteral("Дочерний процесс успешно запущен"), 3000);
}

void MainWindow::onChildFinished(const int exitCode, const QProcess::ExitStatus exitStatus)
{
    updateExitInfo(exitCode, exitStatus);
    setProcessControlsRunning(false);

    statusBar()->showMessage(
        QStringLiteral("Дочерний процесс завершён. Код=%1, статус=%2")
            .arg(exitCode)
            .arg(exitStatusToString(exitStatus)),
        5000
    );
}

void MainWindow::onChildError(const QProcess::ProcessError error)
{
    setProcessControlsRunning(false);

    QMessageBox::warning(
        this,
        QStringLiteral("Ошибка процесса"),
        QStringLiteral("%1.\n%2")
            .arg(processErrorToString(error))
            .arg(process_->errorString())
    );
}

void MainWindow::onChildStandardOutputReady()
{
    handleStdoutChunk(process_->readAllStandardOutput());
}

void MainWindow::initializeSharedMemoryUi()
{
    ui->read_lineEdit->setReadOnly(true);
    setSharedMemoryControlsReady(false);
}

void MainWindow::setSharedMemoryControlsReady(const bool ready)
{
    ui->creationMemory_button->setEnabled(!ready);

    ui->record_button->setEnabled(ready);
    ui->record_lineEdit->setEnabled(ready);

    ui->read_button->setEnabled(ready);
    ui->memoryShutdown_button->setEnabled(ready);
}

/**
 * @brief Создаёт или подключает сегмент разделяемой памяти.
 *
 * @details
 * После успешной операции активирует элементы интерфейса, связанные
 * с записью, чтением и отключением shared memory.
 */
void MainWindow::on_creationMemory_button_clicked()
{
    QString errorMessage;

    if (!sharedMemory_->create(&errorMessage)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Shared memory"),
            errorMessage
        );
        return;
    }

    setSharedMemoryControlsReady(true);

    statusBar()->showMessage(
        QStringLiteral("Сегмент разделяемой памяти создан/подключён"),
        3000
    );
}

void MainWindow::on_record_button_clicked()
{
    QString errorMessage;

    if (!sharedMemory_->writeString(ui->record_lineEdit->text(), &errorMessage)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Shared memory"),
            errorMessage
        );
        return;
    }

    statusBar()->showMessage(
        QStringLiteral("Данные записаны в разделяемую память"),
        3000
    );
}

void MainWindow::on_read_button_clicked()
{
    QString errorMessage;
    QString value;

    if (!sharedMemory_->readString(&value, &errorMessage)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Shared memory"),
            errorMessage
        );
        return;
    }

    ui->read_lineEdit->setText(value);

    statusBar()->showMessage(
        QStringLiteral("Данные прочитаны из разделяемой памяти"),
        3000
    );
}

void MainWindow::on_memoryShutdown_button_clicked()
{
    QString errorMessage;

    if (!sharedMemory_->detach(&errorMessage)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Shared memory"),
            errorMessage
        );
        return;
    }

    setSharedMemoryControlsReady(false);

    ui->record_lineEdit->clear();
    ui->read_lineEdit->clear();

    statusBar()->showMessage(
        QStringLiteral("Отключение от разделяемой памяти выполнено"),
        3000
    );
}
