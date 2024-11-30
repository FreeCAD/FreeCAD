
#cmake_minimum_required(VERSION 3.23)
#project(FCQuickLook)

add_library(
  QuicklookFCStd
  SHARED
  GeneratePreviewForURL.m
  GenerateThumbnailForURL.m
  main.c
)

set_target_properties(
  QuicklookFCStd
  PROPERTIES
  FRAMEWORK TRUE
  MACOSX_FRAMEWORK_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/QuicklookFCStd.qlgenerator/Contents/Info.plist"
  #SUFFIX .qlgenerator
)

target_link_libraries(
  QuicklookFCStd
  "-framework AppKit"
  "-framework ApplicationServices"
  "-framework CoreData"
  "-framework CoreFoundation"
  "-framework CoreServices"
  "-framework Foundation"
  "-framework QuickLook"
)

set_target_properties(
  QuicklookFCStd
  PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks"
)
