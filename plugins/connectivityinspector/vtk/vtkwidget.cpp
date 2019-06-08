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
#include "connectionmodel.h"

#include "common/objectid.h"

#include <QAbstractItemModel>
#include <QDebug>
#include <QElapsedTimer>
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

inline QString objectLabel(QAbstractItemModel *model, int row, int col)
{
    constexpr int role = Qt::DisplayRole;
    const QModelIndex index = model->index(row, col);
    return index.data(role).toString();
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
{
    m_objectIdArray = vtkUnsignedLongLongArray::New();
    m_objectIdArray->SetName(s_ObjectIdArrayName);
    m_threadIdArray = vtkUnsignedLongLongArray::New();
    m_threadIdArray->SetName(s_ThreadIdArrayName);
    m_objectLabelArray = vtkStringArray::New();
    m_objectLabelArray->SetName(s_ObjectLabelArrayName);
    m_connWeightArray = vtkIntArray::New();
    m_connWeightArray->SetName(s_connWeightArrayName);

    m_graph = vtkMutableDirectedGraph::New();
    m_layout = vtkSmartPointer<vtkGraphLayout>::New();
    m_layout->SetInputDataObject(m_graph);
    m_layoutView = vtkSmartPointer<vtkGraphLayoutView>::New();
    m_layoutView->AddRepresentationFromInputConnection(m_layout->GetOutputPort());
    createArrowDecorator(m_layout->GetOutputPort());

    m_layoutView->SetVertexColorArrayName("VertexDegree");
    m_layoutView->SetColorVertices(true);
    m_layoutView->SetVertexLabelArrayName(s_ObjectLabelArrayName);
    m_layoutView->SetVertexLabelVisibility(true);
    m_layoutView->SetGlyphType(vtkGraphToGlyphs::SPHERE);
    m_layoutView->SetEdgeColorArrayName(s_connWeightArrayName);
    m_layoutView->SetColorEdges(true);
    m_layoutView->SetEdgeLabelArrayName(s_connWeightArrayName);
    m_layoutView->SetEdgeLabelVisibility(true);
    m_layoutView->SetLayoutStrategyToPassThrough();

    m_interactorStyle = vtkInteractorStyleTrackballCamera::New();
    m_interactor = vtkSmartPointer<QVTKInteractor>::New();
    m_interactor->SetRenderWindow(m_layoutView->GetRenderWindow());
    m_interactor->SetInteractorStyle(m_interactorStyle);
    m_interactor->Initialize();
    SetRenderWindow(m_layoutView->GetRenderWindow());
}

VtkWidget::~VtkWidget() {}

void VtkWidget::setModel(QAbstractItemModel *model)
{
    if (m_model) {
        m_model->disconnect(this);
    }

    m_model = model;

    if (m_model) {
        connect(m_model, &QAbstractItemModel::modelReset, this, [this]() {
            qWarning() << "MODEL RESET";
            m_inputHasChanged = true;
            //updateGraph();
        });
        connect(m_model,
                &QAbstractItemModel::rowsRemoved,
                this,
                [this](const QModelIndex &parent, int first, int last) {
                    qWarning() << "ROW REMOVED" << first << last;
                    m_inputHasChanged = true;
                    //updateGraph();
                });
        connect(m_model,
                &QAbstractItemModel::rowsInserted,
                this,
                [this](const QModelIndex &parent, int first, int last) {
                    qWarning() << "ROW INSERTED" << first << last;
                    m_inputHasChanged = true;
                    //updateGraph();
                });
        connect(m_model, &QAbstractItemModel::dataChanged, this, [this]() {
            qWarning() << "DATA CHANGED";
            m_inputHasChanged = true;
            //updateGraph();
        });
    }
}

void VtkWidget::updateGraph()
{
    qWarning() << "UPDATE GRAPH" << m_inputHasChanged << m_configHasChanged << m_done;
    if (!m_model)
        return;

    if (!(m_inputHasChanged || m_configHasChanged))
        return;

    if (!m_done)
        return;

    qWarning() << "UPDATING GRAPH...";
    m_done = false;
    m_dataTimer.start();
    m_fetchDuration = 0;
    setEnabled(false);
    m_objectIds.clear();
    m_objects.clear();
    m_connections.clear();
    m_connectionIds.clear();
    tryUpdateGraph();
}

bool VtkWidget::tryUpdateGraph()
{
    updateSatus("Fetching data");
    qWarning() << "FETCHING DATA...";
    if (!fetchData()) {
        QTimer::singleShot(250, this, [this]() { tryUpdateGraph(); });
        m_fetchDuration = m_dataTimer.elapsed();
        qWarning() << "FETCHING RESCEDULED";
        return false;
    }
    qWarning() << "FETCHING DONE";

    m_fetchDuration = m_dataTimer.elapsed();
    buildGraph();
    renderGraph();
    m_layoutView->ResetCamera();
    setEnabled(true);
    updateSatus("Done");
    m_inputHasChanged = false;
    m_configHasChanged = false;
    m_done = true;
    return true;
}

bool VtkWidget::fetchData()
{
    if (!m_inputHasChanged)
        return true;

    // Collect unique and valid objects and their connection counters
    // Keep everything ordered for the graph data arrays
    for (int row = 0; row < m_model->rowCount(); ++row) {
        auto senderObjectId = objectId(m_model, row, ConnectionModel::SenderColumn);
        auto senderThreadId = threadId(m_model, row, ConnectionModel::SenderColumn);
        auto senderLabel = objectLabel(m_model, row, ConnectionModel::SenderColumn);
        auto receiverObjectId = objectId(m_model, row, ConnectionModel::ReceiverColumn);
        auto receiverThreadId = threadId(m_model, row, ConnectionModel::ReceiverColumn);
        auto receiverLabel = objectLabel(m_model, row, ConnectionModel::ReceiverColumn);
        auto edgeWeight = weight(m_model, row);
        const bool done = senderObjectId && senderThreadId && !senderLabel.isNull()
                          && receiverObjectId && receiverThreadId && !receiverLabel.isNull()
                          && edgeWeight;
        if (done) {
            if (!m_objectIds.contains(senderObjectId)) {
                m_objects.append({senderObjectId, senderThreadId, senderLabel.toStdString()});
                m_objectIds.insert(senderObjectId);
            }
            if (!m_objectIds.contains(receiverObjectId)) {
                m_objects.append({receiverObjectId, receiverThreadId, receiverLabel.toStdString()});
                m_objectIds.insert(receiverObjectId);
            }
            if (!m_connectionIds.contains({senderObjectId, receiverObjectId})) {
                m_connections.append({senderObjectId, receiverObjectId, edgeWeight});
                m_connectionIds.insert({senderObjectId, receiverObjectId});
            }
        }
    }
    return m_connectionIds.count() == m_model->rowCount();
}

bool VtkWidget::buildGraph()
{
    if (!m_inputHasChanged)
        return true;

    m_graphDuration = 0;
    updateSatus("Building graph");
    QElapsedTimer timer;
    timer.start();

    // Create all the nodes and only then the edges to avoid auto-created nodes
    m_graph->Initialize();
    m_objectIdArray->Reset();
    m_graph->GetVertexData()->AddArray(m_objectIdArray);
    m_graph->GetVertexData()->SetPedigreeIds(m_objectIdArray);
    m_threadIdArray->Reset();
    m_graph->GetVertexData()->AddArray(m_threadIdArray);
    m_objectLabelArray->Reset();
    m_graph->GetVertexData()->AddArray(m_objectLabelArray);
    m_connWeightArray->Reset();
    m_graph->GetEdgeData()->AddArray(m_connWeightArray);
    for (const auto &data : m_objects) {
        m_graph->AddVertex();
        m_objectIdArray->InsertNextValue(std::get<0>(data));
        m_threadIdArray->InsertNextValue(std::get<1>(data));
        m_objectLabelArray->InsertNextValue(std::get<2>(data));
    }
    for (const auto &data : m_connections) {
        const auto senderId = std::get<0>(data);
        const auto receiverId = std::get<1>(data);
        const auto weight = std::get<2>(data);
        m_graph->AddEdge(vtkVariant(senderId), vtkVariant(receiverId));
        m_connWeightArray->InsertNextValue(weight);
    }
    m_graphDuration = timer.elapsed();
    return true;
}

// FIXME: Arrow size need to scale with graph representation size.
void VtkWidget::createArrowDecorator(vtkAlgorithmOutput *input)
{
    vtkSmartPointer<vtkGraphToPolyData> graphToPoly = vtkSmartPointer<vtkGraphToPolyData>::New();
    graphToPoly->SetInputConnection(input);
    graphToPoly->EdgeGlyphOutputOn();
    graphToPoly->SetEdgeGlyphPosition(0.6);
    auto arrowSource = vtkSmartPointer<vtkArrowSource>::New();
    // arrowSource->SetTipLength();
    // arrowSource->SetTipRadius();
    // arrowSource->SetTipResolution();
    // arrowSource->SetShaftRadius(1.0);
    // arrowSource->SetShaftResolution();
    arrowSource->Update();
    auto arrowGlyph = vtkSmartPointer<vtkGlyph3D>::New();
    arrowGlyph->SetInputConnection(0, graphToPoly->GetOutputPort(1));
    arrowGlyph->SetInputConnection(1, arrowSource->GetOutputPort());
    //arrowGlyph->SetScaling(1);
    auto arrowMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    arrowMapper->SetInputConnection(arrowGlyph->GetOutputPort());    
    m_arrowDecorator = vtkSmartPointer<vtkActor>::New();
    m_arrowDecorator->SetMapper(arrowMapper);
}

void VtkWidget::updateSatus(const QString &state)
{
    const auto status = QStringLiteral("State: %1, Fetch: %2 ms, Build: %3 ms, "
                                       "Render: %4 ms")
                            .arg(state)
                            .arg(m_fetchDuration)
                            .arg(m_graphDuration)
                            .arg(m_renderDuration);
    emit statusChanged(status);
}

void VtkWidget::renderGraph()
{
    m_renderDuration = 0;
    updateSatus("Rendering graph");
    QElapsedTimer timer;
    timer.start();
    m_layoutView->Render();
    m_renderDuration = timer.elapsed();
}

void VtkWidget::setLayoutStrategy(Vtk::LayoutStrategy strategy)
{
    switch (strategy) {
    case Vtk::LayoutStrategy::Cone: {
        auto *strategy = vtkConeLayoutStrategy::New();
        // strategy->SetSpacing(1.0);
        // strategy->SetCompactness(1.0);
        // strategy->SetCompression(1);
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::Tree: {
        auto strategy = vtkTreeLayoutStrategy::New();
        // strategy->SetRotation(0.0);
        // strategy->SetLeafSpacing(1.0);
        // strategy->SetLogSpacingValue(1.0);
        // strategy->SetDistanceArrayName("distance");
        strategy->SetRadial(true);
        strategy->SetAngle(360);
        strategy->SetLogSpacingValue(1);
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::Fast2D: {
        auto *strategy = vtkFast2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetJitter(true);
        // strategy->SetRestDistance(1.0);
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::Random: {
        auto *strategy = vtkRandomLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetGraphBounds(...);
        // strategy->SetThreeDimensionalLayout(1);
        // strategy->SetAutomaticBoundsComputation(1);
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
        strategy->SetRandomSeed(0);
        // strategy->SetJitter(true);
        // strategy->SetRestDistance(1.0);
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::SpanTree: {
        auto *strategy = vtkSpanTreeLayoutStrategy::New();
        // strategy->SetDepthFirstSpanningTree(true);
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::Community2D: {
        auto *strategy = vtkCommunity2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetRestDistance(1.0);
        // strategy->SetCommunityStrength(1.0);
        strategy->SetCommunityArrayName(s_ThreadIdArrayName);
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::Clustering2D: {
        auto *strategy = vtkClustering2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetRestDistance(1.0);
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::ForceDirected2D: {
        auto *strategy = vtkForceDirectedLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetRandomInitialPoints(1);
        // strategy->SetGraphBounds(...);
        // strategy->SetThreeDimensionalLayout(1);
        // strategy->SetAutomaticBoundsComputation(1);
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::ForceDirected3D: {
        // same as above
        auto *strategy = vtkForceDirectedLayoutStrategy::New();
        strategy->SetThreeDimensionalLayout(true);
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::AssignCoordinates: {
        auto *strategy = vtkAssignCoordinatesLayoutStrategy::New();
        // strategy->SetXCoordArrayName();
        // strategy->SetYCoordArrayName();
        // strategy->SetZCoordArrayName();
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::AttributeClustering2D: {
        auto *strategy = vtkAttributeClustering2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetRestDistance();
        strategy->SetVertexAttribute(s_ThreadIdArrayName);
        m_layoutStrategy = strategy;
        break;
    }
    case Vtk::LayoutStrategy::Constrained2D: {
        auto *strategy = vtkConstrained2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        strategy->SetInputArrayName(s_connWeightArrayName);
        m_layoutStrategy = strategy;

        break;
    }
    case Vtk::LayoutStrategy::Simple3DCircles: {
        auto *strategy = vtkSimple3DCirclesStrategy::New();
        // strategy->SetHeight();
        // strategy->SetMethod();
        // strategy->SetOrigin();
        // strategy->SetRadius();
        // strategy->SetDirection();
        // strategy->SetAutoHeight();
        // strategy->SetMarkedValue();
        // strategy->SetMinimumDegree();
        // strategy->SetMinimumRadian();
        // strategy->SetHierarchicalOrder();
        // strategy->SetHierarchicalLayers();
        // strategy->SetMarkedStartVertices();
        // strategy->SetForceToUseUniversalStartPointsFinder();
        m_layoutStrategy = strategy;
        break;
    }
    }

    // m_layoutStrategy->SetWeightEdges(true);
    // m_layoutStrategy->SetEdgeWeightField(s_connWeightArrayName);
    m_layoutStrategy->DebugOn();
    m_layoutStrategy->GlobalWarningDisplayOn();
    m_layout->SetLayoutStrategy(m_layoutStrategy);
    m_configHasChanged = true;
    updateGraph();
}

void VtkWidget::setStereoMode(Vtk::StereoMode mode)
{
    GetRenderWindow()->SetStereoType(static_cast<int>(mode));
    m_configHasChanged = true;
    updateGraph();
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
    m_configHasChanged = true;
    updateGraph();
}

void VtkWidget::setShowNodeLabel(bool show)
{
    if (m_layoutView->GetVertexLabelVisibility() == show)
        return;
    m_layoutView->SetVertexLabelVisibility(show);
    m_configHasChanged = true;
    updateGraph();
}

void VtkWidget::setShowEdgeLabel(bool show)
{
    if (m_layoutView->GetEdgeLabelVisibility() == show)
        return;
    m_layoutView->SetEdgeLabelVisibility(show);
    m_configHasChanged = true;
    updateGraph();
}

/* See:
 * https://vtk.org/Wiki/VTK/Examples/Cxx/Graphs/VisualizeDirectedGraph
 * https://lorensen.github.io/VTKExamples/site/Cxx/GeometricObjects/Arrow/
 */
void VtkWidget::setShowEdgeArrow(bool show)
{
    if (m_showEdgeArrow == show)
        return;
    m_showEdgeArrow = show;
    if (m_showEdgeArrow) {
        m_layoutView->GetRenderer()->AddActor(m_arrowDecorator);
        m_layoutView->SetEdgeLayoutStrategyToPassThrough();
        // m_layoutView->SetScaledGlyphs(true); // only applies to nodes
    } else {
        m_layoutView->GetRenderer()->RemoveActor(m_arrowDecorator);
        m_layoutView->SetEdgeLayoutStrategyToArcParallel();
        // m_layoutView->SetScaledGlyphs(false);  // only applies to nodes
    }
    m_configHasChanged = true;
    updateGraph();
}
