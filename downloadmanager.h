#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QTimer>

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = 0);

signals:
    void downloadComplete();

    void progress( int percentage);

public slots:

    void download(QUrl url);

    void pause();

    void resume();

private slots:

    void download();

    void finishedHead();

    void finished();

    void downloadProgress ( qint64 bytesReceived, qint64 bytesTotal );

    void error ( QNetworkReply::NetworkError code );

    void timeout();

private:
    QUrl mURL;
    QString mFileName;
    QNetworkAccessManager* mManager;
    QNetworkRequest mCurrentRequest;
    QNetworkReply* mCurrentReply;
    QFile* mFile;
    int mDownloadTotal;
    bool bAcceptRanges;
    int mDownloadSize;
    int mDownloadSizeAtPause;
    QTimer *mTimer;
};

#endif // DOWNLOADMANAGER_H
