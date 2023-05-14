#include "device.h"

#include <qbluetoothaddress.h>
#include <qbluetoothdevicediscoveryagent.h>
#include <qbluetoothlocaldevice.h>
#include <qbluetoothdeviceinfo.h>
#include <qbluetoothservicediscoveryagent.h>
#include <QDebug>
#include <QList>
#include <QMetaEnum>
#include <QTimer>

Device::Device()
{

    discoveryAgent = new QBluetoothDeviceDiscoveryAgent();
    discoveryAgent->setLowEnergyDiscoveryTimeout(10000);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &Device::addDevice);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this,
            &Device::deviceScanError);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &Device::deviceScanFinished);

}

Device::~Device()
{
    delete discoveryAgent;
    delete controller;
    qDeleteAll(devices);
    qDeleteAll(m_services);
    qDeleteAll(m_characteristics);
    devices.clear();
    m_services.clear();
    m_characteristics.clear();

}


void Device::startDeviceDiscovery()
{
    qDeleteAll(devices);
    devices.clear();

    emit consoleOutput("Scanning for devices ...");

    discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);

    if (discoveryAgent->isActive()) {
        m_deviceScanState = true;
        emit consoleOutput("Discovery agent is active.");
    }
    else{
        m_deviceScanState = false;
        emit consoleOutput("Discovery agent is not active.");
    }
}

void Device::addDevice(const QBluetoothDeviceInfo &info)
{
    if (info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
        emit consoleOutput("Last device added: " + info.name());
        emit sendDeviceName(info.name());
}

void Device::deviceScanFinished()
{
    const QList<QBluetoothDeviceInfo> foundDevices = discoveryAgent->discoveredDevices();
    for (auto &nextDevice : foundDevices)
        if (nextDevice.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
            devices.append(new DeviceInfo(nextDevice));

    m_deviceScanState = false;

    if (devices.isEmpty())
        emit consoleOutput("No Low Energy devices found...");
    else
        emit consoleOutput("Done! Scan Again!");
}


void Device::deviceScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    if (error == QBluetoothDeviceDiscoveryAgent::PoweredOffError)
        emit consoleOutput("The Bluetooth adaptor is powered off, power it on before doing discovery.");
    else if (error == QBluetoothDeviceDiscoveryAgent::InputOutputError)
        emit consoleOutput("Writing or reading from the device resulted in an error.");
    else {
        static QMetaEnum qme = discoveryAgent->metaObject()->enumerator(
                    discoveryAgent->metaObject()->indexOfEnumerator("Error"));
        emit consoleOutput("Error: " + QLatin1String(qme.valueToKey(error)));
    }

    m_deviceScanState = false;
    //emit devicesUpdated();
    //emit stateChanged();
}


void Device::scanService(const QString &name)
{

    //Find the selected device by name
    for (auto d: std::as_const(devices)) {
        if (auto device = qobject_cast<DeviceInfo *>(d)) {
            if (device->getName() == name ) {
                currentDevice.setDevice(device->getDevice());
                break;
            }
        }
    }

    if (!currentDevice.getDevice().isValid()) {
        qWarning() << "Not a valid device";
        return;
    }

    qDeleteAll(m_characteristics);
    m_characteristics.clear();
    emit refreshCharacteristicsUUID();
    emit consoleOutput("Characteristics cleared");
    qDeleteAll(m_services);
    m_services.clear();
    emit refreshServiceUUID();
    emit consoleOutput("Service cleared");

    emit consoleOutput("Start connecting to Device");

    if (controller && m_previousName != currentDevice.getName()) {
        controller->disconnectFromDevice();
        delete controller;
        controller = nullptr;
    }

    if (!controller) {
        // Connecting signals and slots for connecting to LE services.
        controller = QLowEnergyController::createCentral(currentDevice.getDevice());
        connect(controller, &QLowEnergyController::connected,this, &Device::deviceConnected);
        connect(controller, &QLowEnergyController::errorOccurred, this, &Device::errorReceived);
        connect(controller, &QLowEnergyController::disconnected,this, &Device::deviceDisconnected);
        connect(controller, &QLowEnergyController::serviceDiscovered,this, &Device::addLowEnergyService);
        connect(controller, &QLowEnergyController::discoveryFinished,this, &Device::serviceScanDone);
    }

    if (isRandomAddress())
        controller->setRemoteAddressType(QLowEnergyController::RandomAddress);
    else
        controller->setRemoteAddressType(QLowEnergyController::PublicAddress);
    controller->connectToDevice();

}


void Device::deviceConnected()
{
    emit consoleOutput("Discovering services...");
    connected = true;
    controller->discoverServices();

}

void Device::disconnectFromDevice()
{
    // UI always expects disconnect() signal when calling this signal
    // TODO what is really needed is to extend state() to a multi value
    // and thus allowing UI to keep track of controller progress in addition to
    // device scan progress
    if(controller!=nullptr && connected==true)
    {
        if (controller->state() != QLowEnergyController::UnconnectedState)
        {
            controller->disconnectFromDevice();
            connected = false;
        }
        else
        {
            emit consoleOutput("No device connected");
        }
    }
}

void Device::errorReceived(QLowEnergyController::Error /*error*/)
{
    qWarning() << "Error: " << controller->errorString();
    emit consoleOutput(QString("Back\n(%1)").arg(controller->errorString()));
}

void Device::deviceDisconnected()
{
    qWarning() << "Disconnect from device";
    emit consoleOutput("Device Disconnected!");
}

void Device::addLowEnergyService(const QBluetoothUuid &serviceUuid)
{

    QLowEnergyService *service = controller->createServiceObject(serviceUuid);
    if (!service) {
        qWarning() << "Cannot create service for uuid";
        return;
    }

    QString myUUID = serviceUuid.toString();
    myUUID.remove("{");
    myUUID.remove("}");

    emit sendServiceUUID(myUUID);

    auto serv = new ServiceInfo(service);
    m_services.append(serv);

    emit consoleOutput("Services Updated!");
}

void Device::serviceScanDone()
{
    emit consoleOutput("Service scan done!");
    // force UI in case we didn't find anything
    if (m_services.isEmpty())
        emit consoleOutput("Services Updated!");
}

bool Device::isRandomAddress() const
{
    return randomAddress;
}


void Device::connectToService(const QString &uuid)
{

    QLowEnergyService *service = nullptr;
    for (auto s: std::as_const(m_services)) {
        auto serviceInfo = qobject_cast<ServiceInfo *>(s);
        if (!serviceInfo)
            continue;

        if (serviceInfo->getUuid() == uuid) {
            service = serviceInfo->service();
            break;
        }
    }

    if (!service)
        return;

    currentService = service;

    qDeleteAll(m_characteristics);
    m_characteristics.clear();
    emit consoleOutput("Characteristics reset occured");

    if (service->state() == QLowEnergyService::RemoteService) {

        connect(service, &QLowEnergyService::stateChanged,
                this, &Device::serviceDetailsDiscovered);
        service->discoverDetails();

        emit consoleOutput("Discovering details...");

        return;
    }

    //discovery already done
    const QList<QLowEnergyCharacteristic> chars = service->characteristics();
    for (const QLowEnergyCharacteristic &ch : chars) {
        auto cInfo = new CharacteristicInfo(ch);
        m_characteristics.append(cInfo);
        emit sendCharacteristicsUUID(cInfo->getUuid());
    }

    //QTimer::singleShot(0, this, &Device::characteristicsUpdated);
}


void Device::serviceDetailsDiscovered(QLowEnergyService::ServiceState newState)
{
    if (newState != QLowEnergyService::RemoteServiceDiscovered) {
        // do not hang in "Scanning for characteristics" mode forever
        // in case the service discovery failed
        // We have to queue the signal up to give UI time to even enter
        // the above mode
        if (newState != QLowEnergyService::RemoteServiceDiscovering) {
            QMetaObject::invokeMethod(this, "characteristicsUpdated",
                                      Qt::QueuedConnection);
        }
        return;
    }

    auto service = qobject_cast<QLowEnergyService *>(sender());
    if (!service)
        return;

    const QList<QLowEnergyCharacteristic> chars = service->characteristics();
    for (const QLowEnergyCharacteristic &ch : chars) {
        auto cInfo = new CharacteristicInfo(ch);
        m_characteristics.append(cInfo);
        emit sendCharacteristicsUUID(cInfo->getUuid());
    }


    emit consoleOutput("Characteristics updated");
}


void Device::connectToRXCharacteristic(const QString &uuid)
{

    QLowEnergyCharacteristic characteristic;
    for (auto c: std::as_const(m_characteristics)) {
        auto characteristicInfo = qobject_cast<CharacteristicInfo *>(c);
        if (!characteristicInfo)
            continue;

        if (characteristicInfo->getUuid() == uuid) {
            characteristic = characteristicInfo->getCharacteristic();
            break;
        }
    }

    if(!characteristic.isValid())
        return;


    // subcribing to a characteristic
    auto cccd = characteristic.clientCharacteristicConfiguration();
    currentService->writeDescriptor(cccd, QLowEnergyCharacteristic::CCCDEnableNotification);

    // set up Data transmition when characteristic is changed
    connect(currentService,&QLowEnergyService::characteristicChanged,this,&Device::updateRXValue);

}

void Device::updateRXValue(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    //qDebug() << value;
    emit sendRXValue(value);
}

void Device::setTXCharacteristic(const QString &uuid)
{
    QLowEnergyCharacteristic characteristic;
    for (auto c: std::as_const(m_characteristics)) {
        auto characteristicInfo = qobject_cast<CharacteristicInfo *>(c);
        if (!characteristicInfo)
            continue;

        if (characteristicInfo->getUuid() == uuid) {
            characteristic = characteristicInfo->getCharacteristic();
            break;
        }
    }

    if(!characteristic.isValid())
        return;

    writeCharacteristic = characteristic;
}

void Device::writeToTXCharacteristic(QString &message)
{
    QByteArray myMessage = message.toUtf8();
    myMessage.prepend(0x01);
    myMessage.append(0x0D);
    currentService->writeCharacteristic(writeCharacteristic,myMessage,QLowEnergyService::WriteWithoutResponse);
}











