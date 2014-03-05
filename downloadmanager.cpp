#include "downloadmanager.h"
#include "downloadwidget.h"

#include <QFileInfo>
#include <QDateTime>
#include <QDebug>


DownloadManager::DownloadManager(QObject *parent) :
    QObject(parent)
#if QT_VERSION < 0x050000
    , _pFTP(NULL)
#endif
    , _pHTTP(NULL)
{
}


void DownloadManager::download(QUrl url)
{
    qDebug() << __FUNCTION__ << "(" << url.toString() << ")";

    _URL = url;

#if QT_VERSION < 0x050000
    if (_URL.scheme().toLower() == "ftp")
    {
        _pFTP = new DownloadManagerFTP(this);

        connect(_pFTP, SIGNAL(addLine(QString)), this, SLOT(localAddLine(QString)));
        connect(_pFTP, SIGNAL(progress(int)), this, SLOT(localProgress(int)));
        connect(_pFTP, SIGNAL(downloadComplete()), this, SLOT(localDownloadComplete()));

        _pFTP->download(_URL);
    }
    else
#endif
    {
        _pHTTP = new DownloadManagerHTTP(this);

        connect(_pHTTP, SIGNAL(addLine(QString)), this, SLOT(localAddLine(QString)));
        connect(_pHTTP, SIGNAL(progress(int)), this, SLOT(localProgress(int)));
        connect(_pHTTP, SIGNAL(downloadComplete()), this, SLOT(localDownloadComplete()));

        _pHTTP->download(_URL);
    }
}


void DownloadManager::pause()
{
    qDebug() << __FUNCTION__;

#if QT_VERSION < 0x050000
    if (_URL.scheme().toLower() == "ftp")
    {
        _pFTP->pause();
    }
    else
#endif
    {
        _pHTTP->pause();
    }
}


void DownloadManager::resume()
{
    qDebug() << __FUNCTION__;

#if QT_VERSION < 0x050000
    if (_URL.scheme().toLower() == "ftp")
    {
        _pFTP->resume();
    }
    else
#endif
    {
        _pHTTP->resume();
    }
}


void DownloadManager::localAddLine(QString qsLine)
{
    emit addLine(qsLine);
}


void DownloadManager::localProgress(int nPercentage)
{
    emit progress(nPercentage);
}


void DownloadManager::localDownloadComplete()
{
    emit downloadComplete();
}
