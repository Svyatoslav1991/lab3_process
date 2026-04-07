#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QStatusBar>

#include <cstdio>
#include <iostream>

#include "sharedmemorychannel.h"

/**
 * @file mainwindow.cpp
 * @brief Реализация главного окна дочернего процесса.
 *
 * @details
 * Файл содержит реализацию пользовательского интерфейса дочернего приложения,
 * которое взаимодействует с родительским процессом через стандартные потоки
 * ввода-вывода и через сегмент разделяемой памяти.
 */

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , sharedMemory_(std::make_unique<SharedMemoryChannel>())
{
    ui->setupUi(this);

    initializeUi();
}

MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief Настраивает начальное состояние окна.
 */
void MainWindow::initializeUi()
{
    setWindowTitle(QStringLiteral("Child"));

    ui->dataReception_lineEdit->setReadOnly(true);
    ui->dataReception_button->setEnabled(true);

    statusBar()->showMessage(QStringLiteral("Child готов"), 3000);

    initializeSharedMemoryUi();
}

/**
 * @brief Удаляет символы перевода строки в конце строки.
 */
QString MainWindow::normalizeLine(QString text)
{
    while (!text.isEmpty() && (text.endsWith('\n') || text.endsWith('\r'))) {
        text.chop(1);
    }

    return text;
}

/**
 * @brief Отправляет строку родительскому процессу через stdout.
 *
 * @details
 * Запись выполняется через std::cout с последующим flush(),
 * чтобы данные были немедленно переданы во внешний процесс.
 */
void MainWindow::on_sendData_button_clicked()
{
    QByteArray payload = ui->sendingData_lineEdit->text().toUtf8();
    payload.append('\n');

    std::cout.write(payload.constData(), static_cast<std::streamsize>(payload.size()));
    std::cout.flush();

    if (!std::cout.good()) {
        QMessageBox::warning(
            this,
            QStringLiteral("Ошибка"),
            QStringLiteral("Не удалось отправить данные родительскому процессу.")
        );
        return;
    }

    statusBar()->showMessage(QStringLiteral("Данные отправлены родителю"), 3000);
}

/**
 * @brief Считывает строку от родительского процесса из stdin.
 *
 * @details
 * Используется буфер фиксированного размера и функция std::fgets().
 * Полученные данные преобразуются из UTF-8 и отображаются в lineEdit.
 */
void MainWindow::on_dataReception_button_clicked()
{
    char buffer[4096] = {};

    if (std::fgets(buffer, static_cast<int>(sizeof(buffer)), stdin) == nullptr) {
        QMessageBox::warning(
            this,
            QStringLiteral("Ошибка чтения"),
            QStringLiteral("Не удалось прочитать данные от родительского процесса.")
        );
        return;
    }

    ui->dataReception_lineEdit->setText(
        normalizeLine(QString::fromUtf8(buffer))
    );

    statusBar()->showMessage(QStringLiteral("Строка от родителя получена"), 3000);
}

/**
 * @brief Настраивает секцию интерфейса для работы с shared memory.
 */
void MainWindow::initializeSharedMemoryUi()
{
    ui->read_lineEdit->setReadOnly(true);
    setSharedMemoryControlsReady(false);
}

/**
 * @brief Переключает доступность элементов управления shared memory.
 */
void MainWindow::setSharedMemoryControlsReady(const bool ready)
{
    ui->connection_button->setEnabled(!ready);

    ui->disconnection_button->setEnabled(ready);

    ui->write_button->setEnabled(ready);
    ui->write_lineEdit->setEnabled(ready);

    ui->read_button->setEnabled(ready);
}

/**
 * @brief Подключается к существующему сегменту разделяемой памяти.
 */
void MainWindow::on_connection_button_clicked()
{
    QString errorMessage;

    if (!sharedMemory_->attach(&errorMessage)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Shared memory"),
            errorMessage
        );
        return;
    }

    setSharedMemoryControlsReady(true);

    statusBar()->showMessage(
        QStringLiteral("Подключение к разделяемой памяти выполнено"),
        3000
    );
}

/**
 * @brief Отключается от сегмента разделяемой памяти.
 */
void MainWindow::on_disconnection_button_clicked()
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

    ui->write_lineEdit->clear();
    ui->read_lineEdit->clear();

    statusBar()->showMessage(
        QStringLiteral("Отключение от разделяемой памяти выполнено"),
        3000
    );
}

/**
 * @brief Записывает строку в разделяемую память.
 */
void MainWindow::on_write_button_clicked()
{
    QString errorMessage;

    if (!sharedMemory_->writeString(ui->write_lineEdit->text(), &errorMessage)) {
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

/**
 * @brief Считывает строку из разделяемой памяти и отображает её в интерфейсе.
 */
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
