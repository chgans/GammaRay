/*
  vtkgraphadapter.h

  This file is part of QGraphViz, a Qt wrapper around GraphViz libraries.

  Copyright (C) 2019

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

#ifndef VTKGRAPHADAPTER_H
#define VTKGRAPHADAPTER_H

#include <QObject>

#include <vtkSmartPointer.h>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

class vtkGraph;
class vtkMutableDirectedGraph;
class vtkStringArray;
class vtkUnsignedLongLongArray;
class vtkIntArray;

namespace GammaRay {

class vtkGraphAdapter : public QObject
{
     Q_OBJECT
public:
    explicit vtkGraphAdapter(QObject *parent = nullptr);
    ~vtkGraphAdapter() override;

    void setSourceModel(const QAbstractItemModel *model);
    vtkGraph *graph();

    void update();

signals:

public slots:

private:
    const QAbstractItemModel *m_input;
    vtkSmartPointer<vtkMutableDirectedGraph> m_output;
    vtkStringArray *m_objectLabelArray;
    vtkUnsignedLongLongArray *m_objectIdArray;
    vtkUnsignedLongLongArray *m_threadIdArray;
    vtkIntArray *m_connWeightArray;
};

} // namespace GammaRay
#endif // VTKGRAPHADAPTER_H
