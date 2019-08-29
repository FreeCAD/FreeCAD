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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/elements/SoGLCacheContextElement.h>
# include <Inventor/fields/SoSFImage.h>
# include <Inventor/nodes/SoNode.h>
# include <QBuffer>
# include <QDateTime>
# include <QFile>
# include <QImage>
# include <QImageWriter>
#endif

#if !defined(FC_OS_MACOSX)
# include <GL/gl.h>
# include <GL/glu.h>
# include <GL/glext.h>
#endif

//gcc
# include <iomanip>
# include <ios>
# include <sstream>

#include <Base/FileInfo.h>
#include <Base/Exception.h>
#include <Base/Console.h>
#include <App/Application.h>

#include "SoFCOffscreenRenderer.h"
#include "BitmapFactory.h"

#if defined(HAVE_QT5_OPENGL)
# include <QOffscreenSurface>
# include <QOpenGLContext>
#endif

using namespace Gui;
using namespace std;


void writeJPEGComment(const std::string&, QByteArray&);

// ---------------------------------------------------------------

SoFCOffscreenRenderer* SoFCOffscreenRenderer::inst = 0;


SoFCOffscreenRenderer& SoFCOffscreenRenderer::instance()
{
    if (inst==0)
        inst = new SoFCOffscreenRenderer(SbViewportRegion());
    return *inst;
}

SoFCOffscreenRenderer::SoFCOffscreenRenderer (const SbViewportRegion &viewportregion)
  : SoOffscreenRenderer(viewportregion)
{
}

SoFCOffscreenRenderer::SoFCOffscreenRenderer (SoGLRenderAction *action)
  : SoOffscreenRenderer(action)
{
}

SoFCOffscreenRenderer::~SoFCOffscreenRenderer()
{
}

void SoFCOffscreenRenderer::writeToImage (QImage& img) const
{
    const unsigned char * bytes = getBuffer();
    SbVec2s size = getViewportRegion().getViewportSizePixels();
    int numcomponents = (int) this->getComponents();

    SoSFImage image;
    image.setValue(size, numcomponents, bytes, SoSFImage::NO_COPY);
    BitmapFactory().convert(image, img);
}

void SoFCOffscreenRenderer::writeToImageFile(const char* filename, const char* comment, const SbMatrix& mat, const QImage& image)
{
    if (image.isNull()) {
        std::stringstream str;
        str << "Cannot save null image.";
        throw Base::ValueError(str.str());
    }

    Base::FileInfo file(filename);
    if (file.hasExtension("JPG") || file.hasExtension("JPEG")) {
        // writing comment in case of jpeg (Qt ignores setText() in case of jpeg)
        std::string com;
        if (strcmp(comment,"")==0)
            com = "Screenshot created by FreeCAD";
        else if (strcmp(comment,"$MIBA")==0)
            com = createMIBA(mat);
        else
            com = comment;

        // write into memory
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "JPG");
        writeJPEGComment(com, ba);

        QFile file(QString::fromUtf8(filename));
        if (file.open(QFile::WriteOnly)) {
            file.write(ba);
            file.close();
        }
        else {
            std::stringstream str;
            str << "Cannot open file '" << filename << "' for writing.";
            throw Base::FileSystemError(str.str());
        }
    }
    else {
        // check for all QImage formats
        bool supported = false;
        QByteArray format;
        QList<QByteArray> qtformats = QImageWriter::supportedImageFormats();
        for (QList<QByteArray>::Iterator it = qtformats.begin(); it != qtformats.end(); ++it) {
            if (file.hasExtension((*it).data())) {
                format = *it;
                supported = true;
                break;
            }
        }

        // Supported by Qt
        if (supported) {
            QImage img = image;
            // set keywords for PNG format
            if (file.hasExtension("PNG")) {
                img.setText(QLatin1String("Title"), QString::fromUtf8(filename));
                img.setText(QLatin1String("Author"), QLatin1String("FreeCAD (http://www.freecadweb.org)"));
                if (strcmp(comment,"")==0)
                    img.setText(QLatin1String("Description"), QLatin1String("Screenshot created by FreeCAD"));
                else if (strcmp(comment,"$MIBA")==0)
                    img.setText(QLatin1String("Description"), QLatin1String(createMIBA(mat).c_str()));
                else 
                    img.setText(QLatin1String("Description"), QString::fromUtf8(comment));
                img.setText(QLatin1String("Creation Time"), QDateTime::currentDateTime().toString());
                img.setText(QLatin1String("Software"), 
                    QString::fromUtf8(App::GetApplication().getExecutableName()));
            }

            QFile f(QString::fromUtf8(filename));
            if (f.open(QFile::WriteOnly)) {
                if (img.save(&f, format.data())) {
                    f.close();
                }
                else {
                    f.close();
                    std::stringstream str;
                    str << "Cannot save image to file '" << filename << "'.";
                    throw Base::ValueError(str.str());
                }
            }
            else {
                std::stringstream str;
                str << "Cannot open file '" << filename << "' for writing.";
                Base::FileException e;
                e.setMessage(str.str());
                throw e;
            }
        }
        //
        // Use internal buffer instead of QImage
        //
        else if (isWriteSupported(file.extension().c_str())) {
            // Any format which is supported by Coin only
            if (!writeToFile(filename, file.extension().c_str()))
                throw Base::FileException("Error writing image file", filename);
        }
        else if (file.hasExtension("EPS") || file.hasExtension("PS")) {
            // Any format which is supported by Coin only
#ifdef FC_OS_WIN32
            FILE* fd = _wfopen(file.toStdWString().c_str(), L"w");
#else
            FILE* fd = fopen(filename, "w");
#endif
            bool ok = writeToPostScript(fd);
            fclose(fd);
            if (!ok)
                throw Base::FileException("Error writing image file", filename);
        }
        else if (file.hasExtension("RGB") || file.hasExtension("SGI")) {
            // Any format which is supported by Coin only
#ifdef FC_OS_WIN32
            FILE* fd = _wfopen(file.toStdWString().c_str(), L"w");
#else
            FILE* fd = fopen(filename, "w");
#endif
            bool ok = writeToRGB(fd);
            fclose(fd);
            if (!ok)
                throw Base::FileException("Error writing image file", filename);
        }
    }
}

QStringList SoFCOffscreenRenderer::getWriteImageFiletypeInfo()
{
    QStringList formats;

    // get all supported formats by Coin3D
    int num = getNumWriteFiletypes();
    for (int i=0; i < num; i++) {
#if   (COIN_MAJOR_VERSION < 2) // Coin3D <= 1.x
        SbList<SbName> extlist;
#elif (COIN_MAJOR_VERSION < 3) // Coin3D <= 2.x
# if  (COIN_MINOR_VERSION < 3) // Coin3D <= 2.2.x
        SbList<SbName> extlist;
# else                         // Coin3D >= 2.3.x
        SbPList extlist;
# endif                        
#else                          // Coin3D >= 3.x
        SbPList extlist;
#endif

        SbString fullname, description;
        getWriteFiletypeInfo(i, extlist, fullname, description);

        for (int j=0; j < extlist.getLength(); j++) {
            QString ext = QLatin1String((const char*) extlist[j]);
            if (formats.indexOf(ext.toUpper()) == -1)
                formats << ext.toUpper();
        }
    }

    // add now all further QImage formats
    QList<QByteArray> qtformats = QImageWriter::supportedImageFormats();
    for (QList<QByteArray>::Iterator it = qtformats.begin(); it != qtformats.end(); ++it) {
        // not supported? then append
        if (!isWriteSupported((*it).data()) && formats.indexOf(QLatin1String(*it)) == -1)
            formats << QLatin1String(*it);
    }

    // now add PostScript and SGI RGB
    if (formats.indexOf(QLatin1String("EPS")) == -1)
        formats << QLatin1String("EPS");
    else if (formats.indexOf(QLatin1String("SGI")) == -1)
        formats << QLatin1String("SGI");

    formats.sort();

    return formats;
}

std::string SoFCOffscreenRenderer::createMIBA(const SbMatrix& mat) const
{
    std::stringstream com;
    const std::map<std::string, std::string>& cfg = App::Application::Config();
    std::map<std::string, std::string>::const_iterator it;
    it = cfg.find("BuildVersionMajor");
    std::string major = (it != cfg.end() ? it->second : "");
    it = cfg.find("BuildVersionMinor");
    std::string minor = (it != cfg.end() ? it->second : "");

    com << setw(7) << setfill(' ') << fixed;
    com << "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n" ;
    com << "<MIBA xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://juergen-riegel.net/Miba/Miba2.xsd\" Version=\"2\"> \n" ;
    com << " <View>\n"; 
    com << "  <Matrix \n"; 
    com << "     a11=\"" << mat[0][0] <<"\" a12=\"" << mat[1][0] <<"\" a13=\"" << mat[2][0] <<"\" a14=\"" << mat[3][0] << "\"\n";
    com << "     a21=\"" << mat[0][1] <<"\" a22=\"" << mat[1][1] <<"\" a23=\"" << mat[2][1] <<"\" a24=\"" << mat[3][1] << "\"\n";
    com << "     a31=\"" << mat[0][2] <<"\" a32=\"" << mat[1][2] <<"\" a33=\"" << mat[2][2] <<"\" a34=\"" << mat[3][2] << "\"\n";
    com << "     a41=\"" << mat[0][3] <<"\" a42=\"" << mat[1][3] <<"\" a43=\"" << mat[2][3] <<"\" a44=\"" << mat[3][3] << "\"\n";
    com << "   />\n" ; 
    com << " </View>\n" ; 
    com << " <Source>\n" ; 
    com << "  <Creator>Unknown</Creator>\n" ;  
    com << "  <CreationDate>" << QDateTime::currentDateTime().toString().toLatin1().constData() << "</CreationDate>\n" ;  
    com << "  <CreatingSystem>" << App::GetApplication().getExecutableName() << " " << major << "." << minor << "</CreatingSystem>\n" ;
    com << "  <PartNumber>Unknown</PartNumber>\n";
    com << "  <Revision>1.0</Revision>\n";
    com << " </Source>\n" ;
    com << "</MIBA>\n" ;

    return com.str();
}

void writeJPEGComment(const std::string& comment, QByteArray& ba)
{
    const unsigned char M_SOF0  = 0xc0;
    const unsigned char M_SOF1  = 0xc1;
    const unsigned char M_SOF2  = 0xc2;
    const unsigned char M_SOF3  = 0xc3;
    const unsigned char M_SOF5  = 0xc5;
    const unsigned char M_SOF6  = 0xc6;
    const unsigned char M_SOF7  = 0xc7;
    const unsigned char M_SOF9  = 0xc9;
    const unsigned char M_SOF10 = 0xcA;
    const unsigned char M_SOF11 = 0xcb;
    const unsigned char M_SOF13 = 0xcd;
    const unsigned char M_SOF14 = 0xce;
    const unsigned char M_SOF15 = 0xcf;
    const unsigned char M_SOI   = 0xd8;
    const unsigned char M_EOI   = 0xd9;
    const unsigned char M_COM   = 0xfe;

    union Byte {
        char c; unsigned char u;
    };

    if (comment.empty() || ba.length() < 2)
        return;

    // first marker
    Byte a,b;
    a.c = ba[0];
    b.c = ba[1];
    if (a.u == 0xff && b.u == M_SOI) {
        int index = 2;
        int len = ba.length();
        while (index < len) {
            // next marker
            a.c = ba[index++];
            while (a.u != 0xff && index < len) {
                a.c = ba[index++];
            }
            do {
                b.c = ba[index++];
            } while (b.u == 0xff && index < len);
            switch (b.u) {
                case M_SOF0:
                case M_SOF1:
                case M_SOF2:
                case M_SOF3:
                case M_SOF5:
                case M_SOF6:
                case M_SOF7:
                case M_SOF9:
                case M_SOF10:
                case M_SOF11:
                case M_SOF13:
                case M_SOF14:
                case M_SOF15:
                case M_EOI:
                    {
                        Byte a, b;
                        a.u = 0xff;
                        b.u = M_COM;
                        index -= 2; // insert comment before marker
                        ba.insert(index++, a.c);
                        ba.insert(index++, b.c);
                        int val = comment.size() + 2;
                        ba.insert(index++,(val >> 8) & 0xff);
                        ba.insert(index++,val & 0xff);
                        ba.insert(index, comment.c_str());
                        index = len; // finished
                    }   break;
                case M_COM:
                default:
                    {
                        Byte a, b;
                        a.c = ba[index++];
                        b.c = ba[index++];
                        int off = ((unsigned int)a.u << 8) + (unsigned int)b.u;
                        index += off;
                        index -= 2; // next marker
                    }   break;
            }
        }
    }
}

// ---------------------------------------------------------------

#define PRIVATE(p) p
#define PUBLIC(p) p

void SoQtOffscreenRenderer::init(const SbViewportRegion & vpr,
                                 SoGLRenderAction * glrenderaction)
{
    this->backgroundcolor.setValue(0,0,0);

    if (glrenderaction) {
        this->renderaction = glrenderaction;
    }
    else {
        this->renderaction = new SoGLRenderAction(vpr);
        this->renderaction->setCacheContext(SoGLCacheContextElement::getUniqueCacheContext());
        this->renderaction->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_BLEND);
    }

    this->didallocation = glrenderaction ? false : true;
    this->viewport = vpr;

#if !defined(HAVE_QT5_OPENGL)
    this->pixelbuffer = NULL;                // constructed later
#endif
    this->framebuffer = NULL;
    this->numSamples = -1;
    this->cache_context = 0;
    this->pbuffer = false;
}

/*!
  Constructor. Argument is the \a viewportregion we should use when
  rendering. An internal SoGLRenderAction will be constructed.
*/
SoQtOffscreenRenderer::SoQtOffscreenRenderer(const SbViewportRegion & viewportregion)
{
    init(viewportregion);
}

/*!
  Constructor. Argument is the \a action we should apply to the
  scene graph when rendering the scene. Information about the
  viewport is extracted from the \a action.
*/
SoQtOffscreenRenderer::SoQtOffscreenRenderer(SoGLRenderAction * action)
{
    init(action->getViewportRegion(), action);
}

/*!
  Destructor.
*/
SoQtOffscreenRenderer::~SoQtOffscreenRenderer()
{
#if !defined(HAVE_QT5_OPENGL)
    delete pixelbuffer;
#endif
    delete framebuffer;

    if (this->didallocation) {
        delete this->renderaction;
    }
}

/*!
  Sets the viewport region.

  This will invalidate the current buffer, if any. The buffer will not
  contain valid data until another call to
  SoOffscreenRendererQt::render() happens.
*/
void
SoQtOffscreenRenderer::setViewportRegion(const SbViewportRegion & region)
{
    PRIVATE(this)->viewport = region;
}

/*!
  Returns the viewerport region.
*/
const SbViewportRegion &
SoQtOffscreenRenderer::getViewportRegion(void) const
{
    return PRIVATE(this)->viewport;
}

/*!
  Sets the background color. The buffer is cleared to this color
  before rendering.
*/
void
SoQtOffscreenRenderer::setBackgroundColor(const SbColor4f & color)
{
    PRIVATE(this)->backgroundcolor = color;
    PRIVATE(this)->backgroundopaque = color;
    if (color[3] < 1.0)
        PRIVATE(this)->backgroundopaque.setValue(1,1,1,1);
}

/*!
  Returns the background color.
*/
const SbColor4f &
SoQtOffscreenRenderer::getBackgroundColor(void) const
{
    return PRIVATE(this)->backgroundcolor;
}

/*!
  Sets the render action. Use this if you have special rendering needs.
*/
void
SoQtOffscreenRenderer::setGLRenderAction(SoGLRenderAction * action)
{
    if (action == PRIVATE(this)->renderaction) { return; }

    if (PRIVATE(this)->didallocation) { delete PRIVATE(this)->renderaction; }
    PRIVATE(this)->renderaction = action;
    PRIVATE(this)->didallocation = false;
}

/*!
  Returns the rendering action currently used.
*/
SoGLRenderAction *
SoQtOffscreenRenderer::getGLRenderAction(void) const
{
    return PRIVATE(this)->renderaction;
}

void
SoQtOffscreenRenderer::setNumPasses(const int num)
{
    PRIVATE(this)->numSamples = num;
}

int
SoQtOffscreenRenderer::getNumPasses(void) const
{
    return PRIVATE(this)->numSamples;
}

void
SoQtOffscreenRenderer::setPbufferEnable(SbBool enable)
{
    PRIVATE(this)->pbuffer = enable;
}

SbBool
SoQtOffscreenRenderer::getPbufferEnable(void) const
{
    return PRIVATE(this)->pbuffer;
}

// *************************************************************************

void
SoQtOffscreenRenderer::pre_render_cb(void * /*userdata*/, SoGLRenderAction * action)
{
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    action->setRenderingIsRemote(false);
}

#if !defined(HAVE_QT5_OPENGL)
void
SoQtOffscreenRenderer::makePixelBuffer(int width, int height, int samples)
{
    if (pixelbuffer) {
        delete pixelbuffer;
        pixelbuffer = NULL;
    }

    viewport.setWindowSize(width, height);

    QGLFormat fmt;
    // With enabled alpha a transparent background is supported but
    // at the same time breaks semi-transparent models. A workaround
    // is to use a certain background color using GL_RGB as texture
    // format and in the output image search for the above color and
    // replaces it with the color requested by the user.
    //fmt.setAlpha(true);
    if (samples > 0) {
        fmt.setSampleBuffers(true);
        fmt.setSamples(samples);
    }
    else {
        fmt.setSampleBuffers(false);
    }

    pixelbuffer = new QGLPixelBuffer(width, height, fmt);
    cache_context = SoGLCacheContextElement::getUniqueCacheContext(); // unique per pixel buffer object, just to be sure
}
#endif

void
SoQtOffscreenRenderer::makeFrameBuffer(int width, int height, int samples)
{
    if (framebuffer) {
        delete framebuffer;
        framebuffer = NULL;
    }

    viewport.setWindowSize(width, height);

#if QT_VERSION >= 0x040600
    QtGLFramebufferObjectFormat fmt;
    fmt.setSamples(samples);
    fmt.setAttachment(QtGLFramebufferObject::Depth);
    // With enabled alpha a transparent background is supported but
    // at the same time breaks semi-transparent models. A workaround
    // is to use a certain background color using GL_RGB as texture
    // format and in the output image search for the above color and
    // replaces it with the color requested by the user.
#if defined(HAVE_QT5_OPENGL)
    //fmt.setInternalTextureFormat(GL_RGBA32F_ARB);
    fmt.setInternalTextureFormat(GL_RGB32F_ARB);
#else
    //fmt.setInternalTextureFormat(GL_RGBA);
    fmt.setInternalTextureFormat(GL_RGB);
#endif
#else
    QtGLFramebufferObject::Attachment fmt;
    fmt = QtGLFramebufferObject::Depth;
#endif

    framebuffer = new QtGLFramebufferObject(width, height, fmt);
    cache_context = SoGLCacheContextElement::getUniqueCacheContext(); // unique per pixel buffer object, just to be sure
}

SbBool
SoQtOffscreenRenderer::renderFromBase(SoBase * base)
{
    const SbVec2s fullsize = this->viewport.getViewportSizePixels();

#if defined(HAVE_QT5_OPENGL)
    QSurfaceFormat format;
    format.setSamples(PRIVATE(this)->numSamples);
    QOpenGLContext context;
    context.setFormat(format);
    if (!context.create())
        return false;
    QOffscreenSurface offscreen;
    offscreen.setFormat(format);
    offscreen.create();
    context.makeCurrent(&offscreen);
#endif

#if !defined(HAVE_QT5_OPENGL)
    if (PRIVATE(this)->pbuffer) {
        if (!pixelbuffer) {
            makePixelBuffer(fullsize[0], fullsize[1], PRIVATE(this)->numSamples);
        }
        else if (pixelbuffer->width() != fullsize[0] || pixelbuffer->height() != fullsize[1]) {
            // get the size right!
            makePixelBuffer(fullsize[0], fullsize[1], PRIVATE(this)->numSamples);
        }

        pixelbuffer->makeCurrent();                // activate us!
    }
    else
#endif
    {
        if (!framebuffer) {
            makeFrameBuffer(fullsize[0], fullsize[1], PRIVATE(this)->numSamples);
        }
        else if (framebuffer->width() != fullsize[0] || framebuffer->height() != fullsize[1]) {
            // get the size right!
            makeFrameBuffer(fullsize[0], fullsize[1], PRIVATE(this)->numSamples);
        }

        framebuffer->bind();                // activate us!
    }

    // oldcontext is used to restore the previous context id, in case
    // the render action is not allocated by us.
    const uint32_t oldcontext = this->renderaction->getCacheContext();
    this->renderaction->setCacheContext(cache_context);

    glEnable(GL_DEPTH_TEST);
    glClearColor(this->backgroundopaque[0],
                 this->backgroundopaque[1],
                 this->backgroundopaque[2],
                 this->backgroundopaque[3]);

    // needed to clear viewport after glViewport() is called from
    // SoGLRenderAction
    this->renderaction->addPreRenderCallback(pre_render_cb, NULL);
    this->renderaction->setViewportRegion(this->viewport);

    if (base->isOfType(SoNode::getClassTypeId()))
        this->renderaction->apply((SoNode *)base);
    else if (base->isOfType(SoPath::getClassTypeId()))
        this->renderaction->apply((SoPath *)base);
    else  {
        assert(false && "Cannot apply to anything else than an SoNode or an SoPath");
    }

    this->renderaction->removePreRenderCallback(pre_render_cb, NULL);

#if !defined(HAVE_QT5_OPENGL)
    if (pixelbuffer) {
        pixelbuffer->doneCurrent();
    }
    else
#endif
    {
        framebuffer->release();
    }

    this->renderaction->setCacheContext(oldcontext); // restore old

#if defined(HAVE_QT5_OPENGL)
    glImage = framebuffer->toImage();
    context.doneCurrent();
#endif

    return true;
}

/*!
  Render the scenegraph rooted at \a scene into our internal pixel
  buffer.

  Important note: make sure you pass in a \a scene node pointer which
  has both a camera and at least one lightsource below it -- otherwise
  you are likely to end up with just a blank or black image buffer.

  This mistake is easily made if you use an SoQtOffscreenRenderer on a
  scenegraph from one of the standard viewer components, as you will
  often just leave the addition of a camera and a headlight
  lightsource to the viewer to set up. This camera and lightsource are
  then part of the viewer's private "super-graph" outside of the scope
  of the scenegraph passed in by the application programmer. To make
  sure the complete scenegraph (including the viewer's "private parts"
  (*snicker*)) are passed to this method, you can get the scenegraph
  root from the viewer's internal SoSceneManager instance instead of
  from the viewer's own getSceneGraph() method, like this:

  \code
  SoQtOffscreenRenderer * myRenderer = new SoQtOffscreenRenderer(vpregion);
  SoNode * root = myViewer->getSceneManager()->getSceneGraph();
  SbBool ok = myRenderer->render(root);
  // [then use image buffer in a texture, or write it to file, or whatever]
  \endcode

  If you do this and still get a blank buffer, another common problem
  is to have a camera which is not actually pointing at the scene
  geometry you want a snapshot of. If you suspect that could be the
  cause of problems on your end, take a look at SoCamera::pointAt()
  and SoCamera::viewAll() to see how you can make a camera node
  guaranteed to be directed at the scene geometry.

  Yet another common mistake when setting up the camera is to specify
  values for the SoCamera::nearDistance and SoCamera::farDistance
  fields which doesn't not enclose the full scene. This will result in
  either just the background color, or that parts at the front or the
  back of the scene will not be visible in the rendering.

  \sa writeToImage()
*/
SbBool
SoQtOffscreenRenderer::render(SoNode * scene)
{
    return PRIVATE(this)->renderFromBase(scene);
}

/*!
  Render the \a scene path into our internal memory buffer.
*/
SbBool
SoQtOffscreenRenderer::render(SoPath * scene)
{
    return PRIVATE(this)->renderFromBase(scene);
}

/*! 
   Writes the rendered image buffer directly into a QImage object.
*/
void
SoQtOffscreenRenderer::writeToImage (QImage& img) const
{
#if !defined(HAVE_QT5_OPENGL)
    if (pixelbuffer)
        img = pixelbuffer->toImage();
    else if (framebuffer)
        img = framebuffer->toImage();
#else
    img = this->glImage;
#endif
    if (PRIVATE(this)->backgroundcolor[3] < 1.0) {
        QColor c1, c2;
        c1.setRedF(PRIVATE(this)->backgroundcolor[0]);
        c1.setGreenF(PRIVATE(this)->backgroundcolor[1]);
        c1.setBlueF(PRIVATE(this)->backgroundcolor[2]);
        c1.setAlphaF(PRIVATE(this)->backgroundcolor[3]);
        c2.setRedF(PRIVATE(this)->backgroundopaque[0]);
        c2.setGreenF(PRIVATE(this)->backgroundopaque[1]);
        c2.setBlueF(PRIVATE(this)->backgroundopaque[2]);
        c2.setAlphaF(PRIVATE(this)->backgroundopaque[3]);

        QImage image(img.constBits(), img.width(), img.height(), QImage::Format_ARGB32);
        img = image.copy();
        QRgb rgba = c1.rgba();
        QRgb rgb = c2.rgb();
        QRgb * bits = (QRgb*) img.bits();
        int height = img.height();
        int width = img.width();
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (*bits == rgb)
                    *bits = rgba;
                bits++;
            }
        }
    }
}

/*!
   This method returns all image file formats supported by Coin3D (see getWriteFiletypeInfo()) with all QImage file formats that are 
   not directly supported by Coin3D, if so.
*/
QStringList SoQtOffscreenRenderer::getWriteImageFiletypeInfo() const
{
    QList<QByteArray> qtformats = QImageWriter::supportedImageFormats();

    QStringList formats;
    for (QList<QByteArray>::Iterator it = qtformats.begin(); it != qtformats.end(); ++it) {
        formats << QLatin1String(*it);
    }
    formats.sort();
    return formats;
}

#undef PRIVATE
#undef PUBLIC
