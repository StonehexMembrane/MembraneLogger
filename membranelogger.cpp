#include "membranelogger.h"
#include "ui_membranelogger.h"
#include <QApplication>
#include <QDebug>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QFile>
#include <QCoreApplication>
#include <QTextStream>

MembraneLogger::MembraneLogger(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MembraneLogger)
{
    ui->setupUi(this);
    open_close_status=0;
    ui->ClosePort_pushButton->setText("Start");
    timer0Id = 0;
    port_counter = 0;
    sensors_counter = 0;
    packets = 0;
    cmd_counter = 0;
}

MembraneLogger::~MembraneLogger()
{
    delete ui;
}

QByteArray MembraneLogger::serial_tx(char *cmd)
{
    QPixmap redled (":/ledred.png");
    QByteArray reply;
    if ( serial_started == 0 )
        return "1";
    serial.flush();
    serial.write(cmd);
    if(serial.waitForReadyRead(WAIT_REPLY))
    {
        reply = serial.readAll();
        packets++;
        QString result;
        result = QString::number(packets).rightJustified(5, '0');
        ui->PacketsReceived_lineEdit->setText(result);
        return reply;
    }
    return "1";
}

void MembraneLogger::on_Port_comboBox_currentTextChanged(const QString &arg1)
{
    QPixmap redled (":/ledred.png");
    QPixmap greenled(":/ledgreen.png");

    serial.close();
    serial_started = 0;
    serial.setPortName(arg1);
    if(serial.open(QIODevice::ReadWrite))
    {
        if(!serial.setBaudRate(QSerialPort::Baud115200))
        {
            ui->Comm_label->setPixmap(redled);
            qDebug()<< arg1 << " : " << serial.errorString();
            ui->statusbar->showMessage(arg1+" : "+serial.errorString());
        }
        else
        {
            ui->Comm_label->setPixmap(greenled);
            serial_started = 1;
            qDebug()<< "Serial port opened";
            ui->statusbar->showMessage(arg1+" : Serial port opened");
            serial.setReadBufferSize (1024);
            QByteArray reply;
            if(serial.waitForReadyRead(WAIT_REPLY))
            {
                reply = serial.readAll();
                qDebug()<<reply;
            }
        }
    }
    else
    {
        ui->Comm_label->setPixmap(redled);
        qDebug()<< arg1 << " : " << serial.errorString();
        ui->statusbar->showMessage(arg1+" : "+serial.errorString());
    }
}

int     pnum = 0;
int     toggle_led = 0;
QString dirPath= "c:/MembraneLogs";

void MembraneLogger::send_sensor_data(int port , int sens )
{
    char    cmd[32];
    int     line,sensor,result,scale_down_sine,floor_noise,temperature;
    char    c0,c1,status,address;
    int     fexists = 0;
    QByteArray reply;
    QPixmap redled (":/ledred.png");
    QPixmap greenled(":/ledgreen.png");

    QDateTime currentDateTime = QDateTime::currentDateTime();
    QDate currentDate = currentDateTime.date();
    QTime currentTime = currentDateTime.time();
    QString timestamp = currentTime.toString("hh:mm:ss");
    QString hourtimestamp = currentTime.toString("hh");
    QString dayPath = "Day_" + currentDate.toString("dd_MM_yyyy");
    QString hourPath = "Hour_" + currentTime.toString("hh");

    if ( toggle_led == 0 )
        ui->Tim_label->setPixmap(greenled);
    else
        ui->Tim_label->setPixmap(redled);
    toggle_led++;
    toggle_led &= 1;
    /* check for dirs */
    QDir top_directory(dirPath);
    if ( !top_directory.exists())
    {
        qDebug()<< dirPath << " created";
        top_directory.mkpath(dirPath);
    }
    QDir day_directory(dirPath+"/"+dayPath);
    if ( !day_directory.exists())
    {
        qDebug()<< dirPath+"/"+dayPath << " created";
        day_directory.mkpath(dirPath+"/"+dayPath);
    }

    QDir hour_directory(dirPath+"/"+dayPath+"/"+hourPath);
    if ( !hour_directory.exists())
    {
        qDebug()<< dirPath+"/"+dayPath+"/"+hourPath << " created";
        day_directory.mkpath(dirPath+"/"+dayPath+"/"+hourPath);
    }

    QString filename = dirPath+"/"+dayPath+"/"+hourPath+"/"+"SensorsLog_Hour"+ hourtimestamp + "__"  + currentDate.toString("dd_MM_yyyy.log") ;
    //qDebug()<< "File is : " << filename;

    QFile file(filename);
    if ( ! file.exists())
    {
        qDebug()<< filename << " not present";
        fexists = 1;
    }
    CsvFile.setFileName(filename);
    CsvFile.open(QIODevice::Append | QIODevice::Text);
    CsvFileStream.setDevice(&CsvFile);
    if ( fexists == 1)
        CsvFileStream << "Command Counter,Time Stamp,Line,Sensor Result,Floor Noise,Temperature,timer @ "<<timerint<<"mSec.\n\r";

    if ( ui->single_loop_checkBox->isChecked())
        sprintf(cmd,"<d 0 %d %d>",ui->setloop_line_comboBox->currentIndex(),ui->setloop_sensors_comboBox->currentIndex());
    else
        sprintf(cmd,"<d 0 %d %d>",port,sens);
    qDebug()<< cmd;

    cmd_counter ++;
    if ( (reply = serial_tx(cmd)) != "1" )
    {
        status = 'A';
        pnum = sscanf(reply,"<%c %d %c %d %c %c %d %d %d %d>",&c0,&line,&c1,&sensor,&status,&address,&result,&scale_down_sine,&floor_noise,&temperature);
        qDebug()<< cmd_counter << c0 << " " << line << " " << c1 << " " << sensor << " " << status << " " << address << " " << result << " " << scale_down_sine << " " << floor_noise << " " << temperature;
        CsvFileStream << cmd_counter << "," << timestamp << ", " << line  << "," << sensor << "," << result << "," << floor_noise << "," << scale_down_sine  << "," << temperature << "\n";
    }
    else
    {
        qDebug()<< port << " " << sens << " " << " Not Connected";
        CsvFileStream << cmd_counter << ", Not Connected\n";
    }
    CsvFile.close();

}

void MembraneLogger::timerEvent(QTimerEvent *event)
{
    if ( event->timerId() == timer0Id )
    {
        ui->statusbar->showMessage("Running");
        sensors_counter ++;
        if ( sensors_counter >= ui->setloop_sensors_comboBox->currentIndex()+1 )
        {
            sensors_counter = 0;
            port_counter++;
            if ( port_counter >= ui->setloop_line_comboBox->currentIndex()+1 )
            {
                port_counter = 0;
            }
        }
        send_sensor_data(port_counter,sensors_counter);
    }
}

void MembraneLogger::on_Exit_pushButton_clicked()
{
    CsvFile.close();
    close();
}

void MembraneLogger::on_ClosePort_pushButton_clicked()
{
    QPixmap redled(":/ledred.png");
    char    cmd[32];
    QByteArray reply;

    if ( open_close_status == 0 ) //  closed
    {
        on_Port_comboBox_currentTextChanged(ui->Port_comboBox->currentText());
        qDebug()<<"Port opened";
        cmd_counter = 0;
        open_close_status=1;
        ui->ClosePort_pushButton->setText("Stop");
        ui->statusbar->showMessage("Started");

        on_SetLoop_pushButton_clicked();
        QString timerloop = ui->setloop_looptime_comboBox->currentText();
        timerint = timerloop.toInt();
        timer0Id = startTimer(timerint);
        qDebug()<<"Timer @ "<<timerint;
    }
    else
    {
        ui->Comm_label->setPixmap(redled);
        serial.close();
        qDebug()<<"Port closed";
        ui->statusbar->showMessage("Stopped");
        ui->Tim_label->setPixmap(redled);

        ui->ClosePort_pushButton->setText("Start");
        open_close_status=0;
        if ( timer0Id > 0 )
            killTimer(timer0Id);
        port_counter = 0;
        sensors_counter = 0;
        packets = 0;
    }
}


void MembraneLogger::on_SetLoop_pushButton_clicked()
{
char    cmd[32];
QByteArray reply;

    if ( ! serial.isOpen() )
    {
        serial.open(QIODevice::ReadWrite);
    }
    sprintf(cmd,"<f>");
    qDebug()<<cmd;
    if ( (reply = serial_tx(cmd)) != "1" )
            qDebug()<<reply;
    sprintf(cmd,"<h>");
    qDebug()<<cmd;
    if ( (reply = serial_tx(cmd)) != "1" )
            qDebug()<<reply;

    sprintf(cmd,"<c 0 %d %d>", ui->setloop_line_comboBox->currentIndex(),ui->setloop_sensors_comboBox->currentIndex()+1);
    qDebug()<<cmd;
    if ( (reply = serial_tx(cmd)) != "1" )
            qDebug()<<reply;

    sprintf(cmd,"<t 5>");
    qDebug()<<cmd;
    if ( (reply = serial_tx(cmd)) != "1" )
            qDebug()<<reply;

    sprintf(cmd,"<f>");
    qDebug()<<cmd;
    if ( (reply = serial_tx(cmd)) != "1" )
            qDebug()<<reply;

}


/*
void MembraneLogger::on_Single_pushButton_clicked()
{
    char    cmd[32];
    int     line,sensor,result,scale_down_sine,floor_noise,temperature;
    char    c0,c1,status,address;
    QByteArray reply;

    serial.setPortName(ui->Port_comboBox->currentText());
    if(!serial.open(QIODevice::ReadWrite))
            return;
    if(!serial.setBaudRate(QSerialPort::Baud115200))
            return;

    sprintf(cmd,"<d 0 %d %d>",ui->single_line_comboBox->currentIndex()  ,ui->single_sensors_comboBox->currentIndex());
    qDebug()<<cmd;
    if ( (reply = serial_tx(cmd)) != "1" )
    {
            status = 'A';
            pnum = sscanf(reply,"<%c %d %c %d %c %c %d %d %d %d>",&c0,&line,&c1,&sensor,&status,&address,&result,&scale_down_sine,&floor_noise,&temperature);
            qDebug()<< cmd_counter << c0 << " " << line << " " << c1 << " " << sensor << " " << status << " " << address << " " << result << " " << scale_down_sine << " " << floor_noise << " " << temperature;
    }
    else
    {
            qDebug()<< ui->single_line_comboBox->currentIndex() << " " << ui->single_sensors_comboBox->currentIndex() << " " << " Not Connected";
    }
    serial.close();
}

*/
