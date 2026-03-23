# source files
set(COIN_SCXML_FILES
	SbStringConvert.cpp
	ScXML.cpp
	ScXMLP.cpp
	ScXMLObject.cpp
	ScXMLEvent.cpp
	ScXMLEventTarget.cpp
	ScXMLStateMachine.cpp
	ScXMLDocument.cpp
	ScXMLElt.cpp
	ScXMLScxmlElt.cpp
	ScXMLInitialElt.cpp
	ScXMLAbstractStateElt.cpp
	ScXMLStateElt.cpp
	ScXMLParallelElt.cpp
	ScXMLFinalElt.cpp
	ScXMLOnEntryElt.cpp
	ScXMLOnExitElt.cpp
	ScXMLTransitionElt.cpp
	ScXMLHistoryElt.cpp
	ScXMLExecutableElt.cpp
	ScXMLEventElt.cpp
	ScXMLIfElt.cpp
	ScXMLElseIfElt.cpp
	ScXMLElseElt.cpp
	ScXMLLogElt.cpp
	ScXMLDataModelElt.cpp
	ScXMLDataElt.cpp
	ScXMLAssignElt.cpp
	ScXMLValidateElt.cpp
	ScXMLSendElt.cpp
	ScXMLInvokeElt.cpp
	ScXMLParamElt.cpp
	ScXMLFinalizeElt.cpp
	ScXMLContentElt.cpp
	ScXMLAnchorElt.cpp
	ScXMLScriptElt.cpp
	ScXMLEvaluator.cpp
	ScXMLMinimumEvaluator.cpp
	eval-minimum.cpp
	eval-minimum-tab.cpp
	ScXMLXPathEvaluator.cpp
	ScXMLECMAScriptEvaluator.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_SCXML_INTERNAL_FILES
	eval-minimum.h
	eval-minimum.cpp
	eval-minimum-tab.cpp
	SbStringConvert.h
	SbStringConvert.cpp
	ScXMLP.h
	ScXMLP.cpp
	ScXMLCommonP.h
	ScXMLMinimumEvaluatorP.h
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_SCXML_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} scxml/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(scxml OBJECT ${COIN_SCXML_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_SCXML_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/scxml/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_SCXML_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/scxml/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
