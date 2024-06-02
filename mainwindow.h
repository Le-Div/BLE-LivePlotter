#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QDataStream>
#include <QTimer>
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
    void saveQVectorToFile(const QVector<double>& data, const QString& filePath);
    void convertRawToIntData();

    QByteArray rawData;
    QByteArray rawValue;
    int value_length = 0;


    int plotCNT = 0;
    QVector<double> plotDataValues_x;
    QVector<double> plotDataValues_y;
    QVector<double> plotDataValues_z;
    QVector<double> plotDataKeys;
    QString DataFolder = "/Users/davidlohuis/Documents/Projekte/Projekt-Nocken/Projekt-Bluetoothnocken/Data";
    QTimer updatePlot_timer;

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

    void on_clearPlotButton_clicked();

    void on_setMaxPointsSlider_valueChanged(int value);
    void on_save_to_file_button_clicked();
    void on_set_folder_button_clicked();
    void on_get_folder_button_clicked();
    void on_Run_Measure_stateChanged(int arg1);
    void saveAccDataToByteArray(const QByteArray &value);
    void convertAndPlot();
    void on_get_single_point_clicked();
    void on_get_Data_stateChanged(int arg1);
};
#endif // MAINWINDOW_H
