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
#include <QMouseEvent>
#include <QTimer>

#include <vtkAnnotationLink.h>
#include <vtkArrowSource.h>
#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkDataRepresentation.h>
#include <vtkDataSetAttributes.h>
#include <vtkGlyph3D.h>
#include <vtkGlyphSource2D.h>
#include <vtkGraphLayout.h>
#include <vtkGraphLayoutView.h>
#include <vtkGraphToGlyphs.h>
#include <vtkGraphToPolyData.h>
#include <vtkInteractorStyleRubberBand2D.h>
#include <vtkInteractorStyleRubberBand3D.h>
#include <vtkInteractorStyleRubberBandZoom.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkPickingManager.h>
#include <vtkPolyDataMapper.h>
#include <vtkPropPicker.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderedGraphRepresentation.h>
#include <vtkRenderer.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
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

VtkWidget::VtkWidget(QWidget *parent)
    : QVTKWidget(parent)
{
    m_layout = vtkSmartPointer<vtkGraphLayout>::New();
    m_layoutView = vtkSmartPointer<vtkGraphLayoutView>::New();
    m_layoutView->AddRepresentationFromInputConnection(m_layout->GetOutputPort());
    createArrowDecorator(m_layout->GetOutputPort());

    m_layoutView->SetColorVertices(true);
    m_layoutView->SetVertexLabelVisibility(true);
    m_layoutView->SetGlyphType(vtkGraphToGlyphs::SPHERE);
    m_layoutView->SetColorEdges(true);
    m_layoutView->SetEdgeLabelVisibility(true);
    m_layoutView->SetLayoutStrategyToPassThrough();

    auto representation = m_layoutView->GetRepresentation(0);
    representation->SetSelectionType(vtkSelectionNode::PEDIGREEIDS);
    m_annotationLink = vtkSmartPointer<vtkAnnotationLink>::New();
    m_annotationLink->AddObserver(vtkCommand::AnnotationChangedEvent,
                                  this,
                                  &VtkWidget::annotationChangedEvent);
    representation->SetAnnotationLink(m_annotationLink);

    // 2D/3D not needed: m_layoutView->SetInteractionModeTo[2,3]D();
    // We wight still want to switch to box zoom
    m_interactor3dStyle = vtkSmartPointer<vtkInteractorStyleRubberBand3D>::New();
    m_interactor2dStyle = vtkSmartPointer<vtkInteractorStyleRubberBand2D>::New();
    m_interactorZoomStyle = vtkSmartPointer<vtkInteractorStyleRubberBandZoom>::New();
    m_layoutView->SetInteractor(GetInteractor());
    SetRenderWindow(m_layoutView->GetRenderWindow());
}

VtkWidget::~VtkWidget() {}

void VtkWidget::setGraph(vtkGraph *graph)
{
    m_graph = graph;
    m_layout->SetInputDataObject(m_graph);
    m_layoutView->SetVertexLabelArrayName("NodeLabel"); // FIXME
    m_layoutView->SetEdgeLabelArrayName("EdgeLabel");   // FIXME
    m_layoutView->SetEdgeColorArrayName("centrality");
    m_layoutView->SetVertexColorArrayName("VertexDegree"); // magic value
    m_inputHasChanged = true;
}

void VtkWidget::updateGraph()
{
    qWarning() << "UPDATE GRAPH" << m_inputHasChanged << m_configHasChanged;
    if (!m_graph)
        return;

    if (!(m_inputHasChanged || m_configHasChanged))
        return;

    qWarning() << "UPDATING GRAPH...";
#if 0
    renderGraph();
    m_layoutView->ResetCamera();
    auto camera = m_layoutView->GetRenderer()->GetActiveCamera();
    double bounds[6]; // xmin,xmax, ymin,ymax, zmin,zmax
    //m_graph->ComputeBounds();
    //m_graph->GetBounds(bounds);
    auto represenation = vtkRenderedGraphRepresentation::SafeDownCast(
        m_layoutView->GetRepresentation(0));
    represenation->ComputeSelectedGraphBounds(bounds);
    qDebug() << "Graph bounds" << QStringLiteral("[%1, %2]").arg(bounds[0]).arg(bounds[1])
             << QStringLiteral("[%1, %2]").arg(bounds[2]).arg(bounds[3])
             << QStringLiteral("[%1, %2]").arg(bounds[4]).arg(bounds[5]);
    if (m_is3dLayout) {
        m_layoutView->SetInteractorStyle(m_interactor3dStyle);
        camera->SetFocalPoint(bounds[0], bounds[2], bounds[4]);
        camera->SetPosition(2 * bounds[1], 2 * bounds[3], 2 * bounds[5]);
        camera->SetViewAngle(60);
    } else {
        m_layoutView->SetInteractorStyle(m_interactor2dStyle);
        camera->SetFocalPoint(bounds[0], bounds[2], bounds[4]);
        camera->SetPosition((bounds[1] - bounds[0]) / 2, (bounds[3] - bounds[2]) / 2, 0);
    }
#else
    renderGraph();
    if (m_is3dLayout) {
        m_layoutView->SetInteractionModeTo3D();
    } else {
        m_layoutView->SetInteractionModeTo2D();
    }
    m_layoutView->ResetCamera();
#endif
    updateSatus("Done");
    m_inputHasChanged = false;
    m_configHasChanged = false;
}

void VtkWidget::annotationChangedEvent()
{
    qDebug() << Q_FUNC_INFO;
    auto selection = m_annotationLink->GetCurrentSelection();
    Q_ASSERT(selection->GetNumberOfNodes() == 2);
    auto node0 = selection->GetNode(0);
    auto node1 = selection->GetNode(1);
    if (node0->GetFieldType() == vtkSelectionNode::EDGE)
        std::swap(node0, node1);
    Q_ASSERT(node0->GetFieldType() == vtkSelectionNode::VERTEX);
    Q_ASSERT(node0->GetContentType() == vtkSelectionNode::PEDIGREEIDS);
    auto vertexList = node0->GetSelectionList();
    Q_ASSERT(node1->GetFieldType() == vtkSelectionNode::EDGE);
    Q_ASSERT(node1->GetContentType() == vtkSelectionNode::PEDIGREEIDS);
    auto edgeList = node1->GetSelectionList();

    for (int i = 0; i < vertexList->GetNumberOfValues(); ++i) {
        qDebug() << "Vertex selected: "
                 << QString::number(vertexList->GetVariantValue(i).ToTypeUInt64(), 16);
    }
    for (int i = 0; i < edgeList->GetNumberOfValues(); ++i) {
        qDebug() << "Edge selected: "
                 << QString::number(edgeList->GetVariantValue(i).ToTypeUInt64(), 16);
    }
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
    const auto status = QStringLiteral("State: %1, Render: %4 ms").arg(state).arg(m_renderDuration);
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
        m_is3dLayout = true;
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
        m_is3dLayout = true;
        break;
    }
    case Vtk::LayoutStrategy::Fast2D: {
        auto *strategy = vtkFast2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetJitter(true);
        // strategy->SetRestDistance(1.0);
        m_layoutStrategy = strategy;
        m_is3dLayout = false;
        break;
    }
    case Vtk::LayoutStrategy::Random: {
        auto *strategy = vtkRandomLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetGraphBounds(...);
        // strategy->SetThreeDimensionalLayout(1);
        // strategy->SetAutomaticBoundsComputation(1);
        m_layoutStrategy = strategy;
        m_is3dLayout = false; // FIXME: can choose
        break;
    }
    case Vtk::LayoutStrategy::Circular: {
        auto *strategy = vtkCircularLayoutStrategy::New();
        m_layoutStrategy = strategy;
        m_is3dLayout = false;
        break;
    }
    case Vtk::LayoutStrategy::Simple2D: {
        auto *strategy = vtkSimple2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetJitter(true);
        // strategy->SetRestDistance(1.0);
        m_layoutStrategy = strategy;
        m_is3dLayout = false;
        break;
    }
    case Vtk::LayoutStrategy::SpanTree: {
        auto *strategy = vtkSpanTreeLayoutStrategy::New();
        // strategy->SetDepthFirstSpanningTree(true);
        m_layoutStrategy = strategy;
        m_is3dLayout = true;
        break;
    }
    case Vtk::LayoutStrategy::Community2D: {
        auto *strategy = vtkCommunity2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetRestDistance(1.0);
        // strategy->SetCommunityStrength(1.0);
        //strategy->SetCommunityArrayName(s_ThreadIdArrayName); // FIXME
        m_layoutStrategy = strategy;
        m_is3dLayout = false;
        break;
    }
    case Vtk::LayoutStrategy::Clustering2D: {
        auto *strategy = vtkClustering2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetRestDistance(1.0);
        m_layoutStrategy = strategy;
        m_is3dLayout = false;
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
        m_is3dLayout = false;
        break;
    }
    case Vtk::LayoutStrategy::ForceDirected3D: {
        // same as above
        auto *strategy = vtkForceDirectedLayoutStrategy::New();
        strategy->SetThreeDimensionalLayout(true);
        m_layoutStrategy = strategy;
        m_is3dLayout = true;
        break;
    }
    case Vtk::LayoutStrategy::AssignCoordinates: {
        auto *strategy = vtkAssignCoordinatesLayoutStrategy::New();
        // strategy->SetXCoordArrayName();
        // strategy->SetYCoordArrayName();
        // strategy->SetZCoordArrayName();
        m_layoutStrategy = strategy;
        m_is3dLayout = true;
        break;
    }
    case Vtk::LayoutStrategy::AttributeClustering2D: {
        auto *strategy = vtkAttributeClustering2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetRestDistance();
        // strategy->SetVertexAttribute(s_ThreadIdArrayName); // FIXME
        m_layoutStrategy = strategy;
        m_is3dLayout = false;
        break;
    }
    case Vtk::LayoutStrategy::Constrained2D: {
        auto *strategy = vtkConstrained2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        //strategy->SetInputArrayName(s_connWeightArrayName); // FIXME
        m_layoutStrategy = strategy;
        m_is3dLayout = false;

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
        m_is3dLayout = true;
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
    // viewTheme->SetOutlineColor();

    //viewTheme->SetCellOpacity(0.3);

    //viewTheme->SetSelectedPointOpacity(0.3);
    //viewTheme->SetSelectedPointColor(0.0, 0.5, 1.0);
    // viewTheme->SetSelectedCellOpacity();
    // viewTheme->SetSelectedCellColor();

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

#include <vtkPropAssembly.h>

// https://vtk.org/Wiki/VTK/Examples/Python/Graphs/SelectedVerticesAndEdges
// https://itk.org/Wiki/VTK/Examples/Python/Infovis/SelectedGraphIDs
// https://vtk.org/Wiki/VTK/Examples/Cxx/Graphs/SelectedVerticesAndEdges
// https://vtk.org/gitweb?p=VTK.git;a=blob;f=Examples/Modelling/Python/SpherePuzzle.py
// https://vtk.org/Wiki/VTK/Examples/Cxx/Picking/CellPicking
void GammaRay::VtkWidget::mousePressEvent(QMouseEvent *event)
{
#if 0
    m_layoutView->GetRepresentation(0)->SetSelectionType(vtkSelectionNode::PEDIGREEIDS);

    int *pos = m_interactor->GetEventPosition();
    vtkSmartPointer<vtkCellPicker> picker = vtkCellPicker::New();
    picker->Pick(pos[0], pos[1], 0, m_layoutView->GetRenderer());
    qDebug() << "Node" << picker->GetPointId() << "Cell" << picker->GetCellId();
    auto actor = picker->GetActor();
    if (actor) {
        //        qDebug() << "Actor picking";
        //        actor->GetProperty()->SetPointSize(30);
        //        actor->GetProperty()->SetColor(0, 0, 1);
        //        actor->PrintSelf(std::cout, vtkIndent(0));
        //        m_layoutView->GetRenderWindow()->Render();
    } else {
        qDebug() << "No actor picking";
    }
    if (picker->GetProp3D()) {
        qDebug() << "Prop3D picking";
        //        auto prop3D = picker->GetProp3D();
        //        prop3D->PrintSelf(std::cout, vtkIndent(0));
        //        prop3D->SetVisibility(0);
        //        m_layoutView->GetRenderWindow()->Render();
    }
    if (picker->GetVolume())
        qDebug() << "Volume picking";
    if (picker->GetAssembly())
        qDebug() << "Assy picking";
    if (picker->GetPropAssembly())
        qDebug() << "Prop Assy picking";

    //    vtkPropAssembly *assy = picker->GetPropAssembly();
    //    int num = assy->GetNumberOfPaths();
    //    for (int i = 0; i < num; i++) {
    //        var prop = collection.GetNextProp3D();
    //    }
#endif
    QVTKWidget::mousePressEvent(event);
}
