#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QtGlobal>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QTimer>

#if QT_VERSION < 0x050000
#include <QFtp>
#endif


class DownloadManager : public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = 0);

signals:
    void addLine(QString qsLine);

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

    void ftpStateChanged(int state);
    void ftpCommandStarted(int id);
    void ftpCommandFinished(int id, bool error);
    void ftpRawCommandReply(int replyCode, const QString & detail);
    void ftpDataTransferProgress(qint64 bytesReceived, qint64 bytesTotal);
    void ftpReadyRead();

private:
    void ftpFinishedHelp();

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
#if QT_VERSION < 0x050000
    QFtp *mFTP;
#endif
};

#endif // DOWNLOADMANAGER_H
