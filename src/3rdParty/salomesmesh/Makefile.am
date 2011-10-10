# dir variable for easy change of Makefile.am locations
smeshdir = ./

# copy some files
targetdir = $(prefix)
#dist_target_DATA = LICENCE.lgpl
EXTRA_DIST = CMakeLists.txt LICENCE.lgpl.txt

# set compiler flags and include directories, -fpermissive and -ffriend-injection not needed for occ6.3
AM_CPPFLAGS = -I$(OCC_INC) -I$(srcdir)/$(smeshdir)inc/
AM_CXXFLAGS = -Wno-sign-compare -Wno-switch -Wno-reorder -Wno-unused -Wno-parentheses -Wno-comment

# set library names
if NETGEN
lib_LTLIBRARIES = libNETGENPlugin.la libSMDS.la libDriver.la libDriverSTL.la libDriverDAT.la libDriverUNV.la libSMESHDS.la libSMESH.la libStdMeshers.la
else
lib_LTLIBRARIES = libDriver.la libSMDS.la libDriverSTL.la libDriverDAT.la libDriverUNV.la libSMESHDS.la libSMESH.la libStdMeshers.la
endif

# set global linker flags (remove '-Wl,--no-undefined' since this doesn't seem to work on MacOSX)
#AM_LDFLAGS = -version-info 1:0:0 -Wl,--no-undefined -L$(OCC_LIB)
AM_LDFLAGS = -version-info 1:0:0 -L$(OCC_LIB) \
		-lTKernel -lTKService -lTKMath -lTKBRep -lTKTopAlgo -lTKGeomAlgo \
		-lTKGeomBase -lTKG3d -lTKG2d -lTKMeshVS -lTKShHealing -lTKPrim -lTKAdvTools

#uninstall-local:
#	-rm -rf $(DESTDIR)$(libdir)
#	-rm -rf $(DESTDIR)$(includedir)
#	-rm $(DESTDIR)$(prefix)/env_smesh.sh
#
#install-exec-local:
#	test -z "$(DESTDIR)$(prefix)" || $(mkdir_p) "$(DESTDIR)$(prefix)"
#	@echo "#!/bin/bash" 											> $(DESTDIR)$(prefix)/env_smesh.sh
#	@echo "export SMESHROOT="$(prefix) 								>> $(DESTDIR)$(prefix)/env_smesh.sh
#	@echo "if [ -z "LD_LIBRARY_PATH" ];" 							>> $(DESTDIR)$(prefix)/env_smesh.sh
#	@echo "then LD_LIBRARY_PATH="$(libdir)";" 						>> $(DESTDIR)$(prefix)/env_smesh.sh
#	@echo "else LD_LIBRARY_PATH="$(libdir)":\$$LD_LIBRARY_PATH;" 	>> $(DESTDIR)$(prefix)/env_smesh.sh 
#	@echo "fi" 														>> $(DESTDIR)$(prefix)/env_smesh.sh 
#	chmod a+x $(DESTDIR)$(prefix)/env_smesh.sh

# set SMDS sources
libSMDS_la_SOURCES = $(srcdir)/$(smeshdir)src/SMDS/SMDS_EdgePosition.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_FaceOfEdges.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_FaceOfNodes.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_FacePosition.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_IteratorOfElements.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_Mesh.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_MeshEdge.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_MeshElement.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_MeshElementIDFactory.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_MeshFace.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_MeshGroup.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_MeshIDFactory.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_MeshNode.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_MeshObject.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_MeshVolume.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_PolygonalFaceOfNodes.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_PolyhedralVolumeOfNodes.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_Position.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_QuadraticEdge.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_QuadraticFaceOfNodes.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_QuadraticVolumeOfNodes.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_SpacePosition.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_VertexPosition.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_VolumeOfFaces.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_VolumeOfNodes.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_MemoryLimit.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_VolumeTool.cpp \
$(srcdir)/$(smeshdir)src/SMDS/SMDS_Mesh0DElement.cpp

# set Driver sources
libDriver_la_SOURCES = $(srcdir)/$(smeshdir)src/Driver/Driver_Document.cpp \
$(srcdir)/$(smeshdir)src/Driver/Driver_Mesh.cpp \
$(srcdir)/$(smeshdir)src/Driver/Driver_SMDS_Mesh.cpp \
$(srcdir)/$(smeshdir)src/Driver/Driver_SMESHDS_Mesh.cpp

# set DriverSTL sources
libDriverSTL_la_SOURCES = $(srcdir)/$(smeshdir)src/DriverSTL/DriverSTL_R_SMDS_Mesh.cpp \
$(srcdir)/$(smeshdir)src/DriverSTL/DriverSTL_W_SMDS_Mesh.cpp

libDriverSTL_la_LIBADD = -lSMDS -lDriver
libDriverSTL_la_DEPENDENCIES = libSMDS.la libDriver.la

# set DriverDAT sources
libDriverDAT_la_SOURCES = $(srcdir)/$(smeshdir)src/DriverDAT/DriverDAT_R_SMDS_Mesh.cpp \
$(srcdir)/$(smeshdir)src/DriverDAT/DriverDAT_R_SMESHDS_Document.cpp \
$(srcdir)/$(smeshdir)src/DriverDAT/DriverDAT_R_SMESHDS_Mesh.cpp \
$(srcdir)/$(smeshdir)src/DriverDAT/DriverDAT_W_SMDS_Mesh.cpp \
$(srcdir)/$(smeshdir)src/DriverDAT/DriverDAT_W_SMESHDS_Document.cpp \
$(srcdir)/$(smeshdir)src/DriverDAT/DriverDAT_W_SMESHDS_Mesh.cpp

libDriverDAT_la_LIBADD = -lSMDS -lDriver
libDriverDAT_la_DEPENDENCIES = libSMDS.la libDriver.la

# set DriverUNV sources
libDriverUNV_la_SOURCES = $(srcdir)/$(smeshdir)src/DriverUNV/DriverUNV_R_SMDS_Mesh.cpp \
$(srcdir)/$(smeshdir)src/DriverUNV/DriverUNV_R_SMESHDS_Document.cpp \
$(srcdir)/$(smeshdir)src/DriverUNV/DriverUNV_R_SMESHDS_Mesh.cpp \
$(srcdir)/$(smeshdir)src/DriverUNV/DriverUNV_W_SMDS_Mesh.cpp \
$(srcdir)/$(smeshdir)src/DriverUNV/DriverUNV_W_SMESHDS_Document.cpp \
$(srcdir)/$(smeshdir)src/DriverUNV/DriverUNV_W_SMESHDS_Mesh.cpp \
$(srcdir)/$(smeshdir)src/DriverUNV/UNV2411_Structure.cpp \
$(srcdir)/$(smeshdir)src/DriverUNV/UNV2412_Structure.cpp \
$(srcdir)/$(smeshdir)src/DriverUNV/UNV2417_Structure.cpp \
$(srcdir)/$(smeshdir)src/DriverUNV/UNV_Utilities.cpp

libDriverUNV_la_LIBADD = -lSMDS -lDriver
libDriverUNV_la_DEPENDENCIES = libSMDS.la libDriver.la

# set SMESHDS sources
libSMESHDS_la_SOURCES = $(srcdir)/$(smeshdir)src/SMESHDS/SMESHDS_Command.cpp \
$(srcdir)/$(smeshdir)src/SMESHDS/SMESHDS_Document.cpp \
$(srcdir)/$(smeshdir)src/SMESHDS/SMESHDS_Group.cpp \
$(srcdir)/$(smeshdir)src/SMESHDS/SMESHDS_GroupBase.cpp \
$(srcdir)/$(smeshdir)src/SMESHDS/SMESHDS_GroupOnGeom.cpp \
$(srcdir)/$(smeshdir)src/SMESHDS/SMESHDS_Hypothesis.cpp \
$(srcdir)/$(smeshdir)src/SMESHDS/SMESHDS_Mesh.cpp \
$(srcdir)/$(smeshdir)src/SMESHDS/SMESHDS_Script.cpp \
$(srcdir)/$(smeshdir)src/SMESHDS/SMESHDS_SubMesh.cpp

libSMESHDS_la_LIBADD = -lSMDS
libSMESHDS_la_DEPENDENCIES = libSMDS.la

# set SMESH sources
libSMESH_la_SOURCES = $(srcdir)/$(smeshdir)src/SMESH/SMESH_0D_Algo.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_1D_Algo.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_2D_Algo.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_3D_Algo.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_Algo.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_Block.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_Exception.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_Gen.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_Group.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_HypoFilter.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_Hypothesis.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_Mesh.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_MeshEditor.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_MesherHelper.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_Octree.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_OctreeNode.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_Pattern.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_subMesh.cpp \
$(srcdir)/$(smeshdir)src/SMESH/SMESH_MeshVSLink.cpp \
$(srcdir)/$(smeshdir)src/Controls/SMESHControls.cpp \
$(srcdir)/$(smeshdir)src/Controls/SMESH_Controls.cpp

libSMESH_la_LIBADD = -lSMDS -lSMESHDS -lDriver -lDriverSTL -lDriverUNV -lDriverDAT
libSMESH_la_DEPENDENCIES = libSMDS.la libSMESHDS.la libDriver.la \
		libDriverSTL.la libDriverUNV.la libDriverDAT.la

# set StdMeshers sources
libStdMeshers_la_SOURCES = $(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_Arithmetic1D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_AutomaticLength.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_CompositeSegment_1D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_Deflection1D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_Distribution.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_FaceSide.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_Hexa_3D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_LayerDistribution.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_LengthFromEdges.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_LocalLength.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_MaxElementArea.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_MaxElementVolume.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_MEFISTO_2D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_NotConformAllowed.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_NumberOfLayers.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_NumberOfSegments.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_Penta_3D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_Prism_3D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_ProjectionSource1D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_ProjectionSource2D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_ProjectionSource3D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_ProjectionUtils.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_Projection_1D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_Projection_2D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_Projection_3D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_Propagation.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_QuadranglePreference.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_Quadrangle_2D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_QuadraticMesh.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_RadialPrism_3D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_Regular_1D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_SegmentAroundVertex_0D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_SegmentLengthAroundVertex.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_QuadToTriaAdaptor.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_TrianglePreference.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_UseExisting_1D2D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_CompositeHexa_3D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_MaxLength.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_StartEndLength.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_FixedPoints1D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_LayerDistribution2D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_NumberOfLayers2D.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_QuadrangleParams.cpp \
$(srcdir)/$(smeshdir)src/StdMeshers/StdMeshers_RadialQuadrangle_1D2D.cpp \
$(srcdir)/$(smeshdir)src/MEFISTO2/aptrte.cpp \
$(srcdir)/$(smeshdir)src/MEFISTO2/trte.f

libStdMeshers_la_LIBADD = $(FCLIBS) -lSMESH -lSMDS -lSMESHDS
libStdMeshers_la_DEPENDENCIES = libSMESH.la libSMDS.la libSMESHDS.la
if STDMESHERS64BIT
libStdMeshers_la_CXXFLAGS = $(AM_CXXFLAGS) -DPCLINUX64
endif

# set NETGENPlugin sources
libNETGENPlugin_la_SOURCES = $(srcdir)/$(smeshdir)src/NETGENPlugin/NETGENPlugin_Hypothesis.cpp \
$(srcdir)/$(smeshdir)src/NETGENPlugin/NETGENPlugin_Hypothesis_2D.cpp \
$(srcdir)/$(smeshdir)src/NETGENPlugin/NETGENPlugin_Mesher.cpp \
$(srcdir)/$(smeshdir)src/NETGENPlugin/NETGENPlugin_NETGEN_2D.cpp \
$(srcdir)/$(smeshdir)src/NETGENPlugin/NETGENPlugin_NETGEN_2D3D.cpp \
$(srcdir)/$(smeshdir)src/NETGENPlugin/NETGENPlugin_NETGEN_2D_ONLY.cpp \
$(srcdir)/$(smeshdir)src/NETGENPlugin/NETGENPlugin_SimpleHypothesis_2D.cpp \
$(srcdir)/$(smeshdir)src/NETGENPlugin/NETGENPlugin_SimpleHypothesis_3D.cpp \
$(srcdir)/$(smeshdir)src/NETGENPlugin/NETGENPlugin_NETGEN_3D.cpp

# include files but do not install otherwise use include_HEADERS
#includedir = @includedir@/salomesmesh
noinst_HEADERS = \
$(srcdir)/$(smeshdir)inc/aptrte.h \
$(srcdir)/$(smeshdir)inc/DriverDAT_R_SMDS_Mesh.h \
$(srcdir)/$(smeshdir)inc/DriverDAT_R_SMESHDS_Document.h \
$(srcdir)/$(smeshdir)inc/DriverDAT_R_SMESHDS_Mesh.h \
$(srcdir)/$(smeshdir)inc/DriverDAT_W_SMDS_Mesh.h \
$(srcdir)/$(smeshdir)inc/DriverDAT_W_SMESHDS_Document.h \
$(srcdir)/$(smeshdir)inc/DriverDAT_W_SMESHDS_Mesh.h \
$(srcdir)/$(smeshdir)inc/DriverSTL_R_SMDS_Mesh.h \
$(srcdir)/$(smeshdir)inc/DriverSTL_W_SMDS_Mesh.h \
$(srcdir)/$(smeshdir)inc/DriverUNV_R_SMDS_Mesh.h \
$(srcdir)/$(smeshdir)inc/DriverUNV_R_SMESHDS_Document.h \
$(srcdir)/$(smeshdir)inc/DriverUNV_R_SMESHDS_Mesh.h \
$(srcdir)/$(smeshdir)inc/DriverUNV_W_SMDS_Mesh.h \
$(srcdir)/$(smeshdir)inc/DriverUNV_W_SMESHDS_Document.h \
$(srcdir)/$(smeshdir)inc/DriverUNV_W_SMESHDS_Mesh.h \
$(srcdir)/$(smeshdir)inc/Driver_Document.h \
$(srcdir)/$(smeshdir)inc/Driver_Mesh.h \
$(srcdir)/$(smeshdir)inc/Driver_SMDS_Mesh.h \
$(srcdir)/$(smeshdir)inc/Driver_SMESHDS_Mesh.h \
$(srcdir)/$(smeshdir)inc/NETGENPlugin_Hypothesis.hxx \
$(srcdir)/$(smeshdir)inc/NETGENPlugin_Hypothesis_2D.hxx \
$(srcdir)/$(smeshdir)inc/NETGENPlugin_Mesher.hxx \
$(srcdir)/$(smeshdir)inc/NETGENPlugin_NETGEN_2D.hxx \
$(srcdir)/$(smeshdir)inc/NETGENPlugin_NETGEN_2D3D.hxx \
$(srcdir)/$(smeshdir)inc/NETGENPlugin_NETGEN_3D.hxx \
$(srcdir)/$(smeshdir)inc/Rn.h \
$(srcdir)/$(smeshdir)inc/SMDSAbs_ElementType.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_EdgePosition.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_ElemIterator.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_FaceOfEdges.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_FaceOfNodes.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_FacePosition.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_Iterator.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_IteratorOfElements.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_Mesh.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_MeshEdge.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_MeshElement.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_MeshElementIDFactory.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_MeshFace.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_MeshGroup.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_MeshIDFactory.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_MeshNode.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_MeshObject.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_MeshVolume.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_PolygonalFaceOfNodes.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_PolyhedralVolumeOfNodes.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_Position.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_QuadraticEdge.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_QuadraticFaceOfNodes.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_QuadraticVolumeOfNodes.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_SetIterator.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_SpacePosition.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_TypeOfPosition.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_VertexPosition.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_VolumeOfFaces.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_VolumeOfNodes.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_VolumeTool.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_Mesh0DElement.hxx \
$(srcdir)/$(smeshdir)inc/SMESHDS_Command.hxx \
$(srcdir)/$(smeshdir)inc/SMESHDS_CommandType.hxx \
$(srcdir)/$(smeshdir)inc/SMESHDS_Document.hxx \
$(srcdir)/$(smeshdir)inc/SMESHDS_Group.hxx \
$(srcdir)/$(smeshdir)inc/SMESHDS_GroupBase.hxx \
$(srcdir)/$(smeshdir)inc/SMESHDS_GroupOnGeom.hxx \
$(srcdir)/$(smeshdir)inc/SMESHDS_Hypothesis.hxx \
$(srcdir)/$(smeshdir)inc/SMESHDS_Mesh.hxx \
$(srcdir)/$(smeshdir)inc/SMESHDS_Script.hxx \
$(srcdir)/$(smeshdir)inc/SMESHDS_SubMesh.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_0D_Algo.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_1D_Algo.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_2D_Algo.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_3D_Algo.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_Algo.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_Array1.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_Array2.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_Block.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_Comment.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_ComputeError.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_Controls.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_ControlsDef.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_DataMapOfElemPtrSequenceOfElemPtr.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_DefineArray2.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_DefineIndexedMap.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_ExceptHandlers.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_Exception.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_Gen.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_Group.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_HypoFilter.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_Hypothesis.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_IndexedDataMapOfShapeIndexedMapOfShape.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_IndexedMap.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_Mesh.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_MeshEditor.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_MesherHelper.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_MeshVSLink.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_MeshVSLink.ixx \
$(srcdir)/$(smeshdir)inc/SMESH_MeshVSLink.jxx \
$(srcdir)/$(smeshdir)inc/Handle_SMESH_MeshVSLink.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_Octree.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_OctreeNode.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_Pattern.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_SequenceOfElemPtr.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_SequenceOfNode.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_subMesh.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_subMeshEventListener.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_Arithmetic1D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_AutomaticLength.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_CompositeSegment_1D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_Deflection1D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_Distribution.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_FaceSide.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_Hexa_3D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_LayerDistribution.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_LengthFromEdges.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_LocalLength.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_MaxElementArea.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_MaxElementVolume.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_MEFISTO_2D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_NotConformAllowed.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_NumberOfLayers.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_NumberOfSegments.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_Penta_3D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_Prism_3D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_ProjectionSource1D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_ProjectionSource2D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_ProjectionSource3D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_ProjectionUtils.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_Projection_1D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_Projection_2D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_Projection_3D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_Propagation.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_QuadranglePreference.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_Quadrangle_2D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_QuadraticMesh.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_RadialPrism_3D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_Regular_1D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_SegmentAroundVertex_0D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_SegmentLengthAroundVertex.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_StartEndLength.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_FixedPoints1D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_LayerDistribution2D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_NumberOfLayers2D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_QuadrangleParams.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_RadialQuadrangle_1D2D.hxx \
$(srcdir)/$(smeshdir)inc/UNV2411_Structure.hxx \
$(srcdir)/$(smeshdir)inc/UNV2412_Structure.hxx \
$(srcdir)/$(smeshdir)inc/UNV2417_Structure.hxx \
$(srcdir)/$(smeshdir)inc/UNV_Utilities.hxx \
$(srcdir)/$(smeshdir)inc/NETGENPlugin_Defs.hxx \
$(srcdir)/$(smeshdir)inc/NETGENPlugin_NETGEN_2D_ONLY.hxx \
$(srcdir)/$(smeshdir)inc/NETGENPlugin_SimpleHypothesis_2D.hxx \
$(srcdir)/$(smeshdir)inc/NETGENPlugin_SimpleHypothesis_3D.hxx \
$(srcdir)/$(smeshdir)inc/SMDS_MeshInfo.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_DriverDAT.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_DriverSTL.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_DriverUNV.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_SMDS.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_SMESH.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_SMESHDS.hxx \
$(srcdir)/$(smeshdir)inc/SMESH_StdMeshers.hxx \
$(srcdir)/$(smeshdir)inc/SMESHDS_DataMapOfShape.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_QuadToTriaAdaptor.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_TrianglePreference.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_UseExisting_1D2D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_CompositeHexa_3D.hxx \
$(srcdir)/$(smeshdir)inc/StdMeshers_MaxLength.hxx \
$(srcdir)/$(smeshdir)inc/OpUtil.hxx \
$(srcdir)/$(smeshdir)inc/Utils_ExceptHandlers.hxx \
$(srcdir)/$(smeshdir)inc/Utils_SALOME_Exception.hxx \
$(srcdir)/$(smeshdir)inc/utilities.h

