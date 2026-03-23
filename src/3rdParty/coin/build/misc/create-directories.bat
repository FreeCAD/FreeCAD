@echo off

REM **********************************************************************
REM * Create all the directories for the installed files

pushd %COINDIR%

if exist bin\*.* goto binexists
echo mkdir %COINDIR%\bin
mkdir bin
:binexists

if exist data\*.* goto dataexists
echo mkdir %COINDIR%\data
mkdir data
:dataexists

if exist include\*.* goto includeexists
echo mkdir %COINDIR%\include
mkdir include
:includeexists

if exist lib\*.* goto libexists
echo mkdir %COINDIR%\lib
mkdir lib
:libexists

pushd data

if exist draggerDefaults\*.* goto draggerdefaultsexists
echo mkdir %COINDIR%\data\draggerDefaults
mkdir draggerDefaults
:draggerdefaultsexists

if exist shaders\*.* goto shadersexists
echo mkdir %COINDIR%\data\shaders
mkdir shaders
:shadersexists

if exist scxml\*.* goto scxmlexists
echo mkdir %COINDIR%\data\scxml
mkdir scxml
:scxmlexists

popd

pushd data\shaders

if exist lights\*.* goto lightsexists
echo mkdir %COINDIR%\data\shaders\lights
mkdir lights
:lightsexists

if exist vsm\*.* goto vsmexists
echo mkdir %COINDIR%\data\shaders\vsm
mkdir vsm
:vsmexists

popd

pushd data\scxml

if exist navigation\*.* goto scxmlnavigationexists
echo mkdir %COINDIR%\data\scxml\navigation
mkdir navigation
:scxmlnavigationexists

popd

pushd include

if exist Inventor\*.* goto inventorexists
echo mkdir %COINDIR%\include\Inventor
mkdir Inventor
:inventorexists

popd

pushd include\Inventor

if exist C\*.* goto cexists
echo mkdir %COINDIR%\include\Inventor\C
mkdir C
:cexists

if exist MPEG\*.* goto mpegexists
echo mkdir %COINDIR%\include\Inventor\MPEG
mkdir MPEG
:mpegexists

if exist VRMLnodes\*.* goto vrmlnodesexists
echo mkdir %COINDIR%\include\Inventor\VRMLnodes
mkdir VRMLnodes
:vrmlnodesexists

if exist actions\*.* goto actionsexists
echo mkdir %COINDIR%\include\Inventor\actions
mkdir actions
:actionsexists

if exist annex\*.* goto annexexists
echo mkdir %COINDIR%\include\Inventor\annex
mkdir annex
:annexexists

if exist bundles\*.* goto bundlesexists
echo mkdir %COINDIR%\include\Inventor\bundles
mkdir bundles
:bundlesexists

if exist caches\*.* goto cachesexists
echo mkdir %COINDIR%\include\Inventor\caches
mkdir caches
:cachesexists

if exist collision\*.* goto collisionexists
echo mkdir %COINDIR%\include\Inventor\collision
mkdir collision
:collisionexists

if exist details\*.* goto detailsexists
echo mkdir %COINDIR%\include\Inventor\details
mkdir details
:detailsexists

if exist draggers\*.* goto draggersexists
echo mkdir %COINDIR%\include\Inventor\draggers
mkdir draggers
:draggersexists

if exist elements\*.* goto elementsexists
echo mkdir %COINDIR%\include\Inventor\elements
mkdir elements
:elementsexists

if exist engines\*.* goto enginesexists
echo mkdir %COINDIR%\include\Inventor\engines
mkdir engines
:enginesexists

if exist errors\*.* goto errorsexists
echo mkdir %COINDIR%\include\Inventor\errors
mkdir errors
:errorsexists

if exist events\*.* goto eventsexists
echo mkdir %COINDIR%\include\Inventor\events
mkdir events
:eventsexists

if exist fields\*.* goto fieldsexists
echo mkdir %COINDIR%\include\Inventor\fields
mkdir fields
:fieldsexists

if exist lists\*.* goto listsexists
echo mkdir %COINDIR%\include\Inventor\lists
mkdir lists
:listsexists

if exist lock\*.* goto lockexists
echo mkdir %COINDIR%\include\Inventor\lock
mkdir lock
:lockexists

if exist manips\*.* goto manipsexists
echo mkdir %COINDIR%\include\Inventor\manips
mkdir manips
:manipsexists

if exist misc\*.* goto miscexists
echo mkdir %COINDIR%\include\Inventor\misc
mkdir misc
:miscexists

if exist navigation\*.* goto navigationexists
echo mkdir %COINDIR%\include\Inventor\navigation
mkdir navigation
:navigationexists

if exist nodekits\*.* goto nodekitsexists
echo mkdir %COINDIR%\include\Inventor\nodekits
mkdir nodekits
:nodekitsexists

if exist nodes\*.* goto nodesexists
echo mkdir %COINDIR%\include\Inventor\nodes
mkdir nodes
:nodesexists

if exist projectors\*.* goto projectorsexists
echo mkdir %COINDIR%\include\Inventor\projectors
mkdir projectors
:projectorsexists

if exist scxml\*.* goto scxmlexists
echo mkdir %COINDIR%\include\Inventor\scxml
mkdir scxml
:scxmlexists

if exist sensors\*.* goto sensorsexists
echo mkdir %COINDIR%\include\Inventor\sensors
mkdir sensors
:sensorsexists

if exist system\*.* goto systemexists
echo mkdir %COINDIR%\include\Inventor\system
mkdir system
:systemexists

if exist threads\*.* goto threadsexists
echo mkdir %COINDIR%\include\Inventor\threads
mkdir threads
:threadsexists

if exist tools\*.* goto toolsexists
echo mkdir %COINDIR%\include\Inventor\tools
mkdir tools
:toolsexists

if exist scxml\*.* goto scxmlexists
echo mkdir %COINDIR%\include\Inventor\scxml
mkdir scxml
:scxmlexists

popd

pushd include\Inventor\C

if exist XML\*.* goto cxmlexists
echo mkdir %COINDIR%\include\Inventor\C\XML
mkdir XML
:cxmlexists

if exist base\*.* goto cbaseexists
echo mkdir %COINDIR%\include\Inventor\C\base
mkdir base
:cbaseexists

if exist errors\*.* goto cerrorsexists
echo mkdir %COINDIR%\include\Inventor\C\errors
mkdir errors
:cerrorsexists

if exist glue\*.* goto cglueexists
echo mkdir %COINDIR%\include\Inventor\C\glue
mkdir glue
:cglueexists

if exist threads\*.* goto cthreadsexists
echo mkdir %COINDIR%\include\Inventor\C\threads
mkdir threads
:cthreadsexists

popd

pushd include\Inventor\annex

if exist HardCopy\*.* goto hardcopyexists
echo mkdir %COINDIR%\include\Inventor\annex\HardCopy
mkdir HardCopy
:hardcopyexists

if exist ForeignFiles\*.* goto foreignfilesexists
echo mkdir %COINDIR%\include\Inventor\annex\ForeignFiles
mkdir ForeignFiles
:foreignfilesexists

if exist FXViz\*.* goto fxvizexists
echo mkdir %COINDIR%\include\Inventor\annex\FXViz
mkdir FXViz
:fxvizexists

if exist Profiler\*.* goto profilerexists
echo mkdir %COINDIR%\include\Inventor\annex\Profiler
mkdir Profiler
:profilerexists

popd

pushd include\Inventor\annex\FXViz

if exist nodes\*.* goto fxviznodesexists
echo mkdir %COINDIR%\include\Inventor\annex\FXViz\nodes
mkdir nodes
:fxviznodesexists

if exist elements\*.* goto fxvizelementsexists
echo mkdir %COINDIR%\include\Inventor\annex\FXViz\elements
mkdir elements
:fxvizelementsexists

popd

pushd include\Inventor\annex\Profiler

if exist nodes\*.* goto pnodesexists
echo mkdir %COINDIR%\include\Inventor\annex\Profiler\nodes
mkdir nodes
:pnodesexists

if exist elements\*.* goto pelementsexists
echo mkdir %COINDIR%\include\Inventor\annex\Profiler\elements
mkdir elements
:pelementsexists

if exist nodekits\*.* goto pnodekitsexists
echo mkdir %COINDIR%\include\Inventor\annex\Profiler\nodekits
mkdir nodekits
:pnodekitsexists

if exist engines\*.* goto penginesexists
echo mkdir %COINDIR%\include\Inventor\annex\Profiler\engines
mkdir engines
:penginesexists

if exist utils\*.* goto putilsexists
echo mkdir %COINDIR%\include\Inventor\annex\Profiler\utils
mkdir utils
:putilsexists

popd

popd

