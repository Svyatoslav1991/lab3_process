#pragma once

#include <QMainWindow>
#include <QProcess>
#include <QStringList>

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
    void on_startDetached_button_clicked();
    void on_startDetachedAndParams_button_clicked();

    void on_execute_button_clicked();
    void on_executeAndParams_button_clicked();

    void on_start_button_clicked();
    void on_kill_button_clicked();

    void onChildStarted();
    void onChildFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onChildError(QProcess::ProcessError error);

private:
    void initializeUi();
    void connectProcessSignals();

    void launchDetached(const QStringList& arguments);
    void executeProgram(const QStringList& arguments);
    void startInteractiveProcess();

    QString programText() const;
    QStringList programArguments() const;
    bool validateProgram() const;

    void setProcessControlsRunning(bool running);
    void clearExitInfo();
    void updateExitInfo(int exitCode, QProcess::ExitStatus exitStatus);

    static QString exitStatusToString(QProcess::ExitStatus exitStatus);
    static QString processErrorToString(QProcess::ProcessError error);

private:
    Ui::MainWindow *ui = nullptr;
    QProcess *process_ = nullptr;
};
