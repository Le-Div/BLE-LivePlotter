#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "device.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    Device *device = new Device;
    void refreshDeviceList();
    QVector<double> plotDataValues;
    QVector<double> plotDataKeys;

private slots:
    void addDeviceNames(QString name);
    void writeToConsole(QString msg);
    void addServiceUUID(QString uuid);
    void addCharacteristicsUUID(QString uuid);
    void refreshServiceUUID();
    void refreshCharacteristicsUUID();
    void receiveRXValue(const QByteArray &value);
    void receiveRXValueToInt(const QByteArray &value);
    void updatePlot();

    void on_searchButton_clicked();
    void on_disconnectButton_clicked();
    void on_sendButton_clicked();

    void on_radioButton_clicked(bool checked);
    void on_clearPlotButton_clicked();

    void on_setMaxPointsSlider_valueChanged(int value);
};
#endif // MAINWINDOW_H
