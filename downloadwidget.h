#ifndef DOWNLOADWIDGET_H
#define DOWNLOADWIDGET_H

#include <QWidget>
#include "downloadmanager.h"

namespace Ui {
class Form;
}

//class QProgressBar;
//class QPushButton;

class DownloadWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DownloadWidget(QWidget *parent = 0);

private slots:
    void addLine(QString qsLine);
    void finished();
    void progress(int percentage);

    void on_downloadBtn_clicked();
    void on_pauseBtn_clicked();
    void on_resumeBtn_clicked();

private:
    Ui::Form *ui;
    DownloadManager* mManager;
};

#endif // DOWNLOADWIDGET_H
