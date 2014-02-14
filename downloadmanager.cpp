#include "downloadmanager.h"
#include "downloadwidget.h"

#include <QFileInfo>
#include <QDateTime>
#include <QDebug>


DownloadManager::DownloadManager(QObject *parent) :
    QObject(parent),
    mCurrentReply(0),
    mFile(0),
    mDownloadTotal(0),
    bAcceptRanges(false),
    mDownloadSize(0),
    mDownloadSizeAtPause(0)
{
    mManager = new QNetworkAccessManager( this );
}

void DownloadManager::download( QUrl url )
{
    qDebug() << "download: URL=" <<url.toString();

    mURL = url;
    {
        QFileInfo fileInfo(url.toString());
        mFileName = fileInfo.fileName();
    }
    mDownloadSize = 0;
    mDownloadSizeAtPause = 0;
    mCurrentRequest = QNetworkRequest(url);

    mCurrentReply = mManager->head(mCurrentRequest);

    mTimer = new QTimer(this);
    mTimer->setInterval(5000);
    mTimer->setSingleShot(true);
    connect(mTimer, SIGNAL(timeout()), this, SLOT(timeout()));
    mTimer->start();

    connect(mCurrentReply,SIGNAL(finished()),this,SLOT(finishedHead()));
    connect(mCurrentReply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(error(QNetworkReply::NetworkError)));
}

void DownloadManager::pause()
{
    qDebug() << "pause() = " << mDownloadSize;
    if( mCurrentReply == 0 ) {
        return;
    }
    mTimer->stop();
    disconnect(mTimer, SIGNAL(timeout()), this, SLOT(timeout()));
    disconnect(mCurrentReply,SIGNAL(finished()),this,SLOT(finished()));
    disconnect(mCurrentReply,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(downloadProgress(qint64,qint64)));
    disconnect(mCurrentReply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(error(QNetworkReply::NetworkError)));

    mCurrentReply->abort();
//    mFile->write( mCurrentReply->readAll());
    mFile->flush();
    mCurrentReply = 0;
    mDownloadSizeAtPause = mDownloadSize;
    mDownloadSize = 0;
}

void DownloadManager::resume()
{
    qDebug() << "resume() = " << mDownloadSizeAtPause;

    download();
}

void DownloadManager::download()
{
    qDebug() << "download()";

    if (bAcceptRanges)
    {
        QByteArray rangeHeaderValue = "bytes=" + QByteArray::number(mDownloadSizeAtPause) + "-";
        if (mDownloadTotal > 0)
        {
            rangeHeaderValue += QByteArray::number(mDownloadTotal);
        }
        mCurrentRequest.setRawHeader("Range", rangeHeaderValue);
    }

    mCurrentReply = mManager->get(mCurrentRequest);

    mTimer->setInterval(5000);
    mTimer->setSingleShot(true);
    connect(mTimer, SIGNAL(timeout()), this, SLOT(timeout()));
    mTimer->start();

    connect(mCurrentReply,SIGNAL(finished()),this,SLOT(finished()));
    connect(mCurrentReply,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(downloadProgress(qint64,qint64)));
    connect(mCurrentReply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(error(QNetworkReply::NetworkError)));
}

void DownloadManager::finishedHead()
{
    mTimer->stop();
    bAcceptRanges = false;

    QList<QByteArray> list = mCurrentReply->rawHeaderList();
    foreach (QByteArray header, list)
    {
        QString qsLine = QString(header) + " = " + mCurrentReply->rawHeader(header);
        addLine(qsLine);
    }

    if (mCurrentReply->hasRawHeader("Accept-Ranges"))
    {
        QString qstrAcceptRanges = mCurrentReply->rawHeader("Accept-Ranges");
        bAcceptRanges = (qstrAcceptRanges.compare("bytes", Qt::CaseInsensitive) == 0);
        qDebug() << "Accept-Ranges = " << qstrAcceptRanges << bAcceptRanges;
    }

    mDownloadTotal = mCurrentReply->header(QNetworkRequest::ContentLengthHeader).toInt();

//    mCurrentRequest = QNetworkRequest(url);
    mCurrentRequest.setRawHeader("Connection", "Keep-Alive");
    mCurrentRequest.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    mFile = new QFile(mFileName + ".part");
    if (!bAcceptRanges)
    {
        mFile->remove();
    }
    mFile->open(QIODevice::ReadWrite | QIODevice::Append);

    mDownloadSizeAtPause = mFile->size();
    download();
}

void DownloadManager::finished()
{
    qDebug() << "finished";
    mTimer->stop();
    mFile->close();
    QFile::remove(mFileName);
    mFile->rename(mFileName + ".part", mFileName);
    mFile = 0;
    mCurrentReply = 0;
    emit downloadComplete();
}

void DownloadManager::downloadProgress ( qint64 bytesReceived, qint64 bytesTotal )
{
    mTimer->stop();
    mDownloadSize = mDownloadSizeAtPause + bytesReceived;
    qDebug() << "Download Progress: Received=" << mDownloadSize <<": Total=" << mDownloadSizeAtPause+bytesTotal;

    mFile->write( mCurrentReply->readAll() );
    int percentage = ((mDownloadSizeAtPause+bytesReceived) * 100 )/ (mDownloadSizeAtPause+bytesTotal);
    qDebug() << percentage;
    emit progress(percentage);

    mTimer->start(5000);
//    mTimer->singleShot(5000, this, SLOT(timeout()));
}

void DownloadManager::error(QNetworkReply::NetworkError code)
{
    qDebug() << "Error:" << code;
}

void DownloadManager::timeout()
{
    qDebug() << "Timeout";
}
