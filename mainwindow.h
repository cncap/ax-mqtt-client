#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtMqtt/QMqttClient>
#include <QUuid>
#include <QCryptographicHash>
#include <QSettings>

const QString TIME_FORMAT = "yyyy-MM-dd hh:mm:ss";
const QString TIME_FORMAT_SHORT = "MM-dd hh:mm:ss";

struct UserSetting
{
    UserSetting() {

    }
    QString address = "127.0.0.1";
    quint16 port = 1883;
    bool clean_session = true;
    quint16 keep_alive = 60;
    QString version = "v3.1.1";
    QString clientid =  QCryptographicHash::hash ( QUuid::createUuid().toString().toUtf8(), QCryptographicHash::Md5 ).toHex();
    QString username = "";
    QString password = "";

    QString pub_topic="";
    QString pub_message="";
    int qos = 0;
    bool retained = false;

    QString sub_topic = "";
    QString sub_topics ="------- Subscribed -------";
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

//    void brokerDisconnected();

    void stateChanged(int state);

    void subsSelectionChanged();

    void on_buttonConnect_clicked();

    void on_buttonQuit_clicked();

    void on_buttonPublish_clicked();

    void on_buttonSubscribe_clicked();

    void on_buttonClear_clicked();

    void on_buttonSave_clicked();

    void on_buttonDelete_clicked();

    void on_buttonUnsubscribe_clicked();

    void on_actionAbout_triggered();

    void on_actionExit_triggered();


private:
    Ui::MainWindow *ui;
    QMqttClient *m_client;
    UserSetting us;
    void console(const QString &t);
    void saveSetting();
    void loadSetting(QString setting);
    void reloadSettings();

};

#endif // MAINWINDOW_H
