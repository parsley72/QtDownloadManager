#include "DownloadManagerFTP.h"
#include "downloadwidget.h"

#include <QtGlobal>

#if QT_VERSION < 0x050000

#include <QFileInfo>
#include <QDateTime>
#include <QDebug>


DownloadManagerFTP::DownloadManagerFTP(QObject *parent) :
    QObject(parent)
    , _pManager(NULL)
    , _pCurrentReply(NULL)
    , _pFile(NULL)
    , _nDownloadTotal(0)
    , _bAcceptRanges(false)
    , _nDownloadSize(0)
    , _nDownloadSizeAtPause(0)
    , _pFTP(NULL)
{
}


DownloadManagerFTP::~DownloadManagerFTP()
{
    if (_pFTP != NULL)
    {
        pause();

        _pFTP->deleteLater();
        _pFTP = NULL;
    }

    if (_pFile->isOpen())
    {
        _pFile->close();
        delete _pFile;
        _pFile = NULL;
    }
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


void DownloadManagerFTP::download(QUrl url)
{
    qDebug() << "download: URL=" <<url.toString();

    _URL = url;
    {
        QFileInfo fileInfo(url.toString());
        _qsFileName = fileInfo.fileName();
    }
    _nDownloadSize = 0;
    _nDownloadSizeAtPause = 0;

    if (url.scheme().toLower() == "ftp")
    {
        _pFTP = new QFtp(this);
        connect(_pFTP, SIGNAL(stateChanged(int)), this, SLOT(ftpStateChanged(int)));
        connect(_pFTP, SIGNAL(commandStarted(int)), this, SLOT(ftpCommandStarted(int)));
        connect(_pFTP, SIGNAL(commandFinished(int,bool)), this, SLOT(ftpCommandFinished(int,bool)));
        connect(_pFTP, SIGNAL(rawCommandReply(int, const QString &)), this, SLOT(ftpRawCommandReply(int, const QString &)));
        //connect(_pFTP, SIGNAL(dataTransferProgress(qint64,qint64)), this, SLOT(ftpDataTransferProgress(qint64,qint64)));
        connect(_pFTP, SIGNAL(readyRead()), this, SLOT(ftpReadyRead()));

        qDebug() << "connectToHost(" << _URL.host() << "," << url.port(21) << ")";
        _pFTP->connectToHost(_URL.host(), url.port(21));
        _pFTP->login();
        qDebug() << "HELP";
        _pFTP->rawCommand("HELP");
    }
    else
    {
        _pManager = new QNetworkAccessManager(this);
        _CurrentRequest = QNetworkRequest(url);

        _pCurrentReply = _pManager->head(_CurrentRequest);

        connect(_pCurrentReply, SIGNAL(finished()), this, SLOT(finishedHead()));
        connect(_pCurrentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
    }
}


void DownloadManagerFTP::pause()
{
    qDebug() << "pause() = " << _nDownloadSize;
    if (_pCurrentReply == 0)
    {
        return;
    }
    disconnect(_pCurrentReply,SIGNAL(finished()),this,SLOT(finished()));
    disconnect(_pCurrentReply,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(downloadProgress(qint64,qint64)));
    disconnect(_pCurrentReply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(error(QNetworkReply::NetworkError)));
    _Timer.stop();

    _pCurrentReply->abort();
//    _pFile->write( _pCurrentReply->readAll());
    _pFile->flush();
    _pCurrentReply = 0;
    _nDownloadSizeAtPause = _nDownloadSize;
    _nDownloadSize = 0;
}


void DownloadManagerFTP::resume()
{
    qDebug() << "resume() = " << _nDownloadSizeAtPause;

    download();
}


void DownloadManagerFTP::download()
{
    qDebug() << "download()";

    if (_bAcceptRanges)
    {
        QByteArray rangeHeaderValue = "bytes=" + QByteArray::number(_nDownloadSizeAtPause) + "-";
        if (_nDownloadTotal > 0)
        {
            rangeHeaderValue += QByteArray::number(_nDownloadTotal);
        }
        _CurrentRequest.setRawHeader("Range", rangeHeaderValue);
    }

    _pCurrentReply = _pManager->get(_CurrentRequest);


    connect(_pCurrentReply, SIGNAL(finished()), this, SLOT(finished()));
    connect(_pCurrentReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
    connect(_pCurrentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
}


void DownloadManagerFTP::finishedHead()
{
    _pTimer->stop();
    _bAcceptRanges = false;

    QList<QByteArray> list = _pCurrentReply->rawHeaderList();
    foreach (QByteArray header, list)
    {
        QString qsLine = QString(header) + " = " + _pCurrentReply->rawHeader(header);
        addLine(qsLine);
    }

    if (_pCurrentReply->hasRawHeader("Accept-Ranges"))
    {
        QString qstrAcceptRanges = _pCurrentReply->rawHeader("Accept-Ranges");
        _bAcceptRanges = (qstrAcceptRanges.compare("bytes", Qt::CaseInsensitive) == 0);
        qDebug() << "Accept-Ranges = " << qstrAcceptRanges << _bAcceptRanges;
    }

    _nDownloadTotal = _pCurrentReply->header(QNetworkRequest::ContentLengthHeader).toInt();

//    _CurrentRequest = QNetworkRequest(url);
    _CurrentRequest.setRawHeader("Connection", "Keep-Alive");
    _CurrentRequest.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    _pFile = new QFile(_qsFileName + ".part");
    if (!_bAcceptRanges)
    {
        _pFile->remove();
    }
    _pFile->open(QIODevice::ReadWrite | QIODevice::Append);

    _nDownloadSizeAtPause = _pFile->size();
    download();
}


void DownloadManagerFTP::finished()
{
    qDebug() << __FUNCTION__;

    _Timer.stop();
    _pFile->close();
    QFile::remove(_qsFileName);
    _pFile->rename(_qsFileName + ".part", _qsFileName);
    _pFile = NULL;
    _pCurrentReply = 0;
    emit downloadComplete();
}


void DownloadManagerFTP::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    _nDownloadSize = _nDownloadSizeAtPause + bytesReceived;
    qDebug() << "Download Progress: Received=" << _nDownloadSize <<": Total=" << _nDownloadSizeAtPause + bytesTotal;

    _pFile->write(_pCurrentReply->readAll());
    int percentage = ((_nDownloadSizeAtPause + bytesReceived) * 100) / (_nDownloadSizeAtPause + bytesTotal);
    qDebug() << percentage;
    emit progress(percentage);

}


void DownloadManagerFTP::error(QNetworkReply::NetworkError code)
{
    qDebug() << __FUNCTION__ << "(" << code << ")";
}


void DownloadManagerFTP::timeout()
{
    qDebug() << __FUNCTION__;
}


void DownloadManagerFTP::ftpStateChanged(int state)
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


void DownloadManagerFTP::ftpCommandStarted(int id)
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


void DownloadManagerFTP::ftpCommandFinished(int id, bool error)
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
        _pFile->close();
        QFile::remove(_qsFileName);
        _pFile->rename(_qsFileName + ".part", _qsFileName);
        _pFile = NULL;
        emit downloadComplete();
*/
    }
    else if (eCmd == QFtp::ConnectToHost)
    {
/*
        _pFTP->rawCommand(QString("RETR " + _qsFileName));
        _pFTP->rawCommand("REST 1000"); // 68,741,120 bytes
        qDebug() << "get(" << _qsFileName << ")";
        _pFTP->get(_qsFileName);
*/
/*
        qDebug() << "REST 1000";
        _pFTP->rawCommand("REST 1000"); // 68,741,120 bytes
        qDebug() << "RETR " << _qsFileName;
        _pFTP->rawCommand(QString("RETR " + _qsFileName));
*/
    }
}


void DownloadManagerFTP::ftpRawCommandReply(int replyCode, const QString &detail)
{
    qDebug() << __FUNCTION__ << "(" << replyCode << "," << detail << ")";

    switch (replyCode)
    {
        case 213: // SIZE
        {
            _nDownloadTotal = detail.toInt();

            if (_bAcceptRanges && (_nDownloadSizeAtPause > 0))
            {
/*
                qDebug() << "REST 1000";
                _pFTP->rawCommand("REST 1000"); // 68,741,120 bytes
                qDebug() << "RETR " << _qsFileName;
                _pFTP->rawCommand(QString("RETR " + _qsFileName));
                qDebug() << "get(" << _qsFileName << ")";
                _pFTP->get(_qsFileName);
*/
                qDebug() << "TYPE I";
                _pFTP->rawCommand("TYPE I"); // Binary mode
//                qDebug() << "PASV";
//                _pFTP->rawCommand("PASV");
                qDebug() << "PORT";
                _pFTP->rawCommand("PORT"); // Active mode
                qDebug() << "REST " << _nDownloadSizeAtPause;
                _pFTP->rawCommand(QString("REST %1").arg(_nDownloadSizeAtPause));
                qDebug() << "RETR " << _qsFileName;
                _pFTP->rawCommand(QString("RETR " + _qsFileName));
            }
            else
            {
                _pFTP->get(_qsFileName);
            }
        }
        break;

        case 214: // HELP
        {
            if (detail.contains(QLatin1String("REST"), Qt::CaseSensitive))
            {
                _bAcceptRanges = true;
            }

            ftpFinishedHelp();
        }
        break;

        case 226: // RETR
        {
            if (_pFile->isOpen())
            {
                _pFile->close();
                QFile::remove(_qsFileName);
                _pFile->rename(_qsFileName + ".part", _qsFileName);
                _pFile = NULL;
                emit downloadComplete();
            }
        }
        break;
    }
}


#if 0
void DownloadManagerFTP::ftpDataTransferProgress(qint64 bytesReceived, qint64 bytesTotal)
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
        bytesTotal = _nDownloadTotal - _nDownloadSizeAtPause;
    }

    _nDownloadSize = _nDownloadSizeAtPause + bytesReceived;
    qDebug() << "Download Progress: Received=" << _nDownloadSize <<": Total=" << _nDownloadSizeAtPause + bytesTotal;

    int percentage = ((_nDownloadSizeAtPause + bytesReceived) * 100) / (_nDownloadSizeAtPause + bytesTotal);
    qDebug() << percentage;
    emit progress(percentage);
}
#endif // 0


void DownloadManagerFTP::ftpReadyRead()
{
//    qDebug() << __FUNCTION__ << "()";

    QByteArray data = _pFTP->readAll();
    if (data.size() == 0)
    {
        return;
    }

    _pFile->write(data);

    if (_nDownloadSize == 0)
    {
        _nDownloadSize = _nDownloadSizeAtPause;
    }
    _nDownloadSize += data.size();

    qDebug() << "Download Progress: Received=" << _nDownloadSize <<": Total=" << _nDownloadTotal;

    int nPercentage = static_cast<int>((static_cast<float>(_nDownloadSize) * 100.0) / static_cast<float>(_nDownloadTotal));
    qDebug() << nPercentage;
    emit progress(nPercentage);

    if (_nDownloadSize == _nDownloadTotal)
    {
        if (_pFile->isOpen())
        {
            _pFile->close();
            QFile::remove(_qsFileName);
            _pFile->rename(_qsFileName + ".part", _qsFileName);
            _pFile = NULL;
            emit downloadComplete();
        }
    }
}


void DownloadManagerFTP::ftpFinishedHelp()
{
    // From finishedHead()
    _pFile = new QFile(_qsFileName + ".part");
    if (!_bAcceptRanges)
    {
        _pFile->remove();
    }
    if (!_pFile->open(QIODevice::ReadWrite | QIODevice::Append))
    {
        qDebug() << "Failed to open file " << _qsFileName << ".part";
    }

    _nDownloadSizeAtPause = _pFile->size();
    // End finishedHead()+

    if (!_URL.path().isEmpty())
    {
        QFileInfo fileInfo(_URL.path());
        qDebug() << "cd(" << fileInfo.path() << ")";
        _pFTP->cd(fileInfo.path());
    }

    qDebug() << "SIZE " << _qsFileName;
    _pFTP->rawCommand("SIZE " + _qsFileName);
}

#endif // QT_VERSION < 0x050000
