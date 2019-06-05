/*
  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Christian Gagneraud <chgans@gmail.com>

  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
  accordance with GammaRay Commercial License Agreement provided with the Software.

  Contact info@kdab.com if any conditions of this licensing are not clear to you.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GAMMARAY_CONNECTIVITYINSPECTOR_CONNECTIVITYINSECTORINTERFACE_H
#define GAMMARAY_CONNECTIVITYINSPECTOR_CONNECTIVITYINSECTORINTERFACE_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QSize;
QT_END_NAMESPACE

namespace GammaRay {

class AcquisitionInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(int bufferSize READ bufferSize WRITE setBufferSize NOTIFY
                   bufferSizeChanged)
    Q_PROPERTY(qreal bufferUsage READ bufferUsage NOTIFY bufferUsageChanged)
    Q_PROPERTY(int bufferOverrunCount READ bufferOverrunCount NOTIFY
                   bufferOverrunCountChanged)
    Q_PROPERTY(bool samplingRate READ samplingRate WRITE setSamplingRate NOTIFY
                   samplingRateChanged)

public:
    enum State { Started, Paused, Stopped };
    Q_ENUM(State)

    explicit AcquisitionInterface(QObject *parent = nullptr);
    ~AcquisitionInterface() override;

    virtual State state() const = 0;
    virtual int bufferSize() const = 0;
    virtual qreal bufferUsage() const = 0;
    virtual int bufferOverrunCount() const = 0;
    virtual int samplingRate() const = 0;

public slots:
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void refresh() = 0;
    virtual void clear() = 0;

    virtual void setBufferSize(int size) = 0;
    virtual void setSamplingRate(int rate) = 0;

signals:
    void stateChanged(State state);
    void bufferSizeChanged(int size);
    void bufferUsageChanged(int usage);
    void bufferOverrunCountChanged(int count);
    void samplingRateChanged(int rate);
};
} // namespace GammaRay

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(GammaRay::AcquisitionInterface,
                    "com.kdab.GammaRay.AcquisitionInterface")
QT_END_NAMESPACE

#endif // GAMMARAY_CONNECTIVITYINSPECTOR_CONNECTIVITYINSECTORINTERFACE_H
