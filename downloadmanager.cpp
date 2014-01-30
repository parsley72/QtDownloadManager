#include "downloadmanager.h"

#include <QDebug>

DownloadManager::DownloadManager(QObject *parent) :
    QObject(parent),mCurrentReply(0),mFile(0),mDownloadSizeAtPause(0)
{
    mManager = new QNetworkAccessManager( this );
}

void DownloadManager::download( QUrl url )
{
    qDebug() << "download: URL=" <<url.toString();

    mDownloadSizeAtPause =0;
    mCurrentRequest = QNetworkRequest(url);
    mFile = new QFile("download.part");
    mFile->open(QIODevice::ReadWrite);

    download(mCurrentRequest);
}

void DownloadManager::pause()
{
    qDebug() << "pause()";
    if( mCurrentReply == 0 ) {
        return;
    }
    disconnect(mCurrentReply,SIGNAL(finished()),this,SLOT(finished()));
    disconnect(mCurrentReply,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(downloadProgress(qint64,qint64)));
    disconnect(mCurrentReply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(error(QNetworkReply::NetworkError)));

    mCurrentReply->abort();
    mFile->write( mCurrentReply->readAll());
    mCurrentReply = 0;
}

void DownloadManager::resume()
{
    qDebug() << "resume()";

    mDownloadSizeAtPause = mFile->size();
    QByteArray rangeHeaderValue = "bytes=" + QByteArray::number(mDownloadSizeAtPause) + "-";
    mCurrentRequest.setRawHeader("Range",rangeHeaderValue);

    download(mCurrentRequest);
}

void DownloadManager::download( QNetworkRequest& request )
{
    qDebug() << "download( QNetworkRequest& request )";
    mCurrentReply = mManager->get(request);

    connect(mCurrentReply,SIGNAL(finished()),this,SLOT(finished()));
    connect(mCurrentReply,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(downloadProgress(qint64,qint64)));
    connect(mCurrentReply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(error(QNetworkReply::NetworkError)));
}

void DownloadManager::finished()
{
    qDebug() << "finihsed";
    mFile->close();
    mFile->rename("download.part","download.complete");
    mFile = 0;
    mCurrentReply = 0;
    emit downloadComplete();
}

void DownloadManager::downloadProgress ( qint64 bytesReceived, qint64 bytesTotal )
{
    qDebug() << "Download Progress: Received=" << mDownloadSizeAtPause+bytesReceived <<": Total=" << mDownloadSizeAtPause+bytesTotal;

    mFile->write( mCurrentReply->readAll() );
    int percentage = ((mDownloadSizeAtPause+bytesReceived) * 100 )/ (mDownloadSizeAtPause+bytesTotal);
    qDebug() << percentage;
    emit progress(percentage);
}

void DownloadManager::error(QNetworkReply::NetworkError code)
{
    qDebug() << "Error:"<<code;
}
