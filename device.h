#ifndef DEVICE_H
#define DEVICE_H

#include <qbluetoothlocaldevice.h>
#include <QObject>
#include <QVariant>
#include <QList>
#include <QBluetoothServiceDiscoveryAgent>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QBluetoothServiceInfo>
#include "deviceinfo.h"
#include "serviceinfo.h"
#include "characteristicinfo.h"

class Device: public QObject
{

    Q_OBJECT

public:
    Device();
    ~Device();

    bool isRandomAddress() const;

private:
    QBluetoothDeviceDiscoveryAgent *discoveryAgent;
    bool m_deviceScanState = false;
    DeviceInfo currentDevice;
    QLowEnergyService *currentService;
    QList<DeviceInfo *> devices;
    QList<ServiceInfo *> m_services;
    QList<CharacteristicInfo *>m_characteristics;
    QString m_previousName;
    QString m_previousService;
    bool connected = false;
    QLowEnergyController *controller = nullptr;
    bool randomAddress = false;
    QLowEnergyCharacteristic writeCharacteristic;


public slots:
    void startDeviceDiscovery();
    void scanService(const QString &name);
    void connectToService(const QString &uuid);
    void disconnectFromDevice();
    void connectToRXCharacteristic(const QString &uuid);
    void writeToTXCharacteristic(QString &message);
    void setTXCharacteristic(const QString &uuid);



private slots:
    // QBluetoothDeviceDiscoveryAgent related
    void addDevice(const QBluetoothDeviceInfo &info);
    void deviceScanFinished();
    void deviceScanError(QBluetoothDeviceDiscoveryAgent::Error error);

    // QLowEnergyController related
    void addLowEnergyService(const QBluetoothUuid &uuid);
    void deviceConnected();
    void errorReceived(QLowEnergyController::Error);
    void serviceScanDone();
    void deviceDisconnected();

    // QLowEnergyService related
    void serviceDetailsDiscovered(QLowEnergyService::ServiceState newState);

    // QLowEnergyCharacteristic related
    void updateRXValue(const QLowEnergyCharacteristic &c, const QByteArray &value);

signals:
    void consoleOutput(QString msg);
    void sendDeviceName(QString name);
    void sendServiceUUID(QString uuid);
    void sendCharacteristicsUUID(QString uuid);
    void refreshServiceUUID();
    void refreshCharacteristicsUUID();
    void sendRXValue(const QByteArray &value);
};

#endif // DEVICE_H
