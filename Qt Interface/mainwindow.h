#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QtSerialPort/QSerialPort>
#include "settingsdialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    void openSerialPort();
    void closeSerialPort();
    void readData();

    void handleError(QSerialPort::SerialPortError error);

private:
    Ui::MainWindow *ui;
    SettingsDialog *settings;
    QSerialPort *serial;
};

#endif // MAINWINDOW_H
