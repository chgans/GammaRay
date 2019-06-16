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
#include <QVTKWidget2.h>

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
class vtkGraph;
class vtkGraphLayout;
class vtkGraphLayoutStrategy;
class vtkGraphLayoutView;
class vtkIntArray;
class vtkInteractorStyle;
class vtkMutableDirectedGraph;
class vtkStringArray;
class vtkUnsignedLongLongArray;
class vtkAnnotationLink;
class vtkDataRepresentation;
class vtkViewUpdater;

namespace GammaRay {

class vtkGraphAdapter;

// TODO: QVTKOpenGLNativeWidget (v8) and QVTKWidget2 (v7)
// QVTKOpenGLWidget, QVTKOpenGLWindow
class VtkWidget : public QVTKWidget // QVTKWidget2
{
    Q_OBJECT

public:
    explicit VtkWidget(QWidget *parent = nullptr);
    virtual ~VtkWidget();

    vtkGraphAdapter *graphAdapter();

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
    vtkGraphAdapter *m_graphAdapter;
    vtkSmartPointer<vtkViewUpdater> m_updater;
    // vtkDataObject: input of the pipeline
    // vtkGraph *m_graph = nullptr;
    // vtkAlgorithm: Layout it's input graph using a strategy
    vtkSmartPointer<vtkGraphLayout> m_layout;
    // vtkObject
    vtkSmartPointer<vtkGraphLayoutStrategy> m_layoutStrategy;
    bool m_is3dLayout = false;
    // vtkActor: takes the graph and an arrow source as input, decorate edges
    vtkSmartPointer<vtkActor> m_arrowDecorator;

    // vtkRenderView
    vtkSmartPointer<vtkGraphLayoutView> m_layoutView;
    // RenderWindowInteractor
    vtkSmartPointer<QVTKInteractor> m_interactor;
    // vtkInteractorObserver
    vtkSmartPointer<vtkInteractorStyle> m_interactorZoomStyle;

    vtkSmartPointer<vtkAnnotationLink> m_annotationLink;

    // Internal state and data
    bool m_showEdgeArrow = false;
    bool m_inputHasChanged = true;
    bool m_configHasChanged = true;
    qint64 m_renderDuration = 0;
    void createArrowDecorator(vtkAlgorithmOutput *input);
    void updateSatus(const QString &state);
    void updateGraph();
};
} // namespace GammaRay

#endif // GAMMARAY_CONNECTIVITYINSPECTOR_VTKWIDGET_H
