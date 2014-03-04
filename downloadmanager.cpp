#include "downloadmanager.h"
#include "downloadwidget.h"

#include <QFileInfo>
#include <QDateTime>
#include <QDebug>


DownloadManager::DownloadManager(QObject *parent) :
    QObject(parent)
    , mManager(NULL)
    , mCurrentReply(NULL)
    , mFile(NULL)
    , mDownloadTotal(0)
    , bAcceptRanges(false)
    , mDownloadSize(0)
    , mDownloadSizeAtPause(0)
    , mTimer(NULL)
#if QT_VERSION < 0x050000
    , mFTP(NULL)
#endif
{
}

/*
// http://www.qtforum.org/article/14254/qftp-get-overloaded.html
int QFtp::get(const QString &file, uint offSet, QIODevice *dev, TransferType type)
{
    QStringList cmds;
    cmds << ("SIZE " + file + "\r\n");
    if (type == Binary)
    {
        cmds << "TYPE I\r\n";
    }
    else
    {
        cmds << "TYPE A\r\n";
    }
    cmds << (d_func()->transferMode == Passive ? "PASV\r\n" : "PORT\r\n");
    cmds << ("REST " + QString::number(offSet) + "\r\n");
    cmds << ("RETR " + file + "\r\n");
    return d_func()->addCommand(new QFtpCommand(Get, cmds, dev));
}
*/

void DownloadManager::download(QUrl url)
{
    qDebug() << "download: URL=" <<url.toString();

    mURL = url;
    {
        QFileInfo fileInfo(url.toString());
        mFileName = fileInfo.fileName();
    }
    mDownloadSize = 0;
    mDownloadSizeAtPause = 0;

#if QT_VERSION < 0x050000
    if (url.scheme().toLower() == "ftp")
    {
        mFTP = new QFtp(this);
        connect(mFTP, SIGNAL(stateChanged(int)), this, SLOT(ftpStateChanged(int)));
        connect(mFTP, SIGNAL(commandStarted(int)), this, SLOT(ftpCommandStarted(int)));
        connect(mFTP, SIGNAL(commandFinished(int,bool)), this, SLOT(ftpCommandFinished(int,bool)));
        connect(mFTP, SIGNAL(rawCommandReply(int, const QString &)), this, SLOT(ftpRawCommandReply(int, const QString &)));
        connect(mFTP, SIGNAL(dataTransferProgress(qint64,qint64)), this, SLOT(ftpDataTransferProgress(qint64,qint64)));
        connect(mFTP, SIGNAL(readyRead()), this, SLOT(ftpReadyRead()));

        qDebug() << "connectToHost(" << mURL.host() << "," << url.port(21) << ")";
        mFTP->connectToHost(mURL.host(), url.port(21));
        mFTP->login();
        qDebug() << "HELP";
        mHelpId = mFTP->rawCommand("HELP");
    }
    else
#endif
    {
        mManager = new QNetworkAccessManager(this);
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
    qDebug() << __FUNCTION__;

    mTimer->stop();
    mFile->close();
    QFile::remove(mFileName);
    mFile->rename(mFileName + ".part", mFileName);
    mFile = NULL;
    mCurrentReply = 0;
    emit downloadComplete();
}

void DownloadManager::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    mTimer->stop();
    mDownloadSize = mDownloadSizeAtPause + bytesReceived;
    qDebug() << "Download Progress: Received=" << mDownloadSize <<": Total=" << mDownloadSizeAtPause + bytesTotal;

    mFile->write(mCurrentReply->readAll());
    int percentage = ((mDownloadSizeAtPause + bytesReceived) * 100) / (mDownloadSizeAtPause + bytesTotal);
    qDebug() << percentage;
    emit progress(percentage);

    mTimer->start(5000);
}

void DownloadManager::error(QNetworkReply::NetworkError code)
{
    qDebug() << __FUNCTION__ << "(" << code << ")";
}

void DownloadManager::timeout()
{
    qDebug() << __FUNCTION__;
}

#if QT_VERSION < 0x050000

void DownloadManager::ftpStateChanged(int state)
{
    QString qsState;
    switch (static_cast<QFtp::State>(state))
    {
        case QFtp::Unconnected: qsState = "Unconnected"; break;
        case QFtp::HostLookup: qsState = "HostLookup"; break;
        case QFtp::Connecting: qsState = "Connecting"; break;
        case QFtp::Connected: qsState = "Connected"; break;
        case QFtp::LoggedIn: qsState = "LoggedIn"; break;
        case QFtp::Closing: qsState = "Closing"; break;
        default: qsState = "Unknown"; break;
    }

    qDebug() << __FUNCTION__ << "(" << state << "=" << qsState << ")";
}

void DownloadManager::ftpCommandStarted(int id)
{
    QString qsCmd;
    switch (static_cast<QFtp::Command>(id))
    {
        case QFtp::None: qsCmd = "None"; break;
        case QFtp::SetTransferMode: qsCmd = "SetTransferMode"; break;
        case QFtp::SetProxy: qsCmd = "SetProxy"; break;
        case QFtp::ConnectToHost: qsCmd = "ConnectToHost"; break;
        case QFtp::Login: qsCmd = "Login"; break;
        case QFtp::Close: qsCmd = "Close"; break;
        case QFtp::List: qsCmd = "List"; break;
        case QFtp::Cd: qsCmd = "Cd"; break;
        case QFtp::Get: qsCmd = "Get"; break;
        case QFtp::Put: qsCmd = "Put"; break;
        case QFtp::Remove: qsCmd = "Remove"; break;
        case QFtp::Mkdir: qsCmd = "Mkdir"; break;
        case QFtp::Rmdir: qsCmd = "Rmdir"; break;
        case QFtp::Rename: qsCmd = "Rename"; break;
        case QFtp::RawCommand: qsCmd = "RawCommand"; break;
        default: qsCmd = "Unknown"; break;
    }

    qDebug() << __FUNCTION__ << "(" << id << "=" << qsCmd << ")";
}

void DownloadManager::ftpCommandFinished(int id, bool error)
{
    QFtp::Command eCmd = static_cast<QFtp::Command>(id);
    QString qsCmd;
    switch (eCmd)
    {
        case QFtp::None: qsCmd = "None"; break;
        case QFtp::SetTransferMode: qsCmd = "SetTransferMode"; break;
        case QFtp::SetProxy: qsCmd = "SetProxy"; break;
        case QFtp::ConnectToHost: qsCmd = "ConnectToHost"; break;
        case QFtp::Login: qsCmd = "Login"; break;
        case QFtp::Close: qsCmd = "Close"; break;
        case QFtp::List: qsCmd = "List"; break;
        case QFtp::Cd: qsCmd = "Cd"; break;
        case QFtp::Get: qsCmd = "Get"; break;
        case QFtp::Put: qsCmd = "Put"; break;
        case QFtp::Remove: qsCmd = "Remove"; break;
        case QFtp::Mkdir: qsCmd = "Mkdir"; break;
        case QFtp::Rmdir: qsCmd = "Rmdir"; break;
        case QFtp::Rename: qsCmd = "Rename"; break;
        case QFtp::RawCommand: qsCmd = "RawCommand"; break;
        default: qsCmd = "Unknown"; break;
    }

    qDebug() << __FUNCTION__ << "(" << id << "=" << qsCmd << "," << error << ")";

    if ((eCmd == QFtp::Get) || (eCmd == QFtp::Login))
    {
/*
        mFile->close();
        QFile::remove(mFileName);
        mFile->rename(mFileName + ".part", mFileName);
        mFile = NULL;
        emit downloadComplete();
*/
    }
    else if (eCmd == QFtp::ConnectToHost)
    {
/*
        mFTP->rawCommand(QString("RETR " + mFileName));
        mFTP->rawCommand("REST 1000"); // 68,741,120 bytes
        qDebug() << "get(" << mFileName << ")";
        mFTP->get(mFileName);
*/
/*
        qDebug() << "REST 1000";
        mFTP->rawCommand("REST 1000"); // 68,741,120 bytes
        qDebug() << "RETR " << mFileName;
        mFTP->rawCommand(QString("RETR " + mFileName));
*/
    }
}

void DownloadManager::ftpRawCommandReply(int replyCode, const QString &detail)
{
    qDebug() << __FUNCTION__ << "(" << replyCode << "," << detail << ")";

    switch (replyCode)
    {
        case 213: // SIZE
        {
            mDownloadTotal = detail.toInt();

            if (bAcceptRanges && (mDownloadSizeAtPause > 0))
            {
/*
                qDebug() << "REST 1000";
                mFTP->rawCommand("REST 1000"); // 68,741,120 bytes
                qDebug() << "RETR " << mFileName;
                mFTP->rawCommand(QString("RETR " + mFileName));
                qDebug() << "get(" << mFileName << ")";
                mFTP->get(mFileName);
*/
                qDebug() << "TYPE I";
                mFTP->rawCommand("TYPE I"); // Binary mode
//                qDebug() << "PASV";
//                mFTP->rawCommand("PASV");
                qDebug() << "PORT";
                mFTP->rawCommand("PORT"); // Active mode
                qDebug() << "REST " << mDownloadSizeAtPause;
                mFTP->rawCommand(QString("REST %1").arg(mDownloadSizeAtPause));
                qDebug() << "RETR " << mFileName;
                mFTP->rawCommand(QString("RETR " + mFileName));
            }
            else
            {
                mFTP->get(mFileName);
            }
        }
        break;

        case 214: // HELP
        {
            if (detail.contains(QLatin1String("REST"), Qt::CaseSensitive))
            {
                bAcceptRanges = true;
            }

            ftpFinishedHelp();
        }
        break;

        case 226: // RETR
        {
            mFile->close();
            QFile::remove(mFileName);
            mFile->rename(mFileName + ".part", mFileName);
            mFile = NULL;
            emit downloadComplete();
        }
        break;
    }
}

void DownloadManager::ftpDataTransferProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal == 0xCDCDCDCDCDCDCDCD)
    {
        return;
    }

    qDebug() << __FUNCTION__ << "(" << bytesReceived << "," << bytesTotal << ")";

/*
    if (bytesReceived < 0)
    {
        bytesReceived &= 0x7FFFFFFFFFFFFFFF;
    }
    if (bytesTotal < 0)
    {
        bytesTotal &= 0x7FFFFFFFFFFFFFFF;
    }
*/
    // 0x418E800 -> 7FFFFFF
    // 0x64FD2F8
/*
    bytesReceived &= 0x7FFFFFF;
    bytesTotal &= 0x7FFFFFF;
*/
/*
    if (bytesReceived < 0)
    {
        bytesReceived = -bytesReceived;
    }
    if (bytesTotal < 0)
    {
        bytesTotal = -bytesTotal;
    }
*/
    // CDCDCDCDCDCDDB5D -> 56153
    // CDCDCDCDCDCDE8ED -> 59629
    // CDCDCDCDCDCDF67D -> 63101

    // CDCDCDCDD0A88006 -> 3500703750
    //          418E800 ->   68741120

    if (bytesTotal == 0xCDCDCDCDCDCDCDCD)
    {
        bytesTotal = mDownloadTotal - mDownloadSizeAtPause;
    }

    mDownloadSize = mDownloadSizeAtPause + bytesReceived;
    qDebug() << "Download Progress: Received=" << mDownloadSize <<": Total=" << mDownloadSizeAtPause + bytesTotal;

    int percentage = ((mDownloadSizeAtPause + bytesReceived) * 100) / (mDownloadSizeAtPause + bytesTotal);
    qDebug() << percentage;
    emit progress(percentage);
}


void DownloadManager::ftpReadyRead()
{
//    qDebug() << __FUNCTION__ << "()";

    QByteArray data = mFTP->readAll();
    if (data.size() == 0)
    {
        return;
    }

    mFile->write(data);

    if (mDownloadSize == 0)
    {
        mDownloadSize = mDownloadSizeAtPause;
    }
    mDownloadSize += data.size();

    qDebug() << "Download Progress: Received=" << mDownloadSize <<": Total=" << mDownloadTotal;

    int percentage = static_cast<int>((static_cast<float>(mDownloadSize) * 100.0) / static_cast<float>(mDownloadTotal));
    qDebug() << percentage;
    emit progress(percentage);
}


void DownloadManager::ftpFinishedHelp()
{
    // From finishedHead()
    mFile = new QFile(mFileName + ".part");
    if (!bAcceptRanges)
    {
        mFile->remove();
    }
    if (!mFile->open(QIODevice::ReadWrite | QIODevice::Append))
    {
        qDebug() << "Failed to open file " << mFileName << ".part";
    }

    mDownloadSizeAtPause = mFile->size();
    // End finishedHead()+

    if (!mURL.path().isEmpty())
    {
        QFileInfo fileInfo(mURL.path());
        qDebug() << "cd(" << fileInfo.path() << ")";
        mFTP->cd(fileInfo.path());
    }

    qDebug() << "SIZE " << mFileName;
    mFTP->rawCommand("SIZE " + mFileName);
}

#endif // QT_VERSION < 0x050000
