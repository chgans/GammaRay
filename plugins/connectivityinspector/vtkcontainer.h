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

#ifndef GAMMARAY_CONNECTIVITYINSPECTOR_VTKCONTAINER_H
#define GAMMARAY_CONNECTIVITYINSPECTOR_VTKCONTAINER_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QLabel;
QT_END_NAMESPACE

namespace GammaRay {
class VtkPanel;
class VtkWidget;

class VtkContainer : public QWidget {
    Q_OBJECT

public:
    explicit VtkContainer(QWidget *parent = nullptr);
    virtual ~VtkContainer();

    void setModel(QAbstractItemModel *model);
    void updateGraph();

private:
    VtkWidget *m_vtkWidget;
    VtkPanel *m_vtkPanel;
    QLabel *m_statusLabel;
};
}

#endif // GAMMARAY_CONNECTIVITYINSPECTOR_VTKCONTAINER_H
