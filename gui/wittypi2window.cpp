#include <QMessageBox>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QStyle>
#include <QDesktopWidget>
#include <QKeyEvent>
#include <QDebug>
#include <QFileDialog>
#include <QTextStream>

#include "wittypi2window.h"
#include "ui_wittypi2window.h"

WittyPi2Window::WittyPi2Window(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WittyPi2Window)
{
    ui->setupUi(this);
    setWindowTitle(TXT_WINDOW_TITLE);

    // center the window
    setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            size(),
            qApp->desktop()->availableGeometry()
        )
    );

    rpiDateTimeEdit = findChild<QDateTimeEdit*>("rpiDateTimeEdit");
    rpiTimeEditButton = findChild<QPushButton*>("rpiTimeEditButton");

    wpiDateTimeEdit = findChild<QDateTimeEdit*>("wpiDateTimeEdit");
    wpiTimeEditButton = findChild<QPushButton*>("wpiTimeEditButton");

    rpi2WpiButton = findChild<QPushButton*>("rpi2WpiButton");
    wpi2RpiButton = findChild<QPushButton*>("wpi2RpiButton");
    ntpUpdateButton = findChild<QPushButton*>("ntpUpdateButton");

    temperatureLabel = findChild<QLabel*>("temperatureLabel");

    scriptLabel = findChild<QLabel*>("scriptLabel");
    clearScriptButton = findChild<QPushButton*>("clearScriptButton");
    chooseScriptButton = findChild<QPushButton*>("chooseScriptButton");

    shutdownDateEdit = findChild<QLineEdit*>("shutdownDateEdit");
    shutdownHourEdit = findChild<QLineEdit*>("shutdownHourEdit");
    shutdownMinEdit = findChild<QLineEdit*>("shutdownMinEdit");
    shutdownSecEdit = findChild<QLineEdit*>("shutdownSecEdit");
    clearShutdownButton = findChild<QPushButton*>("clearShutdownButton");
    editShutdownButton = findChild<QPushButton*>("editShutdownButton");

    startupDateEdit = findChild<QLineEdit*>("startupDateEdit");
    startupHourEdit = findChild<QLineEdit*>("startupHourEdit");
    startupMinEdit = findChild<QLineEdit*>("startupMinEdit");
    startupSecEdit = findChild<QLineEdit*>("startupSecEdit");
    clearStartupButton = findChild<QPushButton*>("clearStartupButton");
    editStartupButton = findChild<QPushButton*>("editStartupButton");

    enableButtons();

    // update display regularly
    timerPaused = false;
    timerEvent(NULL);
    startTimer(1000);
}

WittyPi2Window::~WittyPi2Window()
{
    delete ui;
}

void WittyPi2Window::enableButtons()
{
    rpiTimeEditButton->setEnabled(true);
    wpiTimeEditButton->setEnabled(true);

    rpi2WpiButton->setEnabled(true);
    wpi2RpiButton->setEnabled(true);
    int exitCode;
    callUtilFunc(FUNC_HAS_INTERNET, NULL, &exitCode);
    ntpUpdateButton->setEnabled(exitCode == 0);

    editShutdownButton->setEnabled(true);
    clearShutdownButton->setEnabled(scheduledShutdown());

    editStartupButton->setEnabled(true);
    clearStartupButton->setEnabled(scheduledStartup());

    chooseScriptButton->setEnabled(true);
    clearScriptButton->setEnabled(usingScript());

    QApplication::processEvents();
}

void WittyPi2Window::disableButtons()
{
    rpiTimeEditButton->setEnabled(!rpiDateTimeEdit->isReadOnly());
    wpiTimeEditButton->setEnabled(!wpiDateTimeEdit->isReadOnly());

    rpi2WpiButton->setEnabled(false);
    wpi2RpiButton->setEnabled(false);
    ntpUpdateButton->setEnabled(false);

    editShutdownButton->setEnabled(!shutdownDateEdit->isReadOnly());
    clearShutdownButton->setEnabled(false);

    editStartupButton->setEnabled(!startupDateEdit->isReadOnly());
    clearStartupButton->setEnabled(false);

    chooseScriptButton->setEnabled(false);
    clearScriptButton->setEnabled(false);

    QApplication::processEvents();
}

void WittyPi2Window::keyPressEvent(QKeyEvent *event)
{
    // press ESC key to cancel current editing
    if (Qt::Key_Escape == event->key())
    {
        // cancel RPi time editing
        if (!rpiDateTimeEdit->isReadOnly())
        {
            rpiDateTimeEdit->setReadOnly(true);
            setFocus();
            rpiTimeEditButton->setText(TXT_EDIT);
            rpiDateTimeEdit->setDateTime(rpiDateTimeEdit->property(PROP_PREVIOUS_VALUE).toDateTime());
            timerPaused = false;
            enableButtons();
        }

        // cancel Witty Pi time editing
        if (!wpiDateTimeEdit->isReadOnly())
        {
            wpiDateTimeEdit->setReadOnly(true);
            setFocus();
            wpiTimeEditButton->setText(TXT_EDIT);
            wpiDateTimeEdit->setDateTime(wpiDateTimeEdit->property(PROP_PREVIOUS_VALUE).toDateTime());
            timerPaused = false;
            enableButtons();
        }

        // cancel shutdown time editing
        if (!shutdownDateEdit->isReadOnly())
        {
            shutdownDateEdit->setReadOnly(true);
            shutdownHourEdit->setReadOnly(true);
            shutdownMinEdit->setReadOnly(true);
            editShutdownButton->setText(TXT_EDIT);
            shutdownDateEdit->setText(shutdownDateEdit->property(PROP_PREVIOUS_VALUE).toString());
            shutdownHourEdit->setText(shutdownHourEdit->property(PROP_PREVIOUS_VALUE).toString());
            shutdownMinEdit->setText(shutdownMinEdit->property(PROP_PREVIOUS_VALUE).toString());
            timerPaused = false;
            enableButtons();
        }

        // cancel startup time editing
        if (!startupDateEdit->isReadOnly())
        {
            startupDateEdit->setReadOnly(true);
            startupHourEdit->setReadOnly(true);
            startupMinEdit->setReadOnly(true);
            editStartupButton->setText(TXT_EDIT);
            startupDateEdit->setText(startupDateEdit->property(PROP_PREVIOUS_VALUE).toString());
            startupHourEdit->setText(startupHourEdit->property(PROP_PREVIOUS_VALUE).toString());
            startupMinEdit->setText(startupMinEdit->property(PROP_PREVIOUS_VALUE).toString());
            startupSecEdit->setText(startupSecEdit->property(PROP_PREVIOUS_VALUE).toString());
            timerPaused = false;
            enableButtons();
        }
    }

}

/**
 * Parse date time string and store information into a list
 *
 * @brief WittyPi2Window::parseDateTimeString
 * @param datetime
 * @param mode: 0 for "dd HH:mm:ss", and 1 for "dd HH mm ss"
 * @return
 */
QList<QString> WittyPi2Window::parseDateTimeString(QString datetime, int mode)
{
    QList<QString> list;
    QStringList parts = datetime.split(' ');
    if (mode == 0)  // dd HH:mm:ss
    {
        if (parts.size() == 2)
        {
            QStringList hhmmss = parts.value(1).split(':');
            if (hhmmss.size() == 3)
            {
                list << parts.value(0);
                list << hhmmss.value(0);
                list << hhmmss.value(1);
                list << hhmmss.value(2);
            }
            else
            {
                qDebug() << "Date time string parsing error 2: " + datetime;
            }
        }
        else
        {
            qDebug() << "Date time string parsing error 1: " + datetime;
        }
    }
    else if (mode == 1) // dd HH mm ss
    {
        if (parts.size() == 4)
        {
            list << parts.value(0);
            list << parts.value(1);
            list << parts.value(2);
            list << parts.value(3);
        }
        else
        {
            qDebug() << "Date time string parsing error 3: " + datetime;
        }

    }
    return list;
}

QString WittyPi2Window::callUtilFunc(QString funcName, QString args, int* exitCode)
{
    QString cmd = QString("sudo bash -c \". ");
    cmd += WITTYPI_UTILITIES;
    cmd += QString("; ");
    cmd += funcName;
    if (args != NULL && !args.isEmpty()) {
        cmd += QString(" ");
        cmd += args;
    }
    cmd += QString("\"");
    QProcess proc;
    proc.start(cmd);
    proc.waitForFinished();
    if (exitCode != NULL)
    {
      *exitCode = proc.exitCode();
    }
    return QString(proc.readAllStandardOutput()) + QString("\n") + QString(proc.readAllStandardError());
}

QString WittyPi2Window::runSyncTime()
{
    QProcess proc;
    proc.start(QString("sudo ./") + WITTYPI_SYNCTIME);
    proc.waitForFinished();
    return QString(proc.readAllStandardOutput()) + QString("\n") + QString(proc.readAllStandardError());
}

QString WittyPi2Window::runScript()
{
    QProcess proc;
    proc.start(QString("sudo ./") + WITTYPI_RUN_SCRIPT);
    proc.waitForFinished();
    return QString(proc.readAllStandardOutput()) + QString("\n") + QString(proc.readAllStandardError());
}

void WittyPi2Window::reloadRaspberryPiTime()
{
    rpiDateTimeEdit->setDateTime(QDateTime::currentDateTime());
}

void WittyPi2Window::reloadWittyPiTime()
{
    QDateTime dt;
    dt.setTime_t(callUtilFunc(FUNC_GET_RTC_TIMESTAMP, NULL).toLong());

    wpiDateTimeEdit->setDateTime(dt);
}

void WittyPi2Window::reloadTemperature()
{
    temperatureLabel->setText(TXT_CUR_TEMPERATURE + callUtilFunc(FUNC_GET_TEMPERATURE, NULL));
}

void WittyPi2Window::reloadShutdownTime()
{
   QString result = callUtilFunc(FUNC_GET_SHUTDOWN_TIME, NULL).trimmed();
   if (result.length() > 2 && result != "0 0:0:00")
   {
       result = callUtilFunc(FUNC_GET_LOCAL_DATETIME, "'" + result + "'").trimmed();
       QStringList parts = result.split(' ');
       if (parts.size() == 2)
       {
           QStringList hhmmss = parts.value(1).split(':');
           if (hhmmss.size() == 3)
           {
               shutdownDateEdit->setText(parts.value(0));
               shutdownHourEdit->setText(hhmmss.value(0));
               shutdownMinEdit->setText(hhmmss.value(1));
           }
       }
       clearShutdownButton->setEnabled(true);
   }
   else
   {
       shutdownDateEdit->clear();
       shutdownHourEdit->clear();
       shutdownMinEdit->clear();
       clearShutdownButton->setEnabled(false);
   }
}

void WittyPi2Window::reloadStartupTime()
{
    QString result = callUtilFunc(FUNC_GET_STARTUP_TIME, NULL).trimmed();
    if (result.length() > 2 && result != "0 0:0:0")
    {
        result = callUtilFunc(FUNC_GET_LOCAL_DATETIME, "'" + result + "'").trimmed();
        QList<QString> list = parseDateTimeString(result);
        if (list.size() == 4)
        {
            startupDateEdit->setText(list.at(0));
            startupHourEdit->setText(list.at(1));
            startupMinEdit->setText(list.at(2));
            startupSecEdit->setText(list.at(3));
        }
        clearStartupButton->setEnabled(true);
    }
    else
    {
        startupDateEdit->clear();
        startupHourEdit->clear();
        startupMinEdit->clear();
        startupSecEdit->clear();
        clearStartupButton->setEnabled(false);
    }
}

void WittyPi2Window::reloadScriptStatus()
{
    if (usingScript())
    {
        scriptLabel->setText(TXT_IN_USE);
        QFile file(WITTYPI_SCHEDULE);
        if (file.open(QFile::ReadOnly | QFile::Text))
        {
            QTextStream in(&file);
            scriptLabel->setToolTip(in.readAll());
        }
        clearScriptButton->setEnabled(true);
    }
    else
    {
        scriptLabel->setText(TXT_NOT_IN_USE);
        scriptLabel->setToolTip(NULL);
        clearScriptButton->setEnabled(false);
    }
}

bool WittyPi2Window::scheduledShutdown()
{
    QString result(callUtilFunc(FUNC_GET_SHUTDOWN_TIME, NULL).trimmed());
    return result != "0 0:0:00";
}

bool WittyPi2Window::scheduledStartup()
{
    QString result(callUtilFunc(FUNC_GET_STARTUP_TIME, NULL).trimmed());
    return result != "0 0:0:0";
}

bool WittyPi2Window::usingScript()
{
    QFileInfo file(WITTYPI_SCHEDULE);
    return file.exists() && file.isFile();
}

void WittyPi2Window::timerEvent(QTimerEvent*)
{
    if (!timerPaused)
    {
        // load Raspberry Pi time and update display
        reloadRaspberryPiTime();

        // load Witty Pi time and update display
        reloadWittyPiTime();

        // load current temperature and update display
        reloadTemperature();

        // load scheduled shutdown time and update display
        reloadShutdownTime();

        // load scheduled startup time and update display
        reloadStartupTime();

        // load schedule script usage status
        reloadScriptStatus();
    }
}

void WittyPi2Window::on_rpiTimeEditButton_clicked()
{
    if (rpiDateTimeEdit->isReadOnly())
    {
        timerPaused = true;
        rpiDateTimeEdit->setProperty(PROP_PREVIOUS_VALUE, rpiDateTimeEdit->dateTime());
        rpiDateTimeEdit->setReadOnly(false);
        rpiDateTimeEdit->setFocus();
        rpiTimeEditButton->setText(TXT_DONE);
        disableButtons();
    }
    else
    {
        rpiDateTimeEdit->setReadOnly(true);
        QProcess proc;
        proc.start(QString("sudo date -s @")
                   + QString::number(rpiDateTimeEdit->dateTime().toTime_t()));
        proc.waitForFinished();
        timerPaused = false;
        rpiTimeEditButton->setText(TXT_EDIT);
        enableButtons();
    }
}

void WittyPi2Window::on_wpiTimeEditButton_clicked()
{
    if (wpiDateTimeEdit->isReadOnly())
    {
        timerPaused = true;
        wpiDateTimeEdit->setProperty(PROP_PREVIOUS_VALUE, wpiDateTimeEdit->dateTime());
        wpiDateTimeEdit->setReadOnly(false);
        wpiDateTimeEdit->setFocus();
        wpiTimeEditButton->setText(TXT_DONE);
        disableButtons();
    }
    else
    {
        wpiDateTimeEdit->setReadOnly(true);
        qDebug() << callUtilFunc(FUNC_SET_RTC_TIME, wpiDateTimeEdit->dateTime().toString(QString("yyyy-MM-dd HH:mm:ss")));
        timerPaused = false;
        wpiTimeEditButton->setText(TXT_EDIT);
        enableButtons();
    }
}

void WittyPi2Window::on_rpi2WpiButton_clicked()
{
    disableButtons();
    qDebug() << callUtilFunc(FUNC_SYS_TO_RTC, NULL);
    enableButtons();
}

void WittyPi2Window::on_wpi2RpiButton_clicked()
{
    disableButtons();
    qDebug() << callUtilFunc(FUNC_RTC_TO_SYS, NULL);
    enableButtons();
}

void WittyPi2Window::on_ntpUpdateButton_clicked()
{
    disableButtons();
    qDebug() << runSyncTime();
    enableButtons();
}

void WittyPi2Window::on_editShutdownButton_clicked()
{
    if (shutdownDateEdit->isReadOnly())
    {
        timerPaused = true;
        shutdownDateEdit->setProperty(PROP_PREVIOUS_VALUE, shutdownDateEdit->text());
        shutdownHourEdit->setProperty(PROP_PREVIOUS_VALUE, shutdownHourEdit->text());
        shutdownMinEdit->setProperty(PROP_PREVIOUS_VALUE, shutdownMinEdit->text());
        shutdownDateEdit->setReadOnly(false);
        shutdownHourEdit->setReadOnly(false);
        shutdownMinEdit->setReadOnly(false);
        shutdownDateEdit->setFocus();
        editShutdownButton->setText(TXT_DONE);
        disableButtons();
    }
    else
    {
        shutdownDateEdit->setReadOnly(true);
        shutdownHourEdit->setReadOnly(true);
        shutdownMinEdit->setReadOnly(true);
        QString utcDateTime = callUtilFunc(FUNC_GET_UTC_DATETIME,
                     shutdownDateEdit->text() + ' '
                     + shutdownHourEdit->text() + ' '
                     + shutdownMinEdit->text() + " 00").trimmed();
        QList<QString> list = parseDateTimeString(utcDateTime);
        if (list.size() == 4)
        {
            qDebug() << callUtilFunc(FUNC_SET_SHUTDOWN_TIME,
                         list.at(0) + ' ' + list.at(1) + ' ' + list.at(2));
        }
        timerPaused = false;
        editShutdownButton->setText(TXT_EDIT);
        enableButtons();
    }
}

void WittyPi2Window::on_clearShutdownButton_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, TXT_PLEASE_CONFIRM, TXT_ARE_YOU_SURE,
        QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        qDebug() << callUtilFunc(FUNC_CLEAR_SHUTDOWN_TIME, NULL);
    }
}

void WittyPi2Window::on_editStartupButton_clicked()
{
    if (startupDateEdit->isReadOnly())
    {
        timerPaused = true;
        startupDateEdit->setProperty(PROP_PREVIOUS_VALUE, startupDateEdit->text());
        startupHourEdit->setProperty(PROP_PREVIOUS_VALUE, startupHourEdit->text());
        startupMinEdit->setProperty(PROP_PREVIOUS_VALUE, startupMinEdit->text());
        startupSecEdit->setProperty(PROP_PREVIOUS_VALUE, startupSecEdit->text());
        startupDateEdit->setReadOnly(false);
        startupHourEdit->setReadOnly(false);
        startupMinEdit->setReadOnly(false);
        startupSecEdit->setReadOnly(false);
        startupDateEdit->setFocus();
        editStartupButton->setText(TXT_DONE);
        disableButtons();
    }
    else
    {
        startupDateEdit->setReadOnly(true);
        startupHourEdit->setReadOnly(true);
        startupMinEdit->setReadOnly(true);
        startupSecEdit->setReadOnly(true);
        QString utcDateTime = callUtilFunc(FUNC_GET_UTC_DATETIME,
                     startupDateEdit->text() + ' '
                     + startupHourEdit->text() + ' '
                     + startupMinEdit->text() + ' '
                     + startupSecEdit->text()).trimmed();
        QList<QString> list = parseDateTimeString(utcDateTime);
        if (list.size() == 4)
        {
            qDebug() << callUtilFunc(FUNC_SET_STARTUP_TIME,
                         list.at(0) + ' ' + list.at(1) + ' ' + list.at(2) + ' ' + list.at(3));
        }
        timerPaused = false;
        editStartupButton->setText(TXT_EDIT);
        enableButtons();
    }
}

void WittyPi2Window::on_clearStartupButton_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, TXT_PLEASE_CONFIRM, TXT_ARE_YOU_SURE,
        QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        qDebug() << callUtilFunc(FUNC_CLEAR_STARTUP_TIME, NULL);
    }
}

void WittyPi2Window::on_chooseScriptButton_clicked()
{
    QString scriptFile(QFileDialog::getOpenFileName(this,
        TXT_CHOOSE_SCRIPT, WITTYPI_SCHEDULES, TXT_SCRIPT_FILETYPE, NULL, QFileDialog::DontUseNativeDialog));
    if (QFile::copy(scriptFile, WITTYPI_SCHEDULE))
    {
        qDebug() << runScript();
    }
    else
    {
        qDebug() << "Schedule script file copy failed: " << scriptFile;
    }
}

void WittyPi2Window::on_clearScriptButton_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, TXT_PLEASE_CONFIRM, TXT_ARE_YOU_SURE,
        QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QFile file(WITTYPI_SCHEDULE);
        file.remove();
    }
}
