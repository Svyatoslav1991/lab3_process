#pragma once

#include <QMainWindow>
#include <memory>

class SharedMemoryChannel;

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
    void on_connection_button_clicked();
    void on_disconnection_button_clicked();
    void on_write_button_clicked();
    void on_read_button_clicked();

private:
    void initializeUi();
    static QString normalizeLine(QString text);
    void initializeSharedMemoryUi();
    void setSharedMemoryControlsReady(bool ready);

private:
    Ui::MainWindow *ui = nullptr;

    std::unique_ptr<SharedMemoryChannel> sharedMemory_;
};
