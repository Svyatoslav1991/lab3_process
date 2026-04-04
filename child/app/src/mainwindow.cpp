#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QStatusBar>

#include <cstdio>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initializeUi();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initializeUi()
{
    setWindowTitle(QStringLiteral("Child"));

    ui->dataReception_lineEdit->setReadOnly(true);
    ui->dataReception_button->setEnabled(true);

    // Эти элементы задействуем позже, на этапе shared memory.
    ui->connection_button->setEnabled(false);
    ui->disconnection_button->setEnabled(false);
    ui->read_button->setEnabled(false);
    ui->write_button->setEnabled(false);
    ui->read_lineEdit->setEnabled(false);
    ui->write_lineEdit->setEnabled(false);

    statusBar()->showMessage(QStringLiteral("Child готов"), 3000);
}

QString MainWindow::normalizeLine(QString text)
{
    while (!text.isEmpty() && (text.endsWith('\n') || text.endsWith('\r'))) {
        text.chop(1);
    }

    return text;
}

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
