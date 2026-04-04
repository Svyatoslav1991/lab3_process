#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_sendData_button_clicked();
    void on_dataReception_button_clicked();

private:
    void initializeUi();
    static QString normalizeLine(QString text);

private:
    Ui::MainWindow *ui = nullptr;
};
