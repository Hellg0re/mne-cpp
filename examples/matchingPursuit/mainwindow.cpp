//=============================================================================================================
/**
* @file     mainwindow.cpp
* @author   Martin Henfling <martin.henfling@tu-ilmenau.de>;
*           Daniel Knobl <daniel.knobl@tu-ilmenau.de>;
*           Sebastian Krause <sebastian.krause@tu-ilmenau.de>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Martin Henfling, Daniel Knobl and Sebastian Krause. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of MainWindow class.
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>
#include <fiff/fiff.h>
#include <mne/mne.h>
#include <utils/mp/atom.h>
#include <utils/mp/adaptivemp.h>
#include <utils/mp/fixdictmp.h>
#include <disp/plot.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "math.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editorwindow.h"
#include "ui_editorwindow.h"
#include "formulaeditor.h"
#include "ui_formulaeditor.h"
#include "enhancededitorwindow.h"
#include "ui_enhancededitorwindow.h"
#include "processdurationmessagebox.h"
#include "ui_processdurationmessagebox.h"
#include "treebaseddictwindow.h"
#include "ui_treebaseddictwindow.h"
#include "settingwindow.h"
#include "ui_settingwindow.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTableWidgetItem>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QMap>
#include <QtConcurrent>

#include "QtGui"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace UTILSLIB;
using namespace DISPLIB;

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


bool _tbv_is_loading = false;
bool _auto_change = false;
bool _was_partialchecked = false;
bool _come_from_sample_count = false;
bool _come_from_from =false;

qint32 _sample_rate = 1;
qreal _from = 47.151f;
qreal _to = 48.000f;
qreal _soll_energy = 0;
qreal _signal_energy = 0;
qreal _signal_maximum = 0;
qreal _signal_negative_scale = 0;
qreal _max_pos = 0;
qreal _max_neg = 0;
qreal _draw_factor = 0;

QString _file_name = "";
QMap<qint32, bool> _select_channel_map;
QMap<qint32, bool> _select_atoms_map;

QList<QColor> _colors;
QList<QColor> _original_colors;
QList<GaborAtom> _my_atom_list;

MatrixXd _datas;
MatrixXd _times;
MatrixXd _signal_matrix(0, 0);
MatrixXd _original_signal_matrix(0, 0);
MatrixXd _atom_sum_matrix(0, 0);
MatrixXd _residuum_matrix(0, 0);
MatrixXd _real_residuum_matrix(0, 0);

QTime _counter_time(0,0);
QTimer *_counter_timer = new QTimer();

//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================


//*************************************************************************************************************************************
// constructor
MainWindow::MainWindow(QWidget *parent) :    QMainWindow(parent),    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    callGraphWindow = new GraphWindow();    
    callGraphWindow->setMinimumHeight(140);
    callGraphWindow->setMinimumWidth(500);
    callGraphWindow->setMaximumHeight(400);
    ui->l_Graph->addWidget(callGraphWindow);

    callAtomSumWindow = new AtomSumWindow();
    callAtomSumWindow->setMinimumHeight(140);
    callAtomSumWindow->setMinimumWidth(500);
    callAtomSumWindow->setMaximumHeight(400);
    ui->l_atoms->addWidget(callAtomSumWindow);

    callResidumWindow = new ResiduumWindow();
    callResidumWindow->setMinimumHeight(140);
    callResidumWindow->setMinimumWidth(500);
    callResidumWindow->setMaximumHeight(400);
    ui->l_res->addWidget(callResidumWindow);

    callYAxisWindow = new YAxisWindow();
    callYAxisWindow->setMinimumHeight(22);
    callYAxisWindow->setMinimumWidth(500);
    callYAxisWindow->setMaximumHeight(22);
    ui->l_YAxis->addWidget(callYAxisWindow);

    ui->progressBarCalc->setMinimum(0);         // set progressbar
    ui->progressBarCalc->setHidden(true);

    ui->sb_Iterations->setMaximum(1999);        // max iterations
    ui->sb_Iterations->setMinimum(1);           // min iterations
    ui->sb_Iterations->setValue(80);

    ui->splitter->setStretchFactor(1,4);
    ui->dsb_energy->setValue(3.0);

    ui->lb_from->setHidden(true);
    ui->dsb_from->setHidden(true);
    ui->lb_to->setHidden(true);
    ui->dsb_to->setHidden(true);
    ui->lb_samples->setHidden(true);
    ui->sb_sample_count->setHidden(true);

    // set result tableview
    ui->tbv_Results->setColumnCount(5);
    ui->tbv_Results->setHorizontalHeaderLabels(QString("energy\n[%];scale\n[sec];trans\n[sec];modu\n[Hz];phase\n[rad]").split(";"));
    ui->tbv_Results->setColumnWidth(0,55);
    ui->tbv_Results->setColumnWidth(1,45);
    ui->tbv_Results->setColumnWidth(2,40);
    ui->tbv_Results->setColumnWidth(3,40);
    ui->tbv_Results->setColumnWidth(4,40);    

    this->cb_model = new QStandardItemModel;
    connect(this->cb_model, SIGNAL(dataChanged ( const QModelIndex&, const QModelIndex&)), this, SLOT(cb_selection_changed(const QModelIndex&, const QModelIndex&)));
    connect(ui->tbv_Results->model(), SIGNAL(dataChanged ( const QModelIndex&, const QModelIndex&)), this, SLOT(tbv_selection_changed(const QModelIndex&, const QModelIndex&)));
    connect(_counter_timer, SIGNAL(timeout()), this, SLOT(on_time_out()));

    // build config file at init
    bool hasEntry1 = false;
    bool hasEntry2 = false;
    bool hasEntry3 = false;
    QString contents;

    QDir dir("Matching-Pursuit-Toolbox");
    if(!dir.exists())
        dir.mkdir(dir.absolutePath());
    QFile configFile("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox.config");
    if(!configFile.exists())
    {
        if (configFile.open(QIODevice::ReadWrite | QIODevice::Text))
        configFile.close();
    }

    if (configFile.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        while(!configFile.atEnd())
        {
            contents = configFile.readLine(0).constData();
            if(contents.startsWith("ShowDeleteMessageBox=") == 0)
                hasEntry1 = true;
            if(contents.startsWith("ShowProcessDurationMessageBox=") == 0)
                hasEntry2 = true;
            if(contents.startsWith("ShowDeleteFormelMessageBox=") == 0)
                hasEntry3 = true;
        }
    }
    configFile.close();

    if(!hasEntry1)
    {
        if (configFile.open (QIODevice::WriteOnly| QIODevice::Append))
        {
            QTextStream stream( &configFile );
            stream << QString("ShowDeleteMessageBox=true;") << "\n";
        }
        configFile.close();
    }

    if(!hasEntry2)
    {
        if (configFile.open (QIODevice::WriteOnly| QIODevice::Append))
        {
            QTextStream stream( &configFile );
            stream << QString("ShowProcessDurationMessageBox=true;") << "\n";
        }
        configFile.close();
    }

    if(!hasEntry3)
    {
        if (configFile.open (QIODevice::WriteOnly| QIODevice::Append))
        {
            QTextStream stream( &configFile );
            stream << QString("ShowDeleteFormelMessageBox=true;") << "\n";
        }
        configFile.close();
    }

    fill_dict_combobox();
}

//*************************************************************************************************************************************

MainWindow::~MainWindow()
{
    delete ui;
}

//*************************************************************************************************************************************

void MainWindow::fill_dict_combobox()
{
    QDir dir("Matching-Pursuit-Toolbox");
    QStringList filterList;
    filterList.append("*.dict");
    QFileInfoList fileList =  dir.entryInfoList(filterList);

    for(int i = 0; i < fileList.length(); i++)
        ui->cb_Dicts->addItem(QIcon(":/images/icons/DictIcon.png"), fileList.at(i).baseName());
}

void MainWindow::open_file()
{
    QFileDialog* fileDia;
    QString temp_file_name = fileDia->getOpenFileName(this, "Please select signal file.",QDir::currentPath(),"(*.fif *.txt)");
    if(temp_file_name.isNull()) return;

    _file_name = temp_file_name;
     this->cb_model->clear();
    this->cb_items.clear();

    ui->sb_sample_rate->setEnabled(true);

    QFile file(_file_name);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: disable to open signal file."));
        return;
    }
    file.close();    
    _colors.clear();
    _colors.append(QColor(0, 0, 0));

    if(_file_name.endsWith(".fif", Qt::CaseInsensitive))
    {        
        ui->dsb_from->setValue(47.151f);
        ui->dsb_to->setValue(48.000f);
        _from = 47.151f;
        //read_fiff_ave(_file_name);
        read_fiff_file(_file_name);
        ui->lb_from->setHidden(false);
        ui->dsb_from->setHidden(false);
        ui->lb_to->setHidden(false);
        ui->dsb_to->setHidden(false);
        ui->lb_samples->setHidden(false);
        ui->sb_sample_count->setHidden(false);
        ui->sb_sample_count->setValue((ui->dsb_to->value() - ui->dsb_from->value()) * ui->sb_sample_rate->value());
    }
    else
    {
        _from = 0;
        _signal_matrix.resize(0,0);
        read_matlab_file(_file_name);
        ui->lb_from->setHidden(true);
        ui->dsb_from->setHidden(true);
        ui->lb_to->setHidden(true);
        ui->dsb_to->setHidden(true);
        ui->lb_samples->setHidden(true);
        ui->sb_sample_count->setHidden(true);
    }

    _original_signal_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols());
    _original_signal_matrix = _signal_matrix;
    ui->tbv_Results->setRowCount(0);

    for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
    {
        _colors.append(QColor::fromHsv(qrand() % 256, 255, 190));

        this->cb_item = new QStandardItem;

        this->cb_item->setText(QString("Channel %1").arg(channels));
        this->cb_item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        this->cb_item->setData(Qt::Checked, Qt::CheckStateRole);
        this->cb_model->insertRow(channels, this->cb_item);
        this->cb_items.push_back(this->cb_item);
        _select_channel_map.insert(channels, true);
    }
    ui->cb_channels->setModel(this->cb_model);
    _original_colors = _colors;
    _atom_sum_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols()); //resize
    _residuum_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols()); //resize

    update();   
}

//*************************************************************************************************************************************

void MainWindow::cb_selection_changed(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    ui->tbv_Results->setRowCount(0);

    QStandardItem* cb_item = this->cb_items[topLeft.row()];
    if(cb_item->checkState() == Qt::Unchecked)
        _select_channel_map[topLeft.row()] = false;
    else if(cb_item->checkState() == Qt::Checked)
        _select_channel_map[topLeft.row()] = true;

    qint32 size = 0;

    for(qint32 i = 0; i < _original_signal_matrix.cols(); i++)    
        if(_select_channel_map[i] == true)
        {
            size++;
        }

    _signal_matrix.resize(_original_signal_matrix.rows(), size);
    _atom_sum_matrix.resize(_original_signal_matrix.rows(), size);
    _residuum_matrix.resize(_original_signal_matrix.rows(), size);
    //_real_residuum_matrix.resize(_original_signal_matrix.rows(), size);

    _colors.clear();
    qint32 selected_chn = 0;

    for(qint32 channels = 0; channels < _original_signal_matrix.cols(); channels++)    
        if(_select_channel_map[channels] == true)
        {
            _colors.append(_original_colors.at(channels));
            _signal_matrix.col(selected_chn) = _original_signal_matrix.col(channels);
            //_real_residuum_matrix.col(selected_chn) = _real_original_residuum_matrix.col(channels);
            selected_chn++;
        }
    update();
}

//*************************************************************************************************************************************

void MainWindow::read_fiff_ave(QString file_name)
{
    QList<QIODevice*> t_listSampleFilesIn;
    t_listSampleFilesIn.append(new QFile(file_name));
    FiffIO p_fiffIO(t_listSampleFilesIn);

    //std::cout << p_fiffIO << std::endl;

    //Read raw data samples
    p_fiffIO.m_qlistRaw[0]->read_raw_segment_times(_datas, _times, _from, _to);

    ui->sb_sample_rate->setValue(600);//raw.info.sfreq);
    ui->sb_sample_rate->setEnabled(false);
    _sample_rate = 600; //ui->sb_sample_rate->value();

    qint32 cols = 5;
    if(_datas.cols() <= 5)   cols = _datas.cols();
    _signal_matrix.resize(_datas.cols(),cols);

    for(qint32 channels = 0; channels < cols; channels++)
        _signal_matrix.col(channels) = _datas.row(channels);
}

//*************************************************************************************************************************************

qint32 MainWindow::read_fiff_file(QString fileName)
{
    QFile t_fileRaw(fileName);
    bool in_samples = false;
    bool keep_comp = true;

    //
    //   Setup for reading the raw data
    //
    FiffRawData raw(t_fileRaw);

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    //
    QStringList include;
    include << "STI 014";
    bool want_meg   = true;
    bool want_eeg   = false;
    bool want_stim  = false;

    RowVectorXi picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);


    //
    //   Set up projection
    //
    qint32 k = 0;
    if (raw.info.projs.size() == 0)
       printf("No projector specified for these data\n");
    else
    {
        //
        //   Activate the projection items
        //
        for (k = 0; k < raw.info.projs.size(); ++k)
           raw.info.projs[k].active = true;

       printf("%d projection items activated\n",raw.info.projs.size());
       //
       //   Create the projector
       //
       fiff_int_t nproj = raw.info.make_projector(raw.proj);

       if (nproj == 0)
           printf("The projection vectors do not apply to these channels\n");
       else
           printf("Created an SSP operator (subspace dimension = %d)\n",nproj);
    }

    //
    //   Set up the CTF compensator
    //
    qint32 current_comp = raw.info.get_current_comp();
    qint32 dest_comp = -1;

    if (current_comp > 0)
       printf("Current compensation grade : %d\n",current_comp);

    if (keep_comp)
        dest_comp = current_comp;

    if (current_comp != dest_comp)
    {
       qDebug() << "This part needs to be debugged";
       if(MNE::make_compensator(raw.info, current_comp, dest_comp, raw.comp))
       {
          raw.info.set_current_comp(dest_comp);
          printf("Appropriate compensator added to change to grade %d.\n",dest_comp);
       }
       else
       {
          printf("Could not make the compensator\n");
          return -1;
       }
    }

    //
    //   Read a data segment
    //   times output argument is optional
    //
    bool readSuccessful = false;

    if (in_samples)
        readSuccessful = raw.read_raw_segment(_datas, _times, (qint32)_from, (qint32)_to, picks);
    else
        readSuccessful = raw.read_raw_segment_times(_datas, _times, _from, _to, picks);

    if (!readSuccessful)
    {
       printf("Could not read raw segment.\n");
       return -1;
    }

    printf("Read %d samples.\n",(qint32)_datas.cols());

    ui->sb_sample_rate->setValue(raw.info.sfreq);    
    ui->sb_sample_rate->setEnabled(false);
    _sample_rate = ui->sb_sample_rate->value();

    //ToDo: read all channels, or only a few?!
    qint32 cols = 305;
    if(_datas.cols() <= cols)   cols = _datas.cols();
    _signal_matrix.resize(_datas.cols(),cols);

    for(qint32 channels = 0; channels < cols; channels++)
        _signal_matrix.col(channels) = _datas.row(channels);

    //std::cout << _datas.block(0,0,10,10) << std::endl;

    return 0;
}

//*************************************************************************************************************************************

void MainWindow::read_fiff_file_new(QString file_name)
{
    this->cb_model->clear();
    this->cb_items.clear();
    _colors.clear();
    _colors.append(QColor(0, 0, 0));

    read_fiff_file(file_name);

    _original_signal_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols());
    _original_signal_matrix = _signal_matrix;
    ui->tbv_Results->setRowCount(0);

    for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
    {
        _colors.append(QColor::fromHsv(qrand() % 256, 255, 190));

        this->cb_item = new QStandardItem;

        this->cb_item->setText(QString("Channel %1").arg(channels));
        this->cb_item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        this->cb_item->setData(Qt::Checked, Qt::CheckStateRole);
        this->cb_model->insertRow(channels, this->cb_item);
        this->cb_items.push_back(this->cb_item);
        _select_channel_map.insert(channels, true);
    }
    ui->cb_channels->setModel(this->cb_model);
    _original_colors = _colors;
    _atom_sum_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols()); //resize
    _residuum_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols()); //resize

    update();
}

//*************************************************************************************************************************************

void MainWindow::read_matlab_file(QString fileName)
{
    QFile file(fileName);
    QString contents;
    QList<QString> strList;
    file.open(QIODevice::ReadOnly);
    while(!file.atEnd())
    {
        strList.append(file.readLine(0).constData());
    }
    int rowNumber = 0;
    _signal_matrix.resize(strList.length(), 1);
    file.close();
    file.open(QIODevice::ReadOnly);
    while(!file.atEnd())
    {
        contents = file.readLine(0).constData();

        bool isFloat;
        qreal value = contents.toFloat(&isFloat);
        if(!isFloat)
        {
            QString errorSignal = QString("The signal could not completly read. Line %1 from file %2 coud not be readed.").arg(rowNumber).arg(fileName);
            QMessageBox::warning(this, tr("error"),
            errorSignal);
            return;
        }
        _signal_matrix(rowNumber, 0) = value;
        rowNumber++;
    }


    file.close();
    _signal_energy = 0;
    for(qint32 i = 0; i < _signal_matrix.rows(); i++)
        _signal_energy += (_signal_matrix(i, 0) * _signal_matrix(i, 0));
}

//*************************************************************************************************************************************

void GraphWindow::paintEvent(QPaintEvent* event)
{
    paint_signal(_signal_matrix, this->size());
}

//*************************************************************************************************************************************

void GraphWindow::paint_signal(MatrixXd signalMatrix, QSize windowSize)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));


    if(signalMatrix.rows() > 0 && signalMatrix.cols() > 0)
    {
        qint32 borderMarginHeigth = 15;     // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 5;       // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 i = 0;
        qreal maxNeg = 0;                   // smalest signalvalue
        qreal maxPos = 0;                   // highest signalvalue
        qreal absMin = 0;                   // minimum of abs(maxNeg and maxPos)
        qint32 drawFactor = 0;              // shift factor for decimal places (linear)
        qint32 startDrawFactor = 1;         // shift factor for decimal places (exponential-base 10)
        qint32 decimalPlace = 0;            // decimal places for axis title
        QList<QPolygonF> polygons;          // points for drawing the signal
        MatrixXd internSignalMatrix = signalMatrix; // intern representation of y-axis values of the signal (for painting only)

        // paint window white
        painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

        // find min and max of signal

        for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
        {
            i = 0;
            while(i < signalMatrix.rows())
            {
                if(signalMatrix(i, channels) > maxPos)
                    maxPos = signalMatrix(i, channels);

                if(signalMatrix(i, channels) < maxNeg )
                    maxNeg = signalMatrix(i, channels);
                i++;
            }
        }

        if(maxPos > fabs(maxNeg)) absMin = maxNeg;        // find absolute minimum of (maxPos, maxNeg)
        else     absMin = maxPos;

        if(absMin != 0)                                   // absMin must not be zero
        {
            while(true)                                   // shift factor for decimal places?
            {
                if(fabs(absMin) < 1)                      // if absMin > 1 , no shift of decimal places nescesary
                {
                    absMin = absMin * 10;
                    drawFactor++;                         // shiftfactor counter
                }
                if(fabs(absMin) >= 1) break;
            }
        }
        _draw_factor = drawFactor;  // to globe draw_factor

        // shift of decimal places with drawFactor for all signalpoints and save to intern list
        while(drawFactor > 0)
        {

            for(qint32 channel = 0; channel < signalMatrix.cols(); channel++)
                for(qint32 sample = 0; sample < signalMatrix.rows(); sample++)
                    internSignalMatrix(sample, channel) *= 10;

            startDrawFactor = startDrawFactor * 10;
            decimalPlace++;
            maxPos = maxPos * 10;
            maxNeg = maxNeg * 10;
            drawFactor--;
        }

        _max_pos = maxPos;      // to globe max_pos
        _max_neg = maxNeg;      // to globe min_pos

        qreal maxmax;
        // absolute signalheight
        if(maxNeg <= 0)     maxmax = maxPos - maxNeg;
        else  maxmax = maxPos + maxNeg;

        _signal_maximum = maxmax;

        // scale axis title
        //qreal scaleXText = (qreal)signalMatrix.rows() / _sample_rate / (qreal)20;     // divide signallength
        qreal scaleYText = (qreal)maxmax / (qreal)10;
        qint32 negScale =  floor((maxNeg * 10 / maxmax)+0.5);
        _signal_negative_scale = negScale;
        //find lenght of text of y-axis for shift of y-axis to the right (so the text will stay readable and is not painted into the y-axis
        qint32 maxStrLenght = 55;

        while((windowSize.width() - maxStrLenght -borderMarginWidth) % 20)borderMarginWidth++;

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth))/ (qreal)signalMatrix.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)maxmax;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        // position of title of x-axis
        qint32 xAxisTextPos = 8;
        if(maxNeg == 0) xAxisTextPos = -10; // if signal only positiv: titles above axis

        i = 1;
        while(i <= 11)
        {
            qreal scaledYText = negScale * scaleYText / (qreal)startDrawFactor;           // scalevalue y-axis
            QString string  = QString::number(scaledYText, 'g', 3);                 // scalevalue as string

            if(negScale == 0)                                                       // x-Axis reached (y-value = 0)
            {
                // append scaled signalpoints
                for(qint32 channel = 0; channel < signalMatrix.cols(); channel++)   // over all Channels
                {
                    QPolygonF poly;
                    qint32 h = 0;
                    while(h < signalMatrix.rows())
                    {
                        poly.append(QPointF((h * scaleX) + maxStrLenght,  -((internSignalMatrix(h, channel) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                        h++;
                    }
                    polygons.append(poly);
                }

                // paint x-axis
                qint32 j = 1;
                while(j <= 21)
                {
                    if(fmod(j, 4.0) == 0)
                    {
                        QPen pen(Qt::darkGray, 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                        painter.setPen(pen);
                        painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - windowSize.height())), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + windowSize.height())));   // scalelines
                    }
                    else
                    {
                        QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                        painter.setPen(pen);
                        painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - 2)), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + 2)));   // scalelines
                    }
                    j++;
                }
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }

            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, string);     // paint scalevalue y-axis
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // scalelines y-axis
            i++;
            negScale++;
        }

        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);     // paint y-axis



        for(qint32 channel = 0; channel < signalMatrix.cols(); channel++)             // Butterfly
        {
            QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
            painter.setPen(pen);
            painter.drawPolyline(polygons.at(channel));                               // paint signal
        }
    }
    painter.end();    
}

//*************************************************************************************************************************************

 void AtomSumWindow::paintEvent(QPaintEvent* event)
{
   paint_atom_sum(_atom_sum_matrix, this->size(), _signal_maximum, _signal_negative_scale);
}

//*************************************************************************************************************************************

void AtomSumWindow::paint_atom_sum(MatrixXd atom_matrix, QSize windowSize, qreal signalMaximum, qreal signalNegativeMaximum)
{
    // paint window white
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

    // can also checked of zerovector, then you paint no empty axis
    if(atom_matrix.rows() > 0 && atom_matrix.cols() > 0  && _signal_matrix.rows() > 0 && _signal_matrix.cols() > 0)
    {
        qint32 borderMarginHeigth = 15;                     // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 5;                       // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 i = 0;
        qreal maxNeg = _max_neg;                            // smalest signalvalue
        qint32 drawFactor = _draw_factor;                   // shift factor for decimal places (linear)
        qint32 startDrawFactor = 1;                         // shift factor for decimal places (exponential-base 10)
        qint32 decimalPlace = 0;                            // decimal places for axis title
        QList<QPolygonF> polygons;                          // points for drawing the signal
        MatrixXd internSignalMatrix = atom_matrix;          // intern representation of y-axis values of the signal (for painting only)


        while(drawFactor > 0)
        {
            for(qint32 channels = 0; channels < atom_matrix.cols(); channels++)
                for(qint32 sample = 0; sample < atom_matrix.rows(); sample++)
                    internSignalMatrix(sample, channels) *= 10;

            startDrawFactor = startDrawFactor * 10;
            decimalPlace++;

            drawFactor--;
        }


        // scale axis title
        //qreal scaleXText = (qreal)atom_matrix.rows() / (qreal)_sample_rate / (qreal)20;     // divide signallegnth
        qreal scaleYText = (qreal)signalMaximum / (qreal)10;

        //find lenght of text of y-axis for shift of y-axis to the right (so the text will stay readable and is not painted into the y-axis
        qint32 maxStrLenght = 55;


        while((windowSize.width() - maxStrLenght -borderMarginWidth) % 20)borderMarginWidth++;

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth))/ (qreal)atom_matrix.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)signalMaximum;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        // position of title of x-axis
        qint32 xAxisTextPos = 8;
        if(maxNeg == 0) xAxisTextPos = -10; // if signal only positiv: titles above axis

        i = 1;
        while(i <= 11)
        {
            QString string;

            qreal scaledYText = signalNegativeMaximum * scaleYText / (qreal)startDrawFactor;    // scala Y-axis
            string  = QString::number(scaledYText, 'g', 3);                                     // scala as string

            if(signalNegativeMaximum == 0)                                                      // x-Axis reached (y-value = 0)
            {
                // append scaled signalpoints
                for(qint32 channel = 0; channel < atom_matrix.cols(); channel++)                // over all Channels
                {
                    // append scaled signalpoints
                    QPolygonF poly;
                    qint32 h = 0;
                    while(h < atom_matrix.rows())
                    {
                        poly.append(QPointF((h * scaleX) + maxStrLenght,  -((internSignalMatrix(h, channel) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                        h++;
                    }
                    polygons.append(poly);
                }                
                // paint x-axis
                qint32 j = 1;
                while(j <= 21)
                {
                    if(fmod(j, 4.0) == 0)
                    {
                        QPen pen(Qt::darkGray, 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                        painter.setPen(pen);
                        painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - windowSize.height())), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + windowSize.height())));   // scalelines
                    }else
                    {
                        QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                        painter.setPen(pen);
                        painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - 2)), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + 2)));   // scalelines
                    }
                    j++;
                }
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }

            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, string);     // paint scalvalue Y-axis
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // scalelines y-axis
            i++;
            signalNegativeMaximum++;
        }
        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);     // paint y-axis

        for(qint32 channel = 0; channel < atom_matrix.cols(); channel++)             // Butterfly
        {
            QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
            painter.setPen(pen);
            painter.drawPolyline(polygons.at(channel));                               // paint signal
        }
    }
    painter.end();
}

//*************************************************************************************************************************************

void ResiduumWindow::paintEvent(QPaintEvent* event)
{
   paint_residuum(_residuum_matrix, this->size(), _signal_maximum, _signal_negative_scale);
}

//*************************************************************************************************************************************

void ResiduumWindow::paint_residuum(MatrixXd residuum_matrix, QSize windowSize, qreal signalMaximum, qreal signalNegativeMaximum)
{
    // paint window white
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

    if(residuum_matrix.rows() > 0 && residuum_matrix.cols() > 0 && _signal_matrix.rows() > 0 && _signal_matrix.cols() > 0)
    {
        qint32 borderMarginHeigth = 15;                 // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 5;                   // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 i = 0;
        qreal maxNeg = _max_neg;                        // smalest signalvalue vom AusgangsSignal
        qint32 drawFactor = _draw_factor;               // shift factor for decimal places (linear)
        qint32 startDrawFactor = 1;                     // shift factor for decimal places (exponential-base 10)
        qint32 decimalPlace = 0;                        // decimal places for axis title
        QList<QPolygonF> polygons;                      // points for drawing the signal
        MatrixXd internSignalVector = residuum_matrix;  // intern representation of y-axis values of the signal (for painting only)

        while(drawFactor > 0)
        {
            for(qint32 channels = 0; channels < residuum_matrix.cols(); channels++)
                for(qint32 sample = 0; sample < residuum_matrix.rows(); sample++)
                    internSignalVector(sample,channels) *= 10;

            startDrawFactor = startDrawFactor * 10;
            decimalPlace++;

            drawFactor--;
        }

        // scale axis title
        //qreal scaleXText = (qreal)residuum_matrix.rows() /  (qreal)_sample_rate / (qreal)20;     // divide signallegnth
        qreal scaleYText = (qreal)signalMaximum / (qreal)10;

        //find lenght of text of y-axis for shift of y-axis to the right (so the text will stay readable and is not painted into the y-axis
        qint32 maxStrLenght = 55;

        while((windowSize.width() - maxStrLenght -borderMarginWidth) % 20)borderMarginWidth++;

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth))/ (qreal)residuum_matrix.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)signalMaximum;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        // position of title of x-axis
        qint32 xAxisTextPos = 8;
        if(maxNeg == 0) xAxisTextPos = -10; // if signal only positiv: titles above axis

        i = 1;
        while(i <= 11)
        {            
            qreal scaledYText = signalNegativeMaximum * scaleYText / (qreal)startDrawFactor;        // scalevalue y-axis
            QString string  = QString::number(scaledYText, 'g', 3);                          // scalevalue as string

            if(signalNegativeMaximum == 0)                                                          // x-axis reached (y-value = 0)
            {
                // append scaled signalpoints
                for(qint32 channel = 0; channel < residuum_matrix.cols(); channel++)       // over all Channels
                {
                    // append scaled signalpoints
                    QPolygonF poly;                    
                    qint32 h = 0;
                    while(h < residuum_matrix.rows())
                    {
                        poly.append(QPointF((h * scaleX) + maxStrLenght,  -((internSignalVector(h, channel) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                        h++;
                    }
                    polygons.append(poly);
                }                
                qint32 j = 1;
                while(j <= 21)
                {
                    if(fmod(j, 4.0) == 0)
                    {
                        QPen pen(Qt::darkGray, 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                        painter.setPen(pen);
                        painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - windowSize.height())), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + windowSize.height())));   // scalelines
                    }
                    else
                    {
                        QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                        painter.setPen(pen);
                        painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - 2)), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + 2)));   // scalelines
                    }
                    j++;
                }
                // paint x-axis               
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }

            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, string);     // paint scalevalue y-axis
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // scalelines y-axis
            i++;
            signalNegativeMaximum++;
        }

        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);       // paint y-axis

        for(qint32 channel = 0; channel < residuum_matrix.cols(); channel++)            // Butterfly
        {
            QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
            painter.setPen(pen);
            painter.drawPolyline(polygons.at(channel));                               // paint signal
        }
    }
    painter.end();
}

//*************************************************************************************************************************************

void YAxisWindow::paintEvent(QPaintEvent* event)
{
   paint_axis(_signal_matrix, this->size());
}

//*************************************************************************************************************************************

void YAxisWindow::paint_axis(MatrixXd signalMatrix, QSize windowSize)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

    if(signalMatrix.rows() > 0 && signalMatrix.cols() > 0)
    {
        qint32 borderMarginWidth = 15;
        while((windowSize.width() - 55 - borderMarginWidth) % 20)borderMarginWidth++;
        qreal scaleXText = (qreal)signalMatrix.rows() /  (qreal)_sample_rate / (qreal)20;     // divide signallegnth
        qreal scaleXAchse = (qreal)(windowSize.width() - 55 - borderMarginWidth) / (qreal)20;

        qint32 j = 0;
        while(j <= 21)
        {
            QString str;
            painter.drawText(j * scaleXAchse + 45, 20, str.append(QString::number(j * scaleXText + _from, 'f', 2)));  // scalevalue as string
            painter.drawLine(j * scaleXAchse + 55, 5 + 2, j * scaleXAchse + 55 , 5 - 2);                            // scalelines
            j++;
        }
        painter.drawText(5 , 20, "[sec]");  // unit
        painter.drawLine(5, 5, windowSize.width()-5, 5);  // paint y-line
    }
}

//*************************************************************************************************************************************

// starts MP-algorithm
void MainWindow::on_btt_Calc_clicked()
{    
    // ToDo: paused Thread
    if(ui->btt_Calc->text() == "break")
        return;

    TruncationCriterion criterion;    
    ui->progressBarCalc->setValue(0);
    ui->progressBarCalc->setHidden(false);

    if(ui->chb_Iterations->isChecked() && !ui->chb_ResEnergy->isChecked())
        criterion = Iterations;
    if(ui->chb_Iterations->isChecked() && ui->chb_ResEnergy->isChecked())
        criterion = Both;
    if(ui->chb_ResEnergy->isChecked() && !ui->chb_Iterations->isChecked())
        criterion = SignalEnergy;

    if(_signal_matrix.rows() == 0)
    {
        QString title = "Warning";
        QString text = "No signalfile found.";
        QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
        msgBox.exec();

        return;
    }

    if(ui->chb_Iterations->checkState()  == Qt::Unchecked && ui->chb_ResEnergy->checkState() == Qt::Unchecked)
    {
        QString title = "Error";
        QString text = "No truncation criterion choose.";
        QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
        msgBox.exec();
        return;
    }

    if(((ui->dsb_energy->value() <= 1 && ui->dsb_energy->isEnabled()) && (ui->sb_Iterations->value() >= 500 && ui->sb_Iterations->isEnabled())) || (ui->dsb_energy->value() <= 1 && ui->dsb_energy->isEnabled() && !ui->sb_Iterations->isEnabled()) || (ui->sb_Iterations->value() >= 500 && ui->sb_Iterations->isEnabled() && !ui->dsb_energy->isEnabled()) )
    {
        QFile configFile("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox.config");
        bool showMsgBox = false;
        QString contents;
        if (configFile.open(QIODevice::ReadWrite | QIODevice::Text))
        {
            while(!configFile.atEnd())
            {
                contents = configFile.readLine(0).constData();
                if(QString::compare("ShowProcessDurationMessageBox=true;\n", contents) == 0)
                    showMsgBox = true;
            }
        }
        configFile.close();

        if(showMsgBox)
        {
            processdurationmessagebox* msgBox = new processdurationmessagebox(this);
            msgBox->setModal(true);
            msgBox->exec();
            msgBox->close();
        }
    }

    if(ui->chb_ResEnergy->isChecked())
    {
        if(ui->dsb_energy->value() >= 100)
        {
            QString title = "Error";
            QString text = "Please enter a number less than 100.";
            QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
            msgBox.exec();
            ui->dsb_energy->setFocus();
            ui->dsb_energy->selectAll();
            return;
        }
        _soll_energy =  _signal_energy / 100 * ui->dsb_energy->value();
    } 


    ui->frame->setEnabled(false);
    ui->btt_OpenSignal->setEnabled(false);
    ui->btt_Calc->setText("break");
    ui->tbv_Results->setEnabled(false);
    ui->cb_channels->setEnabled(false);
    ui->cb_all_select->setEnabled(false);
    ui->dsb_from->setEnabled(false);
    ui->dsb_to->setEnabled(false);
    ui->sb_sample_count ->setEnabled(false);


    _my_atom_list.clear();
    _residuum_matrix = _signal_matrix;
    update();

    _counter_time = QTime(0,0);
    _counter_timer->setInterval(100);
    _counter_timer->start();

    if(ui->rb_OwnDictionary->isChecked())
    {
        QFile ownDict(QString("Matching-Pursuit-Toolbox/%1.dict").arg(ui->cb_Dicts->currentText()));
        // ToDo: size from dict
        _atom_sum_matrix = MatrixXd::Zero(256,1);
//        QFuture<void> f1 = QtConcurrent::run(&mpCalc, ownDict, _signal_matrix.col(0), ui->sb_Iterations->value());
//        f1.waitForFinished();
        calc_fix_mp(ownDict, _signal_matrix.col(0), ui->sb_Iterations->value());
        //update();
    }
    else if(ui->rb_adativMp->isChecked())
    {
        calc_adaptiv_mp(_signal_matrix, criterion);
    }    
}

//*************************************************************************************************************

void MainWindow::on_time_out()
{
    _counter_time = _counter_time.addMSecs(100);
    ui->lb_timer->setText(_counter_time.toString("hh:mm:ss.zzz"));
    _counter_timer->start();
}

//*************************************************************************************************************

void MainWindow::recieve_result(qint32 current_iteration, qint32 max_iterations, qreal current_energy, qreal max_energy, gabor_atom_list atom_res_list)
{
    _tbv_is_loading = true;
    //update();
    qreal percent = ui->dsb_energy->value();

    qreal residuum_energy = 100 * (max_energy - current_energy) / max_energy;

    //remaining energy and iterations update

    ui->lb_IterationsProgressValue->setText(QString::number(current_iteration));
    ui->lb_RestEnergieResiduumValue->setText(QString::number(residuum_energy, 'f', 2) + "%");

    //current atoms list update
    ui->tbv_Results->setRowCount(atom_res_list.length());
    _my_atom_list.append(atom_res_list.last());

    qreal percent_atom_energy = 100 * atom_res_list.last().energy / max_energy;
    qreal phase = atom_res_list.last().phase_list.first();

    if(atom_res_list.last().phase_list.first() > 2*PI)
         phase = atom_res_list.last().phase_list.first() - 2*PI;

    QTableWidgetItem* atomEnergieItem = new QTableWidgetItem(QString::number(percent_atom_energy, 'f', 2));
    QTableWidgetItem* atomScaleItem = new QTableWidgetItem(QString::number(atom_res_list.last().scale / ui->sb_sample_rate->value(), 'g', 3));
    QTableWidgetItem* atomTranslationItem = new QTableWidgetItem(QString::number(atom_res_list.last().translation / qreal(ui->sb_sample_rate->value()) + _from, 'g', 4));
    QTableWidgetItem* atomModulationItem = new QTableWidgetItem(QString::number(atom_res_list.last().modulation * _sample_rate / atom_res_list.last().sample_count, 'g', 3));
    QTableWidgetItem* atomPhaseItem = new QTableWidgetItem(QString::number(phase, 'g', 3));


    atomEnergieItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    atomScaleItem->setFlags(Qt::ItemIsEnabled);
    atomTranslationItem->setFlags(Qt::ItemIsEnabled);
    atomModulationItem->setFlags(Qt::ItemIsEnabled);
    atomPhaseItem->setFlags(Qt::ItemIsEnabled);

    atomEnergieItem->setCheckState(Qt::Checked);

    atomEnergieItem->setTextAlignment(0x0082);
    atomScaleItem->setTextAlignment(0x0082);
    atomTranslationItem->setTextAlignment(0x0082);
    atomModulationItem->setTextAlignment(0x0082);
    atomPhaseItem->setTextAlignment(0x0082);
    ui->tbv_Results->setItem(atom_res_list.length() - 1, 0, atomEnergieItem);
    ui->tbv_Results->setItem(atom_res_list.length() - 1, 1, atomScaleItem);
    ui->tbv_Results->setItem(atom_res_list.length() - 1, 2, atomTranslationItem);
    ui->tbv_Results->setItem(atom_res_list.length() - 1, 3, atomModulationItem);
    ui->tbv_Results->setItem(atom_res_list.length() - 1, 4, atomPhaseItem);

    qint32 prgrsbar_adapt = 99;

    //progressbar update
    if(max_iterations > 1999 && current_iteration < 100)
             ui->progressBarCalc->setMaximum(100);
    if(ui->chb_ResEnergy->isChecked() && (current_iteration >= (prgrsbar_adapt)) && (max_energy - current_energy) > (0.01 * percent * max_energy))
        ui->progressBarCalc->setMaximum(current_iteration + 5);
    if(max_iterations < 1999)
        ui->progressBarCalc->setMaximum(max_iterations);


    ui->progressBarCalc->setValue(current_iteration);

    if(((current_iteration == max_iterations) || (max_energy - current_energy) < (0.01 * percent * max_energy))&&ui->chb_ResEnergy->isChecked())
        ui->progressBarCalc->setValue(ui->progressBarCalc->maximum());

    //recieve the resulting atomparams    
    GaborAtom gaborAtom = atom_res_list.last();

    for(qint32 i = 0; i < _signal_matrix.cols(); i++)
    {
        VectorXd discret_atom = gaborAtom.create_real(gaborAtom.sample_count, gaborAtom.scale, gaborAtom.translation, gaborAtom.modulation, gaborAtom.phase_list.at(i));

        _atom_sum_matrix.col(i) += gaborAtom.max_scalar_list.at(i) * discret_atom;
        _residuum_matrix.col(i) -= gaborAtom.max_scalar_list.at(i)  * discret_atom;
    }
    update();
    _tbv_is_loading = false;
}

//*************************************************************************************************************

void MainWindow::tbv_selection_changed(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    bool all_selected = true;
    bool all_deselected = true;
    for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)     // last item is residuum
        if(ui->tbv_Results->item(i, 0)->checkState())
            all_deselected = false;
        else
            all_selected = false;

    if(all_selected)
        ui->cb_all_select->setCheckState(Qt::Checked);
    else if(all_deselected)
        ui->cb_all_select->setCheckState(Qt::Unchecked);
    else
        ui->cb_all_select->setCheckState(Qt::PartiallyChecked);


    if(_tbv_is_loading)
        return;

    QTableWidgetItem* item = ui->tbv_Results->item(topLeft.row(), 0);
    if(topLeft.row() == _my_atom_list.count())
    {
        if(item->checkState())
        {
            for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
            {
                _atom_sum_matrix.col(channels) += _real_residuum_matrix.col(channels);
            }
        }
        else
        {
            for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
            {
                _atom_sum_matrix.col(channels) -= _real_residuum_matrix.col(channels);
            }
        }
    }
    else
    {
        GaborAtom  atom = _my_atom_list.at(topLeft.row());
        if(!_auto_change)
            _select_atoms_map[topLeft.row()] = item->checkState();

        if(item->checkState())
        {
            for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
            {
                _atom_sum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
                _residuum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
            }
        }
        else
        {
            for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)//atom.phase_list.length()
            {
                _atom_sum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
                _residuum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
            }
        }
    }
    update();
}

//*************************************************************************************************************

void MainWindow::calc_thread_finished()
{
    _counter_timer->stop();
    ui->frame->setEnabled(true);
    ui->btt_OpenSignal->setEnabled(true);
    ui->btt_Calc->setText("calculate");
    ui->tbv_Results->setEnabled(true);
    ui->cb_channels->setEnabled(true);
    ui->cb_all_select->setEnabled(true);
    ui->dsb_from->setEnabled(true);
    ui->dsb_to->setEnabled(true);
    ui->sb_sample_count ->setEnabled(true);

    _real_residuum_matrix = _residuum_matrix;

    for(qint32 i = 0; i < _my_atom_list.count(); i++)
        _select_atoms_map.insert(i, true);

    _tbv_is_loading = true;

    ui->tbv_Results->setRowCount(_my_atom_list.count() + 1);
    QTableWidgetItem* residuumItem = new QTableWidgetItem("residuum");
    residuumItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    residuumItem->setCheckState(Qt::Unchecked);
    residuumItem->setTextAlignment(Qt::AlignCenter);
    ui->tbv_Results->setItem(_my_atom_list.count(), 0, residuumItem);
    ui->tbv_Results->setSpan(_my_atom_list.count(), 0, 1, 5);

    _tbv_is_loading = false;

}

//*************************************************************************************************************

void MainWindow::calc_adaptiv_mp(MatrixXd signal, TruncationCriterion criterion)
{
    AdaptiveMp *adaptive_Mp = new AdaptiveMp();
    _atom_sum_matrix = MatrixXd::Zero(signal.rows(), signal.cols());
    _residuum_matrix = signal;
    qreal res_energy = ui->dsb_energy->value();

    //threading
    QThread* adaptive_Mp_Thread = new QThread;
    adaptive_Mp->moveToThread(adaptive_Mp_Thread);
    qRegisterMetaType<Eigen::MatrixXd>("MatrixXd");
    qRegisterMetaType<gabor_atom_list>("gabor_atom_list");

    connect(this, SIGNAL(send_input(MatrixXd, qint32, qreal, bool, bool, qint32, qreal, qreal, qreal, qreal)),
            adaptive_Mp, SLOT(recieve_input(MatrixXd, qint32, qreal, bool, bool, qint32, qreal, qreal, qreal, qreal)));
    connect(adaptive_Mp, SIGNAL(current_result(qint32, qint32, qreal, qreal, gabor_atom_list)),
                 this, SLOT(recieve_result(qint32, qint32, qreal, qreal, gabor_atom_list)));
    connect(adaptive_Mp, SIGNAL(finished()), adaptive_Mp_Thread, SLOT(quit()));
    //connect(adaptive_Mp, SIGNAL(finished()), adaptive_Mp, SLOT(deleteLater()));
    connect(adaptive_Mp_Thread, SIGNAL(finished()), this, SLOT(calc_thread_finished()));
    //connect(adaptive_Mp_Thread, SIGNAL(finished()), adaptive_Mp_Thread, SLOT(deleteLater()));

    switch(criterion)
    {
        case Iterations:
        {
            emit send_input(signal, ui->sb_Iterations->value(), qreal(MININT32), ui->chb_fix_phase->isChecked(), 1, 1E3, 1.0, 0.2, 0.5, 0.5);
            adaptive_Mp_Thread->start();
        }
        break;

        case SignalEnergy:
        {
            emit send_input(signal, MAXINT32, res_energy, ui->chb_fix_phase->isChecked(), 1, 1E3, 1.0, 0.2, 0.5, 0.5);
            adaptive_Mp_Thread->start();        
        }
        break;

        case Both:
        {           
            emit send_input(signal, ui->sb_Iterations->value(), res_energy, ui->chb_fix_phase->isChecked(), 1, 1E3, 1.0, 0.2, 0.5, 0.5);
            adaptive_Mp_Thread->start();
        }
        break;
    }       
}

//************************************************************************************************************************************

// TODO: Calc MP (new)
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
VectorXd MainWindow::calc_fix_mp(QFile &currentDict, VectorXd signalSamples, qint32 iterationsCount)
{
    _tbv_is_loading = true;
    GaborAtom* gabor_Atom = new GaborAtom;


    bool isDouble = false;

    qint32 atomCount = 0;
    qint32 bestCorrStartIndex;
    qreal sample;
    qreal bestCorrValue = 0;
    qreal residuumEnergie = 0;    

    QString contents;
    QString atomName;
    QString bestCorrName;

    QList<qreal> atomSamples;
    QList<QStringList> correlationList;    
    VectorXd residuum = signalSamples;


    //originalSignalSamples = signalSamples;

    // Liest das Woerterbuch aus und gibt die Samples und den Namen an die Skalarfunktion weiter
    if (currentDict.open (QIODevice::ReadOnly))
    {
        while(!currentDict.atEnd())
        {
            contents = currentDict.readLine();
            if(contents.startsWith("atomcount"))
            {
                atomCount = contents.mid(12).toInt();
                break;
            }
        }        
        while(!currentDict.atEnd())
        {
            while(!currentDict.atEnd())
            {
                if(contents.contains("_ATOM_"))
                {
                    atomName = contents;
                    break;
                }
                contents = currentDict.readLine();
            }

            contents = "";
            while(!contents.contains("_ATOM_"))
            {
                contents = currentDict.readLine();
                sample = contents.toDouble(&isDouble);
                if(isDouble)
                    atomSamples.append(sample);
                if(currentDict.atEnd())
                    break;
            }
            correlationList.append(correlation(signalSamples, atomSamples, atomName));

            atomSamples.clear();
        }
        currentDict.close();

        // Sucht aus allen verglichenen Atomen das beste passende herraus
        for(qint32 i = 0; i < correlationList.length(); i++)
        {
            if(fabs(correlationList.at(i).at(2).toDouble()) > fabs(bestCorrValue))
            {
                bestCorrName =  correlationList.at(i).at(0);
                bestCorrStartIndex = correlationList.at(i).at(1).toInt();
                bestCorrValue = correlationList.at(i).at(2).toDouble();              
            }
        }

        // Sucht das passende Atom im Woerterbuch und traegt dessen Werte in eine Liste
        if (currentDict.open (QIODevice::ReadOnly))
        {
            bool hasFound = false;
            qint32 j = 0;
            while(!currentDict.atEnd() )
            {
                contents = currentDict.readLine();
                if(QString::compare(contents, bestCorrName) == 0)
                {
                    contents = currentDict.readLine();

                    QStringList list = contents.split(':');
                    gabor_Atom->sample_count = 256;
                    QString t = list.at(1);
                    qreal scale = t.remove(t.length() - 5, 5).toDouble(&isDouble);
                    if(isDouble)
                        gabor_Atom->scale = scale;
                    gabor_Atom->translation = bestCorrStartIndex - 128;
                    t = list.at(2);
                    qreal modu = t.remove(t.length() - 6, 6).toDouble(&isDouble);
                    if(isDouble)
                        gabor_Atom->modulation = modu;
                    t = list.at(3);
                    qreal phase = t.remove(t.length() - 6, 6).toDouble(&isDouble);
                    if(isDouble)
                        gabor_Atom->phase = phase;
                    gabor_Atom->max_scalar_product = bestCorrValue;

                    //---------------------------------

                    for(qint32 chn = 0; chn < 1; chn++)
                    {
                        //simplexfunction to find minimum of target among parameters s, p, k
                        std::vector<double> init;

                        init.push_back(gabor_Atom->scale);
                        init.push_back(gabor_Atom->translation);
                        init.push_back(gabor_Atom->modulation);

                        double tol = 1E8 * std::numeric_limits<double>::epsilon();
                        std::vector<std::vector<double> > x = std::vector<std::vector<double> >();
                        qint32 iterations = 1E3;
                        qint32 N = init.size();                     //space dimension

                        VectorXd atom_fxc_params = VectorXd::Zero(5); //initialisation for contraction coefficients

                        const qreal a=1.0, b=0.2, g=0.5, h=0.5;  //coefficients a = 1, b = 0.2, g = 0.5, h = 0.5
                                                                 //a: reflection  -> xr step away from worst siplex found
                                                                 //b: expansion   -> xe if better with a so go in this direction with b
                                                                 //g: contraction -> xc calc new worst point an bring closer to middle of simplex
                                                                 //h: full contraction to x1
                        std::vector<double> xcentroid_old(N,0);  //simplex center * (N+1)
                        std::vector<double> xcentroid_new(N,0);  //simplex center * (N+1)
                        std::vector<double> vf(N+1,0);           //f evaluated at simplex vertices
                        qint32 x1 = 0, xn = 0, xnp1 = 0;         //x1:   f(x1) = min { f(x1), f(x2)...f(x_{n+1} }
                                                                 //xnp1: f(xnp1) = max { f(x1), f(x2)...f(x_{n+1} }
                                                                 //xn:   f(xn)<f(xnp1) && f(xn)> all other f(x_i)
                        qint32 cnt = 0; //iteration step number

                        if(x.size()== 0) //if no initial simplex is specified
                        {
                            //construct the trial simplex
                            //based upon the initial guess parameters
                            std::vector<double> del( init );
                            std::transform(del.begin(), del.end(), del.begin(),
                            std::bind2nd( std::divides<double>() , 20) );//'20' is picked
                                                                 //assuming initial trail close to true

                            for(qint32 i = 0; i < N; ++i)
                            {
                                std::vector<double> tmp( init );
                                tmp[i] +=  del[i];
                                x.push_back( tmp );
                            }

                            x.push_back(init);//x.size()=N+1, x[i].size()=N

                            //xcentriod
                            std::transform(init.begin(), init.end(), xcentroid_old.begin(), std::bind2nd(std::multiplies<double>(), N+1) );
                        }//constructing the simplex finished

                        qint32 sample_count = 256;

                        //optimization begins
                        for(cnt=0; cnt<iterations; ++cnt)
                        {
                            for(qint32 i=0; i < N+1; ++i)
                            {
                                VectorXd atom_fx = VectorXd::Zero(sample_count);

                                if(gabor_Atom->scale == sample_count && gabor_Atom->translation == floor(sample_count / 2))
                                    atom_fx = AdaptiveMp::calculate_atom(sample_count, sample_count, floor(sample_count / 2), x[i][2], chn, residuum, RETURNATOM, false);

                                else
                                    atom_fx = AdaptiveMp::calculate_atom(sample_count, x[i][0], x[i][1], x[i][2], chn, residuum, RETURNATOM, false);

                                //create targetfunction of realGaborAtom and Residuum
                                double target = 0;
                                for(qint32 k = 0; k < atom_fx.rows(); k++)
                                {
                                    target -=atom_fx[k]*residuum(k,0);
                                }

                                vf[i] = target;
                            }

                            x1=0; xn=0; xnp1=0;//find index of max, second max, min of vf.

                            for(quint32 i=0; i < vf.size(); ++i)
                            {
                                if(vf[i]<vf[x1])      x1 = i;
                                if(vf[i]>vf[xnp1])    xnp1 = i;
                            }

                            xn = x1;

                            for(quint32 i=0; i<vf.size();++i) if(vf[i]<vf[xnp1] && vf[i]>vf[xn])  xn=i;

                            //x1, xn, xnp1 are found

                            std::vector<double> xg(N, 0);//xg: centroid of the N best vertexes

                            for(quint32 i=0; i<x.size(); ++i) if(i!=xnp1) std::transform(xg.begin(), xg.end(), x[i].begin(), xg.begin(), std::plus<double>() );

                            std::transform(xg.begin(), xg.end(), x[xnp1].begin(), xcentroid_new.begin(), std::plus<double>());
                            std::transform(xg.begin(), xg.end(), xg.begin(), std::bind2nd(std::divides<double>(), N) );
                            //xg found, xcentroid_new updated

                            //termination condition
                            double diff=0;          //calculate the difference of the simplex centers

                            //see if the difference is less than the termination criteria
                            for(qint32 i=0; i<N; ++i) diff += fabs(xcentroid_old[i]-xcentroid_new[i]);

                            if (diff/N < tol) break;              //terminate the optimizer
                            else xcentroid_old.swap(xcentroid_new); //update simplex center

                            //reflection:
                            std::vector<double> xr(N,0);

                            for( qint32 i=0; i<N; ++i) xr[i]=xg[i]+a*(xg[i]-x[xnp1][i]);
                            //reflection, xr found

                            VectorXd atom_fxr = VectorXd::Zero(sample_count);

                            if(gabor_Atom->scale == sample_count && gabor_Atom->translation == floor(sample_count / 2))
                                atom_fxr = AdaptiveMp::calculate_atom(sample_count, sample_count, floor(sample_count / 2), xr[2], chn, residuum, RETURNATOM, false);

                            else
                                atom_fxr = AdaptiveMp::calculate_atom(sample_count, xr[0], xr[1], xr[2], chn, residuum, RETURNATOM, false);

                            //create targetfunction of realGaborAtom and Residuum
                            double fxr = 0;
                            for(qint32 k = 0; k < atom_fxr.rows(); k++) fxr -=atom_fxr[k]*residuum(k,chn);//ToDo: old residuum(k,0)

                            //double fxr = target;//record function at xr

                            if(vf[x1]<=fxr && fxr<=vf[xn]) std::copy(xr.begin(), xr.end(), x[xnp1].begin());

                            //expansion:
                            else if(fxr<vf[x1])
                            {
                                std::vector<double> xe(N,0);

                                for( qint32 i=0; i<N; ++i) xe[i]=xr[i]+b*(xr[i]-xg[i]);

                                VectorXd atom_fxe = VectorXd::Zero(sample_count);

                                if(gabor_Atom->scale == sample_count && gabor_Atom->translation == floor(sample_count / 2))
                                    atom_fxe = AdaptiveMp::calculate_atom(sample_count, sample_count, floor(sample_count / 2), xe[2], chn, residuum, RETURNATOM, false);

                                else
                                    atom_fxe = AdaptiveMp::calculate_atom(sample_count, xe[0], xe[1], xe[2], chn, residuum, RETURNATOM, false);

                                //create targetfunction of realGaborAtom and Residuum
                                double fxe = 0;
                                for(qint32 k = 0; k < atom_fxe.rows(); k++) fxe -=atom_fxe[k]*residuum(k,chn);//ToDo: old residuum(k,0)

                                if( fxe < fxr ) std::copy(xe.begin(), xe.end(), x[xnp1].begin() );
                                else std::copy(xr.begin(), xr.end(), x[xnp1].begin() );
                            }//expansion finished,  xe is not used outside the scope

                            //contraction:
                            else if( fxr > vf[xn] )
                            {
                                std::vector<double> xc(N,0);

                                for( qint32 i=0; i<N; ++i)
                                    xc[i]=xg[i]+g*(x[xnp1][i]-xg[i]);

                                if(gabor_Atom->scale == sample_count && gabor_Atom->translation == floor(sample_count / 2))
                                    atom_fxc_params = AdaptiveMp::calculate_atom(sample_count, sample_count, floor(sample_count / 2), xc[2], chn, residuum, RETURNPARAMETERS, false);

                                else
                                    atom_fxc_params = AdaptiveMp::calculate_atom(sample_count, xc[0], xc[1], xc[2], chn, residuum, RETURNPARAMETERS, false);

                                VectorXd atom_fxc = gabor_Atom->create_real(gabor_Atom->sample_count, atom_fxc_params[0], atom_fxc_params[1], atom_fxc_params[2], atom_fxc_params[3]);

                                atom_fxc_params[4] = 0;

                                for(qint32 i = 0; i < sample_count; i++)
                                    atom_fxc_params[4] += atom_fxc[i] * residuum(i, chn);

                                //create targetfunction of realGaborAtom and Residuum
                                double fxc = 0;

                                for(qint32 k = 0; k < atom_fxc.rows(); k++)
                                    fxc -=atom_fxc[k]*residuum(k,chn);//ToDo: old residuum(k,0)

                                if( fxc < vf[xnp1] )
                                    std::copy(xc.begin(), xc.end(), x[xnp1].begin() );

                                else
                                    for( quint32 i=0; i<x.size(); ++i )
                                        if( i!=x1 )
                                            for(qint32 j=0; j<N; ++j)
                                                x[i][j] = x[x1][j] + h * ( x[i][j]-x[x1][j] );
                            }//contraction finished, xc is not used outside the scope
                        }//optimization is finished

                        if(gabor_Atom->scale == sample_count && gabor_Atom->translation == floor(sample_count / 2))
                            atom_fxc_params = AdaptiveMp::calculate_atom(sample_count, sample_count, floor(sample_count / 2), x[x1][2], chn, residuum, RETURNPARAMETERS, false);

                        else
                            atom_fxc_params = AdaptiveMp::calculate_atom(sample_count, x[x1][0], x[x1][1], x[x1][2], chn, residuum, RETURNPARAMETERS, false);

                        if(abs(atom_fxc_params[4]) > abs(bestCorrValue) /*&& atom_fxc_params[0] < sample_count && atom_fxc_params[0] > 0*/ && atom_fxc_params[1] < sample_count && atom_fxc_params[1] > 0)//ToDo: find a way to make the simplex not running out of bounds
                        {
                            bestCorrValue = atom_fxc_params[4];             //scalarProduct
                            gabor_Atom->scale              = atom_fxc_params[0];//scale
                            gabor_Atom->translation        = atom_fxc_params[1];//translation
                            gabor_Atom->modulation         = atom_fxc_params[2];//phase
                            gabor_Atom->phase              = atom_fxc_params[3];
                            gabor_Atom->max_scalar_product   = bestCorrValue;
                        }

                        if(cnt==iterations)//max number of iteration achieves before tol is satisfied
                            std::cout<<"Simplex Iteration limit of "<<iterations<<" achieved in channel " << chn << ", result may not be optimal";

                    }//end Maximisation for channels Copyright (C) 2010 Botao Jia


                    //-----------------------------------

                    std::cout << "\n" << "===============" << " found parameters " << 1 << "===============" << ":\n\n"<<
                                 "scale: " << gabor_Atom->scale << " trans: " << gabor_Atom->translation <<
                                 " modu: " << gabor_Atom->modulation << " phase: " << gabor_Atom->phase << " scalarproduct: " << gabor_Atom->max_scalar_product << "\n";

                    _my_atom_list.append(*gabor_Atom);
                    //atom_res_list.append(*gabor_Atom);
                    hasFound = true;
                }
                if(hasFound) break;
            }
        }        
        currentDict.close();

        ui->lb_IterationsProgressValue->setText(QString::number(iterationsCount));

        //current atoms list update
        ui->tbv_Results->setRowCount(_my_atom_list.length());

        qreal phase = _my_atom_list.last().phase;
        if(phase > 2*PI)
            phase = phase - 2*PI;

       QTableWidgetItem* atomEnergieItem = new QTableWidgetItem(QString::number(4.0000, 'f', 2));
       QTableWidgetItem* atomScaleItem = new QTableWidgetItem(QString::number(_my_atom_list.last().scale / ui->sb_sample_rate->value(), 'g', 3));
       QTableWidgetItem* atomTranslationItem = new QTableWidgetItem(QString::number(_my_atom_list.last().translation / qreal(ui->sb_sample_rate->value()) + _from, 'g', 4));
       QTableWidgetItem* atomModulationItem = new QTableWidgetItem(QString::number(_my_atom_list.last().modulation * _sample_rate / _my_atom_list.last().sample_count, 'g', 3));
       QTableWidgetItem* atomPhaseItem = new QTableWidgetItem(QString::number(phase, 'g', 3));


       atomEnergieItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
       atomScaleItem->setFlags(Qt::ItemIsEnabled);
       atomTranslationItem->setFlags(Qt::ItemIsEnabled);
       atomModulationItem->setFlags(Qt::ItemIsEnabled);
       atomPhaseItem->setFlags(Qt::ItemIsEnabled);

       atomEnergieItem->setCheckState(Qt::Checked);

       atomEnergieItem->setTextAlignment(0x0082);
       atomScaleItem->setTextAlignment(0x0082);
       atomTranslationItem->setTextAlignment(0x0082);
       atomModulationItem->setTextAlignment(0x0082);
       atomPhaseItem->setTextAlignment(0x0082);

       ui->tbv_Results->setItem(_my_atom_list.length() - 1, 0, atomEnergieItem);
       ui->tbv_Results->setItem(_my_atom_list.length() - 1, 1, atomScaleItem);
       ui->tbv_Results->setItem(_my_atom_list.length() - 1, 2, atomTranslationItem);
       ui->tbv_Results->setItem(_my_atom_list.length() - 1, 3, atomModulationItem);
       ui->tbv_Results->setItem(_my_atom_list.length() - 1, 4, atomPhaseItem);


        //recieve the resulting atomparams
        GaborAtom gaborAtom = _my_atom_list.last();

        VectorXd discret_atom = gaborAtom.create_real(gaborAtom.sample_count, gaborAtom.scale, gaborAtom.translation, gaborAtom.modulation, gaborAtom.phase);

        _atom_sum_matrix.col(0) += gaborAtom.max_scalar_product * discret_atom;
        _residuum_matrix.col(0) -= gaborAtom.max_scalar_product  * discret_atom;
        residuum.col(0) -= gaborAtom.max_scalar_product  * discret_atom;
        update();
        _tbv_is_loading = false;

    }
    update();
    iterationsCount--;
    if(iterationsCount > 0)
    {
        QFile ownDict(QString("Matching-Pursuit-Toolbox/%1.dict").arg(ui->cb_Dicts->currentText()));
        calc_fix_mp(ownDict, residuum.col(0), iterationsCount);

    }

    calc_thread_finished();
    _counter_timer->stop();
    ui->lb_timer->setText(_counter_time.toString("hh:mm:ss.zzz"));
    return residuum;


/*

        // Subtraktion des Atoms vom Signal
        for(qint32 m = 0; m < normBestCorrAtomSamples.rows(); m++)
        {
            // TODO:
            //signalSamples.append(0);
            //signalSamples.prepend(0);
        }

        residuum = signalSamples;
        for(qint32 i = 0; i < normBestCorrAtomSamples.rows(); i++)
        {
            residuum[normBestCorrAtomSamples.rows() + i + bestCorrStartIndex] = signalSamples[normBestCorrAtomSamples.rows() + i + bestCorrStartIndex] - normBestCorrAtomSamples[i];
            //residuum.removeAt(normBestCorrAtomSamples.rows() + i + bestCorrStartIndex + 1);
        }

        // Loescht die Nullen wieder
        for(qint32 j = 0; j < normBestCorrAtomSamples.rows(); j++)
        {
            // TODO:
            //residuum.removeAt(0);
            //residuum.removeAt(residuum.rows() - 1);
            //signalSamples.removeAt(0);
            //signalSamples.removeAt(signalSamples.rows() - 1);
        }

        iterationsCount++;

        // Traegt das gefunden Atom in eine Liste ein
        bestAtom.append(bestCorrName);
        bestAtom.append(QString("%1").arg(bestCorrStartIndex));
        bestAtom.append(QString("%1").arg(bestCorrValue));
        QString newSignalString = "";
        for(qint32 i = 0; i < normBestCorrAtomSamples.rows(); i++)
            newSignalString.append(QString("%1/n").arg(normBestCorrAtomSamples[i]));

        bestAtom.append(newSignalString);
        //globalResultAtomList.append(bestAtom);


        //***************** DEBUGGOUT **********************************************************************************
        QFile newSignal("Matching-Pursuit-Toolbox/newSignal.txt");
        if(!newSignal.exists())
        {
            if (newSignal.open(QIODevice::ReadWrite | QIODevice::Text))
            newSignal.close();
        }
        else    newSignal.remove();

        if(!newSignal.exists())
        {
            if (newSignal.open(QIODevice::ReadWrite | QIODevice::Text))
            newSignal.close();
        }

        if (newSignal.open (QIODevice::WriteOnly| QIODevice::Append))
        {
            QTextStream stream( &newSignal );
            for(qint32 i = 0; i < residuum.rows(); i++)
            {
                QString temp = QString("%1").arg(residuum[i]);
                stream << temp << "\n";
            }
        }
        newSignal.close();

        //**************************************************************************************************************
/*

        // Eintragen und Zeichnen der Ergebnisss in die Liste der UI
        ui->tw_Results->setRowCount(iterationsCount);
        QTableWidgetItem* atomNameItem = new QTableWidgetItem(bestCorrName);

        // Berechnet die Energie des Atoms mit dem NormFaktor des Signals
        qreal normAtomEnergie = 0;
        for(qint32 i = 0; i < normBestCorrAtomSamples.rows(); i++)
            normAtomEnergie += normBestCorrAtomSamples[i] * normBestCorrAtomSamples[i];

        QTableWidgetItem* atomEnergieItem = new QTableWidgetItem(QString("%1").arg(normAtomEnergie / signalEnergie * 100));

        //AtomWindow *atomWidget = new AtomWindow();
        //atomWidget->update();
        //ui->tw_Results->setItem(iterationsCount - 1, 1, atomWidget);
        //atomWidget->update();
        //QTableWidgetItem* atomItem = new QTableWidgetItem();
        //atomItem->

        ui->tw_Results->setItem(iterationsCount - 1, 0, atomNameItem);
        ui->tw_Results->setItem(iterationsCount - 1, 2, atomEnergieItem);


        ui->lb_IterationsProgressValue->setText(QString("%1").arg(iterationsCount));
        for(qint32 i = 0; i < residuum.rows(); i++)
            residuumEnergie += residuum[i] * residuum[i];
        if(residuumEnergie == 0)    ui->lb_RestEnergieResiduumValue->setText("0%");
        else    ui->lb_RestEnergieResiduumValue->setText(QString("%1%").arg(residuumEnergie / signalEnergie * 100));

        // Ueberprueft die Abbruchkriterien
        if(ui->chb_Iterations->isChecked() && ui->chb_ResEnergy->isChecked())
        {
            ui->progressBarCalc->setMaximum((1-sollEnergie)*100);
            processValue = (1 - residuumEnergie / signalEnergie + sollEnergie)*100;
            ui->progressBarCalc->setValue(processValue);
            if(ui->sb_Iterations->value() <= iterationsCount)
                ui->progressBarCalc->setValue(ui->progressBarCalc->maximum());

            if(ui->sb_Iterations->value() > iterationsCount && sollEnergie < residuumEnergie)
                residuum = mpCalc(currentDict, residuum, iterationsCount);
        }
        else if(ui->chb_Iterations->isChecked())
        {
            ui->progressBarCalc->setMaximum(ui->sb_Iterations->value());
            processValue++;
            ui->progressBarCalc->setValue(processValue);

            if(ui->sb_Iterations->value() > iterationsCount)
                residuum = mpCalc(currentDict, residuum, iterationsCount);
        }
        else if(ui->chb_ResEnergy->isChecked())
        {
            ui->progressBarCalc->setMaximum((1-sollEnergie)*100);
            processValue = (1 - residuumEnergie / signalEnergie + sollEnergie)*100;
            ui->progressBarCalc->setValue(processValue);

            if(sollEnergie < residuumEnergie)
                residuum = mpCalc(currentDict, residuum, iterationsCount);
        }
        */
    //}


}


// Berechnung das Skalarprodukt zwischen Atom und Signal
QStringList MainWindow::correlation(VectorXd signalSamples, QList<qreal> atomSamples, QString atomName)
{    
    qreal sum = 0;
    qint32 index = 0;
    qreal maximum = 0;
    qreal sumAtom = 0;

    VectorXd originalSignalList = signalSamples;
    QList<qreal> tempList;
    QList<qreal> scalarList;    
    QStringList resultList;

    resultList.clear();
    tempList.clear();


    // Fuellt das Signal vorne und hinten mit nullen auf damit Randwertproblem umgangen wird
    MatrixXd null_signal_samples(2,2);
    null_signal_samples = MatrixXd::Zero(signalSamples.rows() + (2 * atomSamples.length()), 1);
    qint32 j = 0;
    for(qint32 i = atomSamples.length(); i < atomSamples.length() +  signalSamples.rows(); i++)
    {
        null_signal_samples.row(i) = signalSamples.row(j);
        j++;
    }

    //******************************************************************************************************************

    for(qint32 j = 0; j < originalSignalList.rows() + atomSamples.length() -1; j++)
    {
        // Inners Produkt des Signalteils mit dem Atom
        for(qint32 g = 0; g < atomSamples.length(); g++)
        {
            tempList.append(null_signal_samples(g + j, 0) * atomSamples.at(g));
            sum += tempList.at(g);
        }
        scalarList.append(sum);
        tempList.clear();
        sum = 0;
    }

    //Maximum und Index des Skalarproduktes finden unabhaengig ob positiv oder negativ
    for(qint32 k = 0; k < scalarList.length(); k++)
    {
        if(fabs(maximum) < fabs(scalarList.at(k)))
        {
            maximum = scalarList.at(k);
            index = k;
        }
    }

    // Liste mit dem Name des Atoms, Index und hoechster Korrelationskoeffizent
    resultList.append(atomName);
    resultList.append(QString("%1").arg(index));     // Gibt den Signalindex fuer den Startpunkt des Atoms wieder
    resultList.append(QString("%1").arg(maximum));

    return resultList;
    // die Stelle, an der die Korrelation am groessten ist ergibt sich aus:
    // dem Index des hoechsten Korrelationswertes minus die halbe Atomlaenge,

}


//*****************************************************************************************************************

// Opens Dictionaryeditor
void MainWindow::on_actionW_rterbucheditor_triggered()
{        
    EditorWindow *editor_window = new EditorWindow(this);
    editor_window->show();
}

//*****************************************************************************************************************

// opens advanced Dictionaryeditor
void MainWindow::on_actionErweiterter_W_rterbucheditor_triggered()
{
    Enhancededitorwindow *x = new Enhancededitorwindow();
    x->show();
}

//*****************************************************************************************************************

// opens formula editor
void MainWindow::on_actionAtomformeleditor_triggered()
{
    Formulaeditor *x = new Formulaeditor();
    x->show();
}

//*****************************************************************************************************************

// opens Filedialog for read signal (contextmenue)
void MainWindow::on_actionNeu_triggered()
{
    open_file();
}

//*****************************************************************************************************************

// opens Filedialog for read signal (button)
void MainWindow::on_btt_OpenSignal_clicked()
{
    open_file();
}

//*****************************************************************************************************************

void MainWindow::on_actionCreate_treebased_dictionary_triggered()
{
    TreebasedDictWindow *x = new TreebasedDictWindow();
    x->show();
}

//*****************************************************************************************************************

void MainWindow::on_sb_sample_rate_editingFinished()
{
    _sample_rate = ui->sb_sample_rate->value();
}

//*****************************************************************************************************************

void MainWindow::on_dsb_from_editingFinished()
{   
    _from = ui->dsb_from->value();
    read_fiff_file_new(_file_name);
}

//*****************************************************************************************************************

void MainWindow::on_dsb_to_editingFinished()
{   
    _to = ui->dsb_to->value();
    read_fiff_file_new(_file_name);
}

//*****************************************************************************************************************

void MainWindow::on_dsb_from_valueChanged(double arg1)
{
    _come_from_from = true;
    qreal var = (_to - arg1) * ui->sb_sample_rate->value();
    if(ui->dsb_to->value() <= arg1 || var < 64 || var > 4096)
        ui->dsb_from->setValue(_from);
    else
        ui->sb_sample_count->setValue(var);
    _come_from_from = false;
}

//*****************************************************************************************************************

void MainWindow::on_dsb_to_valueChanged(double arg1)
{
    qreal var  = (arg1 - _from) * ui->sb_sample_rate->value();
    if(ui->dsb_from->value() >= arg1 || var < 64 || var > 4096)
        ui->dsb_to->setValue(_to);

    if(!_come_from_sample_count)
        ui->sb_sample_count->setValue(var);
}

//*****************************************************************************************************************

void MainWindow::on_sb_sample_count_valueChanged(int arg1)
{
    _come_from_sample_count = true;
    if(!_come_from_from)
        ui->dsb_to->setValue( _from  + ((qreal)arg1 / (qreal)ui->sb_sample_rate->value()));
    _come_from_sample_count = false;
}

//*****************************************************************************************************************

void MainWindow::on_sb_sample_count_editingFinished()
{
    if(!_come_from_from)
    {
        _to = ui->dsb_to->value();
        read_fiff_file_new(_file_name);
    }
}

//*****************************************************************************************************************

void MainWindow::on_cb_all_select_clicked()
{
    if(_tbv_is_loading) return;

    if( ui->cb_all_select->checkState() == Qt::Unchecked && !_was_partialchecked)
    {
        ui->cb_all_select->setCheckState(Qt::PartiallyChecked);
        _was_partialchecked = true;
    }
    else if(ui->cb_all_select->checkState() == Qt::Checked && !_was_partialchecked)
    {
        ui->cb_all_select->setCheckState(Qt::Unchecked);
        _was_partialchecked = false;
    }

    _auto_change = true;

    if(ui->cb_all_select->checkState() == Qt::Checked)
        for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)     // last item is residuum
            ui->tbv_Results->item(i, 0)->setCheckState(Qt::Checked);
    else if(ui->cb_all_select->checkState() == Qt::Unchecked)
        for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)     // last item is residuum
            ui->tbv_Results->item(i, 0)->setCheckState(Qt::Unchecked);
    else
    {
        for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)     // last item is residuum
            if(_select_atoms_map[i] == true)
                ui->tbv_Results->item(i, 0)->setCheckState(Qt::Checked);
            else
                ui->tbv_Results->item(i, 0)->setCheckState(Qt::Unchecked);
    }


    bool all_selected = true;
    bool all_deselected = true;
    for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)         // last item is residuum
        if(ui->tbv_Results->item(i, 0)->checkState())
            all_deselected = false;
        else
            all_selected = false;

    if(all_selected)
        ui->cb_all_select->setCheckState(Qt::Checked);
    else if(all_deselected)
    {
        ui->cb_all_select->setCheckState(Qt::Unchecked);
        _was_partialchecked = true;
    }
    else ui->cb_all_select->setCheckState(Qt::PartiallyChecked);

    _auto_change = false;
}

//*****************************************************************************************************************

void MainWindow::on_actionSettings_triggered()
{
    settingwindow *set = new settingwindow();
    set->show();

}
