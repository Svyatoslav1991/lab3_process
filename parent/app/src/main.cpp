#include <QApplication>
#include <QMainWindow>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QMainWindow w;
    w.setWindowTitle("Parent");
    w.show();

    return a.exec();
}
