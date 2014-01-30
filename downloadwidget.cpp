#include "downloadwidget.h"
#include <QProgressBar>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

DownloadWidget::DownloadWidget(QWidget *parent) :
    QWidget(parent)
{
    mManager = new DownloadManager(this);
    connect(mManager,SIGNAL(downloadComplete()),this,SLOT(finished()));
    connect(mManager,SIGNAL(progress(int)),this,SLOT(progress(int)));

    setupUi();
}

void DownloadWidget::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    mProgressBar = new QProgressBar(this);
    mainLayout->addWidget(mProgressBar);

    mDownloadBtn = new QPushButton("Download",this);
    mPauseBtn = new QPushButton("Pause",this);
    mPauseBtn->setEnabled(false);
    mResumeBtn = new QPushButton("Resume",this);
    mResumeBtn->setEnabled(false);

    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addWidget(mDownloadBtn);
    btnLayout->addWidget(mPauseBtn);
    btnLayout->addWidget(mResumeBtn);

    mainLayout->addLayout( btnLayout);

    connect(mDownloadBtn,SIGNAL(clicked()),this,SLOT(download()));
    connect(mPauseBtn,SIGNAL(clicked()),this,SLOT(pause()));
    connect(mResumeBtn,SIGNAL(clicked()),this,SLOT(resume()));
}

void DownloadWidget::download()
{
    mManager->download(QUrl("http://get.qt.nokia.com/qtsdk/Qt_SDK_Win_offline_v1_1_2_en.exe"));
    mDownloadBtn->setEnabled(false);
    mPauseBtn->setEnabled(true);
}

void DownloadWidget::pause()
{
    mManager->pause();
    mPauseBtn->setEnabled(false);
    mResumeBtn->setEnabled(true);
}

void DownloadWidget::resume()
{
    mManager->resume();
    mPauseBtn->setEnabled(true);
    mResumeBtn->setEnabled(false);
}


void DownloadWidget::finished()
{
    mDownloadBtn->setEnabled(true);
    mPauseBtn->setEnabled(false);
    mResumeBtn->setEnabled(false);
}

void DownloadWidget::progress(int percentage)
{
    mProgressBar->setValue( percentage);
}
