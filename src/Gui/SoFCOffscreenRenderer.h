/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef GUI_SOFCOFFSCREENRENDERER_H
#define GUI_SOFCOFFSCREENRENDERER_H

#include <Inventor/SbColor4f.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SoOffscreenRenderer.h>

#include <QImage>
#include <QStringList>
#include <QtOpenGL.h>

#include <FCGlobal.h>


namespace Gui {

/**
 * The SoFCOffscreenRenderer class is used for rendering scenes in offscreen buffers.
 * @author Werner Mayer
 */
class GuiExport SoFCOffscreenRenderer : public SoOffscreenRenderer
{
public:
    /** The SoOffscreenRenderer base class seems to have a huge memory leak. Whenever
     * an instance is created internal memory doesn't get freed when destroying it.
     * Thus, SoFCOffscreenRenderer is implemented as singleton to allow to create only
     * one global instance. So, the memory is leaking for this instance only.
     */
    static SoFCOffscreenRenderer& instance();

    SoFCOffscreenRenderer(const SoFCOffscreenRenderer&) = delete;
    SoFCOffscreenRenderer& operator=(const SoFCOffscreenRenderer&) = delete;

private:
    static SoFCOffscreenRenderer* inst;

protected:
  /**
   * Constructor. Argument is the \a viewportregion we should use when rendering. An internal
   * SoGLRenderAction will be constructed.
   */
  SoFCOffscreenRenderer (const SbViewportRegion &viewportregion);
  /**
   * Constructor. Argument is the \a action we should apply to the scene graph when rendering the
   * scene. Information about the viewport is extracted from the \a action.
   */
  SoFCOffscreenRenderer (SoGLRenderAction *action);
  /**
   * Destructor.
   */
  ~SoFCOffscreenRenderer();

public:
  /**
   * Writes the rendered image buffer directly into a QImage object
   * instead of an image file.
   */
  void writeToImage (QImage&) const;
  /**
   * Saves the buffer to \a filename, in the filetype specified by \a filetypeextensions.
   *
   * Note that you must still specify the full filename for the first argument, i.e. the second argument will
   * not automatically be attached to the filename -- it is only used to decide the filetype.
   *
   * If \a comment is set to '$MIBA' information regarding the MIBA standard is
   * embedded to the picture, otherwise the \a comment is embedded as is.
   * The appropriate file format must support embedding meta information which
   * is provided by JPEG or PNG.
   *
   * This does basically the same as writeToFile() unless that all QImage file formats are supported if not
   * directly supported by Coin3D.
   */
  void writeToImageFile(const char* filename, const char* comment, const SbMatrix& mat, const QImage& img);
  /**
   * This method returns all image file formats supported by Coin3D (see getWriteFiletypeInfo()) with all QImage file formats that are
   * not directly supported by Coin3D, if so.
   */
  QStringList getWriteImageFiletypeInfo();

  std::string createMIBA(const SbMatrix& mat) const;
};

class GuiExport SoQtOffscreenRenderer
{
public:
    SoQtOffscreenRenderer(const SbViewportRegion & viewportregion);
    SoQtOffscreenRenderer(SoGLRenderAction * action);
    ~SoQtOffscreenRenderer();

    void setViewportRegion(const SbViewportRegion & region);
    const SbViewportRegion & getViewportRegion() const;

    void setBackgroundColor(const SbColor4f & color);
    const SbColor4f & getBackgroundColor() const;

    void setGLRenderAction(SoGLRenderAction * action);
    SoGLRenderAction * getGLRenderAction() const;

    void setNumPasses(const int num);
    int getNumPasses() const;

    void setInternalTextureFormat(GLenum internalTextureFormat);
    GLenum internalTextureFormat() const;

    SbBool render(SoNode * scene);
    SbBool render(SoPath * scene);

    void writeToImage (QImage&) const;
    QStringList getWriteImageFiletypeInfo() const;

private:
    void init(const SbViewportRegion & vpr, SoGLRenderAction * glrenderaction = nullptr);
    static void pre_render_cb(void * userdata, SoGLRenderAction * action);
    SbBool renderFromBase(SoBase * base);
    void makeFrameBuffer(int width, int height, int samples);

    QtGLFramebufferObject*  framebuffer;
    uint32_t                cache_context; // our unique context id

    SbViewportRegion viewport;
    SbColor4f backgroundcolor;
    SbColor4f backgroundopaque;
    SoGLRenderAction * renderaction;
    SbBool didallocation;
    int numSamples;
    GLenum texFormat;
    QImage glImage;
};

} // namespace Gui


#endif // GUI_SOFCOFFSCREENRENDERER_H
