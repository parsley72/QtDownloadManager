#include <QApplication>

#include "downloadwidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    DownloadWidget wid;
    wid.show();
    return a.exec();
}
