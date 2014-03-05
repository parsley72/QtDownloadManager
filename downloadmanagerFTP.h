#ifndef DOWNLOADMANAGERFTP_H
#define DOWNLOADMANAGERFTP_H

#include <QtGlobal>

#if QT_VERSION < 0x050000

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QTimer>
#include <QFtp>


class DownloadManagerFTP : public QObject
{
    Q_OBJECT

public:
    explicit DownloadManagerFTP(QObject *parent = 0);
    virtual ~DownloadManagerFTP();

signals:
    void addLine(QString qsLine);
    void downloadComplete();
    void progress(int nPercentage);

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
//    void ftpDataTransferProgress(qint64 bytesReceived, qint64 bytesTotal);
    void ftpReadyRead();

private:
    void ftpFinishedHelp();

    QUrl _URL;
    QString _qsFileName;
    QNetworkAccessManager* _pManager;
    QNetworkRequest _CurrentRequest;
    QNetworkReply* _pCurrentReply;
    QFile* _pFile;
    int _nDownloadTotal;
    bool _bAcceptRanges;
    int _nDownloadSize;
    int _nDownloadSizeAtPause;
    QFtp *_pFTP;
    QTimer _Timer;
};

#endif // QT_VERSION < 0x050000

#endif // DOWNLOADMANAGERFTP_H
