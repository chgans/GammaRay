# shared part
set(gammaray_connectivityinspector_shared_srcs
  acquisitioninterface.cpp
  connectivityinspectorcommon.cpp
  discriminatorinterface.cpp
)
add_library(gammaray_connectivityinspector_shared STATIC ${gammaray_connectivityinspector_shared_srcs})
target_link_libraries(gammaray_connectivityinspector_shared LINK_PRIVATE gammaray_common)
set_target_properties(gammaray_connectivityinspector_shared PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features(gammaray_connectivityinspector_shared PUBLIC ${GAMMARAY_REQUIRED_CXX_FEATURES})

# probe part
set(gammaray_connectivityinspector_plugin_srcs
  connectivityinspector.cpp

  acquisitionengine.cpp
  connectiontypemodel.cpp
  connectionmodel.cpp
  discriminatorproxymodel.cpp
)

gammaray_add_plugin(gammaray_connectivityinspector_plugin
  JSON gammaray_connectivityinspector.json
  SOURCES ${gammaray_connectivityinspector_plugin_srcs}
)

target_include_directories(gammaray_connectivityinspector_plugin SYSTEM PRIVATE
    ${Qt5Core_PRIVATE_INCLUDE_DIRS}
)

target_link_libraries(gammaray_connectivityinspector_plugin
  gammaray_kitemmodels
  gammaray_core
  gammaray_common
  gammaray_common_internal
  gammaray_connectivityinspector_shared
)

######## START VTK VISUALIZATION

#VTK discovery works a lot better if you give CMake a hint using the VTK_DIR variable
find_path(VTK_DIR VTKConfig.cmake
  /usr/lib64/vtk /usr/lib/vtk /usr/local/lib64/vtk /usr/local/lib/vtk
)
set(VTK_LIBS vtkGUISupportQt
  vtkViewsInfovis
  vtkInfovisLayout
  vtkRenderingFreeType
)
find_package(VTK 7.1 COMPONENTS ${VTK_LIBS} CONFIG)
set_package_properties(VTK PROPERTIES
  TYPE OPTIONAL
  DESCRIPTION "Visualization Toolkit"
  PURPOSE "Needed for the object visualizer plugin (requires Qt5). Just for toying around, highly experimental plugin."
  URL "http://www.vtk.org"
)
if(VTK_FOUND)
  find_path(VTK_QT_INCLUDE_DIR NAMES QVTKWidget.h HINTS ${VTK_INCLUDE_DIRS})
  if(NOT VTK_QT_INCLUDE_DIR)
    message(STATUS "Cannot locate QVTKWidget.h in ${VTK_INCLUDE_DIRS}")
    set(VTK_FOUND FALSE)
    add_feature_info("QVTKWidget header" VTK_FOUND "Object visualizer plugin requires QVTKWidget.h header.")
  endif()
endif()

# Only possibly build the plugin under Qt5 (and when VTK was built with Qt5 support as well)
if(VTK_FOUND AND VTK_QT_VERSION EQUAL 5 AND GAMMARAY_BUILD_UI)
  set(OPTIONAL_VTK_LIBRARIES)

  add_definitions(-DVTKGUI_ENABLED -DVTK_EXCLUDE_STRSTREAM_HEADERS)

  include(${VTK_USE_FILE})

  set(gammaray_connectivityinspector_ui_plugin_srcs      
      connectivityinspectorwidget.cpp

      acquisitionwidget.cpp
      acquisitioncontroller.cpp
      countdecoratorproxymodel.cpp
      filtercontroller.cpp
      filterwidget.cpp
      filterwidget.ui
      synchonousproxymodel.cpp

      gv/gvcontainer.cpp
      gv/gvgraphicsview.cpp
      gv/gvpanel.cpp
      gv/gvrenderer.cpp
      gv/gvwidget.cpp

      vtk/vtkcommon.cpp
      vtk/vtkcontainer.cpp
      vtk/vtkpanel.cpp
      vtk/vtkwidget.cpp
      vtk/vtkgraphadapter.cpp
  )

  gammaray_add_plugin(gammaray_connectivityinspector_ui_plugin
    JSON gammaray_connectivityinspector.json
    SOURCES ${gammaray_connectivityinspector_ui_plugin_srcs}
  )
  target_include_directories(gammaray_connectivityinspector_ui_plugin SYSTEM PUBLIC ${VTK_INCLUDE_DIRS})
  target_link_libraries(gammaray_connectivityinspector_ui_plugin
    ${VTK_LIBS}
    gammaray_kitemmodels
    gammaray_common
    gammaray_ui
    gammaray_connectivityinspector_shared
  )
endif()

target_include_directories(gammaray_connectivityinspector_ui_plugin SYSTEM PUBLIC "/home/chgans/Projects/qt-graphviz/qgraphviz/src")
target_link_libraries(gammaray_connectivityinspector_ui_plugin "-L/home/chgans/Projects/qt-graphviz/build-graphviz-qt-Desktop_Qt_5_12_2_GCC_64bit-Debug/Debug/graphviz-qt.5c228652/ -lgraphviz-qt -lcgraph -lcdt -lgvc")
# target_include_directories(gammaray_connectivityinspector_ui_plugin "/usr/local/include")

######## END VTK VISUALIZATION
