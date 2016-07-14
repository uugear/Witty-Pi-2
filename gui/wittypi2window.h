#ifndef WITTYPI2WINDOW_H
#define WITTYPI2WINDOW_H

#include <QList>
#include <QMainWindow>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

#define WITTYPI_UTILITIES QString("utilities.sh")
#define WITTYPI_SYNCTIME QString("syncTime.sh")
#define WITTYPI_SCHEDULE QString("schedule.wpi")
#define WITTYPI_SCHEDULES QString("schedules")
#define WITTYPI_RUN_SCRIPT QString("runScript.sh")

#define FUNC_GET_RTC_TIMESTAMP QString("get_rtc_timestamp")
#define FUNC_GET_TEMPERATURE QString("get_temperature")
#define FUNC_SET_RTC_TIME QString("set_rtc_time")
#define FUNC_SYS_TO_RTC QString("system_to_rtc")
#define FUNC_RTC_TO_SYS QString("rtc_to_system")
#define FUNC_GET_LOCAL_DATETIME QString("get_local_date_time")
#define FUNC_GET_UTC_DATETIME QString("get_utc_date_time")
#define FUNC_GET_SHUTDOWN_TIME QString("get_shutdown_time")
#define FUNC_SET_SHUTDOWN_TIME QString("set_shutdown_time")
#define FUNC_CLEAR_SHUTDOWN_TIME QString("clear_shutdown_time")
#define FUNC_GET_STARTUP_TIME QString("get_startup_time")
#define FUNC_SET_STARTUP_TIME QString("set_startup_time")
#define FUNC_CLEAR_STARTUP_TIME QString("clear_startup_time")
#define FUNC_HAS_INTERNET QString("has_internet")

#define PROP_PREVIOUS_VALUE "prevValue"

#define TXT_WINDOW_TITLE QString("Witty Pi 2")
#define TXT_CUR_TEMPERATURE QString("Current Temparature: ")
#define TXT_EDIT QString("Edit")
#define TXT_DONE QString("Done")
#define TXT_PLEASE_CONFIRM QString("Please Confirm")
#define TXT_ARE_YOU_SURE QString("Are you sure to proceed?")
#define TXT_NOT_IN_USE QString("not in use")
#define TXT_IN_USE QString("in use")
#define TXT_CHOOSE_SCRIPT QString("Please choose a schedule script")
#define TXT_SCRIPT_FILETYPE QString("Schedule Script File (*.wpi)")

namespace Ui {
class WittyPi2Window;
}

class WittyPi2Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit WittyPi2Window(QWidget *parent = 0);
    ~WittyPi2Window();

protected:
    void timerEvent(QTimerEvent *event);

private slots:
    void on_wpi2RpiButton_clicked();

    void on_rpiTimeEditButton_clicked();

    void on_wpiTimeEditButton_clicked();

    void on_rpi2WpiButton_clicked();

    void on_ntpUpdateButton_clicked();

    void on_editShutdownButton_clicked();

    void on_clearShutdownButton_clicked();

    void on_editStartupButton_clicked();


    void on_clearStartupButton_clicked();

    void on_chooseScriptButton_clicked();

    void on_clearScriptButton_clicked();

private:
    Ui::WittyPi2Window *ui;

    bool timerPaused;

    QDateTimeEdit* rpiDateTimeEdit;
    QPushButton* rpiTimeEditButton;

    QDateTimeEdit* wpiDateTimeEdit;
    QPushButton* wpiTimeEditButton;

    QPushButton* rpi2WpiButton;
    QPushButton* wpi2RpiButton;
    QPushButton* ntpUpdateButton;

    QLabel* temperatureLabel;

    QLabel* scriptLabel;
    QPushButton* clearScriptButton;
    QPushButton* chooseScriptButton;

    QLineEdit* shutdownDateEdit;
    QLineEdit* shutdownHourEdit;
    QLineEdit* shutdownMinEdit;
    QLineEdit* shutdownSecEdit;
    QPushButton* clearShutdownButton;
    QPushButton* editShutdownButton;

    QLineEdit* startupDateEdit;
    QLineEdit* startupHourEdit;
    QLineEdit* startupMinEdit;
    QLineEdit* startupSecEdit;
    QPushButton* clearStartupButton;
    QPushButton* editStartupButton;

    void keyPressEvent(QKeyEvent* event);

    QString callUtilFunc(QString funcName, QString args, int* exitCode=NULL);

    QString runSyncTime();

    QString runScript();

    QList<QString> parseDateTimeString(QString datetime, int mode=0);

    bool scheduledShutdown();
    bool scheduledStartup();
    bool usingScript();

    void enableButtons();
    void disableButtons();

    void reloadWittyPiTime();
    void reloadRaspberryPiTime();
    void reloadTemperature();
    void reloadShutdownTime();
    void reloadStartupTime();
    void reloadScriptStatus();
};

#endif // WITTYPI2WINDOW_H
