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
    void finished();
    void error(QNetworkReply::NetworkError code);
    void timeout();
    void stateChanged(int state);
    void commandStarted(int id);
    void commandFinished(int id, bool error);
    void rawCommandReply(int replyCode, const QString & detail);
    void readyRead();

private:
    static QString ftpCmd(QFtp::Command eCmd);
    void ftpFinishedHelp();

    QUrl _URL;
    QString _qsFileName;
    QFtp *_pFTP;
    QFile* _pFile;
    int _nDownloadTotal;
    bool _bAcceptRanges;
    int _nDownloadSize;
    int _nDownloadSizeAtPause;
    QTimer _Timer;
};

#endif // QT_VERSION < 0x050000

#endif // DOWNLOADMANAGERFTP_H
