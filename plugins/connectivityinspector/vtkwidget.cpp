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

#include "vtkwidget.h"
#include "connectivityinspectormodel.h"

#include "common/objectid.h"

#include <QAbstractItemModel>
#include <QHash>
#include <QTimer>

#include <vtkArrowSource.h>
#include <vtkDataSetAttributes.h>
#include <vtkGlyph3D.h>
#include <vtkGlyphSource2D.h>
#include <vtkGraphLayout.h>
#include <vtkGraphLayoutView.h>
#include <vtkGraphToGlyphs.h>
#include <vtkGraphToPolyData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkUnsignedLongLongArray.h>

// TODO: move to common, along with theme and stereo
#include <vtkAssignCoordinatesLayoutStrategy.h>
#include <vtkAttributeClustering2DLayoutStrategy.h>
#include <vtkCircularLayoutStrategy.h>
#include <vtkClustering2DLayoutStrategy.h>
#include <vtkCommunity2DLayoutStrategy.h>
#include <vtkConeLayoutStrategy.h>
#include <vtkConstrained2DLayoutStrategy.h>
#include <vtkCosmicTreeLayoutStrategy.h>
#include <vtkFast2DLayoutStrategy.h>
#include <vtkForceDirectedLayoutStrategy.h>
#include <vtkGraphLayoutView.h>
#include <vtkRandomLayoutStrategy.h>
#include <vtkRenderWindow.h>
#include <vtkSimple2DLayoutStrategy.h>
#include <vtkSimple3DCirclesStrategy.h>
#include <vtkSpanTreeLayoutStrategy.h>
#include <vtkTreeLayoutStrategy.h>
#include <vtkTreeOrbitLayoutStrategy.h>

#include <vtkViewTheme.h>

#include <iostream>
#include <limits>

using namespace GammaRay;

namespace {
const char *s_ObjectIdArrayName = "ObjectId";
const char *s_ThreadIdArrayName = "ThreadId";
const char *s_ObjectLabelArrayName = "ObjectLabel";
const char *s_connWeightArrayName = "ConnWeight";

inline quint64 objectId(QAbstractItemModel *model, int row, int col)
{
    constexpr int role = ConnectionModel::ObjectIdRole;
    const QModelIndex index = model->index(row, col);
    return index.data(role).value<ObjectId>().id();
}

inline quint64 threadId(QAbstractItemModel *model, int row, int col)
{
    constexpr int role = ConnectionModel::ThreadIdRole;
    const QModelIndex index = model->index(row, col);
    return index.data(role).value<ObjectId>().id();
}

inline std::string objectLabel(QAbstractItemModel *model, int row, int col)
{
    constexpr int role = Qt::DisplayRole;
    const QModelIndex index = model->index(row, col);
    return index.data(role).toString().toStdString();
}

inline int weight(QAbstractItemModel *model, int row)
{
    constexpr int role = Qt::DisplayRole;
    constexpr int col = ConnectionModel::CountColumn;
    const QModelIndex index = model->index(row, col);
    return index.data(role).toInt();
}
} // namespace

VtkWidget::VtkWidget(QWidget *parent)
    : QVTKWidget(parent)
    , m_renderTimer(new QTimer(this))
{
    m_layout = vtkSmartPointer<vtkGraphLayout>::New();

    m_layoutView = vtkSmartPointer<vtkGraphLayoutView>::New();

    m_interactorStyle = vtkInteractorStyleTrackballCamera::New();
    m_interactor = vtkSmartPointer<QVTKInteractor>::New();
    m_interactor->SetRenderWindow(m_layoutView->GetRenderWindow());
    m_interactor->SetInteractorStyle(m_interactorStyle);
    m_interactor->Initialize();
    SetRenderWindow(m_layoutView->GetRenderWindow());

    m_layoutView->SetVertexColorArrayName("VertexDegree");
    m_layoutView->SetColorVertices(true);
    m_layoutView->SetVertexLabelArrayName(s_ObjectLabelArrayName);
    m_layoutView->SetVertexLabelVisibility(true);
    m_layoutView->SetGlyphType(vtkGraphToGlyphs::SPHERE);

    m_layoutView->SetEdgeColorArrayName(s_connWeightArrayName);
    m_layoutView->SetColorEdges(true);
    m_layoutView->SetEdgeLabelArrayName(s_connWeightArrayName);
    m_layoutView->SetEdgeLabelVisibility(true);

    m_renderTimer->setInterval(250);
    connect(m_renderTimer, &QTimer::timeout, this, [this]() { renderGraph(); });
}

VtkWidget::~VtkWidget() {}

void VtkWidget::setModel(QAbstractItemModel *model)
{
    if (m_model) {
        m_model->disconnect(this);
        m_renderTimer->stop();
    }

    m_model = model;

    if (m_model) {
        m_renderTimer->start(); // FIXME
    }
}

void VtkWidget::buildGraph()
{
    m_graph = vtkMutableDirectedGraph::New();
    m_objectIdArray = vtkUnsignedLongLongArray::New();
    m_objectIdArray->SetName(s_ObjectIdArrayName);
    m_graph->GetVertexData()->AddArray(m_objectIdArray);
    m_graph->GetVertexData()->SetPedigreeIds(m_objectIdArray);
    m_threadIdArray = vtkUnsignedLongLongArray::New();
    m_threadIdArray->SetName(s_ThreadIdArrayName);
    m_graph->GetVertexData()->AddArray(m_threadIdArray);
    m_objectLabelArray = vtkStringArray::New();
    m_objectLabelArray->SetName(s_ObjectLabelArrayName);
    m_graph->GetVertexData()->AddArray(m_objectLabelArray);
    m_connWeightArray = vtkIntArray::New();
    m_connWeightArray->SetName(s_connWeightArrayName);
    m_graph->GetEdgeData()->AddArray(m_connWeightArray);

    // Collect unique and valid objects
    QHash<quint64, std::pair<std::string, quint64>> objects;
    for (int row = 0; row < m_model->rowCount(); ++row) {
        {
            auto oId = objectId(m_model, row, ConnectionModel::SenderColumn);
            auto tId = threadId(m_model, row, ConnectionModel::SenderColumn);
            auto label = objectLabel(m_model, row, ConnectionModel::SenderColumn);
            if (oId && tId)
                objects.insert(oId, {label, tId});
        }
        {
            auto oId = objectId(m_model, row, ConnectionModel::ReceiverColumn);
            auto tId = threadId(m_model, row, ConnectionModel::ReceiverColumn);
            auto label = objectLabel(m_model, row, ConnectionModel::ReceiverColumn);
            if (oId && tId)
                objects.insert(oId, {label, tId});
        }
    }

    // Create object nodes
    auto next = objects.constKeyValueBegin();
    const auto none = objects.constKeyValueEnd();
    while (next != none) {
        m_graph->AddVertex();
        m_objectIdArray->InsertNextValue((*next).first);
        m_objectLabelArray->InsertNextValue((*next).second.first);
        m_threadIdArray->InsertNextValue((*next).second.second);
        ++next;
    }

    // Create connection edges
    for (int row = 0; row < m_model->rowCount(); ++row) {
        auto senderId = objectId(m_model, row, ConnectionModel::SenderColumn);
        auto receiverId = objectId(m_model, row, ConnectionModel::ReceiverColumn);
        auto edgeWeight = weight(m_model, row);
        if (senderId && receiverId) {
            m_graph->AddEdge(vtkVariant(senderId), vtkVariant(receiverId));
            m_connWeightArray->InsertNextValue(edgeWeight);
        }
    }
}

void VtkWidget::updateGraph()
{
    // TODO: add/remove pending nodes/edges
}

void VtkWidget::renderGraph()
{
    if (!m_model)
        return;

    buildGraph();
    if (m_graph->GetNumberOfVertices() == 0) {
        m_layoutView->ResetCamera();
        return;
    }

    /*
     * https://vtk.org/Wiki/VTK/Examples/Cxx/Graphs/VisualizeDirectedGraph
     * https://lorensen.github.io/VTKExamples/site/Cxx/GeometricObjects/Arrow/
     * 
     * TODO: 
     *  - Should be part of the "pipeline", eg. buildPipeline() which should handle the bits in the ctor.
     *  - Still very buggy ... :(
     */
    if (m_showEdgeArrow) {
        // Do layout manually before handing graph to the view.
        // This allows us to know the positions of edge arrows.
        m_layout->SetInputDataObject(m_graph);
        m_layout->SetLayoutStrategy(m_layoutStrategy);
        // Tell the view to use the vertex layout we provide
        m_layoutView->SetLayoutStrategyToPassThrough();
        // The arrows will be positioned on a straight line between two
        // vertices so tell the view not to draw arcs for parallel edges
        m_layoutView->SetEdgeLayoutStrategyToPassThrough();
        // Add the graph to the view. This will render vertices and edges,
        // but not edge arrows.
        m_layoutView->AddRepresentationFromInputConnection(m_layout->GetOutputPort());
        // Manually create an actor containing the glyphed arrows.
        vtkSmartPointer<vtkGraphToPolyData> graphToPoly = vtkSmartPointer<vtkGraphToPolyData>::New();
        graphToPoly->SetInputConnection(m_layout->GetOutputPort());
        graphToPoly->EdgeGlyphOutputOn();
        // Set the position (0: edge start, 1: edge end) where
        // the edge arrows should go.
        // chgans: the arrow center is the node position, so 0.98 is not goos, and 0.5 is too close to labels...
        graphToPoly->SetEdgeGlyphPosition(0.51);
        // Create an arrow.
        auto arrowSource = vtkSmartPointer<vtkArrowSource>::New();
        //arrowSource->SetShaftRadius(1.0);
        //arrowSource->SetTipLength(1.0);
        arrowSource->Update();
        // Use Glyph3D to repeat the glyph on all edges.
        auto arrowGlyph = vtkSmartPointer<vtkGlyph3D>::New();
        arrowGlyph->SetInputConnection(0, graphToPoly->GetOutputPort(1));
        arrowGlyph->SetInputConnection(1, arrowSource->GetOutputPort());
        // Create a mapper and actor
        auto arrowMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        arrowMapper->SetInputConnection(arrowGlyph->GetOutputPort());
        auto arrowActor = vtkSmartPointer<vtkActor>::New();
        arrowActor->SetMapper(arrowMapper);
        // Visualise
        m_layoutView->GetRenderer()->AddActor(arrowActor);
        m_layoutView->SetRepresentationFromInput(m_layout->GetOutput());
    } else {
        m_layoutView->SetRepresentationFromInput(m_graph);
    }
    m_layoutView->Render();
}

void VtkWidget::setLayoutStrategy(Vtk::LayoutStrategy strategy)
{
    switch (strategy) {
    case Vtk::LayoutStrategy::Cone: {
        auto *strategy = vtkConeLayoutStrategy::New();
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::Tree: {
        auto strategy = vtkTreeLayoutStrategy::New();
        strategy->SetRadial(true);
        strategy->SetAngle(360);
        strategy->SetLogSpacingValue(1);
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::Fast2D: {
        auto *strategy = vtkFast2DLayoutStrategy::New();
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::Random: {
        auto *strategy = vtkRandomLayoutStrategy::New();
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::Circular: {
        auto *strategy = vtkCircularLayoutStrategy::New();
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::Simple2D: {
        auto *strategy = vtkSimple2DLayoutStrategy::New();
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::SpanTree: {
        auto *strategy = vtkSpanTreeLayoutStrategy::New();
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::Community2D: {
        auto *strategy = vtkCommunity2DLayoutStrategy::New();
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::Clustering2D: {
        auto *strategy = vtkClustering2DLayoutStrategy::New();
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::ForceDirected2D: {
        auto *strategy = vtkForceDirectedLayoutStrategy::New();
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::ForceDirected3D: {
        auto *strategy = vtkForceDirectedLayoutStrategy::New();
        strategy->SetThreeDimensionalLayout(true);
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::AssignCoordinates: {
        auto *strategy = vtkAssignCoordinatesLayoutStrategy::New();
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::AttributeClustering2D: {
        auto *strategy = vtkAttributeClustering2DLayoutStrategy::New();
        strategy->SetVertexAttribute(s_ThreadIdArrayName);
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::Constrained2D: {
        auto *strategy = vtkConstrained2DLayoutStrategy::New();
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::Simple3DCircles: {
        auto *strategy = vtkSimple3DCirclesStrategy::New();
        m_layoutStrategy = strategy;
        break;
    }
    }

    renderGraph();
}

void VtkWidget::setStereoMode(Vtk::StereoMode mode)
{
    GetRenderWindow()->SetStereoType(static_cast<int>(mode));
    renderGraph();
}

void VtkWidget::setThemeType(Vtk::ThemeType themeType)
{
    vtkViewTheme *viewTheme = nullptr;
    switch (themeType) {
    case Vtk::ThemeType::Neon:
        viewTheme = vtkViewTheme::CreateNeonTheme();
        break;
    case Vtk::ThemeType::Ocean:
        viewTheme = vtkViewTheme::CreateOceanTheme();
        break;
    case Vtk::ThemeType::Mellow:
        viewTheme = vtkViewTheme::CreateMellowTheme();
        break;
    }
    Q_ASSERT(viewTheme != nullptr);
    viewTheme->SetLineWidth(1.0);
    viewTheme->SetPointSize(15);
    viewTheme->SetCellOpacity(0.3);
    m_layoutView->ApplyViewTheme(viewTheme);
    viewTheme->Delete();
    renderGraph();
}

void VtkWidget::setShowNodeLabel(bool show)
{
    if (m_layoutView->GetVertexLabelVisibility() == show)
        return;
    m_layoutView->SetVertexLabelVisibility(show);
    renderGraph();
}

void VtkWidget::setShowEdgeLabel(bool show)
{
    if (m_layoutView->GetEdgeLabelVisibility() == show)
        return;
    m_layoutView->SetEdgeLabelVisibility(show);
    renderGraph();
}

void VtkWidget::setShowEdgeArrow(bool show)
{
    if (m_showEdgeArrow == show)
        return;
    m_showEdgeArrow = show;
    renderGraph();
}
