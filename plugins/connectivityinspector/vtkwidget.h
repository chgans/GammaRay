/*
  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2010-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Kevin Funk <kevin.funk@kdab.com>
  Author: Volker Krause <volker.krause@kdab.com>

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

#ifndef GAMMARAY_CONNECTIVITYINSPECTOR_VTKWIDGET_H
#define GAMMARAY_CONNECTIVITYINSPECTOR_VTKWIDGET_H

#include <QVTKWidget.h>

#include <vtkSmartPointer.h>

#include "vtkcommon.h"

#include <QElapsedTimer>
#include <QSet>
#include <QVector>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QElapsedTimer;
QT_END_NAMESPACE

class QVTKInteractor;
class vtkActor;
class vtkAlgorithmOutput;
class vtkGraphLayout;
class vtkGraphLayoutStrategy;
class vtkGraphLayoutView;
class vtkIntArray;
class vtkInteractorStyle;
class vtkMutableDirectedGraph;
class vtkStringArray;
class vtkUnsignedLongLongArray;

namespace GammaRay {

// TODO: QVTKOpenGLNativeWidget (v8) and QVTKWidget2 (v7)
class VtkWidget : public QVTKWidget
{
    Q_OBJECT

public:
    explicit VtkWidget(QWidget *parent = nullptr);
    virtual ~VtkWidget();

    void setModel(QAbstractItemModel *model);
    void updateGraph();

signals:
    void statusChanged(const QString &text);

public slots:
    void setLayoutStrategy(GammaRay::Vtk::LayoutStrategy strategy);
    void setStereoMode(GammaRay::Vtk::StereoMode mode);
    void setThemeType(GammaRay::Vtk::ThemeType themeType);
    void setShowNodeLabel(bool show);
    void setShowEdgeLabel(bool show);
    void setShowEdgeArrow(bool show);

private:
    vtkSmartPointer<vtkMutableDirectedGraph> m_graph;
    vtkSmartPointer<vtkGraphLayoutStrategy> m_layoutStrategy;
    vtkSmartPointer<vtkGraphLayout> m_layout;
    vtkSmartPointer<vtkGraphLayoutView> m_layoutView;
    vtkSmartPointer<QVTKInteractor> m_interactor;
    vtkSmartPointer<vtkInteractorStyle> m_interactorStyle;
    vtkSmartPointer<vtkActor> m_arrowDecorator;
    vtkStringArray *m_objectLabelArray;
    vtkUnsignedLongLongArray *m_objectIdArray;
    vtkUnsignedLongLongArray *m_threadIdArray;
    vtkIntArray *m_connWeightArray;
    QAbstractItemModel *m_model = nullptr;
    bool m_showEdgeArrow = false;
    bool m_inputHasChanged = true;
    bool m_configHasChanged = true;
    qint64 m_dataDuration = 0;
    qint64 m_graphDuration = 0;
    qint64 m_renderDuration = 0;
    QElapsedTimer m_dataTimer;
    bool m_done = true;
    QSet<quint64> m_objectIds;
    QVector<std::tuple<quint64, quint64, std::string>> m_objects;
    QVector<std::tuple<quint64, quint64, int>> m_connections;

    bool tryFetchData();
    bool fetchData();
    bool buildGraph();
    void renderGraph();
    void createArrowDecorator(vtkAlgorithmOutput *input);
    void updateSatus(const QString &state);
};
} // namespace GammaRay

#endif // GAMMARAY_CONNECTIVITYINSPECTOR_VTKWIDGET_H
