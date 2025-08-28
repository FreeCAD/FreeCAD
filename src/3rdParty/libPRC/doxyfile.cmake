# Doxyfile 1.7.2

#---------------------------------------------------------------------------
# Project related configuration options
#---------------------------------------------------------------------------
DOXYFILE_ENCODING      = UTF-8
PROJECT_NAME           = Jag3D
PROJECT_NUMBER         = "Jag3D ${JAG3D_VERSION}"
OUTPUT_DIRECTORY       = "${PROJECT_BINARY_DIR}/doc"
CREATE_SUBDIRS         = NO
OUTPUT_LANGUAGE        = English
BRIEF_MEMBER_DESC      = YES
REPEAT_BRIEF           = YES
ABBREVIATE_BRIEF       = "The $name class" \
                         "The $name widget" \
                         "The $name file" \
                         is \
                         provides \
                         specifies \
                         contains \
                         represents \
                         a \
                         an \
                         the
ALWAYS_DETAILED_SEC    = NO
INLINE_INHERITED_MEMB  = NO
FULL_PATH_NAMES        = YES
STRIP_FROM_PATH        = "${PROJECT_SOURCE_DIR}/"
STRIP_FROM_INC_PATH    = 
SHORT_NAMES            = NO
JAVADOC_AUTOBRIEF      = NO
QT_AUTOBRIEF           = NO
MULTILINE_CPP_IS_BRIEF = NO
INHERIT_DOCS           = YES
SEPARATE_MEMBER_PAGES  = NO
TAB_SIZE               = 8

ALIASES                = gl{1}="\note See OpenGL specification \1"
    ALIASES           += glinline{1}="See OpenGL specification \1"
    ALIASES           += glshort{1}="OpenGL specification \1"
    ALIASES           += glparam{2}="\param \1 See OpenGL specification \2"

    ALIASES           += "specBegin=<hr><h2>Specification</h2>\ref JagSpecification \"Specification Index\"\xrefitem jag3dspec \"Specification Table\" \"JAG Specification Table\"" 
    ALIASES           += specTableBegin="<table cellspacing=0 cellpadding=4>"
    ALIASES           += specLog{1}="<tr valign=top bgcolor=#dddddd> <td><b>Log Handle</b></td>    <td>\1</td></tr>"
    ALIASES           += specLogBase{1}="specLog{ See \1}"
    ALIASES           += specThread{1}="<tr valign=top bgcolor=#bbbbbb> <td><b>Thread Safety</b></td>    <td>\1 <small>(\ref ThreadDefinitions")</small></td></tr>"
    ALIASES           += specThreadBase{1}="<tr valign=top bgcolor=#bbbbbb> <td><b>Thread Safety</b></td>    <td>See \1</td></tr>"
    ALIASES           += specGL{1}="<tr valign=top bgcolor=#dddddd>     <td><b>OpenGL Footprint</b></td> <td>\1</td></tr>"
    ALIASES           += specGLBase{1}="\specGL{See \1}"
    ALIASES           += specDepend{1}="<tr valign=top bgcolor=#bbbbbb> <td><b>Dependencies</b></td>     <td>\1</td></tr>"
    ALIASES           += specDependBase{1}="\specDepend{See \1}"
    ALIASES           += specUsage{1}="<tr valign=top bgcolor=#dddddd>  <td><b>Intended Usage</b></td>   <td>\1</td></tr>"
    ALIASES           += specUsageBase{1}="\specUsage{See \1}"
    ALIASES           += specViolations{1}="<tr valign=top bgcolor=#bbbbbb> <td><b>Known Violations</b></td> <td>\1</td></tr>"
    ALIASES           += specViolationsBase{1}="\specViolations{See \1}"
    ALIASES           += specTableEnd="</table>"
    ALIASES           += specEnd="<hr>"

    ALIASES           += specFuncBegin="<hr><h4>Function Specification Requirements</h4>"
    ALIASES           += specFuncEnd="<hr>"

    ALIASES           += specIssue{1}="<dl class="note"><dt><b>Specification ISSUE:</b></dt><dd>\1</dd></dl>"

    ALIASES           += "envvar=\xrefitem jag3denvvars \"Environment Variables\" \"Environment Variables\""
    ALIASES           += "logname=\xrefitem jag3dlognames \"Log Names\" \"Log Names\""

OPTIMIZE_OUTPUT_FOR_C  = NO
OPTIMIZE_OUTPUT_JAVA   = NO
OPTIMIZE_FOR_FORTRAN   = NO
OPTIMIZE_OUTPUT_VHDL   = NO
EXTENSION_MAPPING      = 
BUILTIN_STL_SUPPORT    = NO
CPP_CLI_SUPPORT        = NO
SIP_SUPPORT            = NO
IDL_PROPERTY_SUPPORT   = YES
DISTRIBUTE_GROUP_DOC   = NO
SUBGROUPING            = YES
TYPEDEF_HIDES_STRUCT   = NO
SYMBOL_CACHE_SIZE      = 0

#---------------------------------------------------------------------------
# Build related configuration options
#---------------------------------------------------------------------------
EXTRACT_ALL            = NO
EXTRACT_PRIVATE        = NO
EXTRACT_STATIC         = NO
EXTRACT_LOCAL_CLASSES  = YES
EXTRACT_LOCAL_METHODS  = NO
EXTRACT_ANON_NSPACES   = NO
HIDE_UNDOC_MEMBERS     = NO
HIDE_UNDOC_CLASSES     = NO
HIDE_FRIEND_COMPOUNDS  = NO
HIDE_IN_BODY_DOCS      = NO
INTERNAL_DOCS          = NO
CASE_SENSE_NAMES       = NO
HIDE_SCOPE_NAMES       = NO
SHOW_INCLUDE_FILES     = YES
FORCE_LOCAL_INCLUDES   = NO
INLINE_INFO            = YES
SORT_MEMBER_DOCS       = NO
SORT_BRIEF_DOCS        = NO
SORT_MEMBERS_CTORS_1ST = NO
SORT_GROUP_NAMES       = NO
SORT_BY_SCOPE_NAME     = NO
GENERATE_TODOLIST      = YES
GENERATE_TESTLIST      = YES
GENERATE_BUGLIST       = YES
GENERATE_DEPRECATEDLIST= YES
ENABLED_SECTIONS       = 
MAX_INITIALIZER_LINES  = 30
SHOW_USED_FILES        = YES
SHOW_DIRECTORIES       = NO
SHOW_FILES             = YES
SHOW_NAMESPACES        = YES
FILE_VERSION_FILTER    = 
LAYOUT_FILE            = "${PROJECT_SOURCE_DIR}/doc/DoxygenLayout.xml"

#---------------------------------------------------------------------------
# configuration options related to warning and progress messages
#---------------------------------------------------------------------------
QUIET                  = NO
WARNINGS               = YES
WARN_IF_UNDOCUMENTED   = YES
WARN_IF_DOC_ERROR      = YES
WARN_NO_PARAMDOC       = NO
WARN_FORMAT            = "$file:$line: $text"
WARN_LOGFILE           = 

#---------------------------------------------------------------------------
# configuration options related to the input files
#---------------------------------------------------------------------------
INPUT                  = "${PROJECT_SOURCE_DIR}/src/jag/base" \
                         "${PROJECT_SOURCE_DIR}/src/jag/disk" \
                         "${PROJECT_SOURCE_DIR}/src/jag/draw" \
                         "${PROJECT_SOURCE_DIR}/src/jag/mx" \
                         "${PROJECT_SOURCE_DIR}/src/jag/sg" \
                         "${PROJECT_SOURCE_DIR}/src/jag/util" \
                         "${PROJECT_SOURCE_DIR}/src/plugins" \
                         "${PROJECT_SOURCE_DIR}/src/apps" \
                         "${PROJECT_SOURCE_DIR}/src/examples" \
                         "${PROJECT_SOURCE_DIR}/doc"
INPUT_ENCODING         = UTF-8
FILE_PATTERNS          = *.cpp *.h *.txt
RECURSIVE              = YES
EXCLUDE                = */.svn
EXCLUDE_SYMLINKS       = NO
EXCLUDE_PATTERNS       = */.svn/*
EXCLUDE_SYMBOLS        = 
EXAMPLE_PATH           = 
EXAMPLE_PATTERNS       = *
EXAMPLE_RECURSIVE      = YES
IMAGE_PATH             = 
INPUT_FILTER           = 
FILTER_PATTERNS        = 
FILTER_SOURCE_FILES    = NO

#---------------------------------------------------------------------------
# configuration options related to source browsing
#---------------------------------------------------------------------------
SOURCE_BROWSER         = NO
INLINE_SOURCES         = NO
STRIP_CODE_COMMENTS    = YES
REFERENCED_BY_RELATION = NO
REFERENCES_RELATION    = NO
REFERENCES_LINK_SOURCE = YES
USE_HTAGS              = NO
VERBATIM_HEADERS       = YES

#---------------------------------------------------------------------------
# configuration options related to the alphabetical class index
#---------------------------------------------------------------------------
ALPHABETICAL_INDEX     = YES
COLS_IN_ALPHA_INDEX    = 5
IGNORE_PREFIX          = 

#---------------------------------------------------------------------------
# configuration options related to the HTML output
#---------------------------------------------------------------------------
GENERATE_HTML          = YES
HTML_OUTPUT            = html
HTML_FILE_EXTENSION    = .html
HTML_HEADER            = 
HTML_FOOTER            = 
HTML_STYLESHEET        = 
HTML_COLORSTYLE_HUE    = 220
HTML_COLORSTYLE_SAT    = 100
HTML_COLORSTYLE_GAMMA  = 80
HTML_TIMESTAMP         = YES
HTML_ALIGN_MEMBERS     = YES
HTML_DYNAMIC_SECTIONS  = NO
GENERATE_DOCSET        = NO
DOCSET_FEEDNAME        = "Doxygen generated docs"
DOCSET_BUNDLE_ID       = org.doxygen.Project
DOCSET_PUBLISHER_ID    = org.doxygen.Publisher
DOCSET_PUBLISHER_NAME  = Publisher
GENERATE_HTMLHELP      = NO
CHM_FILE               = 
HHC_LOCATION           = 
GENERATE_CHI           = NO
CHM_INDEX_ENCODING     = 
BINARY_TOC             = NO
TOC_EXPAND             = NO
GENERATE_QHP           = NO
QCH_FILE               = 
QHP_NAMESPACE          = org.doxygen.Project
QHP_VIRTUAL_FOLDER     = doc
QHP_CUST_FILTER_NAME   = 
QHP_CUST_FILTER_ATTRS  = 
QHP_SECT_FILTER_ATTRS  = 
QHG_LOCATION           = 
GENERATE_ECLIPSEHELP   = NO
ECLIPSE_DOC_ID         = org.doxygen.Project
DISABLE_INDEX          = NO
ENUM_VALUES_PER_LINE   = 4
GENERATE_TREEVIEW      = NO
USE_INLINE_TREES       = NO
TREEVIEW_WIDTH         = 250
EXT_LINKS_IN_WINDOW    = NO
FORMULA_FONTSIZE       = 10
FORMULA_TRANSPARENT    = YES
USE_MATHJAX            = NO
MATHJAX_RELPATH        = http://www.mathjax.org/mathjax
SEARCHENGINE           = YES
SERVER_BASED_SEARCH    = NO

#---------------------------------------------------------------------------
# configuration options related to the LaTeX output
#---------------------------------------------------------------------------
GENERATE_LATEX         = NO
LATEX_OUTPUT           = latex
LATEX_CMD_NAME         = latex
MAKEINDEX_CMD_NAME     = makeindex
COMPACT_LATEX          = NO
PAPER_TYPE             = a4
EXTRA_PACKAGES         = 
LATEX_HEADER           = 
PDF_HYPERLINKS         = YES
USE_PDFLATEX           = YES
LATEX_BATCHMODE        = NO
LATEX_HIDE_INDICES     = NO
LATEX_SOURCE_CODE      = NO

#---------------------------------------------------------------------------
# configuration options related to the RTF output
#---------------------------------------------------------------------------
GENERATE_RTF           = NO
RTF_OUTPUT             = rtf
COMPACT_RTF            = NO
RTF_HYPERLINKS         = NO
RTF_STYLESHEET_FILE    = 
RTF_EXTENSIONS_FILE    = 

#---------------------------------------------------------------------------
# configuration options related to the man page output
#---------------------------------------------------------------------------
GENERATE_MAN           = NO
MAN_OUTPUT             = man
MAN_EXTENSION          = .3
MAN_LINKS              = NO

#---------------------------------------------------------------------------
# configuration options related to the XML output
#---------------------------------------------------------------------------
GENERATE_XML           = NO 
XML_OUTPUT             = xml
XML_SCHEMA             = 
XML_DTD                = 
XML_PROGRAMLISTING     = YES

#---------------------------------------------------------------------------
# configuration options for the AutoGen Definitions output
#---------------------------------------------------------------------------
GENERATE_AUTOGEN_DEF   = NO

#---------------------------------------------------------------------------
# configuration options related to the Perl module output
#---------------------------------------------------------------------------
GENERATE_PERLMOD       = NO
PERLMOD_LATEX          = NO
PERLMOD_PRETTY         = YES
PERLMOD_MAKEVAR_PREFIX = 

#---------------------------------------------------------------------------
# Configuration options related to the preprocessor
#---------------------------------------------------------------------------
ENABLE_PREPROCESSING   = YES
MACRO_EXPANSION        = NO
EXPAND_ONLY_PREDEF     = NO
SEARCH_INCLUDES        = YES
INCLUDE_PATH           = 
INCLUDE_FILE_PATTERNS  = 
PREDEFINED             = 
EXPAND_AS_DEFINED      = 
SKIP_FUNCTION_MACROS   = YES

#---------------------------------------------------------------------------
# Configuration::additions related to external references
#---------------------------------------------------------------------------
TAGFILES               = 
GENERATE_TAGFILE       = 
ALLEXTERNALS           = NO
EXTERNAL_GROUPS        = YES
PERL_PATH              = /usr/bin/perl

#---------------------------------------------------------------------------
# Configuration options related to the dot tool
#---------------------------------------------------------------------------

DOT_PATH               = "${DOXYGEN_DOT_PATH}"
HAVE_DOT               = ${HAVE_DOT}
CLASS_DIAGRAMS         = YES 
MSCGEN_PATH            = 
HIDE_UNDOC_RELATIONS   = YES
DOT_NUM_THREADS        = 0
DOT_FONTNAME           = FreeSans.ttf
DOT_FONTSIZE           = 10
DOT_FONTPATH           = 
CLASS_GRAPH            = YES
COLLABORATION_GRAPH    = YES 
GROUP_GRAPHS           = YES
UML_LOOK               = NO
TEMPLATE_RELATIONS     = NO
INCLUDE_GRAPH          = NO
INCLUDED_BY_GRAPH      = NO
CALL_GRAPH             = NO
CALLER_GRAPH           = NO
GRAPHICAL_HIERARCHY    = NO
DIRECTORY_GRAPH        = YES
DOT_IMAGE_FORMAT       = png
DOTFILE_DIRS           = 
MSCFILE_DIRS           = 
DOT_GRAPH_MAX_NODES    = 50
MAX_DOT_GRAPH_DEPTH    = 0
DOT_TRANSPARENT        = NO
DOT_MULTI_TARGETS      = NO
GENERATE_LEGEND        = YES
DOT_CLEANUP            = YES
