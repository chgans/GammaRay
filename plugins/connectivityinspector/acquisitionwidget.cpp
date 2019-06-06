#include "acquisitionwidget.h"
#include "ui_acquisitionwidget.h"

#include "acquisitioninterface.h"

#include <QTimer>

using namespace GammaRay;

#include <QDebug>

#define DEBUG qWarning() << __FUNCTION__ << __LINE__

AcquisitionWidget::AcquisitionWidget(QWidget *parent)
    : QWidget(parent), m_ui(new Ui::AcquisitionWidget) {
    m_ui->setupUi(this);
}

AcquisitionWidget::~AcquisitionWidget() = default;

void AcquisitionWidget::setAcquisitionInterface(
    AcquisitionInterface *interface) {
    Q_ASSERT(m_interface == nullptr);
    Q_ASSERT(interface != nullptr);
    m_interface = interface;

    connect(m_ui->startButton, &QToolButton::clicked, m_interface,
            &AcquisitionInterface::start);
    connect(m_ui->stopButton, &QToolButton::clicked, m_interface,
            &AcquisitionInterface::stop);
    connect(m_ui->pauseButton, &QToolButton::toggled, this, [this](bool checked) {
        DEBUG << checked;
        if (checked)
            m_interface->pause();
        else
            m_interface->resume();
    });
    connect(m_interface,
            &AcquisitionInterface::stateChanged,
            this,
            [this](AcquisitionInterface::State state) {
                DEBUG << state;
                switch (state) {
                case AcquisitionInterface::Stopped:
                    m_ui->stopButton->setEnabled(false);
                    m_ui->stopButton->setChecked(true);
                    m_ui->pauseButton->setEnabled(false);
                    m_ui->pauseButton->setChecked(false);
                    m_ui->startButton->setEnabled(true);
                    m_ui->startButton->setChecked(false);
                    break;
                case AcquisitionInterface::Started:
                    m_ui->stopButton->setEnabled(true);
                    m_ui->stopButton->setChecked(false);
                    m_ui->pauseButton->setEnabled(true);
                    m_ui->pauseButton->setChecked(false);
                    m_ui->startButton->setEnabled(false);
                    m_ui->startButton->setChecked(true);
                    break;
                case AcquisitionInterface::Paused:
                    m_ui->stopButton->setEnabled(true);
                    m_ui->stopButton->setChecked(false);
                    m_ui->pauseButton->setEnabled(true);
                    m_ui->pauseButton->setChecked(false);
                    m_ui->startButton->setEnabled(false);
                    m_ui->startButton->setChecked(false);
                    break;
                }
            });
    connect(m_ui->clearButton, &QToolButton::clicked, m_interface,
            &AcquisitionInterface::clear);
    connect(m_ui->refreshButton, &QToolButton::clicked, m_interface,
            &AcquisitionInterface::refresh);

    connect(m_ui->bufferSizeSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged), m_interface,
            &AcquisitionInterface::setBufferSize);
    connect(m_interface,
            &AcquisitionInterface::bufferSizeChanged,
            m_ui->bufferSizeSpinBox,
            &QSpinBox::setValue);

    //    connect(m_ui->samplingRateSpinBox,
    //            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    //            m_interface,
    //            &AcquisitionInterface::setSamplingRate);
    //    connect(m_interface, &AcquisitionInterface::samplingRateChanged,
    //            m_ui->samplingRateSpinBox, &QDoubleSpinBox::setValue);
    connect(m_ui->samplingRateSpinBox,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            [this](double value) {
                DEBUG << "QDoubleSpinBox::valueChanged" << value;
                m_interface->setSamplingRate(value);
                // throttle rate of change, can go into crazy loop
                m_ui->samplingRateSpinBox->setEnabled(false);
                QTimer::singleShot(200, this, [this]() {
                    m_ui->samplingRateSpinBox->setEnabled(true);
                });
            });
    connect(m_interface, &AcquisitionInterface::samplingRateChanged, this, [this](double value) {
        DEBUG << "AcquisitionInterface::samplingRateChanged" << value;
        m_ui->samplingRateSpinBox->setValue(value);
    });

    connect(m_interface,
            &AcquisitionInterface::bufferOverrunCountChanged,
            m_ui->overrunLabel,
            [this](int value) { m_ui->overrunLabel->setText(QString::number(value)); });

    connect(m_interface,
            &AcquisitionInterface::bufferUsageChanged,
            m_ui->usageBar,
            &QProgressBar::setValue);

    connect(m_interface, &AcquisitionInterface::samplingDone, this, [this](qint64 duration) {
        m_ui->durationLabel->setText(QStringLiteral("%1 ms").arg(duration));
    });
}
