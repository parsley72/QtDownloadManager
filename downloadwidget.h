#ifndef DOWNLOADWIDGET_H
#define DOWNLOADWIDGET_H

#include <QWidget>
#include "downloadmanager.h"

class QProgressBar;
class QPushButton;

class DownloadWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DownloadWidget(QWidget *parent = 0);

public slots:

    void download();

    void pause();

    void resume();

private slots:

    void finished();

    void progress(int percentage);

    void setupUi();

private:

    DownloadManager* mManager;
    QProgressBar *mProgressBar;
    QPushButton* mDownloadBtn;
    QPushButton* mPauseBtn;
    QPushButton* mResumeBtn;
};

#endif // DOWNLOADWIDGET_H
