#include <QApplication>
#include <QMainWindow>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QMainWindow w;
    w.setWindowTitle("Child");
    w.show();

    return a.exec();
}
