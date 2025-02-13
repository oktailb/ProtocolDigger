#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QMediaPlayer>
#include <QVideoWidget>

class videoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit videoDialog(QWidget *parent = nullptr);
    ~videoDialog();

    void playVideo(const QString &fileName);

    void goTo(uint64_t ts);
    void play();
    void pause();
    void stop();

protected:
    void closeEvent(QCloseEvent *e) override;

private:
    QVBoxLayout *layout;
    QMediaPlayer *player;
    QVideoWidget *videoWidget;

};
