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
  \class SoGLCacheList SoGLCacheList.h Inventor/caches/SoGLCacheList.h
  \brief The SoGLCacheList class is used to store and manage OpenGL caches.

  \ingroup coin_caches
*/

#include <Inventor/caches/SoGLCacheList.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/C/tidbits.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/caches/SoGLRenderCache.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/misc/SoContextHandler.h>
#include <Inventor/system/gl.h>

#include "tidbitsp.h"
#include "glue/glp.h"
#include "rendering/SoGL.h"

// *************************************************************************

// SGI Inventor uses an LRU/MRU strategy or something here. We're not
// quite sure we should support multiple caches per SoSeparator
// though. After all, there is some overhead in checking for valid
// caches etc. If a situation occurs where multiple caches would help
// the performance, the user should probably redesign the scene graph
// and enable caching further down the scene graph instead. We will
// store at least one cache per cache context to support rendering in
// multiple contexts though.

static int COIN_AUTO_CACHING = -1;
static int COIN_SMART_CACHING = -1;

// *************************************************************************

class SoGLCacheListP {
public:
  SbList <SoGLRenderCache *> itemlist;
  int numcaches;
  SoGLRenderCache * opencache;
  SbBool savedinvalid;
  int autocachebits;
  int numused;
  int numdiscarded;
  SbBool needclose;
  SoElement * invalidelement;
  int numframesok;
  int numshapes;

  //
  // Callback from SoContextHandler
  //
  static void contextCleanup(uint32_t context, void * closure) {
    SoGLCacheListP * thisp = static_cast<SoGLCacheListP *>(closure);

    int i = 0;
    int n = thisp->itemlist.getLength();
    while (i < n) {
      if (thisp->itemlist[i]->getCacheContext() == static_cast<int>(context)) {
        thisp->itemlist[i]->unref();
        thisp->itemlist.remove(i);
        n--;
      }
      else i++;
    }
  }
};

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

/*!
  Constructor.
*/
SoGLCacheList::SoGLCacheList(int numcaches)
{
  PRIVATE(this) = new SoGLCacheListP;
  PRIVATE(this)->numcaches = numcaches;
  PRIVATE(this)->opencache = NULL;
  PRIVATE(this)->autocachebits = 0;
  PRIVATE(this)->numused = 0;
  PRIVATE(this)->numdiscarded = 0;
  PRIVATE(this)->needclose = FALSE;
  PRIVATE(this)->invalidelement = NULL;
  PRIVATE(this)->numframesok = 0;
  PRIVATE(this)->numshapes = 0;

  // auto caching must be enabled using an environment variable
  if (COIN_AUTO_CACHING < 0) {
    const char * env = coin_getenv("COIN_AUTO_CACHING");
    if (env) COIN_AUTO_CACHING = atoi(env);
    else COIN_AUTO_CACHING = 1;
  }
  if (COIN_SMART_CACHING < 0) {
    const char * env = coin_getenv("COIN_SMART_CACHING");
    if (env) COIN_SMART_CACHING = atoi(env);
    else COIN_SMART_CACHING = 0;
  }

  SoContextHandler::addContextDestructionCallback(SoGLCacheListP::contextCleanup, PRIVATE(this));

#if COIN_DEBUG
  if (coin_debug_caching_level() > 0) {
    SoDebugError::postInfo("SoGLCacheList::SoGLCacheList",
                           "Cache list created: %p", this);
  }
#endif // debug
}

/*!
  Destructor. Frees remaining caches.
*/
SoGLCacheList::~SoGLCacheList()
{
#if COIN_DEBUG
  if (coin_debug_caching_level() > 0) {
    SoDebugError::postInfo("SoGLCacheList::~SoGLCacheList",
                           "Cache list destructed: %p", this);
  }
#endif // debug

  SoContextHandler::removeContextDestructionCallback(SoGLCacheListP::contextCleanup, PRIVATE(this));
  const int n = PRIVATE(this)->itemlist.getLength();
  for (int i = 0; i < n; i++) {
    PRIVATE(this)->itemlist[i]->unref();
  }
  delete PRIVATE(this);
}

/*!
  Test for valid cache and execute. Returns TRUE if a valid cache
  could be found, FALSE otherwise. Note that when a valid cache is
  found, it is executed before returning from this method.
*/
SbBool
SoGLCacheList::call(SoGLRenderAction * action)
{
  // do a quick return if there are no caches in the list
  int n = PRIVATE(this)->itemlist.getLength();
  if (n == 0) return FALSE;

  int i;
  SoState * state = action->getState();
  int context = SoGLCacheContextElement::get(state);

  for (i = 0; i < n; i++) {
    SoGLRenderCache * cache = PRIVATE(this)->itemlist[i];
    if (cache->getCacheContext() == context) {
      if (cache->isValid(state) &&
          SoGLLazyElement::preCacheCall(state, cache->getPreLazyState())) {
        cache->ref();
        // move cache to the end of the list. The MRU cache will be at
        // the end of the list, and the LRU will be the first
        // item. This makes it easy to choose a cache to destroy when
        // the maximum number of caches is exceeded.
        PRIVATE(this)->itemlist.remove(i);
        PRIVATE(this)->itemlist.append(cache);
        // update lazy GL state before calling cache
        SoGLLazyElement::getInstance(state)->send(state, SoLazyElement::ALL_MASK);
        cache->call(state);
        SoGLLazyElement::postCacheCall(state, cache->getPostLazyState());
        cache->unref(state);
        PRIVATE(this)->numused++;

#if COIN_DEBUG
        // The GL error test is default disabled for this optimized
        // path.  If you get a GL error report somewhere else, enable
        // this code by setting the environment variable
        // COIN_GLERROR_DEBUGGING to "1" to see if the error comes
        // only after invoking a GL renderlist.
        //
        // We've seen this happen, which is peculiar -- as the same
        // OpenGL commands are sent when _building_ a cache. This
        // makes it likely that such problems are due to faulty GL
        // drivers.
        //
        // If this happens, try to set IV_SEPARATOR_MAX_CACHES=0 to
        // check if the GL error is still present even when not using
        // GL displaylists.

        static SbBool chkglerr = sogl_glerror_debugging();
        if (chkglerr) {
          GLenum err = glGetError();
          if (err != GL_NO_ERROR) {
            SoDebugError::post("SoGLCacheList::call",
                               "An OpenGL error (%s) was detected after a "
                               "renderlist invocation. This shouldn't happen, "
                               "low-level debugging is needed.",
                               coin_glerror_string(err));
          }
        }

        // For reference, below follows an example case we've had
        // reported. The iv-file of a NURBS surface is compiled to a
        // displaylist and then later rendered -- at which point the
        // GL error occurs (at least on Linux NVidia driver 1.4.0
        // v41.92, with a GeForce2 MX/MX 400 card, and GLU 1.3):
        //
        // -----8<---- [snip] ------------------8<---- [snip] -------------
        // #Inventor V2.1 ascii
        //
        //  Separator {
        //   # renderCaching OFF
        //
        //   Coordinate3 {
        //      point [ 903.23315 -292.8786 -261.79764,
        //       908.05878 -287.45178 -264.75046,
        //       932.02802 -260.18256 -279.56335,
        //       978.21088 -204.28212 -309.72232,
        //       1038.9332 -123.09545 -352.78091,
        //       1099.2452 -32.638832 -398.98602,
        //       1155.6702 60.612404 -445.20407,
        //       1208.1346 156.64369 -490.58151,
        //       1255.5405 256.13852 -533.99921,
        //       1298.7241 358.22876 -575.7113,
        //       1338.1451 462.37573 -615.54578,
        //       1374.2526 568.28442 -653.367,
        //       1407.7961 675.52026 -689.0874,
        //       1439.4993 783.82806 -722.55426,
        //       1470.3623 892.81433 -753.58435,
        //       1501.3269 1002.2544 -781.97571,
        //       1533.0552 1112.0437 -807.32019,
        //       1566.5748 1221.74 -829.26929,
        //       1600.4281 1323.8329 -846.22614,
        //       1634.6821 1417.2887 -858.5614,
        //       1669.7595 1503.1974 -866.97583,
        //       1708.3434 1587.351 -872.23059,
        //       1743.2072 1656.661 -873.22693,
        //       1770.4005 1706.436 -873.10114,
        //       1780.8302 1732.8519 -873.11292,
        //       1785.8423 1747.6866 -873.11188,
        //       1788.0009 1755.2577 -873.11206,
        //       1789.9299 1762.1315 -873.11212,
        //       1790.619 1764.5847 -873.11212,
        //       1791.3671 1767.2466 -873.11212,
        //       1132.113 -331.08054 100.19,
        //       1135.6515 -324.95352 97.25296,
        //       1153.1808 -294.24207 82.499771,
        //       1186.6023 -232.0327 52.220581,
        //       1229.5189 -143.28535 8.486125,
        //       1271.3019 -46.2309 -39.506714,
        //       1309.7157 52.397884 -88.602188,
        //       1344.9164 152.72209 -138.05904,
        //       1375.7001 255.2583 -186.31833,
        //       1403.3052 359.10748 -234.12572,
        //       1428.4164 463.79761 -281.39886,
        //       1451.5635 569.12341 -328.00269,
        //       1473.4384 674.72479 -373.77682,
        //       1494.7687 780.4386 -418.52753,
        //       1516.4795 886.00763 -462.00479,
        //       1539.5337 991.31763 -503.91452,
        //       1565.03 1096.2451 -543.82581,
        //       1594.3169 1200.59 -581.33673,
        //       1626.2183 1297.343 -613.68463,
        //       1660.5347 1385.8333 -640.45953,
        //       1696.7892 1468.0754 -662.0108,
        //       1736.037 1549.4417 -680.32446,
        //       1772.6567 1617.9526 -690.63373,
        //       1801.2168 1667.5009 -696.81317,
        //       1813.1285 1694.1454 -699.78467,
        //       1818.9022 1709.0796 -701.36713,
        //       1821.4705 1716.6644 -702.13947,
        //       1823.7567 1723.4873 -702.80975,
        //       1824.947 1726.8236 -703.06512,
        //       1826.005 1729.8676 -703.3371,
        //       1669.9569 -134.57997 685.43207,
        //       1670.6903 -127.96127 682.6673,
        //       1674.222 -94.879944 668.75208,
        //       1679.8473 -28.921362 639.92487,
        //       1684.1995 62.855659 597.63171,
        //       1685.0945 160.19362 549.54291,
        //       1682.697 256.28564 498.31042,
        //       1677.2521 351.04462 443.96701,
        //       1668.3485 445.28925 388.77441,
        //       1657.3577 537.88531 330.87088,
        //       1645.3646 628.60516 269.94531,
        //       1633.4319 717.65143 206.03265,
        //       1622.6926 805.24896 139.51292,
        //       1614.3499 891.79755 70.902924,
        //       1609.6595 977.73431 1.0185061,
        //       1609.9548 1063.6471 -68.94474,
        //       1616.9481 1150.0125 -137.41048,
        //       1632.2463 1237.3811 -202.65694,
        //       1655.1542 1320.0626 -258.95587,
        //       1684.144 1398.2058 -303.29956,
        //       1717.3192 1471.9259 -339.8316,
        //       1754.276 1544.7949 -373.89639,
        //       1790.1226 1608.4249 -392.34088,
        //       1818.5092 1652.6719 -406.02744,
        //       1831.3823 1676.9386 -411.69763,
        //       1837.6758 1690.8813 -414.05777,
        //       1840.4863 1697.9987 -415.05069,
        //       1842.9985 1704.1809 -416.19083,
        //       1844.6373 1707.9203 -417.28,
        //       1845.9977 1710.9867 -418.20154,
        //       2069.9097 242.0858 906.90302,
        //       2069.2424 248.42937 904.83679,
        //       2065.7732 280.12646 894.45093,
        //       2057.533 343.20288 873.18823,
        //       2042.7882 430.5639 842.27533,
        //       2023.3832 522.88196 807.67621,
        //       2000.5902 613.83093 771.46381,
        //       1974.8794 703.67169 734.07739,
        //       1945.7719 792.84912 697.26788,
        //       1914.0753 880.31677 659.28027,
        //       1880.3435 965.80292 619.60999,
        //       1845.2063 1049.0941 577.67804,
        //       1809.3909 1130.0742 532.95233,
        //       1773.8319 1208.6254 484.89426,
        //       1739.6515 1284.7078 432.93884,
        //       1708.3459 1358.2207 376.54547,
        //       1682.0516 1428.9958 315.14639,
        //       1663.5428 1496.6219 248.10159,
        //       1656.6949 1556.2395 180.31561,
        //       1661.3206 1607.5529 113.77404,
        //       1676.8506 1652.9681 53.936939,
        //       1704.0085 1694.6982 -3.7455604,
        //       1735.9509 1727.7632 -47.178341,
        //       1761.9692 1748.0847 -74.126205,
        //       1775.2965 1759.4961 -87.802773,
        //       1782.5793 1767.0847 -95.980415,
        //       1785.7794 1771.2867 -100.05127,
        //       1788.3989 1774.8469 -103.15611,
        //       1789.7174 1776.8969 -104.41734,
        //       1790.9209 1778.8102 -105.47821,
        //       2222.7271 411.91647 967.50342,
        //       2221.5823 418.09232 965.73175,
        //       2215.7292 448.95306 956.83862,
        //       2202.7493 510.36282 938.77655,
        //       2181.4448 595.32751 912.65607,
        //       2154.9297 685.08112 883.66913,
        //       2124.8262 773.51117 853.56708,
        //       2091.6328 861.00134 822.78912,
        //       2054.9531 947.84894 792.92444,
        //       2015.2894 1033.0553 762.18048,
        //       1972.9683 1116.3486 729.94928,
        //       1928.47 1197.3641 695.45129,
        //       1882.3811 1275.8629 657.84155,
        //       1835.5389 1351.5306 616.25537,
        //       1789.0238 1424.126 569.69537,
        //       1744.4063 1493.2378 517.13385,
        //       1703.903 1558.3778 457.39859,
        //       1670.7245 1618.5844 389.28601,
        //       1650.7424 1669.1328 317.68442,
        //       1644.5143 1709.2839 244.88469,
        //       1652.0626 1742.5808 179.06244,
        //       1674.8881 1770.8495 115.97897,
        //       1705.0859 1789.989 67.866989,
        //       1729.8898 1799.4652 38.898201,
        //       1743.2585 1804.8577 23.995186,
        //       1750.9772 1809.3849 14.711536,
        //       1754.3555 1812.1666 9.9977341,
        //       1757.0004 1814.4926 6.5047188,
        //       1758.0509 1815.6334 5.3017998,
        //       1759.1072 1816.9659 4.2506523 ]
        //   }
        //   NurbsSurface {
        //      numUControlPoints 30
        //      numVControlPoints 5
        //      uKnotVector [ 0, 0, 0, 0,
        //       21.313433, 106.60825, 213.3829, 320.27795,
        //       427.2619, 534.72577, 642.35693, 749.87128,
        //       857.28058, 964.53094, 1071.6017, 1178.4491,
        //       1285.0381, 1391.3175, 1497.1958, 1602.6195,
        //       1707.5751, 1791.2139, 1873.9054, 1956.2173,
        //       2037.5988, 2079.2507, 2100.7368, 2111.6772,
        //       2121.9106, 2122.1035, 2129.9102, 2129.9102,
        //       2129.9102, 2129.9102 ]
        //      vKnotVector [ 0, 0, 0, 0,
        //       871.44604, 1419.9783, 1419.9783, 1419.9783,
        //       1419.9783 ]
        //   }
        // }
        // -----8<---- [snip] ------------------8<---- [snip] -------------
        //
        // UPDATE: the above now works without errors on my Linux dev
        // machine with NVidia v 61.11 drivers. 20050309 mortene.

#endif // COIN_DEBUG

        return TRUE;
      }
    }
  }
#if COIN_DEBUG
  if (coin_debug_caching_level() > 0) {
    SoDebugError::postInfo("SoGLCacheList::call",
                           "no valid cache found for %p. Node has %d caches",
                           this, n);
    for (i = 0; i < n; i++) {
      SoGLRenderCache * cache = PRIVATE(this)->itemlist[i];
      if (cache->getCacheContext() == context) {
        SoDebugError::postInfo("SoGLCacheList::call",
                               "cache %d isValid()? %s", i, cache->isValid(state) ? "TRUE" : "FALSE");
        if (!cache->isValid(state)) {
          const SoElement * elem = cache->getInvalidElement(state);
          if (elem) {
            SoDebugError::postInfo("SoGLCacheList::call",
                                   "cache: %p, invalid element: %s", this,
                                   elem->getTypeId().getName().getString());
          }
        }
      }
    }
  }
#endif // debug
  return FALSE;
}

/*!
  Start recording a new cache. Remember to call close() when you've
  finished recording the cache.

  \sa close()
*/
void
SoGLCacheList::open(SoGLRenderAction * action, SbBool autocache)
{
  // needclose is used to quickly return in close()
  if (PRIVATE(this)->numcaches == 0 || (autocache && COIN_AUTO_CACHING == 0)) {
    PRIVATE(this)->needclose = FALSE;
    return;
  }

  PRIVATE(this)->needclose = TRUE;

  assert(PRIVATE(this)->opencache == NULL);
  SoState * state = action->getState();

  // will be restored in close()
  PRIVATE(this)->savedinvalid = SoCacheElement::setInvalid(FALSE);

  if (SoCacheElement::anyOpen(state)) return;

  SbBool shouldcreate = FALSE;
  if (!autocache) {
    if (PRIVATE(this)->numframesok >= 1) shouldcreate = TRUE;
  }
  else {
    if (PRIVATE(this)->numframesok >= 2 &&
        (PRIVATE(this)->autocachebits == SoGLCacheContextElement::DO_AUTO_CACHE)) {

      if (COIN_SMART_CACHING) {
        if (PRIVATE(this)->numshapes < 2) {
          if (PRIVATE(this)->numframesok >= 5) shouldcreate = TRUE;
        }
        else if (PRIVATE(this)->numshapes < 5) {
          if (PRIVATE(this)->numframesok >= 4) shouldcreate = TRUE;
        }
        else if (PRIVATE(this)->numshapes < 10) {
          if (PRIVATE(this)->numframesok >= 3) shouldcreate = TRUE;
        }
        else if (PRIVATE(this)->numshapes > 1000) {
          if (PRIVATE(this)->numframesok >= 4) shouldcreate = TRUE;
        }
        else if (PRIVATE(this)->numshapes > 100) {
          if (PRIVATE(this)->numframesok >= 3) shouldcreate = TRUE;
        }
        else {
          shouldcreate = TRUE;
        }
      }
      else {
        shouldcreate = TRUE;
      }
#if COIN_DEBUG
      if (coin_debug_caching_level() > 0 && PRIVATE(this)->numframesok >= 2) {
        SoDebugError::postInfo("SoGLCacheList::open",
                               "consider cache create: %p. numframesok: %d, numused: %d, numdiscarded: %d",
                               this, PRIVATE(this)->numframesok, PRIVATE(this)->numused, PRIVATE(this)->numdiscarded);
      }
#endif // debug

    }
  }

  if (shouldcreate && autocache) {
    // determine if we really should create a new cache, based on numused and numdiscarded
    double docreate = static_cast<double>(PRIVATE(this)->numframesok + PRIVATE(this)->numused);
    double dontcreate = (PRIVATE(this)->numdiscarded);
    
    dontcreate *= dontcreate;
    // we used to be more conservative here, and use dontcreate^4 to avoid
    // recreating caches too often. However, display lists are much faster with
    // current drivers than they used to be so we're a bit more aggressive now.
    if (dontcreate >= docreate) shouldcreate = FALSE;
  }

  if (shouldcreate) {
    if (PRIVATE(this)->itemlist.getLength() >= PRIVATE(this)->numcaches) {
      // the cache at position 0 will be the LRU cache. Remove it.
      SoGLRenderCache * cache = PRIVATE(this)->itemlist[0];
      cache->unref(state);
      PRIVATE(this)->itemlist.remove(0);
      PRIVATE(this)->numdiscarded++;
    }
    PRIVATE(this)->opencache = new SoGLRenderCache(state);
    PRIVATE(this)->opencache->ref();
    SoCacheElement::set(state, PRIVATE(this)->opencache);
    SoGLLazyElement::beginCaching(state, PRIVATE(this)->opencache->getPreLazyState(),
                                  PRIVATE(this)->opencache->getPostLazyState());
    PRIVATE(this)->opencache->open(state);

    // force a dependency on the transparency type
    // FIXME: consider adding a new element for storing the
    // transparency type.  The dependency tracking on the transparency
    // type would then work automatically. pederb, 2005-02-18
    (void) SoShapeStyleElement::get(state);

#if COIN_DEBUG // debug
    if (coin_debug_caching_level() > 0) {
      SoDebugError::postInfo("SoGLCacheList::open",
                             "trying to create cache: %p", this);
    }
#endif // debug
  }
  PRIVATE(this)->autocachebits = SoGLCacheContextElement::resetAutoCacheBits(state);
  PRIVATE(this)->numshapes = 0;
}

/*!
  Finish recording the currently open cache.
  \sa open()
 */
void
SoGLCacheList::close(SoGLRenderAction * action)
{
  if (!PRIVATE(this)->needclose) return;

  SoState * state = action->getState();

  // close open cache before accepting it or throwing it away
  if (PRIVATE(this)->opencache) {
    PRIVATE(this)->opencache->close();
    SoGLLazyElement::endCaching(state);
  }
  if (SoCacheElement::setInvalid(PRIVATE(this)->savedinvalid)) {
    // notify parent caches
    SoCacheElement::setInvalid(TRUE);
    PRIVATE(this)->numframesok = 0;
    // just throw away the open cache, it's invalid
    if (PRIVATE(this)->opencache) {
      PRIVATE(this)->opencache->unref();
      PRIVATE(this)->opencache = NULL;
      PRIVATE(this)->numdiscarded += 1;

#if COIN_DEBUG
      if (coin_debug_caching_level() > 0) {
        SoDebugError::postInfo("SoGLCacheList::close",
                               "failed to create cache: %p", this);
      }
#endif // debug
    }
  }
  else {
    PRIVATE(this)->numframesok++;
  }

  // open cache is ok, add it to the cache list
  if (PRIVATE(this)->opencache) {
#if COIN_DEBUG
    if (coin_debug_caching_level() > 0) {
      SoDebugError::postInfo("SoGLCacheList::close",
                             "new cache created: %p", this);
    }
#endif // debug
    PRIVATE(this)->itemlist.append(PRIVATE(this)->opencache);
    PRIVATE(this)->opencache = NULL;
  }

  PRIVATE(this)->numshapes = SoGLCacheContextElement::getNumShapes(state);
  int bits = SoGLCacheContextElement::resetAutoCacheBits(state);
  SoGLCacheContextElement::setAutoCacheBits(state, bits|PRIVATE(this)->autocachebits);
  PRIVATE(this)->autocachebits = bits;
}

/*!
  Invalidate all caches in this instance. Should be called
  from the notify() method of nodes doing caching.
*/
void
SoGLCacheList::invalidateAll(void)
{
  int n = PRIVATE(this)->itemlist.getLength();
#if COIN_DEBUG
  if (n && coin_debug_caching_level() > 1) {
    SoDebugError::postInfo("SoGLCacheList::invalidateAll",
                           "invalidate all: %p (num caches = %d)", this, n);
  }
#endif // debug

  for (int i = 0; i < n; i++) {
    PRIVATE(this)->itemlist[i]->unref();
  }
  PRIVATE(this)->itemlist.truncate(0);
  PRIVATE(this)->numdiscarded += n;
  PRIVATE(this)->numframesok = 0;
}

#undef PRIVATE
