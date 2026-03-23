import sys
from pivy.qt import QtGui, QtWidgets
from pivy import quarter, coin
from OpenGL.GL import *


class CapPlane():
    def __init__(self, plane, node):
        self.plane = plane  # SbPlane
        self.node = node
        self.normal = plane.getNormal()
        self.dist = plane.getDistanceFromOrigin()

    def stencilBuffer(self):

        eq = GLdouble_4(*self.normal, self.dist)
        glClipPlane(GL_CLIP_PLANE0, eq)

        glEnable(GL_CLIP_PLANE0)

        glEnable(GL_STENCIL_TEST)
        glClear(GL_STENCIL_BUFFER_BIT)

        # Draw Front faces and decrease stencil-buffer
        # where the front_face is drawn
        glStencilFunc(GL_ALWAYS, 0, 0);
        glStencilOp(GL_KEEP, GL_KEEP, GL_DECR)
        glCullFace(GL_FRONT)
        glEnable(GL_CULL_FACE)
        self.draw_box()

        # draw the back_face to the stencil-buffer
        # increase the stencil buffer where the back-face is drawn
        glCullFace(GL_BACK)
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE)
        glStencilOp(GL_KEEP, GL_KEEP, GL_INCR)
        self.draw_box()

        # Draw a quad aligned with the stencil plane,
        # but set the stencil test to reject pixels unless the stencil is set.
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE)
        glEnable(GL_DEPTH_TEST)
        glDisable(GL_CLIP_PLANE0)
        glDisable(GL_CULL_FACE)
        glStencilFunc(GL_NOTEQUAL, 0, ~0)

        glColor3f(0.1, 0.2, 1)
        glBegin(GL_QUADS)
        glVertex3fv([-1.1,-1.1, 0])
        glVertex3fv([ 1.1,-1.1, 0])
        glVertex3fv([ 1.1, 1.1, 0])
        glVertex3fv([-1.1, 1.1, 0])
        glEnd()

        glDisable(GL_STENCIL_TEST)
        glClear(GL_STENCIL_BUFFER_BIT)

    def draw_box(self):
        glDisable(GL_LIGHTING)
        glPushMatrix()
        glColor3f(0.0, 0.7, 0.0)
        glBegin(GL_QUADS)
        glVertex3fv([-1,-1, 1])
        glVertex3fv([-1, 1, 1])
        glVertex3fv([ 1, 1, 1])
        glVertex3fv([ 1,-1, 1])

        glVertex3fv([-1,-1,-1])
        glVertex3fv([ 1,-1,-1])
        glVertex3fv([ 1, 1,-1])
        glVertex3fv([-1, 1,-1])

        glVertex3fv([ 1,-1,-1])
        glVertex3fv([ 1,-1, 1])
        glVertex3fv([ 1, 1, 1])
        glVertex3fv([ 1, 1,-1])

        glVertex3fv([-1,-1,-1])
        glVertex3fv([-1, 1,-1])
        glVertex3fv([-1, 1, 1])
        glVertex3fv([-1,-1, 1])

        glVertex3fv([-1, 1,-1])
        glVertex3fv([ 1, 1,-1])
        glVertex3fv([ 1, 1, 1])
        glVertex3fv([-1, 1, 1])

        glVertex3fv([-1,-1,-1])
        glVertex3fv([-1,-1, 1])
        glVertex3fv([ 1,-1, 1])
        glVertex3fv([ 1,-1,-1])

        glEnd()
        glPopMatrix()
        glEnable(GL_LIGHTING)


def myCallbackRoutine(cap, action):
    global handled
    if not action.isOfType(coin.SoGLRenderAction.getClassTypeId()):
        return
    cap.stencilBuffer()


def main():
    app = QtWidgets.QApplication(sys.argv)
    viewer = quarter.QuarterWidget()
    # build a scene (sphere, cube)
    plane = coin.SbPlane(coin.SbVec3f(0, 0, 1), coin.SbVec3f(0, 0, 0))
    root = coin.SoSeparator()
    myCallback = coin.SoCallback()
    cap = CapPlane(plane, root)
    myCallback.setCallback(myCallbackRoutine, cap)
    root += myCallback, coin.SoSphere()

    viewer.setSceneGraph(root)
    viewer.setBackgroundColor(coin.SbColor(.5, .5, .5))
    viewer.setWindowTitle("GL stencil buffer")

    viewer.show()

    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
