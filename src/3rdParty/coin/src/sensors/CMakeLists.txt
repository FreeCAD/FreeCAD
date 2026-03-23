# source files
set(COIN_SENSORS_FILES
	SoAlarmSensor.cpp
	SoDataSensor.cpp
	SoDelayQueueSensor.cpp
	SoFieldSensor.cpp
	SoIdleSensor.cpp
	SoNodeSensor.cpp
	SoOneShotSensor.cpp
	SoPathSensor.cpp
	SoSensor.cpp
	SoSensorManager.cpp
	SoTimerQueueSensor.cpp
	SoTimerSensor.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_SENSORS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} sensors/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(sensors OBJECT ${COIN_SENSORS_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_SENSORS_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/sensors/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
