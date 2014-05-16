#include "downloadmanagerFTP.h"
#include "downloadwidget.h"

#include <QtGlobal>

#if QT_VERSION < 0x050000

#include <QFileInfo>
#include <QDateTime>
#include <QDebug>


DownloadManagerFTP::DownloadManagerFTP(QObject *parent) :
    QObject(parent)
    , _pFTP(NULL)
    , _pFile(NULL)
    , _nDownloadTotal(0)
    , _bAcceptRanges(false)
    , _nDownloadSize(0)
    , _nDownloadSizeAtPause(0)
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

    if (_pFTP == NULL)
    {
        _pFTP = new QFtp(this);

        connect(_pFTP, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));
        connect(_pFTP, SIGNAL(commandStarted(int)), this, SLOT(commandStarted(int)));
        connect(_pFTP, SIGNAL(commandFinished(int,bool)), this, SLOT(commandFinished(int,bool)));
        connect(_pFTP, SIGNAL(rawCommandReply(int, const QString &)), this, SLOT(rawCommandReply(int, const QString &)));
        //connect(_pFTP, SIGNAL(dataTransferProgress(qint64,qint64)), this, SLOT(ftpDataTransferProgress(qint64,qint64)));
        connect(_pFTP, SIGNAL(readyRead()), this, SLOT(readyRead()));
    }

    qDebug() << "connectToHost(" << _URL.host() << "," << url.port(21) << ")";
    _pFTP->connectToHost(_URL.host(), url.port(21));
    _pFTP->login();
    qDebug() << "HELP";
    _pFTP->rawCommand("HELP");

    _Timer.setInterval(15000);
    _Timer.setSingleShot(true);
    connect(&_Timer, SIGNAL(timeout()), this, SLOT(timeout()));
    _Timer.start();
}


void DownloadManagerFTP::pause()
{
    qDebug() << __FUNCTION__ << "(): _nDownloadSize = " << _nDownloadSize;

    _Timer.stop();

    //disconnect(_pFTP, SIGNAL(dataTransferProgress(qint64,qint64)), this, SLOT(ftpDataTransferProgress(qint64,qint64)));
    //disconnect(_pFTP, SIGNAL(readyRead()), this, SLOT(readyRead()));

    _pFTP->abort();
//    qDebug() << "ABOR";
//    _pFTP->rawCommand("ABOR");

    _pFile->flush();
    _nDownloadSizeAtPause = _nDownloadSize;
    _nDownloadSize = 0;
}


void DownloadManagerFTP::resume()
{
    qDebug() << __FUNCTION__ << "(): _nDownloadSizeAtPause = " << _nDownloadSizeAtPause;

    download();
}


void DownloadManagerFTP::download()
{
    qDebug() << __FUNCTION__ << "():";

    //connect(_pFTP, SIGNAL(dataTransferProgress(qint64,qint64)), this, SLOT(ftpDataTransferProgress(qint64,qint64)));
    connect(_pFTP, SIGNAL(readyRead()), this, SLOT(readyRead()));

    if (_bAcceptRanges && (_nDownloadSizeAtPause > 0))
    {
        qDebug() << "TYPE I";
        _pFTP->rawCommand("TYPE I"); // Binary mode
//        qDebug() << "PASV";
//        _pFTP->rawCommand("PASV"); // Passive mode
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

    _Timer.setInterval(15000);
    _Timer.setSingleShot(true);
    connect(&_Timer, SIGNAL(timeout()), this, SLOT(timeout()));
    _Timer.start();
}


void DownloadManagerFTP::finished()
{
    qDebug() << __FUNCTION__;

    _Timer.stop();
    _pFile->close();
    QFile::remove(_qsFileName);
    _pFile->rename(_qsFileName + ".part", _qsFileName);
    delete _pFile;
    _pFile = NULL;

    emit downloadComplete();
}


void DownloadManagerFTP::error(QNetworkReply::NetworkError code)
{
    qDebug() << __FUNCTION__ << "(" << code << ")";
}


void DownloadManagerFTP::timeout()
{
    qDebug() << __FUNCTION__;
}


void DownloadManagerFTP::stateChanged(int state)
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


QString DownloadManagerFTP::ftpCmd(QFtp::Command eCmd)
{
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

    return qsCmd;
}


void DownloadManagerFTP::commandStarted(int id)
{
    qDebug() << __FUNCTION__ << "(" << id << ")";
}


void DownloadManagerFTP::commandFinished(int id, bool error)
{
    qDebug() << __FUNCTION__ << "(" << id << "," << error << ")";
}


void DownloadManagerFTP::rawCommandReply(int replyCode, const QString &detail)
{
    int id = _pFTP->currentId();
    qDebug() << __FUNCTION__ << "(" << replyCode << "," << detail << "): " << id;

    switch (replyCode)
    {
        case 200:
        case 214: // HELP
        {
            addLine(detail);

            if (detail.contains(QLatin1String("REST"), Qt::CaseSensitive))
            {
                _bAcceptRanges = true;
            }

            ftpFinishedHelp();
        }
        break;

        case 213: // SIZE
        {
            _nDownloadTotal = detail.toInt();

            download();
        }
        break;

        case 226: // RETR
        {
            if (_pFile->isOpen())
            {
                finished();
            }
        }
        break;

        case 550:
        {
        }
        break;
    }
}


void DownloadManagerFTP::readyRead()
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

    int nPercentage = static_cast<int>((static_cast<float>(_nDownloadSize) * 100.0) / static_cast<float>(_nDownloadTotal));
    emit progress(nPercentage);

    qDebug() << "Download Progress: Received=" << _nDownloadSize <<": Total=" << _nDownloadTotal << " %" << nPercentage;

    if (_nDownloadSize == _nDownloadTotal)
    {
        if (_pFile->isOpen())
        {
            finished();
        }
    }
    else
    {
        _Timer.start(15000);
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
