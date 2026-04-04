#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , process_(new QProcess(this))
{
    ui->setupUi(this);

    initializeUi();
    connectProcessSignals();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initializeUi()
{
    ui->kill_button->setEnabled(false);

    ui->exitCode_lineEdit->setReadOnly(true);
    ui->exitStatus_lineEdit->setReadOnly(true);

    clearExitInfo();

    statusBar()->showMessage(QStringLiteral("Готово к запуску процессов"), 3000);
}

void MainWindow::connectProcessSignals()
{
    connect(process_, &QProcess::started,
            this, &MainWindow::onChildStarted);

    connect(process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &MainWindow::onChildFinished);

    connect(process_, &QProcess::errorOccurred,
            this, &MainWindow::onChildError);
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

    // Пока простой вариант: делим по пробелам.
    // Поддержку кавычек и сложных аргументов можно добавить следующим этапом.
    return rawArguments.split(' ', Qt::SkipEmptyParts);
}

bool MainWindow::validateProgram() const
{
    if (!programText().isEmpty()) {
        return true;
    }

    QMessageBox::warning(
        const_cast<MainWindow*>(this),
        QStringLiteral("Ошибка"),
        QStringLiteral("Укажите имя или путь к исполняемому файлу.")
    );

    ui->name_lineEdit->setFocus();
    return false;
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
    const bool started = QProcess::startDetached(programText(), arguments, QString(), &pid);

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

    const int result = QProcess::execute(programText(), arguments);

    statusBar()->showMessage(
        QStringLiteral("execute завершён, код возврата: %1").arg(result),
        5000
    );
}

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

    process_->start(programText(), programArguments());

    statusBar()->showMessage(QStringLiteral("Запуск дочернего процесса..."), 3000);
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
