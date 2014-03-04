#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QtGlobal>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QTimer>

#include "downloadmanagerHTTP.h"

#if QT_VERSION < 0x050000
#include <QFtp>
#include "downloadmanagerFTP.h"
#endif


class DownloadManager : public QObject
{
    Q_OBJECT

public:
    explicit DownloadManager(QObject *parent = 0);

signals:
    void addLine(QString qsLine);
    void progress(int nPercentage);
    void downloadComplete();

public slots:
    void download(QUrl url);
    void pause();
    void resume();

private slots:
    void localAddLine(QString qsLine);
    void localProgress(int nPercentage);
    void localDownloadComplete();

private:
    QUrl _URL;
    DownloadManagerHTTP *_pHTTP;
#if QT_VERSION < 0x050000
    DownloadManagerFTP *_pFTP;
#endif
};

#endif // DOWNLOADMANAGER_H
