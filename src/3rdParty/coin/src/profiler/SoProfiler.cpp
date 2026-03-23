/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/


/*!
  \page coin_profiling_intro Scene Graph Profiling

  <h2>Enabling profiling in Coin</h2>

  To enable profiling in Coin, use the environment variable \ref
  COIN_PROFILER.  When profiling is enabled, Coin will gather
  profiling data during every scene graph traversal by any action.

  <h2>Enabling the default profiling display</h2>

  To get some profiling data shown on the screen, you also need to use
  the \ref COIN_PROFILER_OVERLAY environment variable.

  This will give you the default profiling graphics, which
  shows a top list of node timings categorized by node types, a
  scrolling graph of action traversal timings, and a scene graph
  navigator for closer scene graph inspection.

  <h2>Read the profiling data</h2>

  The SoProfilerStats node can be used to fetch the profiling data in the
  scene graph. If it is positioned anywhere in the scene graph, the
  fields of the node will be updated every time SoGLRenderAction is
  applied to the scene graph, with profiling data gathered from every
  traversal through the scene graph since the last SoGLRenderAction, up
  to the point where SoProfilerStats is located. Depending of how you
  wish to use the data, either attach sensors to the fields, or connect
  the fields on other coin nodes to the fields on SoProfilerStats.
*/


/**************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/annex/Profiler/SoProfiler.h>
#include "profiler/SoProfilerP.h"

#include <string>
#include <vector>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SoType.h>
#include <Inventor/actions/SoActions.h>
#include <Inventor/nodekits/SoNodeKit.h>

#include <Inventor/annex/Profiler/elements/SoProfilerElement.h>
#include <Inventor/annex/Profiler/nodes/SoProfilerStats.h>
#include <Inventor/annex/Profiler/engines/SoProfilerTopEngine.h>
#include <Inventor/annex/Profiler/utils/SoProfilingReportGenerator.h>
#ifdef HAVE_NODEKITS
#include <Inventor/annex/Profiler/nodekits/SoNodeVisualize.h>
#include <Inventor/annex/Profiler/nodekits/SoProfilerOverlayKit.h>
#include <Inventor/annex/Profiler/nodekits/SoProfilerTopKit.h>
#include <Inventor/annex/Profiler/nodekits/SoScrollingGraphKit.h>
#include <Inventor/annex/Profiler/nodekits/SoProfilerVisualizeKit.h>
#endif // HAVE_NODEKITS

#include "tidbitsp.h"
#include "misc/SoDBP.h"

// *************************************************************************

/*!
  \class SoProfiler SoProfiler.h Profiler/SoProfiler.h
  \brief Main static class for initializing the scene graph profiling subsystem.

  \ingroup coin_profiler
*/

namespace {

  namespace profiler {
    static SbBool initialized = FALSE;

    static SbBool enabled = FALSE;

    namespace rendering {
      static SbBool syncgl = FALSE;
      static float redraw_rate = -1.0f;
    };

    namespace overlay {
      static SbBool active = FALSE;
    };

    namespace console {
      static SbBool active = FALSE;
      static SbBool clear = FALSE;
      static SbBool header = FALSE;
      static int lines = 20;
      static SoProfilingReportGenerator::DataCategorization category =
        SoProfilingReportGenerator::NODES;
      static SoType actiontype = SoType::badType();
      static SbBool onstdout = FALSE;
      static SbBool onstderr = FALSE;
    };

  };

  void
  tokenize(const std::string & input, const std::string & delimiters, std::vector<std::string> & tokens, int count = -1)
  {
    std::string::size_type last_pos = 0, pos = 0;
    while (TRUE) {
      --count;
      pos = input.find_first_of(delimiters, last_pos);
      if ((pos == std::string::npos) || (count == 0)) {
        tokens.push_back(input.substr(last_pos));
        break;
      } else {
        tokens.push_back(input.substr(last_pos, pos - last_pos));
        last_pos = pos + 1;
      }
    }
  }

} // namespace


/*!
  Initializes the Coin scene graph profiling subsystem.
*/

void
SoProfiler::init(void)
{
  if (profiler::initialized) return;

  SoProfilerStats::initClass();
  SoProfilerTopEngine::initClass();

#ifdef HAVE_NODEKITS
  SoNodeKit::init();
  SoProfilerOverlayKit::initClass();
  SoProfilerVisualizeKit::initClass();
  SoProfilerTopKit::initClass();
  SoScrollingGraphKit::initClass();
  SoNodeVisualize::initClass();
#endif // HAVE_NODEKITS

  SoProfilingReportGenerator::init();

  profiler::enabled = TRUE;

  //SoProfilerP::setActionType(SoRayPickAction::getClassTypeId());
  SoProfilerP::parseCoinProfilerOverlayVariable();

  profiler::initialized = TRUE;
}

/*!
  Returns whether profiling info is shown in an overlay fashion on
  the GL canvas or not.
*/
SbBool
SoProfiler::isOverlayActive(void)
{
  return SoProfiler::isEnabled() && profiler::overlay::active;
}

/*!
  Returns whether profiling info is shown on the console or not.
*/
SbBool
SoProfiler::isConsoleActive(void)
{
  return SoProfiler::isEnabled() && profiler::console::active;
}

/*!
  Enable/disable the profiling subsystem at runtime.
*/
void
SoProfiler::enable(SbBool enable)
{
  if (!profiler::initialized) {
    assert(!"SoProfiler module not initialized");
    SoDebugError::post("SoProfiler::enable", "module not initialized");
    return;
  }
  profiler::enabled = enable;
}

/*!
  Returns whether profiling is enabled or not.
*/

SbBool
SoProfiler::isEnabled(void)
{
  return profiler::enabled;
}

SbBool
SoProfilerP::shouldContinuousRender(void)
{
  return profiler::rendering::redraw_rate != -1.0f;
}

float
SoProfilerP::getContinuousRenderDelay(void)
{
  return profiler::rendering::redraw_rate;
}

SbBool
SoProfilerP::shouldSyncGL(void)
{
  return profiler::rendering::syncgl;
}

SbBool
SoProfilerP::shouldClearConsole(void)
{
  return profiler::console::clear;
}

SbBool
SoProfilerP::shouldOutputHeaderOnConsole(void)
{
  return profiler::console::header;
}

void
SoProfilerP::setActionType(SoType actiontype)
{
#define IF_ACTION(actionname)                                   \
  if (actiontype.isDerivedFrom(actionname::getClassTypeId())) { \
    SO_ENABLE(actionname, SoProfilerElement);                   \
    profiler::console::actiontype = actiontype;                 \
  }

  IF_ACTION(SoGLRenderAction)
  else IF_ACTION(SoPickAction)
  else IF_ACTION(SoCallbackAction)
  else IF_ACTION(SoGetBoundingBoxAction)
  else IF_ACTION(SoGetMatrixAction)
  else IF_ACTION(SoGetPrimitiveCountAction)
  else IF_ACTION(SoHandleEventAction)
  else IF_ACTION(SoToVRMLAction)
  else IF_ACTION(SoAudioRenderAction)
  else IF_ACTION(SoSimplifyAction)
  else {
    SoDebugError::postInfo("SoProfilerP::setActionType",
                           "profiling action of type '%s' is not supported",
                           actiontype.getName().getString());
  }
#undef IF_ACTION
}

SoType
SoProfilerP::getActionType(void)
{
  if (profiler::console::actiontype == SoType::badType()) {
    profiler::console::actiontype = SoGLRenderAction::getClassTypeId();
  }
  return profiler::console::actiontype;
}

void
SoProfilerP::parseCoinProfilerVariable(void)
{
  // variable COIN_PROFILER
  // - on
  // - syncgl - implies on
  // - [nocaching - implies on] // todo

  const char * env = coin_getenv(SoDBP::EnvVars::COIN_PROFILER);
  if (env == NULL) return;
  std::vector<std::string> parameters;
  tokenize(env, ":", parameters);
  if ((parameters.size() == 1) &&
      (parameters[0].find_first_not_of("+-0123456789 \t") == std::string::npos)) {
    // just have a numeral value (or nothing) - old semantics
    profiler::enabled = atoi(parameters[0].data()) > 0 ? TRUE : FALSE;
  }
  else if (parameters.size() > 0) {
    std::vector<std::string>::iterator it = parameters.begin();
    while (it != parameters.end()) {
      if ((*it).compare("on") == 0) {
        profiler::enabled = TRUE;
      }
      else if ((*it).compare("off") == 0) {
        profiler::enabled = FALSE;
      }
      else if ((*it).compare("syncgl") == 0) {
        profiler::enabled = TRUE;
        profiler::rendering::syncgl = TRUE;
      }
      else {
        SoDebugError::postWarning("SoProfilerP::parseCoinProfilerVariable",
                                  "invalid token '%s'", (*it).data());
      }
      ++it;
    }
  }
}

void
SoProfilerP::parseCoinProfilerOverlayVariable(void)
{
  const char * env = coin_getenv(SoDBP::EnvVars::COIN_PROFILER_OVERLAY);
  if (env == NULL) return;
  std::vector<std::string> parameters;
  tokenize(env, ":", parameters);

  if (parameters.size() == 1 && atoi(parameters[0].data()) > 0) {
    // old behaviour, default setup
    profiler::overlay::active = TRUE;
    // SoDebugError::postInfo("SoProfiler::initialize", "default old behaviour parsing");
  }
  else if (parameters.size() > 0) {
    // SoDebugError::postInfo("SoProfiler::initialize", "new tokenized parsing");

    for (std::vector<std::string>::iterator it = parameters.begin(); it != parameters.end(); ++it) {
      if (it == parameters.begin()) {
        profiler::overlay::active = TRUE;
      }

      std::vector<std::string> param, subargs;
      tokenize(*it, "=", param, 2);
      if (param.size() > 1) {
        tokenize(param[1], ",", subargs);
      }

      // configure if the profiling system should continuously
      // schedule redraws to get more "live" data on the overlay
      if (param[0].compare("autoredraw") == 0) {
        if (param.size() == 1) {
          // no argument ->continuous redraws
          profiler::rendering::redraw_rate = 0.0f;
        } else {
          // argument decides redraw-delay-rate
          profiler::rendering::redraw_rate = static_cast<float>(atof(param[1].data()));
        }
        if (profiler::rendering::redraw_rate < 0.0f) {
          profiler::rendering::redraw_rate = -1.0f; // -1 exact means no redraws
        }
      }

      else if (param[0].compare("stdout") == 0) {
        profiler::overlay::active = FALSE;
        profiler::console::active = TRUE;
        profiler::console::onstdout = TRUE;
      }

      else if (param[0].compare("stderr") == 0) {
        profiler::overlay::active = FALSE;
        profiler::console::active = TRUE;
        profiler::console::onstderr = TRUE;
      }

      else if (param[0].compare("clear") == 0 && profiler::console::active) {
        profiler::console::clear = TRUE;
      }

      else if (param[0].compare("header") == 0 && profiler::console::active) {
        profiler::console::header = TRUE;
      }

      else if (param[0].compare("lines") == 0) {
        if (subargs.size() > 0) {
          profiler::console::lines = atoi(subargs[0].data());
          if (profiler::console::lines < 0 || profiler::console::lines > 512) {
            SoDebugError::postWarning("SoProfiler",
                                      "Number of lines out of range. Setting 20.",
                                      profiler::console::lines);
            profiler::console::lines = 20;
          }
        } else {
          SoDebugError::postWarning("SoProfiler",
                                    "'lines' takes a numeric argument.");
        }
      }

      else if (param[0].compare("action") == 0) {
        if (subargs.size() > 0) {
          SoType actiontype = SoType::fromName(subargs[0].data());
          if (actiontype.isDerivedFrom(SoAction::getClassTypeId())) {
            SoProfilerP::setActionType(actiontype);
          } else {
            SoDebugError::postWarning("SoProfiler",
                                      "Classname '%s' does not specify an action type.",
                                      subargs[0].data());
          }
        } else {
          SoDebugError::postWarning("SoProfiler",
                                    "'action' takes a classname as argument.");
        }
      }

      else if (param[0].compare("category") == 0) {
        if (subargs.size() > 0) {
          if (subargs[0].compare("nodes") == 0) {
            profiler::console::category =
              SoProfilingReportGenerator::NODES;
          } else if (subargs[0].compare("types") == 0) {
            profiler::console::category =
              SoProfilingReportGenerator::TYPES;
          } else if (subargs[0].compare("names") == 0) {
            profiler::console::category =
              SoProfilingReportGenerator::NAMES;
          } else {
            SoDebugError::postWarning("SoProfiler",
                                      "'category' argument must be nodes, types, or names - was '%s'.",
                                      subargs[0].data());
          }
        } else {
          SoDebugError::postWarning("SoProfiler",
                                    "'category' must have argument nodes, types, or names.");
        }
      }

      // configure if and how we should display toplists
      else if (param[0].compare("toplist") == 0) {
        enum TopListType { NODE_TYPE, NODE_NAME, ACTION_TYPE, INVALID } toplisttype = INVALID;
        if (subargs.size() > 0 && subargs[0].compare("nodes") == 0) {
          // top-list based on node type statistics
          toplisttype = NODE_TYPE;
        }
        else if (subargs.size() > 0 && subargs[0].compare("names") == 0) {
          // top-list based on named node statistics
          toplisttype = NODE_NAME;
        }
        else if (subargs.size() > 0 && subargs[0].compare("actions") == 0) {
          // top-list based on action type statistics
          toplisttype = ACTION_TYPE;
        }
        else if (subargs.size() > 0) {
          // default to node type
          toplisttype = INVALID;
        }
        if (toplisttype != INVALID) {
          std::vector<std::string>::iterator it = subargs.begin();
          ++it;
          while (it != subargs.end()) {
            std::vector<std::string> subarg;
            tokenize((*it).data(), "=", subarg, 2);
            if (subarg[0].compare("header") == 0) {
              // display header on toplist
            }
            else if (subarg[0].compare("lines") == 0) {
              // needs numeric argument
              if (subarg.size() == 2) {
                /*int lines = atoi(subarg[1].data());*/
              } else {
                // error: toplist.lines needs a numeric argument`
              }
            }
            else if ((subarg[0].compare("action") == 0) && (toplisttype != ACTION_TYPE)) {
              // need name of action
              if (subarg.size() == 2) {
                SoType actiontype = SoType::fromName(subarg[1].data());
                if (actiontype.isDerivedFrom(SoAction::getClassTypeId())) {
                  SoProfilerP::setActionType(actiontype);
                } else {
                  SoDebugError::postWarning("SoProfiler",
                                            "classname '%s' does not specify an action type",
                                            subarg[1].data());
                  // error - must specify action type
                }
              } else {
                // error: toplist.action needs an SoAction-derived type name
              }
            }
            else if ((subarg[0].compare("inclusive") == 0) && (toplisttype != ACTION_TYPE)) {
              // no argument
            }
            else if ((subarg[0].compare("exclusive") == 0) && (toplisttype != ACTION_TYPE)) {
              // no argument
            }
            ++it;
          }
        } else {
          // FIXME: implement proper action
        }
      }

      // configure charts
      else if (param[0].compare("graph") == 0) {
        // FIXME: implement proper action
      }

      // configure scene graph view
      else if (param[0].compare("sceneview") == 0) {
        // FIXME: implement proper action
      }

      // fallthrough
      else {
        SoDebugError::postWarning("SoProfiler::initialize",
                                  "Unknown COIN_PROFILER_OVERLAY parameter '%s'.",
                                  param[0].data());
      }
    }

    // profiler::overlay::active = TRUE;
  }
  else {
    // env variable is empty - don't activate overlay parts
  }
}

/*
  Default implementation for dumping on console instead of overlaying
  statistics over the 3D graphics.
*/
void
SoProfilerP::dumpToConsole(const SbProfilingData & data)
{
  SoProfilingReportGenerator::ReportCB * callback = NULL;
  if (profiler::console::onstdout) {
    callback = SoProfilingReportGenerator::stdoutCB;
  }
  else if (profiler::console::onstderr) {
    callback = SoProfilingReportGenerator::stderrCB;
  }
  if (!callback) {
    return;
  }

  if (SoProfilerP::shouldClearConsole()) {
    // send ansi-console clear screen code
    static const char CLEAR_SEQUENCE[] = "\033c";
    if (profiler::console::onstdout) {
      fputs(CLEAR_SEQUENCE,coin_get_stdout());
    } else if (profiler::console::onstderr) {
      fputs(CLEAR_SEQUENCE,coin_get_stderr());
    }
  }

  SoProfilingReportGenerator::DataCategorization category =
    profiler::console::category;

  // set up how to sort the toplist
  SbProfilingReportSortCriteria * sortsettings =
    SoProfilingReportGenerator::getDefaultReportSortCriteria(category);

  // set up how to print the toplist
  SbProfilingReportPrintCriteria * printsettings =
    SoProfilingReportGenerator::getDefaultReportPrintCriteria(category);

  SoProfilingReportGenerator::generate(data,
                                       category,
                                       sortsettings,
                                       printsettings,
                                       profiler::console::lines,
                                       SoProfilerP::shouldOutputHeaderOnConsole(),
                                       callback,
                                       NULL);

  SoProfilingReportGenerator::freeCriteria(sortsettings);
  SoProfilingReportGenerator::freeCriteria(printsettings);
}
