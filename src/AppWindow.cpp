#include "AppWindow.h"

#include "Gui/StyleLoader.h"

#include "Space/SpaceManager.h"
#include "Space/Calculators/CommonCalculator.h"
#include "Space/Calculators/OpenclCalculator.h"

#include <QDebug>
#include <QFileDialog>
#include <QMenuBar>
#include <QStringListModel>
#include <QMessageBox>
#include <fstream>


AppWindow::AppWindow(QWidget *parent)
    : QWidget(parent),
      _mode(Mode::Common),
      _toolBar(new QToolBar(this)),
      _sceneView(new SceneView(this)),
      _codeEditor(new CodeEditor(this)),
      _lineEditor(new LineEditor(this)),
      _program(nullptr),
      _singleLineProgram(nullptr),
      m_editorModeButton(new ToggleButton(8, 10, this)),
      _imageModeButton(new ToggleButton(8, 10, this)),
      _computeDevice(new ToggleButton(8, 10, this)),
      _addLineButton(new QPushButton("Добавить строку", this)),
      _modelZone(new QComboBox(this)),
      _imageType(new QComboBox(this)),
      _spaceDepth(new QSpinBox(this)),
      _batchSize(new QSlider(this)),
      _batchSizeView(new QSpinBox(this)),
      _currentZone(0),
      _currentImage(0),
      _currentCalculatorName(CalculatorName::Common),
      _progressBar(new QProgressBar(_sceneView)),
      _timer(new QElapsedTimer())

{
    QVBoxLayout* toolVLayout = new QVBoxLayout(this);
    toolVLayout->addWidget(_toolBar);

    _toolBar->addAction(QPixmap("assets/images/playIcon.svg"),
                         "Run", this, &AppWindow::Compute);
    _toolBar->addAction(QPixmap("assets/images/saveIcon.png"),
                         "Save", this, &AppWindow::SaveData);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    QWidget* wrapWidget = new QWidget(this);
    QVBoxLayout* modeLayout = new QVBoxLayout(wrapWidget);
    QHBoxLayout* spinLayout = new QHBoxLayout();
    QLabel* spinLabel = new QLabel("Глубина рекурсии");
    spinLayout->addWidget(spinLabel);
    spinLayout->addWidget(_spaceDepth);

    QHBoxLayout* batchLayout = new QHBoxLayout();
    _batchLabel = new QLabel("Размер пачки");
    _batchSize->setEnabled(false);
    _batchLabel->setStyleSheet("QLabel { color : #888888; }");
    batchLayout->addWidget(_batchLabel);
    batchLayout->addWidget(_batchSize);
    batchLayout->addWidget(_batchSizeView);


    modeLayout->addLayout(spinLayout);
    modeLayout->addLayout(batchLayout);
    modeLayout->addWidget(_codeEditor);
    modeLayout->addWidget(_lineEditor);
    modeLayout->addWidget(_addLineButton);

    QHBoxLayout* editorModeBtnLayout = new QHBoxLayout;
    editorModeBtnLayout->setContentsMargins(0, 10, 0, 0);
    _editorMode1Label = new QLabel("Обычный режим");
    _editorMode1Label->setAlignment(Qt::AlignVCenter);
    _editorMode1Label->setAlignment(Qt::AlignRight);
    _editorMode1Label->setStyleSheet("QLabel { color : #ffffff; }");
    _editorMode2Label = new QLabel("Построчный режим");
    _editorMode2Label->setStyleSheet("QLabel { color : #888888; }");
    _editorMode2Label->setAlignment(Qt::AlignVCenter);
    editorModeBtnLayout->addWidget(_editorMode1Label);
    editorModeBtnLayout->addWidget(m_editorModeButton);
    editorModeBtnLayout->addWidget(_editorMode2Label);
    modeLayout->addLayout(editorModeBtnLayout);

    QHBoxLayout* modelModeBtnLayout = new QHBoxLayout;
    modelModeBtnLayout->setContentsMargins(0, 10, 0, 0);
    _modelLabel = new QLabel("Модель");
    _modelLabel->setAlignment(Qt::AlignVCenter);
    _modelLabel->setAlignment(Qt::AlignRight);
    _modelLabel->setStyleSheet("QLabel { color : #ffffff; }");
    _imageLabel = new QLabel("М-Образ");
    _imageLabel->setStyleSheet("QLabel { color : #888888; }");
    _imageLabel->setAlignment(Qt::AlignVCenter);
    modelModeBtnLayout->addWidget(_modelLabel);
    modelModeBtnLayout->addWidget(_imageModeButton);
    modelModeBtnLayout->addWidget(_imageLabel);
    modeLayout->addLayout(modelModeBtnLayout);


    QHBoxLayout* deviceModeLayout = new QHBoxLayout;
    deviceModeLayout->setContentsMargins(0, 10, 0, 0);
    _computeDevice1 = new QLabel("CPU");
    _computeDevice1->setAlignment(Qt::AlignVCenter);
    _computeDevice1->setAlignment(Qt::AlignRight);
    _computeDevice1->setStyleSheet("QLabel { color : #ffffff; }");
    _computeDevice2 = new QLabel("GPU");
    _computeDevice2->setStyleSheet("QLabel { color : #888888; }");
    _computeDevice2->setAlignment(Qt::AlignVCenter);
    deviceModeLayout->addWidget(_computeDevice1);
    deviceModeLayout->addWidget(_computeDevice);
    deviceModeLayout->addWidget(_computeDevice2);
    modeLayout->addLayout(deviceModeLayout);


    _imageType->setEditable(false);
    QStringList imageNames;
    imageNames<<"Cx"<<"Cy"<<"Cz"<<"Cw"<<"Ct";
    _imageTypeModel = new QStringListModel(imageNames, this);
    _imageType->setModel(_imageTypeModel);
    modeLayout->addWidget(_imageType);
    _imageType->setVisible(false);

    _modelZone->setEditable(false);
    QStringList modelZones;
    modelZones<<"Отрицательная"<<"Нулевая"<<"Положительная";
    _modelZoneModel = new QStringListModel(modelZones, this);
    _modelZone->setModel(_modelZoneModel);
    _modelZone->setCurrentIndex(1);
    modeLayout->addWidget(_modelZone);

    _spaceDepth->setRange(1, 10);
    _spaceDepth->setValue(4);

    _batchSize->setOrientation(Qt::Horizontal);
    _batchSize->setRange(0, 21);
    _batchSize->setValue(0);

    _batchSizeView->setReadOnly(true);
    _batchSizeView->setRange(_batchSize->minimum(), pow(2, _batchSize->maximum()));
    _batchSizeView->setMinimumWidth(100);
    connect(_batchSize, &QSlider::valueChanged, this, &AppWindow::SetBatchSize);

    _addLineButton->setVisible(false);
    wrapWidget->setLayout(modeLayout);
    _lineEditor->setVisible(false);

    splitter->addWidget(wrapWidget);
    splitter->addWidget(_sceneView);
    splitter->setMidLineWidth(2);
    _sceneView->setMinimumWidth(500);
    toolVLayout->addWidget(splitter);

    QMenuBar* menuBar = new QMenuBar(this);
    QMenu *fileMenu = new QMenu("Файл");
    menuBar->addMenu(fileMenu);

    QAction* saveAction = new QAction;
    saveAction->setText("Открыть");
    connect(saveAction, &QAction::triggered, this, &AppWindow::OpenFile);
    fileMenu->addAction(saveAction);

    toolVLayout->setMenuBar(menuBar);


    _progressBar->setRange(0, 100);
    _progressBar->setValue(0);


    CommonCalculatorThread* commonCalculator = new CommonCalculatorThread(this);
    OpenclCalculatorThread* openclCalculator = new OpenclCalculatorThread(this);
    _calculators[CalculatorName::Common] = commonCalculator;
    _calculators[CalculatorName::Opencl] = openclCalculator;

    qRegisterMetaType<CalculatorMode>("CalculatorMode");
    connect(commonCalculator, &CommonCalculatorThread::Computed, this, &AppWindow::ComputeFinished);
    connect(openclCalculator , &OpenclCalculatorThread::Computed, this, &AppWindow::ComputeFinished);


    StyleLoader::attach("../assets/styles/dark.qss");
    _codeEditor->AddFile("../Core/Examples/NewFuncs/lopatka.txt");
    _codeEditor->AddFile("../Core/Examples/NewFuncs/Bone.txt");
    _codeEditor->AddFile("../Core/Examples/NewFuncs/Chainik.txt");

    connect(_imageType, &QComboBox::currentTextChanged, this, &AppWindow::ImageChanged);
    connect(_modelZone, &QComboBox::currentTextChanged, this, &AppWindow::ZoneChanged);
    connect(m_editorModeButton, &QPushButton::clicked, this, &AppWindow::SwitchEditorMode);
    connect(_imageModeButton, &QPushButton::clicked, this, &AppWindow::SwitchModelMode);
    connect(_computeDevice, &QPushButton::clicked, this, &AppWindow::SwitchComputeDevice);
    connect(_addLineButton, &QPushButton::clicked, _lineEditor, &LineEditor::addItem);
    connect(_lineEditor, &LineEditor::runLine, this, &AppWindow::ComputeLine);
}


AppWindow::~AppWindow()
{
    if(IsCalculate())
    {
        for(auto& i: _calculators)
            i->wait();
    }
    StopCalculators();
    delete _timer;
}

void AppWindow::Compute()
{
    if(IsCalculate())
    {
        StopCalculators();
        return;
    }

    QString source = _codeEditor->GetActiveText();
    if(!source.isEmpty())
    {
        _progressBar->setValue(0);
        _parser.SetText(source.toStdString());
        if(_program)
            delete _program;
        _program = _parser.GetProgram();
        _sceneView->ClearObjects();
        auto args = _program->GetSymbolTable().GetAllArgs();
        if(SpaceManager::ComputeSpaceSize(_spaceDepth->value()) !=
                SpaceManager::Self().GetSpaceSize() ||
                _prevArguments != args)
        {
            _prevArguments = args;
            SpaceManager::Self().InitSpace(args[0]->limits, args[1]->limits,
                    args[2]->limits, _spaceDepth->value());
            SpaceManager::Self().ResetBufferSize(pow(2, 29));
        }
        _sceneView->CreateVoxelObject(SpaceManager::Self().GetSpaceSize());

        _activeCalculator = dynamic_cast<ISpaceCalculator*>(_computeDevice->isChecked() ?
                    _calculators[CalculatorName::Opencl] :
                _calculators[CalculatorName::Common]);

        _activeCalculator->SetCalculatorMode(_imageModeButton->isChecked() ?
                                          CalculatorMode::Mimage: CalculatorMode::Model);
        if(_computeDevice->isChecked())
            _activeCalculator->SetBatchSize(_batchSizeView->value());
        else
            _activeCalculator->SetBatchSize(0);
        _activeCalculator->SetProgram(_program);
        _timer->start();
        _calculators[_currentCalculatorName]->start();
        qDebug()<<"Start";
    }
}


void AppWindow::SwitchEditorMode()
{
    if(_mode == Mode::Common)
    {
        _codeEditor->setVisible(false);
        _lineEditor->setVisible(true);
        _addLineButton->setVisible(true);
        _mode = Mode::Line;
        _toolBar->actions()[0]->setEnabled(false);
        //        _modeButton->setText("Построчный режим");
        _editorMode1Label->setStyleSheet("QLabel { color : #888888; }");
        _editorMode2Label->setStyleSheet("QLabel { color : #ffffff; }");
    }
    else
    {
        _codeEditor->setVisible(true);
        _lineEditor->setVisible(false);
        _addLineButton->setVisible(false);
        _mode = Mode::Common;
        _toolBar->actions()[0]->setEnabled(true);
        //        _modeButton->setText("Обычный режим");
        _editorMode1Label->setStyleSheet("QLabel { color : #ffffff; }");
        _editorMode2Label->setStyleSheet("QLabel { color : #888888; }");
        _lineEditor->resetLinesState();
    }
}

void AppWindow::SwitchModelMode()
{
    if(_imageModeButton->isChecked())
    {
        // image mode
        _modelLabel->setStyleSheet("QLabel { color : #888888; }");
        _imageLabel->setStyleSheet("QLabel { color : #ffffff; }");
        _imageType->setVisible(true);
        _modelZone->setVisible(false);
    }
    else
    {
        // model mode
        _modelLabel->setStyleSheet("QLabel { color : #ffffff; }");
        _imageLabel->setStyleSheet("QLabel { color : #888888; }");
        _imageType->setVisible(false);
        _modelZone->setVisible(true);
    }
    _lineEditor->resetLinesState();
}

void AppWindow::SwitchComputeDevice()
{
    if(_computeDevice->isChecked())
    {
        // Gpu
        _computeDevice1->setStyleSheet("QLabel { color : #888888; }");
        _computeDevice2->setStyleSheet("QLabel { color : #ffffff; }");
        _currentCalculatorName = CalculatorName::Opencl;
        _batchSize->setEnabled(true);
        _batchLabel->setStyleSheet("QLabel { color : #ffffff; }");
    }
    else
    {
        // Cpu
        _computeDevice1->setStyleSheet("QLabel { color : #ffffff; }");
        _computeDevice2->setStyleSheet("QLabel { color : #888888; }");
        _currentCalculatorName = CalculatorName::Common;
        _batchSize->setEnabled(false);
        _batchLabel->setStyleSheet("QLabel { color : #888888; }");
    }
}

void AppWindow::ImageChanged(QString name)
{
    if(name == "Cx")
        _currentImage = 0;
    else if(name == "Cy")
        _currentImage = 1;
    else if(name == "Cz")
        _currentImage = 2;
    else if(name == "Cw")
        _currentImage = 3;
    else if(name == "Ct")
        _currentImage = 4;

    if(SpaceManager::Self().WasInited() &&
            SpaceManager::Self().GetMimageBuffer())
    {
        auto size = SpaceManager::Self().GetSpaceSize();
        _sceneView->ClearObjects();
        _sceneView->CreateVoxelObject(size);
        ComputeFinished(CalculatorMode::Mimage, 0, 0, size);
    }
}

void AppWindow::ZoneChanged(QString name)
{
    if(name == "Отрицательная")
        _currentZone = -1;
    else if(name == "Нулевая")
        _currentZone = 0;
    else if(name == "Положительная")
        _currentZone = 1;

    if(SpaceManager::Self().WasInited() &&
            SpaceManager::Self().GetZoneBuffer())
    {
        auto size = SpaceManager::Self().GetSpaceSize();
        _sceneView->ClearObjects();
        _sceneView->CreateVoxelObject(size);
        ComputeFinished(CalculatorMode::Model, 0, 0, size);
    }
}

void AppWindow::ComputeLine(int id, QString line)
{
    if(line.isEmpty())
        return;

    _parser.SetText(line.toStdString());

    if(_singleLineProgram)
    {
        Program* lineProg = _parser.GetProgram(&_singleLineProgram->GetSymbolTable());
        if(!lineProg)
        {
            qDebug()<< "[AppWindow::ComputeLine] Fatal error";
            return;
        }
        lineProg->SetResult(lineProg->GetSymbolTable().GetLastVar());
        if(auto res = _singleLineProgram->MergeProgram(lineProg))
        {
            _singleLineProgram->SetResult(res);
            delete lineProg;
        }
        else
        {
            _lineEditor->setLineState(id, true);
            delete lineProg;
            return;
        }
    }
    else
    {
        _singleLineProgram = _parser.GetProgram();
    }
    auto args = _singleLineProgram->GetSymbolTable().GetAllArgs();
    if(!args.empty())
    {
        if(_prevArguments != args)
        {
            _prevArguments = args;
            SpaceManager::Self().InitSpace(args[0]->limits, args[1]->limits,
                    args[2]->limits, _spaceDepth->value());
            SpaceManager::Self().ResetBufferSize(pow(2, 29));
            _sceneView->CreateVoxelObject(SpaceManager::Self().GetSpaceSize());
        }
        else
            _sceneView->ClearObjects(true);
        _activeCalculator = dynamic_cast<ISpaceCalculator*>(_computeDevice->isChecked() ?
                    _calculators[CalculatorName::Opencl] :
                _calculators[CalculatorName::Common]);

        _activeCalculator->SetCalculatorMode(_imageModeButton->isChecked() ?
                                          CalculatorMode::Mimage: CalculatorMode::Model);
        if(_computeDevice->isChecked())
            _activeCalculator->SetBatchSize(_batchSizeView->value());
        else
            _activeCalculator->SetBatchSize(0);
        _activeCalculator->SetProgram(_singleLineProgram);
//        _timer->start();
        _calculators[_currentCalculatorName]->start();
        qDebug()<<"Start";
        _lineEditor->setLineState(id, true);
    }
}

void AppWindow::ComputeFinished(CalculatorMode mode, int start, int batchStart, int end)
{
    SpaceManager& space = SpaceManager::Self();

    if(mode == CalculatorMode::Model)
    {
        int zone = 0;
        cl_float3 point;
        Color modelColor = _activeCalculator->GetModelColor();
        for(; batchStart < end; ++batchStart)
        {
            point = space.GetPointCoords(batchStart+start);
            zone = space.GetZone(batchStart);
            if(zone == _currentZone)
                _sceneView->AddObject(point.x, point.y, point.z,
                                       modelColor.red, modelColor.green,
                                       modelColor.blue, modelColor.alpha);
        }
    }
    else
    {
        double value = 0;
        cl_float3 point;
        for(; batchStart < end; ++batchStart)
        {
            point = space.GetPointCoords(batchStart+start);
            if(_currentImage == 0)
                value = space.GetMimage(batchStart).Cx;
            else if(_currentImage == 1)
                value = space.GetMimage(batchStart).Cy;
            else if(_currentImage == 2)
                value = space.GetMimage(batchStart).Cz;
            else if(_currentImage == 3)
                value = space.GetMimage(batchStart).Cw;
            else if(_currentImage == 4)
                value = space.GetMimage(batchStart).Ct;

            Color color = _activeCalculator->GetMImageColor(value);
            _sceneView->AddObject(point.x, point.y, point.z,
                                   color.red, color.green,
                                   color.blue, color.alpha);
        }
    }
    _sceneView->Flush();
    int percent = 100.f*(batchStart+start)/space.GetSpaceSize();
    _progressBar->setValue(percent);
    if(percent == 100 && _timer->isValid())
        QMessageBox::information(this, "Расчет окончен", "Время расчета = "+
                                 QString::number(_timer->restart()/1000.f)+"s");
}


void AppWindow::StopCalculators()
{
    for(auto& i: _calculators)
    {
        if(i->isRunning())
        {
            i->terminate();
            qDebug()<<"StopCalculator";
        }
    }
}

bool AppWindow::IsCalculate()
{
    for(auto& i: _calculators)
    {
        if(i->isRunning())
            return true;
    }
    return false;
}

void AppWindow::SaveData()
{
    if(!SpaceManager::Self().WasInited())
    {
        QMessageBox::information(this, "Nothing to save",
                                 "You must calculate model or mimage first");
        return;
    }
    QString file = QFileDialog::getSaveFileName(this,"","","*.bin");

    if(!file.endsWith(".bin"))
        file += ".bin";

    std::ofstream stream(file.toStdString());
    if(!stream)
    {
        QMessageBox::information(this, "Couldn't open file",
                                 "Error while openin file "+file);
        return;
    }

    if(_activeCalculator->GetCalculatorMode() == CalculatorMode::Model)
        SpaceManager::Self().SaveZoneRange(stream, 0);
    else
        SpaceManager::Self().SaveMimageRange(stream, 0);
    stream.close();

    QMessageBox::information(this, "Saved",
                             "Data saved at "+file);
}

void AppWindow::SetBatchSize(int value)
{
    if(value == 0)
        _batchSizeView->setValue(0);
    else
        _batchSizeView->setValue(pow(2, value));

}

void AppWindow::OpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open program file"), "../",
                                                    tr("Plan text(*.txt)"));
    if(!fileName.isEmpty())
        _codeEditor->AddFile(fileName);
}

bool AppWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if(QKeySequence("F6") == QKeySequence(keyEvent->key()))
        {
            QString fileName = QFileDialog::getOpenFileName(this,
                                                            tr("Stylesheet"), "../style",
                                                            tr("Stylesheet(*.qss)"));
            if(!fileName.isEmpty())
            {
                StyleLoader::attach(fileName);
            }
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}
