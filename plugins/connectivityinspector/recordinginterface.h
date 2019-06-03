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

#ifndef GAMMARAY_CONNECTIVITYINSPECTOR_RECORDINGINTERFACE_H
#define GAMMARAY_CONNECTIVITYINSPECTOR_RECORDINGINTERFACE_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QSize;
QT_END_NAMESPACE

namespace GammaRay {
class ConnectivityRecordingInterface : public QObject
{
    Q_OBJECT

public:
    explicit ConnectivityRecordingInterface(const QString &name, QObject *parent = nullptr);
    ~ConnectivityRecordingInterface() override;

public slots:
    virtual void recordAll() = 0;
    virtual void recordNone() = 0;
    virtual void showAll() = 0;
    virtual void showNone() = 0;
};
} // namespace GammaRay

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(GammaRay::ConnectivityRecordingInterface,
                    "com.kdab.GammaRay.ConnectivityInspector.RecordingInterface")
QT_END_NAMESPACE

#endif // GAMMARAY_CONNECTIVITYINSPECTOR_RECORDINGINTERFACE_H
