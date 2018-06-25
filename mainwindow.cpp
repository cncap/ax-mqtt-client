#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore/QDateTime>
#include <QtMqtt/QMqttClient>
#include <QtWidgets/QMessageBox>

/**
 * @brief MainWindow::MainWindow
 * @param parent
 */
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    MainWindow::setWindowTitle(tr("MQTT Client V1.3"));

    //Initialization
    ui->comboBoxVersion->addItem("v3.1", QMqttClient::ProtocolVersion::MQTT_3_1);
    ui->comboBoxVersion->addItem("v3.1.1", QMqttClient::ProtocolVersion::MQTT_3_1_1);
    ui->buttonSave->setEnabled(false);
    ui->buttonDelete->setEnabled(false);
    m_client = new QMqttClient(this);
    reloadSettings();
    loadSetting("");

    //Signal interception

    connect(m_client, &QMqttClient::stateChanged, this, &MainWindow::stateChanged);
//    connect(m_client, &QMqttClient::disconnected, this, &MainWindow::brokerDisconnected);

    connect(m_client, &QMqttClient::messageSent, this, [this](qint32 id){
        console(tr("Message sent: %1").arg(id));
    });
    connect(m_client, &QMqttClient::messageReceived, this, [this](const QByteArray &message, const QMqttTopicName &topic) {
        console(tr("Received Topic: ").toUtf8()
                + topic.name()
                + tr("Message: ").toUtf8()
                + message);
    });



    connect(ui->lineEditHost, &QLineEdit::textChanged, m_client, &QMqttClient::setHostname);
    connect(ui->lineEditClientId, &QLineEdit::textChanged, m_client, &QMqttClient::setClientId);
    connect(ui->lineEditUsername, &QLineEdit::textChanged, m_client, &QMqttClient::setUsername);
    connect(ui->lineEditPassword, &QLineEdit::textChanged, m_client, &QMqttClient::setPassword);
    connect(ui->spinBoxPort, QOverload<int>::of(&QSpinBox::valueChanged), m_client, &QMqttClient::setPort);
    connect(ui->spinBoxKeepAlive, QOverload<int>::of(&QSpinBox::valueChanged), m_client, &QMqttClient::setKeepAlive);

    connect(ui->comboBoxVersion, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, [this](const QString& v){
        if(v=="v3.1.1"){
            m_client->setProtocolVersion(QMqttClient::MQTT_3_1_1);
        }else{
            m_client->setProtocolVersion(QMqttClient::MQTT_3_1);
        }
    });
    connect(ui->listWidgetSubs, &QListWidget::itemSelectionChanged, this, &MainWindow::subsSelectionChanged );
    connect(ui->lineEditSetting, &QLineEdit::textChanged, this, [this](QString address){
        if(address.isEmpty()){
            ui->buttonSave->setEnabled(false);
        }else{
            ui->buttonSave->setEnabled(true);
        }
    });

    connect(ui->comboBoxSettings, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){

        QString setting = ui->comboBoxSettings->itemText(index);
        if(index > 0 ){
            if(m_client->state()==QMqttClient::Disconnected){
                ui->buttonDelete->setEnabled(true);
            }
            ui->lineEditSetting->setText(setting);
        }else{
            ui->buttonDelete->setEnabled(false);
            ui->lineEditSetting->setText("");
        }
        loadSetting(setting);
    });
}

/**
 * @brief MainWindow::~MainWindow
 */
MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * Connect to server
 * @brief MainWindow::on_buttonConnect_clicked
 */
void MainWindow::on_buttonConnect_clicked()
{
    if (m_client->state() == QMqttClient::Disconnected) {

        m_client->connectToHost();

    } else {
        m_client->disconnectFromHost();
    }
}

/**
 * Quit
 * @brief MainWindow::on_buttonQuit_clicked
 */
void MainWindow::on_buttonQuit_clicked()
{
    QApplication::quit();
}

/**
 * State change event
 * @brief MainWindow::stateChanged
 * @param state
 */
void MainWindow::stateChanged(int state)
{
    QStringList subs = us.sub_topics.split(",").mid(1, us.sub_topics.split(",").count());
    switch (state) {
    case QMqttClient::Connecting:
        ui->buttonPublish->setEnabled(false);
        ui->buttonSubscribe->setEnabled(false);
        ui->buttonUnsubscribe->setEnabled(false);

        ui->buttonConnect->setText(tr("Cancel").toUtf8());
        ui->comboBoxSettings->setEnabled(false);
        ui->lineEditSetting->setEnabled(false);
        ui->buttonDelete->setEnabled(false);
        ui->lineEditHost->setEnabled(false);

        ui->spinBoxPort->setEnabled(false);
        ui->spinBoxKeepAlive->setEnabled(false);
        ui->lineEditClientId->setEnabled(false);
        ui->lineEditPassword->setEnabled(false);
        ui->lineEditUsername->setEnabled(false);
        ui->checkBoxCleanSession->setEnabled(false);
        ui->checkBoxRetained->setEnabled(false);
        ui->comboBoxVersion->setEnabled(false);
        statusBar()->showMessage(tr("Connecting to %1...").arg(ui->lineEditHost->text()), 0);
        console(tr("Connecting to %1...").arg(ui->lineEditHost->text()));
        break;
    case QMqttClient::Connected:
        // subscribe saved topic
        foreach (QString t, subs) {
            m_client->subscribe(t);
        }
        ui->buttonConnect->setText(tr("Disconnect").toUtf8());
        ui->buttonPublish->setEnabled(true);
        ui->buttonSubscribe->setEnabled(true);
        statusBar()->showMessage("Connected", 0);
        console(tr("Connected"));
        break;
    case QMqttClient::Disconnected:
        ui->buttonPublish->setEnabled(false);
        ui->buttonSubscribe->setEnabled(false);
        ui->buttonUnsubscribe->setEnabled(false);

        ui->buttonConnect->setText(tr("Connect").toUtf8());
        ui->comboBoxSettings->setEnabled(true);
        ui->lineEditSetting->setEnabled(true);
        ui->buttonDelete->setEnabled(true);

        ui->lineEditHost->setEnabled(true);
        ui->spinBoxPort->setEnabled(true);
        ui->lineEditHost->setEnabled(true);
        ui->spinBoxPort->setEnabled(true);
        ui->spinBoxKeepAlive->setEnabled(true);
        ui->lineEditClientId->setEnabled(true);
        ui->lineEditPassword->setEnabled(true);
        ui->lineEditUsername->setEnabled(true);
        ui->checkBoxCleanSession->setEnabled(true);
        ui->checkBoxRetained->setEnabled(true);
        ui->comboBoxVersion->setEnabled(true);
        statusBar()->showMessage("Disconnected", 0);
        console(tr("Disconnected"));
        break;
    default:
        break;
    }

}

/**
 * Publish message
 * @brief MainWindow::on_buttonPublish_clicked
 */
void MainWindow::on_buttonPublish_clicked()
{
    int ret = m_client->publish(ui->lineEditPubTopic->text(),
                                ui->lineEditPubMessage->toPlainText().toUtf8(),
                                ui->comboBoxQos->currentIndex(),
                                ui->checkBoxRetained->isChecked()
                                );
    if ( ret <0){
        console("Could not publish message.");
    }else{
        console("Publish message successful.");
    }
}

/**
 * Subscribe
 * @brief MainWindow::on_buttonSubscribe_clicked
 */
void MainWindow::on_buttonSubscribe_clicked()
{
    QString t = ui->lineEditSubTopic->text();
    auto subscription = m_client->subscribe(t);
    if (!subscription) {
        console("Could not subscribe. Is there a valid connection?");
    }else{
        if(ui->listWidgetSubs->findItems(t, Qt::MatchExactly).count()==0){
            ui->listWidgetSubs->addItem(t);
        }
    }
}

/**
 * Unsubscribe
 * @brief MainWindow::on_buttonUnsubscribe_clicked
 */
void MainWindow::on_buttonUnsubscribe_clicked()
{
    QString t = ui->listWidgetSubs->currentItem()->text();
    int r = ui->listWidgetSubs->currentRow();
    if(r>0){
        QListWidgetItem *item = ui->listWidgetSubs->item(r);
        ui->listWidgetSubs->removeItemWidget(item);
        delete item;
         m_client->unsubscribe(t);
    }
}
/**
 * Subs selection changed slot
 * @brief MainWindow::subsSelectionChanged
 */
void MainWindow::subsSelectionChanged()
{
    if(ui->listWidgetSubs->currentRow()>0){
        ui->lineEditSubTopic->setText(ui->listWidgetSubs->currentItem()->text());
        ui->buttonUnsubscribe->setEnabled(true);
    }
}

/**
 * Clear console button click
 * @brief MainWindow::on_buttonClear_clicked
 */
void MainWindow::on_buttonClear_clicked()
{
    ui->editLog->clear();
}

/**
 * Save button click
 * @brief MainWindow::on_buttonSave_clicked
 */
void MainWindow::on_buttonSave_clicked()
{
    saveSetting();
}

/**
 * Console
 * @brief MainWindow::console
 * @param t
 */
void MainWindow::console(const QString &t)
{
  ui->editLog->appendPlainText(QDateTime::currentDateTime().toString(TIME_FORMAT_SHORT) +" - "+ t);
}


/**
 * @brief MainWindow::saveSetting
 */
void MainWindow::saveSetting()
{

    QSettings app("Aiotx", "MClientSetting");
    if(app.isWritable()){

        //connection
        QString group = ui->lineEditSetting->text();
        QString address = ui->lineEditHost->text();
        if(group.isEmpty()) return;
        //subs
        int c = ui->listWidgetSubs->count();
        QStringList subsString;
        for(int i = 0; i< c; i++){
            subsString.append(ui->listWidgetSubs->item(i)->text());
        }

        //save settings
        app.beginGroup(group);
        app.setValue("address", address);
        app.setValue("port", ui->spinBoxPort->value());
        app.setValue("version", ui->comboBoxVersion->currentText());
        app.setValue("clientid", ui->lineEditClientId->text());
        app.setValue("username", ui->lineEditUsername->text());
        app.setValue("password", ui->lineEditPassword->text());
        app.setValue("keep_alive", ui->spinBoxKeepAlive->value());
        app.setValue("clean_session", ui->checkBoxCleanSession->isChecked());

        app.setValue("qos", ui->comboBoxQos->currentIndex());
        app.setValue("retained", ui->checkBoxRetained->isChecked());
        app.setValue("pub_topic", ui->lineEditPubTopic->text());
        app.setValue("pub_message", ui->lineEditPubMessage->document()->toPlainText());

        app.setValue("sub_topic", ui->lineEditSubTopic->text());
        app.setValue("sub_topics", subsString.join(','));
        app.endGroup();

        console(tr("Settings saved."));
        reloadSettings();  //reload settings when its saved.
        ui->comboBoxSettings->setCurrentText(group);

    }else{
        console(tr("Save failure."));
    }
}

/**
 * @brief MainWindow::reloadSettings
 */
void MainWindow::reloadSettings()
{
    QSettings app("Aiotx", "MClientSetting");
    ui->comboBoxSettings->clear();
    ui->comboBoxSettings->addItem(tr("New setting"));
    foreach (QString item, app.childGroups()) {
        ui->comboBoxSettings->addItem(item);
    }
}

/**
 * @brief MainWindow::loadSetting
 * @param setting
 */
void MainWindow::loadSetting(QString setting)
{    
    if(ui->comboBoxSettings->currentIndex()>0){
        ui->lineEditSetting->setText(setting);
        console("Load setting: " + setting);
    }
    qDebug()<< setting;
    QSettings app("Aiotx", "MClientSetting");
    app.beginGroup(setting);

    UserSetting defus;
    us.address = app.contains("address") ? app.value("address").toString() : defus.address;
    us.port = app.contains("port") ? app.value("port").toInt(): defus.port;
    us.keep_alive = app.contains("keep_alive") ? app.value("keep_alive").toInt(): defus.keep_alive;
    us.clean_session = app.contains("clean_session") ? app.value("clean_session").toBool():defus.clean_session;
    us.version = app.contains("version") ? app.value("version").toString():defus.version;
    us.clientid = app.contains("clientid") ? app.value("clientid").toString() : defus.clientid;
    us.username = app.contains("username") ? app.value("username").toString(): defus.username;
    us.password = app.contains("password") ? app.value("password").toString(): defus.password;

    us.qos = app.contains("qos") ? app.value("qos").toInt(): defus.qos;
    us.retained = app.contains("retained") ? app.value("retained").toBool() : defus.retained;
    us.pub_topic = app.contains("pub_topic") ? app.value("pub_topic").toString() : defus.pub_topic;
    us.pub_message = app.contains("pub_message") ? app.value("pub_message").toString() : defus.pub_message;

    us.sub_topic = app.contains("sub_topic") ? app.value("sub_topic").toString() : defus.sub_topic;
    us.sub_topics = app.contains("sub_topics") ? app.value("sub_topics").toString() : defus.sub_topics;
    app.endGroup();

    //Default settings
    m_client->setHostname(us.address);
    m_client->setPort(us.port);
    m_client->setKeepAlive(us.keep_alive);
    m_client->setClientId(us.clientid);
    m_client->setCleanSession(us.clean_session);
    m_client->setUsername(us.username);
    m_client->setPassword(us.password);
    m_client->setProtocolVersion(QMqttClient::MQTT_3_1_1);

    //Set ui value
    ui->lineEditHost->setText(us.address);
    ui->spinBoxPort->setValue(us.port);
    ui->spinBoxKeepAlive->setValue(us.keep_alive);
    ui->lineEditClientId->setText(us.clientid);
    ui->lineEditUsername->setText(us.username);
    ui->lineEditPassword->setText(us.password);
    ui->comboBoxVersion->setCurrentText(us.version);
    ui->lineEditHost->setText(us.address);
    ui->checkBoxCleanSession->setChecked(us.clean_session);

    ui->comboBoxQos->setCurrentIndex(us.qos);
    ui->checkBoxRetained->setChecked(us.retained);
    ui->lineEditPubTopic->setText(us.pub_topic);
    ui->lineEditPubMessage->setPlainText(us.pub_message);

    ui->lineEditSubTopic->setText(us.sub_topic);
    if(us.sub_topics.split(',').count()>0){
        ui->listWidgetSubs->clear();
        ui->listWidgetSubs->addItems(us.sub_topics.split(','));
    }
}

/**
 * @brief MainWindow::on_actionAbout_triggered
 */
void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::information(this,tr("About"), tr("Author: Chn.vmax@gmail.com \nLicense: GNU GPLv2"), QMessageBox::Close );
}

/**
 * @brief MainWindow::on_actionExit_triggered
 */
void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();

}

/**
 * @brief MainWindow::on_buttonDelete_clicked
 */
void MainWindow::on_buttonDelete_clicked()
{

    int confirm = QMessageBox::question(this,tr("confirm"), tr("Are you sure?"), QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
    if(confirm == QMessageBox::Yes){
        QSettings app("Aiotx", "MClientSetting");
        app.remove(ui->comboBoxSettings->currentText());
        console(tr("Settings deleted."));
        reloadSettings();
    }
}
