/***************************************************************************
 *   Copyright (c) 2017 Kustaa Nyholm  <kustaa.nyholm@sparetimelabs.com>   *
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
# include <algorithm>
# include <cfloat>
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
# else
# include <GL/gl.h>
# endif
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/events/SoEvent.h>
# include <Inventor/events/SoLocation2Event.h>
# include <Inventor/events/SoMouseButtonEvent.h>
#include <QApplication>
#include <QCursor>
#include <QImage>
#include <QMenu>
#include <QOpenGLTexture>
#include <QPainterPath>
#endif

#include <Base/Tools.h>
#include <Eigen/Dense>

#include "NaviCube.h"
#include "Application.h"
#include "Command.h"
#include "MainWindow.h"

#include "View3DInventorViewer.h"
#include "View3DInventor.h"


using namespace Eigen;
using namespace std;
using namespace Gui;

// TODO
// ortho / persp
// stay in window
// corner angle
// menu actions
// size
// XYZ position
// menu feedback
// colors
// antialise cube icon
// translation
// DONE
// - permanent menu ("NaviCube_Menu"
// - improved hit testing
// - improved graphics (text now black)
// - first stab at supporting translations

class Face {
public:
	int m_FirstVertex;
	int m_VertexCount;
	GLuint m_TextureId;
	QColor m_Color;
	int m_PickId;
	int m_PickTexId;
	GLuint m_PickTextureId;
	int m_RenderPass;
	Face(
		 int firstVertex,
		 int vertexCount,
		 GLuint textureId,
		 int pickId,
		 int pickTexId,
		 GLuint pickTextureId,
		 const QColor& color,
		 int  renderPass
	)
	{
		m_FirstVertex = firstVertex;
		m_VertexCount = vertexCount;
		m_TextureId = textureId;
		m_PickId = pickId;
		m_PickTexId = pickTexId;
		m_PickTextureId = pickTextureId;
		m_Color = color;
		m_RenderPass = renderPass;
	}
};

class NaviCubeImplementation : public ParameterGrp::ObserverType {
public:
	NaviCubeImplementation(Gui::View3DInventorViewer*);
	virtual ~NaviCubeImplementation();
	void drawNaviCube();
	void createContextMenu(const std::vector<std::string>& cmd);

	/// Observer message from the ParameterGrp
	virtual void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason);

	bool processSoEvent(const SoEvent* ev);
private:
	bool mousePressed(short x, short y);
	bool mouseReleased(short x, short y);
	bool mouseMoved(short x, short y);
	int pickFace(short x, short y);
	bool inDragZone(short x, short y);

	void handleResize();
	void handleMenu();

	void setHilite(int);

	void initNaviCube(QtGLWidget*);
	void addFace(float gap, const Vector3f&, const Vector3f&, int, int, int, bool flag = false);

	GLuint createCubeFaceTex(QtGLWidget* gl, float gap, const char* text, int shape);
	GLuint createButtonTex(QtGLWidget*, int);
	GLuint createMenuTex(QtGLWidget*, bool);

	SbRotation setView(float, float) const;
	SbRotation rotateView(SbRotation, int axis, float rotAngle, SbVec3f customAxis = SbVec3f(0, 0, 0)) const;
	void rotateView(const SbRotation&);

	QString str(char* str);
	char* enum2str(int);
	QMenu* createNaviCubeMenu();
public:
	enum { //
		TEX_FRONT = 1, // 0 is reserved for 'nothing picked'
		TEX_REAR,
		TEX_TOP,
		TEX_BOTTOM,
		TEX_LEFT,
		TEX_RIGHT,
		TEX_FRONT_FACE,
		TEX_CORNER_FACE,
		TEX_EDGE_FACE,
		TEX_FRONT_TOP,
		TEX_FRONT_BOTTOM,
		TEX_FRONT_LEFT,
		TEX_FRONT_RIGHT,
		TEX_REAR_TOP,
		TEX_REAR_BOTTOM,
		TEX_REAR_LEFT,
		TEX_REAR_RIGHT,
		TEX_TOP_LEFT,
		TEX_TOP_RIGHT,
		TEX_BOTTOM_LEFT,
		TEX_BOTTOM_RIGHT,
		TEX_BOTTOM_RIGHT_REAR,
		TEX_BOTTOM_FRONT_RIGHT,
		TEX_BOTTOM_LEFT_FRONT,
		TEX_BOTTOM_REAR_LEFT,
		TEX_TOP_RIGHT_FRONT,
		TEX_TOP_FRONT_LEFT,
		TEX_TOP_LEFT_REAR,
		TEX_TOP_REAR_RIGHT,
		TEX_ARROW_NORTH,
		TEX_ARROW_SOUTH,
		TEX_ARROW_EAST,
		TEX_ARROW_WEST,
		TEX_ARROW_RIGHT,
		TEX_ARROW_LEFT,
		TEX_DOT_BACKSIDE,
		TEX_VIEW_MENU_ICON,
		TEX_VIEW_MENU_FACE
	};
	enum {
		DIR_UP, DIR_RIGHT, DIR_OUT
	};
	enum {
		SHAPE_SQUARE, SHAPE_EDGE, SHAPE_CORNER
	};
	Gui::View3DInventorViewer* m_View3DInventorViewer;
	void drawNaviCube(bool picking);

	int m_OverSample = 4;
	int m_CubeWidgetSize = 0;
	int m_CubeWidgetPosX = 0;
	int m_CubeWidgetPosY = 0;
	int m_PrevWidth = 0;
	int m_PrevHeight = 0;
	QColor m_TextColor;
	QColor m_HiliteColor;
	QColor m_ButtonColor;
	QColor m_FrontFaceColor;
	QColor m_BorderColor;
	int m_HiliteId = 0;
	bool m_MouseDown = false;
	bool m_Dragging = false;
	bool m_MightDrag = false;
	double m_BorderWidth;
	NaviCube::Corner m_Corner = NaviCube::TopRightCorner;

	QtGLFramebufferObject* m_PickingFramebuffer;

	bool m_NaviCubeInitialised = false;

	vector<GLubyte> m_IndexArray;
	vector<Vector2f> m_TextureCoordArray;
	vector<Vector3f> m_VertexArray;
	vector<Vector3f> m_VertexArray2;
	map<int, GLuint> m_Textures;
	vector<Face*> m_Faces;
	vector<int> m_Buttons;
	vector<QOpenGLTexture*> m_glTextures;
	static vector<string> m_commands;
	static vector<string> m_labels;
	QMenu* m_Menu;
};

NaviCube::NaviCube(Gui::View3DInventorViewer* viewer) {
	m_NaviCubeImplementation = new NaviCubeImplementation(viewer);
}

NaviCube::~NaviCube() {
	delete m_NaviCubeImplementation;
}

void NaviCube::drawNaviCube() {
	m_NaviCubeImplementation->drawNaviCube();
}

void NaviCube::createContextMenu(const std::vector<std::string>& cmd) {
	m_NaviCubeImplementation->createContextMenu(cmd);
}

bool NaviCube::processSoEvent(const SoEvent* ev) {
	return m_NaviCubeImplementation->processSoEvent(ev);
}


vector<string> NaviCubeImplementation::m_commands;
vector<string> NaviCubeImplementation::m_labels;

void NaviCube::setCorner(Corner c) {
	m_NaviCubeImplementation->m_Corner = c;
	m_NaviCubeImplementation->m_PrevWidth = 0;
	m_NaviCubeImplementation->m_PrevHeight = 0;
}

NaviCubeImplementation::NaviCubeImplementation(
	Gui::View3DInventorViewer* viewer) {

	m_View3DInventorViewer = viewer;

	auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/NaviCube");
	hGrp->Attach(this);

	OnChange(*hGrp, "TextColor");
	OnChange(*hGrp, "FrontColor");
	OnChange(*hGrp, "HiliteColor");
	OnChange(*hGrp, "ButtonColor");
	OnChange(*hGrp, "CubeSize");
	OnChange(*hGrp, "BorderWidth");
	OnChange(*hGrp, "BorderColor");

	m_PickingFramebuffer = nullptr;
	m_Menu = createNaviCubeMenu();
}

NaviCubeImplementation::~NaviCubeImplementation() {
	auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/NaviCube");
	hGrp->Detach(this);

	delete m_Menu;
	if (m_PickingFramebuffer)
		delete m_PickingFramebuffer;
	for (vector<Face*>::iterator f = m_Faces.begin(); f != m_Faces.end(); f++)
		delete* f;
	for (vector<QOpenGLTexture*>::iterator t = m_glTextures.begin(); t != m_glTextures.end(); t++)
		delete* t;
}

void NaviCubeImplementation::OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType reason)
{
	const auto& rGrp = static_cast<ParameterGrp&>(rCaller);

	if (strcmp(reason, "TextColor") == 0) {
		m_TextColor.setRgba(rGrp.GetUnsigned(reason, QColor(0, 0, 0, 255).rgba()));
	}
	else if (strcmp(reason, "FrontColor") == 0) {
		m_FrontFaceColor.setRgba(rGrp.GetUnsigned(reason, QColor(226, 233, 239, 192).rgba()));
	}
	else if (strcmp(reason, "HiliteColor") == 0) {
		m_HiliteColor.setRgba(rGrp.GetUnsigned(reason, QColor(170, 226, 255, 255).rgba()));
	}
	else if (strcmp(reason, "ButtonColor") == 0) {
		m_ButtonColor.setRgba(rGrp.GetUnsigned(reason, QColor(226, 233, 239, 128).rgba()));
	}
	else if (strcmp(reason, "CubeSize") == 0) {
		m_CubeWidgetSize = (rGrp.GetInt(reason, 132));
	}
	else if (strcmp(reason, "BorderWidth") == 0) {
		m_BorderWidth = rGrp.GetFloat("BorderWidth", 1.1);
	}
	else if (strcmp(reason, "BorderColor") == 0) {
		m_BorderColor.setRgba(rGrp.GetUnsigned(reason, QColor(50, 50, 50, 255).rgba()));
	}
}

char* NaviCubeImplementation::enum2str(int e) {
	switch (e) {
	default: return "???";
	case TEX_FRONT: return "TEX_FRONT";
	case TEX_REAR: return "TEX_REAR";
	case TEX_TOP: return "TEX_TOP";
	case TEX_BOTTOM: return "TEX_BOTTOM";
	case TEX_RIGHT: return "TEX_RIGHT";
	case TEX_LEFT: return "TEX_LEFT";
	case TEX_FRONT_FACE: return "TEX_FRONT_FACE";
	case TEX_CORNER_FACE: return "TEX_CORNER_FACE";
	case TEX_EDGE_FACE: return "TEX_EDGE_FACE";
	case TEX_FRONT_TOP: return "TEX_FRONT_TOP";
	case TEX_FRONT_BOTTOM: return "TEX_FRONT_BOTTOM";
	case TEX_FRONT_LEFT: return "TEX_FRONT_LEFT";
	case TEX_FRONT_RIGHT: return "TEX_FRONT_RIGHT";
	case TEX_REAR_TOP: return "TEX_REAR_TOP";
	case TEX_REAR_BOTTOM: return "TEX_REAR_BOTTOM";
	case TEX_REAR_LEFT: return "TEX_REAR_LEFT";
	case TEX_REAR_RIGHT: return "TEX_REAR_RIGHT";
	case TEX_BOTTOM_RIGHT_REAR: return "TEX_BOTTOM_RIGHT_REAR";
	case TEX_BOTTOM_FRONT_RIGHT: return "TEX_BOTTOM_FRONT_RIGHT";
	case TEX_BOTTOM_LEFT_FRONT: return "TEX_BOTTOM_LEFT_FRONT";
	case TEX_BOTTOM_REAR_LEFT: return "TEX_BOTTOM_REAR_LEFT";
	case TEX_TOP_RIGHT_FRONT: return "TEX_TOP_RIGHT_FRONT";
	case TEX_TOP_FRONT_LEFT: return "TEX_TOP_FRONT_LEFT";
	case TEX_TOP_LEFT_REAR: return "TEX_TOP_LEFT_REAR";
	case TEX_TOP_REAR_RIGHT: return "TEX_TOP_REAR_RIGHT";
	case TEX_ARROW_NORTH: return "TEX_ARROW_NORTH";
	case TEX_ARROW_SOUTH: return "TEX_ARROW_SOUTH";
	case TEX_ARROW_EAST: return "TEX_ARROW_EAST";
	case TEX_ARROW_WEST: return "TEX_ARROW_WEST";
	case TEX_ARROW_RIGHT: return "TEX_ARROW_RIGHT";
	case TEX_ARROW_LEFT: return "TEX_ARROW_LEFT";
	case TEX_DOT_BACKSIDE: return "TEX_DOT_BACKSIDE";
	case TEX_VIEW_MENU_ICON: return "TEX_VIEW_MENU_ICON";
	case TEX_VIEW_MENU_FACE: return "TEX_VIEW_MENU";
	}
}

GLuint NaviCubeImplementation::createCubeFaceTex(QtGLWidget* gl, float gap, const char* text, int shape) {
	int texSize = m_CubeWidgetSize * m_OverSample;
	float gapi = texSize * gap;
	QImage image(texSize, texSize, QImage::Format_ARGB32);
	image.fill(qRgba(255, 255, 255, 0));
	QPainter paint;
	QPen pen(Qt::black, 10);
	paint.begin(&image);
	paint.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

	if (text) {
		ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/NaviCube");
		paint.setPen(Qt::white);
		QFont sansFont(str("Helvetica"), 0.18 * texSize);
		QString fontString = QString::fromUtf8((hGrp->GetASCII("FontString")).c_str());
		if (fontString.isEmpty()) {
			// Improving readability
			sansFont.setWeight(hGrp->GetInt("FontWeight", 87));
			sansFont.setStretch(hGrp->GetInt("FontStretch", 62));
		}
		else {
			sansFont.fromString(fontString);
		}
		// Override fromString
		if (hGrp->GetInt("FontWeight") > 0) {
			sansFont.setWeight(hGrp->GetInt("FontWeight"));
		}
		if (hGrp->GetInt("FontStretch") > 0) {
			sansFont.setStretch(hGrp->GetInt("FontStretch"));
		}
		paint.setFont(sansFont);
		paint.drawText(QRect(0, 0, texSize, texSize), Qt::AlignCenter, qApp->translate("Gui::NaviCube", text));
	}
	else if (shape == SHAPE_SQUARE) {
		QPainterPath pathSquare;
		pathSquare.addRect(QRectF(gapi, gapi, (qreal)texSize - 2.0 * gapi, (qreal)texSize - 2.0 * gapi));
		paint.fillPath(pathSquare, Qt::white);
	}
	else if (shape == SHAPE_CORNER) {
		QPainterPath pathCorner;
		QRectF rectCorner = QRectF(3.46 * gapi, 3.31 * gapi, sqrt(2) * gapi, 1.3 * gapi);
		pathCorner.moveTo(rectCorner.left() + (rectCorner.width() / 2), rectCorner.top());
		pathCorner.lineTo(rectCorner.bottomLeft());
		pathCorner.lineTo(rectCorner.bottomRight());
		pathCorner.lineTo(rectCorner.left() + (rectCorner.width() / 2), rectCorner.top());
		paint.fillPath(pathCorner, Qt::white);
		paint.setPen(pen);
		paint.drawPath(pathCorner);
	}
	else if (shape == SHAPE_EDGE) {
		QPainterPath pathEdge;
		// since the gap is 0.12, the rect must be geometriclly shifted up with a factor
		pathEdge.addRect(QRectF(gapi, ((qreal)texSize - sqrt(2) * gapi) * 0.5, (qreal)texSize - 2.0 * gapi, sqrt(2) * gapi));
		paint.fillPath(pathEdge, Qt::white);
	}

	paint.end();
	Q_UNUSED(gl);
	QOpenGLTexture* texture = new QOpenGLTexture(image.mirrored());
	m_glTextures.push_back(texture);
	texture->generateMipMaps();
	texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
	texture->setMagnificationFilter(QOpenGLTexture::Linear);
	return texture->textureId();
}


GLuint NaviCubeImplementation::createButtonTex(QtGLWidget* gl, int button) {
	int texSize = m_CubeWidgetSize * m_OverSample;
	QImage image(texSize, texSize, QImage::Format_ARGB32);
	image.fill(qRgba(255, 255, 255, 0));
	QPainter painter;
	painter.begin(&image);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

	QTransform transform;
	transform.translate(texSize / 2, texSize / 2);
	transform.scale(texSize / 2, texSize / 2);
	painter.setTransform(transform);

	QPainterPath path;

	float as1 = 0.18f; // arrow size
	float as3 = as1 / 3;

	switch (button) {
	default:
		break;
	case TEX_ARROW_RIGHT:
	case TEX_ARROW_LEFT: {
		QRectF r(-1.00, -1.00, 2.00, 2.00);
		QRectF r0(r);
		r.adjust(as3, as3, -as3, -as3);
		QRectF r1(r);
		r.adjust(as3, as3, -as3, -as3);
		QRectF r2(r);
		r.adjust(as3, as3, -as3, -as3);
		QRectF r3(r);
		r.adjust(as3, as3, -as3, -as3);
		QRectF r4(r);

		float a0 = 72;
		float a1 = 45;
		float a2 = 32;

		if (TEX_ARROW_LEFT == button) {
			a0 = 180 - a0;
			a1 = 180 - a1;
			a2 = 180 - a2;
		}

		path.arcMoveTo(r0, a1);
		QPointF p0 = path.currentPosition();

		path.arcMoveTo(r2, a2);
		QPointF p1 = path.currentPosition();

		path.arcMoveTo(r4, a1);
		QPointF p2 = path.currentPosition();

		path.arcMoveTo(r1, a0);
		path.arcTo(r1, a0, -(a0 - a1));
		path.lineTo(p0);
		path.lineTo(p1);
		path.lineTo(p2);
		path.arcTo(r3, a1, +(a0 - a1));
		break;
	}
	case TEX_ARROW_EAST: {
		path.moveTo(1, 0);
		path.lineTo(1 - as1, +as1);
		path.lineTo(1 - as1, -as1);
		break;
	}
	case TEX_ARROW_WEST: {
		path.moveTo(-1, 0);
		path.lineTo(-1 + as1, -as1);
		path.lineTo(-1 + as1, +as1);
		break;
	}
	case TEX_ARROW_SOUTH: {
		path.moveTo(0, 1);
		path.lineTo(-as1, 1 - as1);
		path.lineTo(+as1, 1 - as1);
		break;
	}
	case TEX_ARROW_NORTH: {
		path.moveTo(0, -1);
		path.lineTo(+as1, -1 + as1);
		path.lineTo(-as1, -1 + as1);
		break;
	}
	case TEX_DOT_BACKSIDE: {
		path.arcTo(QRectF(1 - as1, -1, as1, as1), 0, 360);
		break;
	}
	}

	painter.fillPath(path, Qt::white);

	painter.end();
	//image.save(str(enum2str(button))+str(".png"));

	Q_UNUSED(gl);
	QOpenGLTexture* texture = new QOpenGLTexture(image.mirrored());
	m_glTextures.push_back(texture);
	texture->generateMipMaps();
	texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
	texture->setMagnificationFilter(QOpenGLTexture::Linear);
	return texture->textureId();
}

GLuint NaviCubeImplementation::createMenuTex(QtGLWidget* gl, bool forPicking) {
	int texSize = m_CubeWidgetSize * m_OverSample;
	QImage image(texSize, texSize, QImage::Format_ARGB32);
	image.fill(qRgba(0, 0, 0, 0));
	QPainter painter;
	painter.begin(&image);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

	QTransform transform;
	transform.translate(texSize * 12 / 16, texSize * 13 / 16);
	transform.scale(texSize / 200.0, texSize / 200.0); // 200 == size at which this was designed
	painter.setTransform(transform);

	QPainterPath path;

	if (forPicking) {
		path.addRoundedRect(-25, -8, 75, 45, 6, 6);
		painter.fillPath(path, Qt::white);
	}
	else {
		// top
		path.moveTo(0, 0);
		path.lineTo(15, 5);
		path.lineTo(0, 10);
		path.lineTo(-15, 5);

		painter.fillPath(path, QColor(240, 240, 240));

		// left
		QPainterPath path2;
		path2.lineTo(0, 10);
		path2.lineTo(-15, 5);
		path2.lineTo(-15, 25);
		path2.lineTo(0, 30);
		painter.fillPath(path2, QColor(190, 190, 190));

		// right
		QPainterPath path3;
		path3.lineTo(0, 10);
		path3.lineTo(15, 5);
		path3.lineTo(15, 25);
		path3.lineTo(0, 30);
		painter.fillPath(path3, QColor(220, 220, 220));

		// outline
		QPainterPath path4;
		path4.moveTo(0, 0);
		path4.lineTo(15, 5);
		path4.lineTo(15, 25);
		path4.lineTo(0, 30);
		path4.lineTo(-15, 25);
		path4.lineTo(-15, 5);
		path4.lineTo(0, 0);
		painter.strokePath(path4, QColor(128, 128, 128));

		// menu triangle
		QPainterPath path5;
		path5.moveTo(20, 10);
		path5.lineTo(40, 10);
		path5.lineTo(30, 20);
		path5.lineTo(20, 10);
		painter.fillPath(path5, QColor(64, 64, 64));
	}
	painter.end();
	Q_UNUSED(gl);
	QOpenGLTexture* texture = new QOpenGLTexture(image.mirrored());
	m_glTextures.push_back(texture);
	texture->generateMipMaps();
	texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
	texture->setMagnificationFilter(QOpenGLTexture::Linear);
	return texture->textureId();
}

void NaviCubeImplementation::addFace(float gap, const Vector3f& x, const Vector3f& z, int frontTex, int pickTex, int pickId, bool text) {
	Vector3f y = x.cross(-z);
	y = y / y.norm() * x.norm();

	auto x2 = x * (1 - gap * 2);
	auto y2 = x2.cross(-z);
	y2 = y2 / y2.norm() * x2.norm();

	int t = m_VertexArray.size();

	m_VertexArray.push_back(z - x - y);
	m_VertexArray2.push_back(z - x2 - y2);
	m_TextureCoordArray.emplace_back(0, 0);
	m_VertexArray.push_back(z + x - y);
	m_VertexArray2.push_back(z + x2 - y2);
	m_TextureCoordArray.emplace_back(1, 0);
	m_VertexArray.push_back(z + x + y);
	m_VertexArray2.push_back(z + x2 + y2);
	m_TextureCoordArray.emplace_back(1, 1);
	m_VertexArray.push_back(z - x + y);
	m_VertexArray2.push_back(z - x2 + y2);
	m_TextureCoordArray.emplace_back(0, 1);

	// TEX_TOP, TEX_FRONT_FACE, TEX_TOP
	// TEX_TOP 			frontTex,
	// TEX_FRONT_FACE	pickTex,
	// TEX_TOP 			pickId
	Face* FaceFront = new Face(
		m_IndexArray.size(),
		4,
		m_Textures[pickTex],
		pickId,
		pickTex,
		m_Textures[pickTex],
		m_FrontFaceColor,
		1);
	m_Faces.push_back(FaceFront);

	if (text) {
		Face* FaceText = new Face(
			m_IndexArray.size(),
			4,
			m_Textures[frontTex],
			pickId,
			pickTex,
			m_Textures[pickTex],
			m_TextColor,
			2);
		m_Faces.push_back(FaceText);

	}

	for (int i = 0; i < 4; i++)
		m_IndexArray.push_back(t + i);
}

void NaviCubeImplementation::initNaviCube(QtGLWidget* gl) {
	Vector3f x(1, 0, 0);
	Vector3f y(0, 1, 0);
	Vector3f z(0, 0, 1);

	float cs, sn;
	cs = cos(90 * M_PI / 180);
	sn = sin(90 * M_PI / 180);
	Matrix3f r90x;
	r90x << 1, 0, 0,
			0, cs, -sn,
			0, sn, cs;

	Matrix3f r90y;
	r90y << cs, 0, sn,
		     0, 1, 0,
		   -sn, 0, cs;

	Matrix3f r90z;
	r90z << cs, sn, 0,
			-sn, cs, 0,
			0, 0, 1;

	cs = cos(45 * M_PI / 180);
	sn = sin(45 * M_PI / 180);
	Matrix3f r45x;
	r45x << 1, 0, 0,
		    0, cs, -sn,
		    0, sn, cs;

	Matrix3f r45z;
	r45z << cs, sn, 0,
			-sn, cs, 0,
			0, 0, 1;

	// first create front and backside of faces
	float gap = 0.12f;
	m_Textures[TEX_FRONT_FACE] = createCubeFaceTex(gl, gap, nullptr, SHAPE_SQUARE);

	vector<string> labels = NaviCubeImplementation::m_labels;

	if (labels.size() != 6) {
		labels.clear();
		ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/NaviCube");
		labels.push_back(hGrp->GetASCII("TextFront", "FRONT"));
		labels.push_back(hGrp->GetASCII("TextRear", "REAR"));
		labels.push_back(hGrp->GetASCII("TextTop", "TOP"));
		labels.push_back(hGrp->GetASCII("TextBottom", "BOTTOM"));
		labels.push_back(hGrp->GetASCII("TextRight", "RIGHT"));
		labels.push_back(hGrp->GetASCII("TextLeft", "LEFT"));
	}
	// create the main faces
	m_Textures[TEX_FRONT] = createCubeFaceTex(gl, gap, labels[0].c_str(), SHAPE_SQUARE);
	m_Textures[TEX_REAR] = createCubeFaceTex(gl, gap, labels[1].c_str(), SHAPE_SQUARE);
	m_Textures[TEX_TOP] = createCubeFaceTex(gl, gap, labels[2].c_str(), SHAPE_SQUARE);
	m_Textures[TEX_BOTTOM] = createCubeFaceTex(gl, gap, labels[3].c_str(), SHAPE_SQUARE);
	m_Textures[TEX_RIGHT] = createCubeFaceTex(gl, gap, labels[4].c_str(), SHAPE_SQUARE);
	m_Textures[TEX_LEFT] = createCubeFaceTex(gl, gap, labels[5].c_str(), SHAPE_SQUARE);

	// create the arrows
	m_Textures[TEX_ARROW_NORTH] = createButtonTex(gl, TEX_ARROW_NORTH);
	m_Textures[TEX_ARROW_SOUTH] = createButtonTex(gl, TEX_ARROW_SOUTH);
	m_Textures[TEX_ARROW_EAST] = createButtonTex(gl, TEX_ARROW_EAST);
	m_Textures[TEX_ARROW_WEST] = createButtonTex(gl, TEX_ARROW_WEST);
	m_Textures[TEX_ARROW_LEFT] = createButtonTex(gl, TEX_ARROW_LEFT);
	m_Textures[TEX_ARROW_RIGHT] = createButtonTex(gl, TEX_ARROW_RIGHT);
	m_Textures[TEX_DOT_BACKSIDE] = createButtonTex(gl, TEX_DOT_BACKSIDE);
	m_Textures[TEX_VIEW_MENU_ICON] = createMenuTex(gl, false);
	m_Textures[TEX_VIEW_MENU_FACE] = createMenuTex(gl, true);

	// front,back,pick,pickid
	addFace(gap, x, z, TEX_TOP, TEX_FRONT_FACE, TEX_TOP, true);
	x = r90x * x;
	z = r90x * z;
	addFace(gap, x, z, TEX_FRONT, TEX_FRONT_FACE, TEX_FRONT, true);
	x = r90z * x;
	z = r90z * z;
	addFace(gap, x, z, TEX_LEFT, TEX_FRONT_FACE, TEX_LEFT, true);
	x = r90z * x;
	z = r90z * z;
	addFace(gap, x, z, TEX_REAR, TEX_FRONT_FACE, TEX_REAR, true);
	x = r90z * x;
	z = r90z * z;
	addFace(gap, x, z, TEX_RIGHT, TEX_FRONT_FACE, TEX_RIGHT, true);
	x = r90x * r90z * x;
	z = r90x * r90z * z;
	addFace(gap, x, z, TEX_BOTTOM, TEX_FRONT_FACE, TEX_BOTTOM, true);

	// add corner faces
	m_Textures[TEX_CORNER_FACE] = createCubeFaceTex(gl, gap, nullptr, SHAPE_CORNER);
	// we need to rotate to the edge, thus matrix for rotation angle of 54.7 deg
	cs = cos(atan(sqrt(2.0)));
	sn = sin(atan(sqrt(2.0)));
	Matrix3f r54x;
	r54x << 1, 0, 0,
		     0, cs, -sn,
		     0, sn, cs;

	z = r45z * r54x * z;
	x = r45z * r54x * x;
	z *= sqrt(3) * (1 - 4 * gap / 3); // corner face position

	addFace(gap, x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_BOTTOM_RIGHT_REAR);
	x = r90z * x;
	z = r90z * z;
	addFace(gap, x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_BOTTOM_FRONT_RIGHT);
	x = r90z * x;
	z = r90z * z;
	addFace(gap, x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_BOTTOM_LEFT_FRONT);
	x = r90z * x;
	z = r90z * z;
	addFace(gap, x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_BOTTOM_REAR_LEFT);
	x = r90x * r90x * r90z * x;
	z = r90x * r90x * r90z * z;
	addFace(gap, x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_TOP_RIGHT_FRONT);
	x = r90z * x;
	z = r90z * z;
	addFace(gap, x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_TOP_FRONT_LEFT);
	x = r90z * x;
	z = r90z * z;
	addFace(gap, x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_TOP_LEFT_REAR);
	x = r90z * x;
	z = r90z * z;
	addFace(gap, x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_TOP_REAR_RIGHT);

	// add edge faces
	m_Textures[TEX_EDGE_FACE] = createCubeFaceTex(gl, gap, nullptr, SHAPE_EDGE);
	// first back to top side
	x[0] = 1; x[1] = 0; x[2] = 0;
	z[0] = 0; z[1] = 0; z[2] = 1;
	// rotate 45 degrees up
	z = r45x * z;
	x = r45x * x;
	z *= sqrt(2) * (1 - gap);
	addFace(gap, x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_FRONT_TOP);
	x = r90x * x;
	z = r90x * z;
	addFace(gap, x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_FRONT_BOTTOM);
	x = r90x * x;
	z = r90x * z;
	addFace(gap, x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_REAR_BOTTOM);
	x = r90x * x;
	z = r90x * z;
	addFace(gap, x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_REAR_TOP);
	x = r90y * x;
	z = r90y * z;
	addFace(gap, x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_REAR_RIGHT);
	x = r90z * x;
	z = r90z * z;
	addFace(gap, x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_FRONT_RIGHT);
	x = r90z * x;
	z = r90z * z;
	addFace(gap, x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_FRONT_LEFT);
	x = r90z * x;
	z = r90z * z;
	addFace(gap, x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_REAR_LEFT);
	x = r90x * x;
	z = r90x * z;
	addFace(gap, x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_TOP_LEFT);
	x = r90y * x;
	z = r90y * z;
	addFace(gap, x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_TOP_RIGHT);
	x = r90y * x;
	z = r90y * z;
	addFace(gap, x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_BOTTOM_RIGHT);
	x = r90y * x;
	z = r90y * z;
	addFace(gap, x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_BOTTOM_LEFT);

	m_Buttons.push_back(TEX_ARROW_NORTH);
	m_Buttons.push_back(TEX_ARROW_SOUTH);
	m_Buttons.push_back(TEX_ARROW_EAST);
	m_Buttons.push_back(TEX_ARROW_WEST);
	m_Buttons.push_back(TEX_ARROW_LEFT);
	m_Buttons.push_back(TEX_ARROW_RIGHT);
	m_Buttons.push_back(TEX_DOT_BACKSIDE);

	m_PickingFramebuffer = new QtGLFramebufferObject(2 * m_CubeWidgetSize, 2 * m_CubeWidgetSize, QtGLFramebufferObject::CombinedDepthStencil);
}

void NaviCubeImplementation::drawNaviCube() {
	glViewport(m_CubeWidgetPosX - m_CubeWidgetSize / 2, m_CubeWidgetPosY - m_CubeWidgetSize / 2, m_CubeWidgetSize, m_CubeWidgetSize);
	drawNaviCube(false);
}

void NaviCubeImplementation::createContextMenu(const std::vector<std::string>& cmd) {
	CommandManager& rcCmdMgr = Application::Instance->commandManager();
	m_Menu->clear();

	for (vector<string>::const_iterator i = cmd.begin(); i != cmd.end(); i++) {
		Command* cmd = rcCmdMgr.getCommandByName(i->c_str());
		if (cmd)
			cmd->addTo(m_Menu);
	}
}

void NaviCubeImplementation::handleResize() {
	SbVec2s view = m_View3DInventorViewer->getSoRenderManager()->getSize();
	if ((m_PrevWidth != view[0]) || (m_PrevHeight != view[1])) {
		if ((m_PrevWidth > 0) && (m_PrevHeight > 0)) {
			// maintain position relative to closest edge
			if (m_CubeWidgetPosX > m_PrevWidth / 2)
				m_CubeWidgetPosX = view[0] - (m_PrevWidth - m_CubeWidgetPosX);
			if (m_CubeWidgetPosY > m_PrevHeight / 2)
				m_CubeWidgetPosY = view[1] - (m_PrevHeight - m_CubeWidgetPosY);
		}
		else { // initial position
			ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/NaviCube");
			int m_CubeWidgetOffsetX = hGrp->GetInt("OffsetX", 0);
			int m_CubeWidgetOffsetY = hGrp->GetInt("OffsetY", 0);
			switch (m_Corner) {
			case NaviCube::TopLeftCorner:
				m_CubeWidgetPosX = m_CubeWidgetSize * 1.1 / 2 + m_CubeWidgetOffsetX;
				m_CubeWidgetPosY = view[1] - m_CubeWidgetSize * 1.1 / 2 - m_CubeWidgetOffsetY;
				break;
			case NaviCube::TopRightCorner:
				m_CubeWidgetPosX = view[0] - m_CubeWidgetSize * 1.1 / 2 - m_CubeWidgetOffsetX;
				m_CubeWidgetPosY = view[1] - m_CubeWidgetSize * 1.1 / 2 - m_CubeWidgetOffsetY;
				break;
			case NaviCube::BottomLeftCorner:
				m_CubeWidgetPosX = m_CubeWidgetSize * 1.1 / 2 + m_CubeWidgetOffsetX;
				m_CubeWidgetPosY = m_CubeWidgetSize * 1.1 / 2 + m_CubeWidgetOffsetY;
				break;
			case NaviCube::BottomRightCorner:
				m_CubeWidgetPosX = view[0] - m_CubeWidgetSize * 1.1 / 2 - m_CubeWidgetOffsetX;
				m_CubeWidgetPosY = m_CubeWidgetSize * 1.1 / 2 + m_CubeWidgetOffsetY;
				break;
			}
		}
		m_PrevWidth = view[0];
		m_PrevHeight = view[1];
		m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
	}
}

void NaviCubeImplementation::drawNaviCube(bool pickMode) {
	// initializes stuff here when we actually have a context
	// FIXME actually now that we have Qt5, we could probably do this earlier (as we do not need the opengl context)
	if (!m_NaviCubeInitialised) {
		QtGLWidget* gl = static_cast<QtGLWidget*>(m_View3DInventorViewer->viewport());
		if (gl == nullptr)
			return;
		initNaviCube(gl);
		m_NaviCubeInitialised = true;
	}

	SoCamera* cam = m_View3DInventorViewer->getSoRenderManager()->getCamera();

	if (!cam)
		return;

	handleResize();

	// Store GL state.
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	GLfloat depthrange[2];
	glGetFloatv(GL_DEPTH_RANGE, depthrange);
	GLdouble projectionmatrix[16];
	glGetDoublev(GL_PROJECTION_MATRIX, projectionmatrix);

	glDepthMask(GL_TRUE);
	glDepthRange(0.0, 1.0);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glLineWidth(2.0);

	glDisable(GL_LIGHTING);
	//glDisable(GL_BLEND);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//glTexEnvf(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDepthMask(GL_TRUE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glShadeModel(GL_SMOOTH);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	glAlphaFunc(GL_GREATER, 0.25);
	glEnable(GL_ALPHA_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	const float NEARVAL = 0.1f;
	const float FARVAL = 10.0f;
	const float dim = NEARVAL * float(tan(M_PI / 8.0)) * 1.2;
	glFrustum(-dim, dim, -dim, dim, NEARVAL, FARVAL);

	SbMatrix mx;
	mx = cam->orientation.getValue();

	mx = mx.inverse();
	mx[3][2] = -5.0;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf((float*)mx);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	if (pickMode) {
		glDisable(GL_BLEND);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glShadeModel(GL_FLAT);
		glDisable(GL_DITHER);
		glDisable(GL_POLYGON_SMOOTH);
	}
	else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	glClear(GL_DEPTH_BUFFER_BIT);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, (void*)m_VertexArray.data());
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, m_TextureCoordArray.data());

	if (!pickMode) {
		// Draw the axes
		ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/NaviCube");
		bool ShowCS = hGrp->GetBool("ShowCS", 1);
		if (ShowCS) {
			glDisable(GL_TEXTURE_2D);
			float a = 1.1f;

			glColor3f(1, 0, 0);
			glBegin(GL_LINES);
			glVertex3f(-1.1f, -1.1f, -1.1f);
			glVertex3f(+0.5f, -1.1f, -1.1f);
			glEnd();
			glRasterPos3d(a, -a, -a);

			glColor3f(0, 1, 0);
			glBegin(GL_LINES);
			glVertex3f(-1.1f, -1.1f, -1.1f);
			glVertex3f(-1.1f, +0.5f, -1.1f);
			glEnd();
			glRasterPos3d(-a, a, -a);

			glColor3f(0, 0, 1);
			glBegin(GL_LINES);
			glVertex3f(-1.1f, -1.1f, -1.1f);
			glVertex3f(-1.1f, -1.1f, +0.5f);
			glEnd();
			glRasterPos3d(-a, -a, a);

			glEnable(GL_TEXTURE_2D);
		}
	}

	// Draw the cube faces
	if (pickMode) {
		for (vector<Face*>::iterator f = m_Faces.begin(); f != m_Faces.end(); f++) {
			glColor3ub((*f)->m_PickId, 0, 0);
			glBindTexture(GL_TEXTURE_2D, (*f)->m_PickTextureId);
			glDrawElements(GL_TRIANGLE_FAN, (*f)->m_VertexCount, GL_UNSIGNED_BYTE, (void*)&m_IndexArray[(*f)->m_FirstVertex]);
		}
	}
	else {
		for (int pass = 0; pass < 3; pass++) {
			for (vector<Face*>::iterator f = m_Faces.begin(); f != m_Faces.end(); f++) {
				//if (pickMode) { // pick should not be drawn in tree passes
				//	glColor3ub((*f)->m_PickId, 0, 0);
				//	glBindTexture(GL_TEXTURE_2D, (*f)->m_PickTextureId);
				//} else {
				if (pass != (*f)->m_RenderPass)
					continue;
				QColor& c = (m_HiliteId == (*f)->m_PickId) && (pass < 2) ? m_HiliteColor : (*f)->m_Color;
				glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
				glBindTexture(GL_TEXTURE_2D, (*f)->m_TextureId);
				//}
				glDrawElements(GL_TRIANGLE_FAN, (*f)->m_VertexCount, GL_UNSIGNED_BYTE, (void*)&m_IndexArray[(*f)->m_FirstVertex]);
			}
		}
	}
	// Draw the rotate buttons
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if (!pickMode && m_BorderWidth >= 1.0f) {
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_TEXTURE_2D);
		const auto& c = m_BorderColor;
		glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
		glLineWidth(m_BorderWidth);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glBegin(GL_QUADS);
		for (int pass = 0; pass < 3; pass++) {
			for (const auto& f : m_Faces) {
				if (pass != f->m_RenderPass)
					continue;
				if (f->m_TextureId == f->m_PickTextureId) {
					if (f->m_PickTexId == TEX_FRONT_FACE) {
						int idx = f->m_FirstVertex;
						const Vector3f& mv1 = m_VertexArray2[m_IndexArray[idx]];
						const Vector3f& mv2 = m_VertexArray2[m_IndexArray[idx + 1]];
						const Vector3f& mv3 = m_VertexArray2[m_IndexArray[idx + 2]];
						const Vector3f& mv4 = m_VertexArray2[m_IndexArray[idx + 3]];
						glVertex3f(mv1[0], mv1[1], mv1[2]);
						glVertex3f(mv2[0], mv2[1], mv2[2]);
						glVertex3f(mv3[0], mv3[1], mv3[2]);
						glVertex3f(mv4[0], mv4[1], mv4[2]);
					}
				}
			}
		}
		glEnd();
		glEnable(GL_TEXTURE_2D);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}


	glDisable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	for (vector<int>::iterator b = m_Buttons.begin(); b != m_Buttons.end(); b++) {
		if (pickMode)
			glColor3ub(*b, 0, 0);
		else {
			QColor& c = (m_HiliteId == (*b)) ? m_HiliteColor : m_ButtonColor;
			glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
		}
		glBindTexture(GL_TEXTURE_2D, m_Textures[*b]);

		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex3f(0.0f, 1.0f, 0.0f);
		glTexCoord2f(1, 0);
		glVertex3f(1.0f, 1.0f, 0.0f);
		glTexCoord2f(1, 1);
		glVertex3f(1.0f, 0.0f, 0.0f);
		glTexCoord2f(0, 1);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glEnd();
	}

	// Draw the view menu icon
	if (pickMode) {
		glColor3ub(TEX_VIEW_MENU_FACE, 0, 0);
		glBindTexture(GL_TEXTURE_2D, m_Textures[TEX_VIEW_MENU_FACE]);
	}
	else {
		if (m_HiliteId == TEX_VIEW_MENU_FACE) {
			QColor& c = m_HiliteColor;
			glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
			glBindTexture(GL_TEXTURE_2D, m_Textures[TEX_VIEW_MENU_FACE]);

			glBegin(GL_QUADS); // DO THIS WITH VERTEX ARRAYS
			glTexCoord2f(0, 0);
			glVertex3f(0.0f, 1.0f, 0.0f);
			glTexCoord2f(1, 0);
			glVertex3f(1.0f, 1.0f, 0.0f);
			glTexCoord2f(1, 1);
			glVertex3f(1.0f, 0.0f, 0.0f);
			glTexCoord2f(0, 1);
			glVertex3f(0.0f, 0.0f, 0.0f);
			glEnd();
		}

		QColor& c = m_ButtonColor;
		glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
		glBindTexture(GL_TEXTURE_2D, m_Textures[TEX_VIEW_MENU_ICON]);
	}

	glBegin(GL_QUADS); // FIXME do this with vertex arrays
	glTexCoord2f(0, 0);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glTexCoord2f(1, 0);
	glVertex3f(1.0f, 1.0f, 0.0f);
	glTexCoord2f(1, 1);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0, 1);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glEnd();


	glPopMatrix();

	// Restore original state.

	glDepthRange(depthrange[0], depthrange[1]);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(projectionmatrix);

	glPopAttrib();
}

int NaviCubeImplementation::pickFace(short x, short y) {
	GLubyte pixels[4] = { 0 };
	if (m_PickingFramebuffer) {
		m_PickingFramebuffer->bind();

		glViewport(0, 0, 2 * m_CubeWidgetSize, 2 * m_CubeWidgetSize);
		glLoadIdentity();

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		drawNaviCube(true);

		glFinish();

		glReadPixels(2 * (x - (m_CubeWidgetPosX - m_CubeWidgetSize / 2)), 2 * (y - (m_CubeWidgetPosY - m_CubeWidgetSize / 2)), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixels);
		m_PickingFramebuffer->release();

		//QImage image = m_PickingFramebuffer->toImage();
		//image.save(QLatin1String("pickimage.png"));
	}
	return pixels[3] == 255 ? pixels[0] : 0;
}

bool NaviCubeImplementation::mousePressed(short x, short y) {
	m_MouseDown = true;
	m_Dragging = false;
	m_MightDrag = inDragZone(x, y);
	int pick = pickFace(x, y);
	// cerr << enum2str(pick) << endl;
	setHilite(pick);
	return pick != 0;
}

SbRotation NaviCubeImplementation::setView(float rotZ, float rotX) const {
	SbRotation rz, rx, t;
	rz.setValue(SbVec3f(0, 0, 1), rotZ * M_PI / 180);
	rx.setValue(SbVec3f(1, 0, 0), rotX * M_PI / 180);
	return rx * rz;
}

SbRotation NaviCubeImplementation::rotateView(SbRotation viewRot, int axis, float rotAngle, SbVec3f customAxis) const {
	SbVec3f up;
	viewRot.multVec(SbVec3f(0, 1, 0), up);

	SbVec3f out;
	viewRot.multVec(SbVec3f(0, 0, 1), out);

	SbVec3f right;
	viewRot.multVec(SbVec3f(1, 0, 0), right);

	SbVec3f direction;
	switch (axis) {
	default:
		return viewRot;
	case DIR_UP:
		direction = up;
		break;
	case DIR_OUT:
		direction = out;
		break;
	case DIR_RIGHT:
		direction = right;
		break;
	}

	if (customAxis != SbVec3f(0, 0, 0))
		direction = customAxis;

	SbRotation rot(direction, -rotAngle * M_PI / 180.0);
	SbRotation newViewRot = viewRot * rot;
	return newViewRot;
}

void NaviCubeImplementation::rotateView(const SbRotation& rot) {
	m_View3DInventorViewer->setCameraOrientation(rot);
}

void NaviCubeImplementation::handleMenu() {
	m_Menu->exec(QCursor::pos());
}

bool NaviCubeImplementation::mouseReleased(short x, short y) {
	setHilite(0);
	m_MouseDown = false;

	// get the current view
	SbMatrix ViewRotMatrix;
	SbRotation CurrentViewRot = m_View3DInventorViewer->getCameraOrientation();
	CurrentViewRot.getValue(ViewRotMatrix);

	if (!m_Dragging) {
		float rot = 45;
		float tilt = 90 - Base::toDegrees(atan(sqrt(2.0)));
		int pick = pickFace(x, y);

		ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
		long step = Base::clamp(hGrp->GetInt("NaviStepByTurn", 8), 4L, 36L);
		float rotStepAngle = 360.0f / step;
		ParameterGrp::handle hGrpNavi = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/NaviCube");
		bool toNearest = hGrpNavi->GetBool("NaviRotateToNearest", true);
		bool applyRotation = true;

		SbRotation viewRot = CurrentViewRot;

		switch (pick) {
		default:
			return false;
			break;
		case TEX_FRONT:
			viewRot = setView(0, 90);
			// we don't want to dumb rotate to the same view since depending on from where the user clicked on FRONT
			// we have one of four suitable end positions.
			// we use here the same rotation logic used by other programs using OCC like "CAD Assistant"
			// when current matrix's 0,0 entry is larger than its |1,0| entry, we already have the final result
			// otherwise rotate around y
			if (toNearest) {
				if (ViewRotMatrix[0][0] < 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_REAR:
			viewRot = setView(180, 90);
			if (toNearest) {
				if (ViewRotMatrix[0][0] > 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_LEFT:
			viewRot = setView(270, 90);
			if (toNearest) {
				if (ViewRotMatrix[0][1] > 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_RIGHT:
			viewRot = setView(90, 90);
			if (toNearest) {
				if (ViewRotMatrix[0][1] < 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_TOP:
			viewRot = setView(0, 0);
			if (toNearest) {
				if (ViewRotMatrix[0][0] < 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_BOTTOM:
			viewRot = setView(0, 180);
			if (toNearest) {
				if (ViewRotMatrix[0][0] < 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_FRONT_TOP:
			// set to FRONT then rotate
			viewRot = setView(0, 90);
			viewRot = rotateView(viewRot, 1, 45);
			if (toNearest) {
				if (ViewRotMatrix[0][0] < 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_FRONT_BOTTOM:
			// set to FRONT then rotate
			viewRot = setView(0, 90);
			viewRot = rotateView(viewRot, 1, -45);
			if (toNearest) {
				if (ViewRotMatrix[0][0] < 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_REAR_BOTTOM:
			// set to REAR then rotate
			viewRot = setView(180, 90);
			viewRot = rotateView(viewRot, 1, -45);
			if (toNearest) {
				if (ViewRotMatrix[0][0] > 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_REAR_TOP:
			// set to REAR then rotate
			viewRot = setView(180, 90);
			viewRot = rotateView(viewRot, 1, 45);
			if (toNearest) {
				if (ViewRotMatrix[0][0] > 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_FRONT_LEFT:
			// set to FRONT then rotate
			viewRot = setView(0, 90);
			viewRot = rotateView(viewRot, 0, 45);
			if (toNearest) {
				if (ViewRotMatrix[1][2] < 0 && abs(ViewRotMatrix[1][2]) >= abs(ViewRotMatrix[0][2]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[0][2] > 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[0][2] < 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_FRONT_RIGHT:
			// set to FRONT then rotate
			viewRot = setView(0, 90);
			viewRot = rotateView(viewRot, 0, -45);
			if (toNearest) {
				if (ViewRotMatrix[1][2] < 0 && abs(ViewRotMatrix[1][2]) >= abs(ViewRotMatrix[0][2]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[0][2] > 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[0][2] < 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_REAR_RIGHT:
			// set to REAR then rotate
			viewRot = setView(180, 90);
			viewRot = rotateView(viewRot, 0, 45);
			if (toNearest) {
				if (ViewRotMatrix[1][2] < 0 && abs(ViewRotMatrix[1][2]) >= abs(ViewRotMatrix[0][2]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[0][2] > 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[0][2] < 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_REAR_LEFT:
			// set to REAR then rotate
			viewRot = setView(180, 90);
			viewRot = rotateView(viewRot, 0, -45);
			if (ViewRotMatrix[1][2] < 0 && abs(ViewRotMatrix[1][2]) >= abs(ViewRotMatrix[0][2]))
				viewRot = rotateView(viewRot, 2, 180);
			else if (ViewRotMatrix[0][2] > 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
				viewRot = rotateView(viewRot, 2, -90);
			else if (ViewRotMatrix[0][2] < 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
				viewRot = rotateView(viewRot, 2, 90);
			break;
		case TEX_TOP_LEFT:
			// set to LEFT then rotate
			viewRot = setView(270, 90);
			viewRot = rotateView(viewRot, 1, 45);
			if (toNearest) {
				if (ViewRotMatrix[0][1] > 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_TOP_RIGHT:
			// set to RIGHT then rotate
			viewRot = setView(90, 90);
			viewRot = rotateView(viewRot, 1, 45);
			if (toNearest) {
				if (ViewRotMatrix[0][1] < 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_BOTTOM_RIGHT:
			// set to RIGHT then rotate
			viewRot = setView(90, 90);
			viewRot = rotateView(viewRot, 1, -45);
			if (toNearest) {
				if (ViewRotMatrix[0][1] < 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_BOTTOM_LEFT:
			// set to LEFT then rotate
			viewRot = setView(270, 90);
			viewRot = rotateView(viewRot, 1, -45);
			if (toNearest) {
				if (ViewRotMatrix[0][1] > 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_BOTTOM_LEFT_FRONT:
			viewRot = setView(rot - 90, 90 + tilt);
			// we have 3 possible end states:
			// - z-axis is not rotated larger than 120 deg from (0, 1, 0) -> we are already there
			// - y-axis is not rotated larger than 120 deg from (0, 1, 0)
			// - x-axis is not rotated larger than 120 deg from (0, 1, 0)
			if (toNearest) {
				if (ViewRotMatrix[1][0] > 0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(1, 1, 1));
				else if (ViewRotMatrix[1][1] > 0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(1, 1, 1));
			}
			break;
		case TEX_BOTTOM_FRONT_RIGHT:
			viewRot = setView(90 + rot - 90, 90 + tilt);
			if (toNearest) {
				if (ViewRotMatrix[1][0] < -0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(-1, 1, 1));
				else if (ViewRotMatrix[1][1] > 0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(-1, 1, 1));
			}
			break;
		case TEX_BOTTOM_RIGHT_REAR:
			viewRot = setView(180 + rot - 90, 90 + tilt);
			if (toNearest) {
				if (ViewRotMatrix[1][0] < -0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(-1, -1, 1));
				else if (ViewRotMatrix[1][1] < -0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(-1, -1, 1));
			}
			break;
		case TEX_BOTTOM_REAR_LEFT:
			viewRot = setView(270 + rot - 90, 90 + tilt);
			if (toNearest) {
				if (ViewRotMatrix[1][0] > 0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(1, -1, 1));
				else if (ViewRotMatrix[1][1] < -0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(1, -1, 1));
			}
			break;
		case TEX_TOP_RIGHT_FRONT:
			viewRot = setView(rot, 90 - tilt);
			if (toNearest) {
				if (ViewRotMatrix[1][0] > 0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(-1, 1, -1));
				else if (ViewRotMatrix[1][1] < -0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(-1, 1, -1));
			}
			break;
		case TEX_TOP_FRONT_LEFT:
			viewRot = setView(rot - 90, 90 - tilt);
			if (toNearest) {
				if (ViewRotMatrix[1][0] < -0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(1, 1, -1));
				else if (ViewRotMatrix[1][1] < -0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(1, 1, -1));
			}
			break;
		case TEX_TOP_LEFT_REAR:
			viewRot = setView(rot - 180, 90 - tilt);
			if (toNearest) {
				if (ViewRotMatrix[1][0] < -0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(1, -1, -1));
				else if (ViewRotMatrix[1][1] > 0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(1, -1, -1));
			}
			break;
		case TEX_TOP_REAR_RIGHT:
			viewRot = setView(rot - 270, 90 - tilt);
			if (toNearest) {
				if (ViewRotMatrix[1][0] > 0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(-1, -1, -1));
				else if (ViewRotMatrix[1][1] > 0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(-1, -1, -1));
			}
			break;
		case TEX_ARROW_LEFT:
			viewRot = rotateView(viewRot, DIR_OUT, rotStepAngle);
			break;
		case TEX_ARROW_RIGHT:
			viewRot = rotateView(viewRot, DIR_OUT, -rotStepAngle);
			break;
		case TEX_ARROW_WEST:
			viewRot = rotateView(viewRot, DIR_UP, -rotStepAngle);
			break;
		case TEX_ARROW_EAST:
			viewRot = rotateView(viewRot, DIR_UP, rotStepAngle);
			break;
		case TEX_ARROW_NORTH:
			viewRot = rotateView(viewRot, DIR_RIGHT, -rotStepAngle);
			break;
		case TEX_ARROW_SOUTH:
			viewRot = rotateView(viewRot, DIR_RIGHT, rotStepAngle);
			break;
		case TEX_DOT_BACKSIDE:
			viewRot = rotateView(viewRot, 0, 180);
			break;
		case TEX_VIEW_MENU_FACE:
			handleMenu();
			applyRotation = false;
			break;
		}

		if (applyRotation)
			rotateView(viewRot);
	}
	return true;
}


void NaviCubeImplementation::setHilite(int hilite) {
	if (hilite != m_HiliteId) {
		m_HiliteId = hilite;
		//cerr << "m_HiliteFace " << m_HiliteId << endl;
		m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
	}
}

bool NaviCubeImplementation::inDragZone(short x, short y) {
	int dx = x - m_CubeWidgetPosX;
	int dy = y - m_CubeWidgetPosY;
	int limit = m_CubeWidgetSize / 4;
	return abs(dx) < limit && abs(dy) < limit;
}

bool NaviCubeImplementation::mouseMoved(short x, short y) {
	setHilite(pickFace(x, y));

	if (m_MouseDown) {
		if (m_MightDrag && !m_Dragging && !inDragZone(x, y))
			m_Dragging = true;
		if (m_Dragging) {
			setHilite(0);
			SbVec2s view = m_View3DInventorViewer->getSoRenderManager()->getSize();
			int width = view[0];
			int height = view[1];
			int len = m_CubeWidgetSize / 2;
			m_CubeWidgetPosX = std::min(std::max(static_cast<int>(x), len), width - len);
			m_CubeWidgetPosY = std::min(std::max(static_cast<int>(y), len), height - len);
			this->m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
			return true;
		}
	}
	return false;
}

bool NaviCubeImplementation::processSoEvent(const SoEvent* ev) {
	short x, y;
	ev->getPosition().getValue(x, y);
	// FIXME find out why do we need to hack the cursor position to get
	// 2019-02-17
	// The above comment is truncated; don't know what it's about
	// The two hacked lines changing the cursor position are responsible for
	// parts of the navigational cluster not being active.
	// Commented them out and everything seems to be working
//    y += 4;
//    x -= 2;
	if (ev->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
		const SoMouseButtonEvent* mbev = static_cast<const SoMouseButtonEvent*>(ev);
		if (mbev->isButtonPressEvent(mbev, SoMouseButtonEvent::BUTTON1))
			return mousePressed(x, y);
		if (mbev->isButtonReleaseEvent(mbev, SoMouseButtonEvent::BUTTON1))
			return mouseReleased(x, y);
	}
	if (ev->getTypeId().isDerivedFrom(SoLocation2Event::getClassTypeId()))
		return mouseMoved(x, y);
	return false;
}


QString NaviCubeImplementation::str(char* str) {
	return QString::fromLatin1(str);
}

void NaviCube::setNaviCubeCommands(const std::vector<std::string>& cmd)
{
	NaviCubeImplementation::m_commands = cmd;
}

void NaviCube::setNaviCubeLabels(const std::vector<std::string>& labels)
{
	NaviCubeImplementation::m_labels = labels;
}



DEF_3DV_CMD(ViewIsometricCmd)
ViewIsometricCmd::ViewIsometricCmd()
	: Command("ViewIsometricCmd")
{
	sGroup        = "";
	sMenuText     = QT_TR_NOOP("Isometric");
	sToolTipText  = QT_TR_NOOP("Set NaviCube to Isometric mode");
	sWhatsThis    = "";
	sStatusTip    = sToolTipText;
    sPixmap       = "";
	sAccel        = "";
	eType         = Alter3DView;
}

void ViewIsometricCmd::activated(int iMsg)
{
	Q_UNUSED(iMsg);
	Command::doCommand(Command::Gui, "Gui.activeDocument().activeView().viewIsometric()");
}

DEF_3DV_CMD(ViewOrthographicCmd)
ViewOrthographicCmd::ViewOrthographicCmd()
	: Command("ViewOrthographicCmd")
{
	sGroup        = "";
	sMenuText     = QT_TR_NOOP("Orthographic");
	sToolTipText  = QT_TR_NOOP("Set View to Orthographic mode");
	sWhatsThis    = "";
	sStatusTip    = sToolTipText;
	sPixmap       = "";
	sAccel        = "";
	eType         = Alter3DView;
}

void ViewOrthographicCmd::activated(int iMsg)
{
	Q_UNUSED(iMsg);
	Command::doCommand(Command::Gui, "Gui.activeDocument().activeView().setCameraType(\"Orthographic\")");
}

DEF_3DV_CMD(ViewPerspectiveCmd)

ViewPerspectiveCmd::ViewPerspectiveCmd()
	: Command("ViewPerspectiveCmd")
{
	sGroup        = "";
	sMenuText     = QT_TR_NOOP("Perspective");
	sToolTipText  = QT_TR_NOOP("Set View to Perspective mode");
	sWhatsThis    = "";
	sStatusTip    = sToolTipText;
	sPixmap       = "";
	sAccel        = "";
	eType         = Alter3DView;
}

void ViewPerspectiveCmd::activated(int iMsg)
{
	Q_UNUSED(iMsg);
	Command::doCommand(Command::Gui, "Gui.activeDocument().activeView().setCameraType(\"Perspective\")");
}

DEF_3DV_CMD(ViewZoomToFitCmd)

ViewZoomToFitCmd::ViewZoomToFitCmd()
	: Command("ViewZoomToFit")
{
	sGroup        = "";
	sMenuText     = QT_TR_NOOP("Zoom to fit");
	sToolTipText  = QT_TR_NOOP("Zoom so that model fills the view");
	sWhatsThis    = "";
	sStatusTip    = sToolTipText;
	sPixmap       = "";
	sAccel        = "";
	eType         = Alter3DView;
}

void ViewZoomToFitCmd::activated(int iMsg)
{
	Q_UNUSED(iMsg);
	Command::doCommand(Command::Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
}


QMenu* NaviCubeImplementation::createNaviCubeMenu() {
	QMenu* menu = new QMenu(getMainWindow());
	menu->setObjectName(str("NaviCube_Menu"));

	CommandManager& rcCmdMgr = Application::Instance->commandManager();
	static bool init = true;
	if (init) {
		init = false;
		rcCmdMgr.addCommand(new ViewOrthographicCmd);
		rcCmdMgr.addCommand(new ViewPerspectiveCmd);
		rcCmdMgr.addCommand(new ViewIsometricCmd);
		rcCmdMgr.addCommand(new ViewZoomToFitCmd);
	}

	vector<string> commands = NaviCubeImplementation::m_commands;
	if (commands.empty()) {
		commands.push_back("ViewOrthographicCmd");
		commands.push_back("ViewPerspectiveCmd");
		commands.push_back("ViewIsometricCmd");
		commands.push_back("Separator");
		commands.push_back("ViewZoomToFit");
	}

	for (vector<string>::iterator i = commands.begin(); i != commands.end(); ++i) {
		if (*i == "Separator") {
			menu->addSeparator();
		}
		else {
			Command* cmd = rcCmdMgr.getCommandByName(i->c_str());
			if (cmd)
				cmd->addTo(menu);
		}
	}
	return menu;
}
