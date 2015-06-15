#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QtSerialPort/QSerialPort>

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    serial = new QSerialPort(this);
    settings = new SettingsDialog;

    ui->connectButton->setEnabled(true);
    ui->disconnectButton->setEnabled(false);
    ui->settingsButton->setEnabled(true);

    connect(ui->connectButton, SIGNAL(clicked()), this, SLOT(openSerialPort()));
    connect(ui->disconnectButton, SIGNAL(clicked()), this, SLOT(closeSerialPort()));
    connect(ui->settingsButton, SIGNAL(clicked()), settings, SLOT(show()));

    connect(serial, SIGNAL(error(QSerialPort::SerialPortError)), this,SLOT(handleError(QSerialPort::SerialPortError)));
    connect(serial, SIGNAL(readyRead()), this, SLOT(readData()));
}

void MainWindow::openSerialPort()
{
    SettingsDialog::Settings p = settings->settings();
    serial->setPortName(p.name);
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    if (serial->open(QIODevice::ReadWrite)) {
            ui->connectButton->setEnabled(false);
            ui->disconnectButton->setEnabled(true);
            ui->settingsButton->setEnabled(false);
            qDebug() << QString("Connected to %1 : %2, %3, %4, %5, %6").arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits).arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl);
    } else {
        QMessageBox::critical(this, tr("Error"), serial->errorString());
        qDebug() << "Open error";
    }
}

void MainWindow::closeSerialPort()
{
    if (serial->isOpen())
        serial->close();
    ui->connectButton->setEnabled(true);
    ui->disconnectButton->setEnabled(false);
    ui->settingsButton->setEnabled(true);
    qDebug() << "Disconnected";
}

void MainWindow::readData()
{
    QByteArray data = serial->readAll();
    int i;
    QString X, Y, Z;
    for (i=7; i<14; i++)
        X += data[i];
    for (i=24; i<31; i++)
        Y += data[i];
    for (i=41; i<47; i++)
        Z += data[i];
    double x, y, z;
    x = X.toDouble();
    y = Y.toDouble();
    z = Z.toDouble();

    ui->view->setXAngle((int)y);
    ui->XValue->display(x);
    ui->view->setYAngle((int)z);
    ui->YValue->display(y);
    ui->view->setZAngle((int)x);
    ui->ZValue->display(z);

    ui->view->update();
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), serial->errorString());
        closeSerialPort();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
