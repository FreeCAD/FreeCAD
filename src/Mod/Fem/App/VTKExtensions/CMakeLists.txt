SET(FemVTK_SRCS
   vtkFemFrameSourceAlgorithm.h
   vtkFemFrameSourceAlgorithm.cpp
    )

target_sources(Fem PRIVATE ${FemVTK_SRCS})
