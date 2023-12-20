#ifndef MEMBRANELOGGER_H
#define MEMBRANELOGGER_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QFile>
#include <QCoreApplication>
#include <QTextStream>

QT_BEGIN_NAMESPACE
namespace Ui { class MembraneLogger; }
QT_END_NAMESPACE
#define WAIT_REPLY          200
#define NUMBER_OF_SENSORS   8
#define NUMBER_OF_PORTS     4
#define TIMER_CYCLE         250

class MembraneLogger : public QMainWindow
{
    Q_OBJECT

public:
    MembraneLogger(QWidget *parent = nullptr);
    ~MembraneLogger();

private slots:

    void on_Port_comboBox_currentTextChanged(const QString &arg1);

    void on_ClosePort_pushButton_clicked();

    void send_sensor_data(int port , int sens);

    void on_Exit_pushButton_clicked();

    void on_SetLoop_pushButton_clicked();

private:
    QSerialPort serial;
    Ui::MembraneLogger *ui;
    QByteArray serial_tx(char *cmd);
    int timer0Id;
    int timerint;
    int serial_started;
    int sensors_counter,port_counter;
    int open_close_status;
    int packets;
    int cmd_counter;
    QFile CsvFile;
    QTextStream CsvFileStream;

protected:
    void timerEvent(QTimerEvent *event);
};
#endif // MEMBRANELOGGER_H
