/***************************************************************************
 *   Copyright (c) 2017 Shai Seger <shaise at gmail>                       *
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

#include "DlgCAMSimulator.h"
#include <QtGui/QMatrix4x4>
#include <QtGui/qopenglshaderprogram.h>
#include <QtGui/qscreen.h>


using namespace CAMSimulator;

namespace CAMSimulator
{


    OpenGLWindow::OpenGLWindow(QWindow* parent)
        : QWindow(parent)
    {
        setSurfaceType(QWindow::OpenGLSurface);
    }

    void OpenGLWindow::render(QPainter* painter)
    {
        Q_UNUSED(painter);
    }

    void OpenGLWindow::initialize()
    {}

    void OpenGLWindow::render()
    {
        if (!m_device) {
            m_device = new QOpenGLPaintDevice;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        m_device->setSize(size() * devicePixelRatio());
        m_device->setDevicePixelRatio(devicePixelRatio());

        QPainter painter(m_device);
        render(&painter);
    }

    void OpenGLWindow::renderLater()
    {
        requestUpdate();
    }

    bool OpenGLWindow::event(QEvent* event)
    {
        switch (event->type()) {
            case QEvent::UpdateRequest:
                renderNow();
                return true;
            default:
                return QWindow::event(event);
        }
    }

    void OpenGLWindow::exposeEvent(QExposeEvent* event)
    {
        Q_UNUSED(event);

        if (isExposed()) {
            renderNow();
        }
    }

    void OpenGLWindow::renderNow()
    {
        if (!isExposed()) {
            return;
        }

        bool needsInitialize = false;

        if (!m_context) {
            m_context = new QOpenGLContext(this);
            m_context->setFormat(requestedFormat());
            m_context->create();

            needsInitialize = true;
        }

        m_context->makeCurrent(this);

        if (needsInitialize) {
            initializeOpenGLFunctions();
            initialize();
        }

        render();

        m_context->swapBuffers(this);

        if (m_animating) {
            renderLater();
        }
    }

    void OpenGLWindow::setAnimating(bool animating)
    {
        m_animating = animating;

        if (animating) {
            renderLater();
        }
    }

    class TriangleWindow: public OpenGLWindow
    {
    public:
        using OpenGLWindow::OpenGLWindow;

        void initialize() override;
        void render() override;

    private:
        GLint m_posAttr = 0;
        GLint m_colAttr = 0;
        GLint m_matrixUniform = 0;

        QOpenGLShaderProgram* m_program = nullptr;
        int m_frame = 0;
    };

    TriangleWindow* gWindow = nullptr;

    int ShowWindow()
    {
        QSurfaceFormat format;
        format.setSamples(16);

        if (gWindow == nullptr) 
        {
            gWindow = new TriangleWindow();
            gWindow->setFormat(format);
            gWindow->resize(800, 600);
            gWindow->show();

            gWindow->setAnimating(true);
        }
        return 0;
    }

    static const char* vertexShaderSource = "attribute highp vec4 posAttr;\n"
                                            "attribute lowp vec4 colAttr;\n"
                                            "varying lowp vec4 col;\n"
                                            "uniform highp mat4 matrix;\n"
                                            "void main() {\n"
                                            "   col = colAttr;\n"
                                            "   gl_Position = matrix * posAttr;\n"
                                            "}\n";

    static const char* fragmentShaderSource = "varying lowp vec4 col;\n"
                                              "void main() {\n"
                                              "   gl_FragColor = col;\n"
                                              "}\n";

    void TriangleWindow::initialize()
    {
        m_program = new QOpenGLShaderProgram(this);
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
        m_program->link();
        m_posAttr = m_program->attributeLocation("posAttr");
        Q_ASSERT(m_posAttr != -1);
        m_colAttr = m_program->attributeLocation("colAttr");
        Q_ASSERT(m_colAttr != -1);
        m_matrixUniform = m_program->uniformLocation("matrix");
        Q_ASSERT(m_matrixUniform != -1);
    }

    void TriangleWindow::render()
    {
        const qreal retinaScale = devicePixelRatio();
        glViewport(0, 0, width() * retinaScale, height() * retinaScale);

        glClear(GL_COLOR_BUFFER_BIT);

        m_program->bind();

        QMatrix4x4 matrix;
        matrix.perspective(60.0f, 4.0f / 3.0f, 0.1f, 100.0f);
        matrix.translate(0, 0, -2);
        matrix.rotate(100.0f * m_frame / screen()->refreshRate(), 0, 1, 0);

        m_program->setUniformValue(m_matrixUniform, matrix);

        static const GLfloat vertices[] = {0.0f, 0.707f, -0.5f, -0.5f, 0.5f, -0.5f};

        static const GLfloat colors[] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};

        glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, vertices);
        glVertexAttribPointer(m_colAttr, 3, GL_FLOAT, GL_FALSE, 0, colors);

        glEnableVertexAttribArray(m_posAttr);
        glEnableVertexAttribArray(m_colAttr);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glDisableVertexAttribArray(m_colAttr);
        glDisableVertexAttribArray(m_posAttr);

        m_program->release();

        ++m_frame;
    }


    //************************************************************************************************************
    // stock
    //************************************************************************************************************
    cStock::cStock(float px, float py, float pz, float lx, float ly, float lz, float res)
        : m_px(px), m_py(py), m_pz(pz), m_lx(lx), m_ly(ly), m_lz(lz)
    {}

    cStock::~cStock()
    {}

}  // namespace CAMSimulator