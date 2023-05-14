#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Set up the plot Widget

    // Add Graph 0
    ui->customplot->addGraph();
    ui->customplot->graph(0)->setScatterStyle(QCPScatterStyle::ssCircle);
    ui->customplot->graph(0)->setLineStyle(QCPGraph::lsLine);
    ui->customplot->graph(0)->setPen(QPen(Qt::blue));

    // Add Graph 1
    ui->customplot->addGraph();
    ui->customplot->graph(1)->setScatterStyle(QCPScatterStyle::ssCircle);
    ui->customplot->graph(1)->setLineStyle(QCPGraph::lsLine);
    ui->customplot->graph(1)->setPen(QPen(Qt::red));

    // Add Graph 2
    ui->customplot->addGraph();
    ui->customplot->graph(2)->setScatterStyle(QCPScatterStyle::ssCircle);
    ui->customplot->graph(2)->setLineStyle(QCPGraph::lsLine);
    ui->customplot->graph(2)->setPen(QPen(Qt::red));

    ui->customplot->xAxis->setLabel("X");
    ui->customplot->yAxis->setLabel("Y");
    ui->customplot->xAxis->setRange(-6000,100);
    ui->customplot->yAxis->setRange(-6000,8000);


    // Set up the slider object
    ui->setMaxPointsSlider->setMinimum(100);
    ui->setMaxPointsSlider->setMaximum(500);
    ui->setMaxPointsSlider->setValue(200);

    //Set up the connections to the device class

    // Device discovery related
    connect(ui->searchButton,&QPushButton::clicked,device,&Device::startDeviceDiscovery);
    connect(device,&Device::consoleOutput,this,&MainWindow::writeToConsole);
    connect(device,&Device::sendDeviceName,this,&MainWindow::addDeviceNames);

    // Service discovery related
    connect(device,&Device::sendServiceUUID,this,&MainWindow::addServiceUUID);
    connect(device,&Device::refreshServiceUUID,this,&MainWindow::refreshServiceUUID);

    // Characteristic discovery related
    connect(device,&Device::sendCharacteristicsUUID,this,&MainWindow::addCharacteristicsUUID);
    connect(device,&Device::refreshCharacteristicsUUID,this,&MainWindow::refreshCharacteristicsUUID);

    // Connect receive RX Data function
    connect(device,&Device::sendRXValue,this,&MainWindow::receiveRXValue);

    // Handle Disconnects
    connect(ui->disconnectButton,&QPushButton::clicked,device,&Device::disconnectFromDevice);

    // Clear Console
    connect(ui->clearConsoleButton,&QPushButton::clicked,ui->console,&QPlainTextEdit::clear);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::addDeviceNames(QString name)
{
    if(name!="")
    {
        ui->comboBox_device->addItem(name);

    }
}

void MainWindow::writeToConsole(QString msg)
{
    ui->console->appendPlainText(msg);
}

void MainWindow::on_searchButton_clicked()
{

    this->refreshDeviceList();

    // Set service discovery on selected combobox item
    connect(ui->comboBox_device,&QComboBox::currentTextChanged,device,&Device::scanService);
    connect(ui->comboBox_service,&QComboBox::currentTextChanged,device,&Device::connectToService);

    // Set characteristics connections
    connect(ui->comboBox_Rx,&QComboBox::currentTextChanged,device,&Device::connectToRXCharacteristic);
    connect(ui->comboBox_Tx,&QComboBox::currentTextChanged,device,&Device::setTXCharacteristic);
}

void MainWindow::refreshDeviceList()
{
    ui->comboBox_device->clear();
    ui->comboBox_device->addItem("No device selected");
}

void MainWindow::addServiceUUID(QString uuid)
{
    ui->comboBox_service->addItem(uuid);
}

void MainWindow::refreshServiceUUID()
{
  ui->comboBox_service->clear();
  ui->comboBox_service->addItem("No service selected");
}

void MainWindow::addCharacteristicsUUID(QString uuid)
{
    ui->comboBox_Rx->addItem(uuid);
    ui->comboBox_Tx->addItem(uuid);
}

void MainWindow::refreshCharacteristicsUUID()
{
    ui->comboBox_Rx->clear();
    ui->comboBox_Tx->clear();

    ui->comboBox_Rx->addItem("No characteristic selected");
    ui->comboBox_Tx->addItem("No characteristic selected");
}

void MainWindow::on_disconnectButton_clicked()
{
    ui->comboBox_device->setCurrentIndex(0);
    ui->comboBox_service->setCurrentIndex(0);
    ui->comboBox_Rx->setCurrentIndex(0);
    ui->comboBox_Tx->setCurrentIndex(0);
}


void MainWindow::on_sendButton_clicked()
{
    QString message = ui->lineEdit->text();
    device->writeToTXCharacteristic(message);
}

void MainWindow::receiveRXValue(const QByteArray &value)
{
    QString Data = QString(value);
    ui->console->appendPlainText("Data send:");
    ui->console->appendPlainText(Data);
}


void MainWindow::receiveRXValueToInt(const QByteArray &value)
{
    qDebug() << value.length();

    if(value.length()<3)
    {
        return;
    }

    int arrayLength = (int)(value.length()/2);
    int16_t dataPoints[arrayLength];

    for(int i=0;i<arrayLength;i++)
    {
        dataPoints[i]=0;
    }

    for(int i=0;i<arrayLength;i++)
    {
        unsigned char HV = value[2*i+1];
        unsigned char LV = value[2*i+2];

        dataPoints[i] |=  (HV<<8)|(LV);
        //qDebug() << dataPoints[i];
        plotDataValues.append(dataPoints[i]);
    }
    this->updatePlot();
}

void MainWindow::on_radioButton_clicked(bool checked)
{
    if(checked)
    {
        disconnect(device,&Device::sendRXValue,this,&MainWindow::receiveRXValue);
        connect(device,&Device::sendRXValue,this,&MainWindow::receiveRXValueToInt);
    }
    else
    {
        connect(device,&Device::sendRXValue,this,&MainWindow::receiveRXValue);
        disconnect(device,&Device::sendRXValue,this,&MainWindow::receiveRXValueToInt);
    }
}

void MainWindow::on_clearPlotButton_clicked()
{
    ui->customplot->graph(0)->data()->clear();
    ui->customplot->replot();
    ui->customplot->update();
}


void MainWindow::updatePlot()
{

    int maxDataPoints = ui->setMaxPointsSlider->value();
    int valueLength = plotDataValues.length();

    if(valueLength>=maxDataPoints)
    {
        for(int i=0;i<=(valueLength-maxDataPoints);i++)
        {
            plotDataValues.pop_front();
        }
    }

    valueLength = plotDataValues.length();
    plotDataKeys.clear();

    for(int i=0;i<valueLength;i++)
    {
        plotDataKeys.append(i);
    }

    ui->customplot->graph(0)->setData(plotDataKeys,plotDataValues);
    ui->customplot->graph(0)->rescaleAxes();
    ui->customplot->replot();
    ui->customplot->update();
}


void MainWindow::on_setMaxPointsSlider_valueChanged(int value)
{
    QString maxPointValue = QString::number(value);
    ui->maxPointsLabel->setText(maxPointValue);
}


























