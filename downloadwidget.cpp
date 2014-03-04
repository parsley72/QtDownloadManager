#include "downloadwidget.h"
#include "ui_form.h"
#include <QProgressBar>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStandardItemModel>


DownloadWidget::DownloadWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Form)
{
    ui->setupUi(this);

// http://get.qt.nokia.com/qtsdk/Qt_SDK_Win_offline_v1_1_2_en.exe
// http://www.simrad-yachting.com/Root/Catalogs/SimradYachting/Simrad_2014_Catalogue_global.pdf
// ftp://software.simrad-yachting.com/NSS/NSS-3.0-46.1.66-25591-r1-Standard-1.upd // 68,741,120 bytes
// http://ffmpeg.zeranoe.com/builds/win32/shared/ffmpeg-2.1.1-win32-shared.7z
// http://www.sciencedirect.com/science/article/pii/S0031920113001465/pdfft?md5=90da8215e1bc18f204403fd352044395&pid=1-s2.0-S0031920113001465-main.pdf
// http://ftp.yz.yamagata-u.ac.jp/pub/qtproject/official_releases/qt/5.2/5.2.0/qt-windows-opensource-5.2.0-msvc2012-x86-offline.exe
// http://hivelocity.dl.sourceforge.net/project/tortoisesvn/1.8.4/Application/TortoiseSVN-1.8.4.24972-x64-svn-1.8.5.msi

//    ui->urlEdit->setText("http://hivelocity.dl.sourceforge.net/project/tortoisesvn/1.8.4/Application/TortoiseSVN-1.8.4.24972-x64-svn-1.8.5.msi");
    ui->urlEdit->setText("ftp://software.simrad-yachting.com/NSS/NSS-3.0-46.1.66-25591-r1-Standard-1.upd");

    QStandardItemModel *model = new QStandardItemModel(0, 1, this);
    ui->listView->setModel(model);

    mManager = new DownloadManager(this);
    connect(mManager, SIGNAL(addLine(QString)), this, SLOT(addLine(QString)));
    connect(mManager, SIGNAL(downloadComplete()), this, SLOT(finished()));
    connect(mManager, SIGNAL(progress(int)), this, SLOT(progress(int)));
}


void DownloadWidget::on_downloadBtn_clicked()
{
    ui->listView->reset();
    QUrl url(ui->urlEdit->text());
    mManager->download(url);
    ui->downloadBtn->setEnabled(false);
    ui->pauseBtn->setEnabled(true);
}


void DownloadWidget::on_pauseBtn_clicked()
{
    mManager->pause();
    ui->pauseBtn->setEnabled(false);
    ui->resumeBtn->setEnabled(true);
}


void DownloadWidget::on_resumeBtn_clicked()
{
    mManager->resume();
    ui->pauseBtn->setEnabled(true);
    ui->resumeBtn->setEnabled(false);
}


void DownloadWidget::addLine(QString qsLine)
{
    int nRow = ui->listView->model()->rowCount();
    ui->listView->model()->insertRow(nRow);
    ui->listView->model()->setData(ui->listView->model()->index(nRow, 0), qsLine);
}


void DownloadWidget::progress(int nPercentage)
{
    ui->progressBar->setValue(nPercentage);
}


void DownloadWidget::finished()
{
    ui->downloadBtn->setEnabled(true);
    ui->pauseBtn->setEnabled(false);
    ui->resumeBtn->setEnabled(false);
}
