#include <QtGlobal>

#if QT_VERSION >= 0x050000
    #include <QtWidgets/QApplication>
#else
    #include <QApplication>
#endif

#include "downloadwidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    DownloadWidget wid;
    wid.show();
    return a.exec();
}
