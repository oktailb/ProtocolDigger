#include "videoDialog.h"
#include <thread>

videoDialog::videoDialog(QWidget *parent) :
    QDialog(parent)
{
}

videoDialog::~videoDialog()
{
    delete videoWidget;
    delete layout;
    delete player;
}

void videoDialog::playVideo(const QString &fileName)
{
    player = new QMediaPlayer;
    videoWidget=new QVideoWidget;
    layout = new QVBoxLayout;
    layout->addWidget(videoWidget);
    player->setVideoOutput(videoWidget) ;
    setLayout(layout);
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    player->setSource(fileName);
#else
    player->setMedia(fileName);
#endif
    open();
    setMinimumWidth(640);
    setMinimumHeight(480);
    videoWidget->show();
}

void videoDialog::closeEvent(QCloseEvent *e)
{
    Q_UNUSED(e);
    player->stop();
    delete player;
    delete videoWidget;
    delete layout;
}

void videoDialog::goTo(int64_t ts)
{
    player->setPosition(ts);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    player->play();
    player->pause();
}

void videoDialog::play()
{
    player->play();
}

void videoDialog::pause()
{
    player->pause();
}

void videoDialog::stop()
{
    player->stop();
}
