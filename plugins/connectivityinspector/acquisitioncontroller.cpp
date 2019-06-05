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

#include "acquisitioncontroller.h"

#include <common/endpoint.h>

using namespace GammaRay;

AcquisitionController::AcquisitionController(QObject *parent)
    : AcquisitionInterface(parent) {}

AcquisitionController::~AcquisitionController() = default;

void AcquisitionController::clear() {
    Endpoint::instance()->invokeObject(objectName(), "clear");
}

AcquisitionInterface::State AcquisitionController::state() const {
    Q_ASSERT(false);
    return Stopped;
}

int AcquisitionController::bufferSize() const { Q_ASSERT(false); }

qreal AcquisitionController::bufferUsage() const {
    Q_ASSERT(false);
    return qreal(0);
}

int AcquisitionController::bufferOverrunCount() const {
    Q_ASSERT(false);
    return 0;
}

int AcquisitionController::samplingRate() const {
    Q_ASSERT(false);
    return 1;
}

void AcquisitionController::start() {
    Endpoint::instance()->invokeObject(objectName(), "start");
}

void AcquisitionController::stop() {
    Endpoint::instance()->invokeObject(objectName(), "stop");
}

void AcquisitionController::pause() {
    Endpoint::instance()->invokeObject(objectName(), "pause");
}

void AcquisitionController::resume() {
    Endpoint::instance()->invokeObject(objectName(), "resume");
}

void AcquisitionController::refresh() {
    Endpoint::instance()->invokeObject(objectName(), "refresh");
}

void AcquisitionController::setBufferSize(int size) {
    Endpoint::instance()->invokeObject(objectName(), "setBufferSize",
                                       QVariantList() << size);
}

void AcquisitionController::setSamplingRate(int rate) {
    Endpoint::instance()->invokeObject(objectName(), "setSamplingRate",
                                       QVariantList() << rate);
}
