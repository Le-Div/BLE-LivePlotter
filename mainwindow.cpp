#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QDataStream>
#include <QTimer>

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
    ui->customplot->graph(2)->setPen(QPen(Qt::green));

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

    // Timer for updating the plot in a specific interval
    //connect(&updatePlot_timer, &QTimer::timeout, this, &MainWindow::convertAndPlot);
    //updatePlot_timer.setInterval(5000);

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
    qDebug() << value.length();
    QString Data = QString(value);
    ui->console->appendPlainText("Data send:");
    ui->console->appendPlainText(Data);
}


void MainWindow::receiveRXValueToInt(const QByteArray &value)
{
    //The value Array contains the bytes send via Bluetooth
    value_length += value.length()-1;
    qDebug() << value_length;

    //Only accept Data in the correct format
    if(value.length()<7)
    {
        return;
    }

    // Initialize an Array where the data will be stored temporarily
    int arrayLength = (int)(value.length()/2);
    int16_t dataPoints[arrayLength];

    // Fill the temp Data array with 0
    for(int i=0;i<arrayLength;i++)
    {
        dataPoints[i]=0;
    }

    // Fill the temp Data array with the correct values
    for(int i=0;i<arrayLength;i++)
    {
        unsigned char HV = value[2*i+1];
        unsigned char LV = value[2*i+2];

        dataPoints[i] |=  (HV<<8)|(LV);
        //qDebug() << dataPoints[i];
    }

    // Add the new data to the correct array
    plotDataValues_x.append(dataPoints[0]);
    plotDataValues_y.append(dataPoints[1]);
    plotDataValues_z.append(dataPoints[2]);

    plotCNT++;
    if(plotCNT==1){
        this->updatePlot();
        plotCNT=0;
    }
}

void MainWindow::saveAccDataToByteArray(const QByteArray &value)
{
    QString testletter = QString(value);
    writeToConsole("Data:");
    writeToConsole(testletter);

    QByteArray data = value;
    data.remove(0,1);

    value_length += data.length();
    uint8_t intdata = (uint8_t)data[0];
    qDebug() << intdata;

    //qDebug() << value.length();
    //qDebug() << data.length();

    //rawData.append(data);
    //rawValue.append(value);
}

void MainWindow::convertRawToIntData()
{


    int16_t xValue = rawData.left(2)[0];
    int16_t yValue = rawData.left(2)[2];
    int16_t zValue = rawData.left(2)[4];

    qDebug() << "x-y-z:";
    qDebug() << xValue;
    qDebug() << yValue;
    qDebug() << zValue;

    //rawData.remove(0,6);

    //plotDataValues_x.append(xValue);
    //plotDataValues_y.append(yValue);
    //plotDataValues_z.append(zValue);

}

void MainWindow::updatePlot()
{

    int valueLength = plotDataValues_x.length();


    if(!(ui->Dis_max_data->isChecked()))
    {
        int maxDataPoints = ui->setMaxPointsSlider->value();
        if(valueLength>=maxDataPoints)
        {
            for(int i=0;i<=(valueLength-maxDataPoints);i++)
            {
                plotDataValues_x.pop_front();
                plotDataValues_y.pop_front();
                plotDataValues_z.pop_front();
            }
        }
        valueLength = plotDataValues_x.length();
    }

    plotDataKeys.clear();
    for(int i=0;i<valueLength;i++)
    {
        plotDataKeys.append(i);
    }

    ui->customplot->graph(0)->setData(plotDataKeys,plotDataValues_x);
    ui->customplot->graph(1)->setData(plotDataKeys,plotDataValues_y);
    ui->customplot->graph(2)->setData(plotDataKeys,plotDataValues_z);

    bool en_x = ui->en_x_axis->isChecked();
    bool en_y = ui->en_y_axis->isChecked();
    bool en_z = ui->en_z_axis->isChecked();

    ui->customplot->graph(0)->setVisible(en_x);
    ui->customplot->graph(1)->setVisible(en_y);
    ui->customplot->graph(2)->setVisible(en_z);

    ui->customplot->rescaleAxes(true);
    ui->customplot->replot();
    ui->customplot->update();
}

void MainWindow::convertAndPlot()
{
    int Data_length = rawData.length()/6;

    for(int i=0;i<Data_length;i++)
    {
        this->convertRawToIntData();
    }

    this->updatePlot();
}



void MainWindow::on_clearPlotButton_clicked()
{
    ui->customplot->graph(0)->data()->clear();
    ui->customplot->graph(1)->data()->clear();
    ui->customplot->graph(2)->data()->clear();

    plotDataValues_x.clear();
    plotDataValues_y.clear();
    plotDataValues_z.clear();

    ui->customplot->replot();
    ui->customplot->update();
}

void MainWindow::on_setMaxPointsSlider_valueChanged(int value)
{
    QString maxPointValue = QString::number(value);
    ui->maxPointsLabel->setText(maxPointValue);
}



void MainWindow::on_save_to_file_button_clicked()
{
    QString DataFileName = ui->edit_file_name->text();
    QString DataFileNumber = ui->file_counter->text();

    QVector<double> allData;
    allData << plotDataValues_x << plotDataValues_y << plotDataValues_z;
    this->saveQVectorToFile(allData,DataFolder+"/"+DataFileName+DataFileNumber+".acc");

    ui->file_counter->setValue(ui->file_counter->value()+1);
}

void MainWindow::saveQVectorToFile(const QVector<double>& data, const QString& filePath)
{
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly))
    {
        QDataStream out(&file);
        out << data;
        file.flush();
        file.close();
        qDebug() << "QVector saved to file:" << filePath;
    }
    else
    {
        qDebug() << "Failed to save QVector to file:" << filePath;
    }
}

void MainWindow::on_set_folder_button_clicked()
{
    //QString filter = "All Files (*.*) ;; Acceleration Files (*.acc) ;; Data Files(*.dat) ;; Binary Files (*.bin)";;
    DataFolder = QFileDialog::getExistingDirectory(this,"Open a folder","/Users/davidlohuis/Documents/Projekte/Projekt-Nocken/Projekt-Bluetoothnocken/Data");
    ui->console->appendPlainText(DataFolder);
}


void MainWindow::on_get_folder_button_clicked()
{
    ui->console->appendPlainText(DataFolder);
}


void MainWindow::on_Run_Measure_stateChanged(int arg1)
{
    if(device->getCharState())
    {
        if(arg1)
        {

            // Pipe Data to different array
            disconnect(device,&Device::sendRXValue,this,&MainWindow::receiveRXValue);
            connect(device,&Device::sendRXValue,this,&MainWindow::receiveRXValueToInt);

            // Send start command
            QString start = "Live";
            device->writeToTXCharacteristic(start);

            // Start the update plot function
            //updatePlot_timer.start();

        }
        else
        {
            // Pipe Data back to the console
            connect(device,&Device::sendRXValue,this,&MainWindow::receiveRXValue);
            disconnect(device,&Device::sendRXValue,this,&MainWindow::receiveRXValueToInt);

            QString end = "Stop";
            device->writeToTXCharacteristic(end);
            //updatePlot_timer.stop();
        }
    }
    else
    {
        writeToConsole("No Tx Characteristic connected!");
    }
}



void MainWindow::on_get_single_point_clicked()
{
    qDebug() << value_length;
    //this->convertRawToIntData();
}




void MainWindow::on_get_Data_stateChanged(int arg1)
{
    if(device->getCharState())
    {
        if(arg1)
        {

            // Pipe Data to different array
            disconnect(device,&Device::sendRXValue,this,&MainWindow::receiveRXValue);
            connect(device,&Device::sendRXValue,this,&MainWindow::receiveRXValueToInt);

            // Send start command
            QString start = "Data";
            device->writeToTXCharacteristic(start);

            // Start the update plot function
            //updatePlot_timer.start();

        }
        else
        {
            // Pipe Data back to the console
            connect(device,&Device::sendRXValue,this,&MainWindow::receiveRXValue);
            disconnect(device,&Device::sendRXValue,this,&MainWindow::receiveRXValueToInt);

            QString end = "Stop";
            device->writeToTXCharacteristic(end);
            //updatePlot_timer.stop();
        }
    }
    else
    {
        writeToConsole("No Tx Characteristic connected!");
    }
}

