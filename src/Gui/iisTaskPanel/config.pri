MAJOR_VERSION = 1
VERSION       = 1.0
#SUFFIX        = "snapshot"

# Do not change this line
CONFIG += qt

# Chose one of the following two lines to configure the build

#CONFIG += release		
#CONFIG += debug		
CONFIG += debug_and_release	

# Configuration for MacOS X
macx {
        # Using gcc
        QMAKESPEC=macx-g++
        # uncoment this line if you want use xcode
        # QMAKESPEC=macx-xcode
}

CONFIG(debug, debug|release) {
        unix {
                TARGET = $$join(TARGET,,,_debug)
        }
        else: TARGET = $$join(TARGET,,,d)

	MOC_DIR = ./debug
        OBJECTS_DIR = ./debug
        UI_DIR = ./debug
        RCC_DIR = ./debug
}

CONFIG(release, debug|release) {
        MOC_DIR = ./release
        OBJECTS_DIR = ./release
        UI_DIR = ./release
        RCC_DIR = ./release
}

# Chose one of the following two lines to configure the build
#LIB_CONFIG = staticlib
LIB_CONFIG = dll

QIISTASKPANELLIB = iistaskpanel

CONFIG(debug, debug|release) {
    unix: QIISTASKPANELLIB = $$join(QIISTASKPANELLIB,,,_debug)
    else: QIISTASKPANELLIB = $$join(QIISTASKPANELLIB,,,d)
}

contains(LIB_CONFIG, staticlib) {
    DEFINES += QIIS_STATICLIB
} else {
    DEFINES += QIIS_SHAREDLIB
    win32 {
        QIISTASKPANELLIB = $$join(QIISTASKPANELLIB,,,$$MAJOR_VERSION)
    }
}

