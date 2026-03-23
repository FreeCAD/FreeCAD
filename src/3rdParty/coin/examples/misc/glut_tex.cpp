/* Simple example that demonstrates how to generate a texture based on 
 * a rendered scene, using offscreen rendering. Based on glutiv.cpp 
 * by Morten Eriksen and 09.2.Texture.cpp.in from the Inventor Mentor.
 *
 * Note: This example uses GLUT, so you do not need to have any of the 
 * SoGUI libraries installed. If you have a working Coin installation,
 * you should be able to build the example as follows:
 * 
 *    UNIX:
 *          coin-config --build glut_tex glut_tex.cpp -lglut3
 *
 *    Windows:
 *          coin-config --build glut_tex glut_tex.cpp -lglut32 
 *
 *    Mac OS X:
 *                                                                              
 *          export LDFLAGS="-framework GLUT" 
 *          coin-config --build glut_tex glut_tex.cpp  
 */

#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoSceneManager.h>
#include <Inventor/SoOffscreenRenderer.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoRotationXYZ.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif 

// ----------------------------------------------------------------------

SoSceneManager * scenemanager;
int glutwin;

static char red_cone_iv[] = {
  "#Inventor V2.1 ascii\n\n"
  "Separator {\n"
  "  BaseColor { rgb 0.8 0 0 }\n"
  "  Rotation { rotation 1 1 0  1.57 }\n"
  "  Cone { }\n"
  "}\n"
};


// ----------------------------------------------------------------------

// Redraw on scenegraph changes.
void
redraw_cb(void * user, SoSceneManager * manager)
{
  glutSetWindow(glutwin);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  scenemanager->render();
  glutSwapBuffers();
}

// Redraw on expose events.
void
expose_cb(void)
{
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  scenemanager->render();
  glutSwapBuffers();
}

// Reconfigure on changes to window dimensions.
void
reshape_cb(int w, int h)
{
  int idx = glutGetWindow();

  scenemanager->setWindowSize(SbVec2s(w, h));
  scenemanager->setSize(SbVec2s(w, h));
  scenemanager->setViewportRegion(SbViewportRegion(w, h));
  scenemanager->scheduleRedraw();
}

// Process the internal Coin queues when idle. Necessary to get the
// animation to work.
void
idle_cb(void)
{
  SoDB::getSensorManager()->processTimerQueue();
  SoDB::getSensorManager()->processDelayQueue(TRUE);
}

// ----------------------------------------------------------------------

SbBool
generateTextureMap (SoNode *root, SoTexture2 *texture,
   short textureWidth, short textureHeight)
{
   SbViewportRegion myViewport(textureWidth, textureHeight);
   SoOffscreenRenderer::Components comp = SoOffscreenRenderer::RGB;

   // Render the scene
   SoOffscreenRenderer *myRenderer = new SoOffscreenRenderer(myViewport);
   myRenderer->setComponents(comp);
   myRenderer->setBackgroundColor(SbColor(0.8, 0.8, 0.0));
   if (!myRenderer->render(root)) {
      delete myRenderer;
      return FALSE;
   }

   // Generate the texture
   texture->image.setValue(SbVec2s(textureWidth, textureHeight), comp, 
                           myRenderer->getBuffer());
   delete myRenderer;
   return TRUE; 
} 
  
SoSeparator *
createScenegraph(void)
{
  SoSeparator * texroot = new SoSeparator;
  texroot->ref();
  SoInput in;
  in.setBuffer(red_cone_iv, strlen(red_cone_iv));
  
  SoSeparator * result = SoDB::readAll(&in);
  if (result == NULL) { exit(1); }
 
  SoPerspectiveCamera *myCamera = new SoPerspectiveCamera;
  SoRotationXYZ *rot = new SoRotationXYZ;
  rot->axis  = SoRotationXYZ::X;
  rot->angle = M_PI_2;
  myCamera->position.setValue(SbVec3f(-0.2, -0.2, 2.0));
  myCamera->scaleHeight(0.4);
  texroot->addChild(myCamera);
  texroot->addChild(new SoDirectionalLight);
  texroot->addChild(rot);
  texroot->addChild(result);
  myCamera->viewAll(texroot, SbViewportRegion());

  // Generate the texture map
  SoTexture2 *texture = new SoTexture2;
  texture->ref();
  if (generateTextureMap(texroot, texture, 128, 128))
    printf ("Successfully generated texture map\n");
  else
    printf ("Could not generate texture map\n");
  texroot->unref();

  // Make a scene with a cube and apply the texture to it
  SoSeparator * root = new SoSeparator;
  root->addChild(texture);
  root->addChild(new SoCube);
  return root;
}

// ----------------------------------------------------------------------

#ifdef _WIN32

#include <windows.h>
#include <winbase.h>

int
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
        int nCmdShow)
{
#else // UNIX

int
main(int argc, char ** argv)
{

#endif

  // initialize Coin and glut libraries
  SoDB::init();

#ifdef _WIN32
  int argc = 1;
  char * argv[] = { "glutiv.exe", (char *) NULL };
  glutInit(&argc, argv);
#else
  glutInit(&argc, argv);
#endif

  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

  SoSeparator * root;
  root = new SoSeparator;
  root->ref();
  SoPerspectiveCamera * camera = new SoPerspectiveCamera;
  root->addChild(camera);
  root->addChild(new SoDirectionalLight);
  root->addChild(createScenegraph());

  scenemanager = new SoSceneManager;
  scenemanager->setRenderCallback(redraw_cb, (void *)1);
  scenemanager->setBackgroundColor(SbColor(0.2f, 0.2f, 0.2f));
  scenemanager->activate();
  camera->viewAll(root, scenemanager->getViewportRegion());
  scenemanager->setSceneGraph(root);
  
  glutInitWindowSize(512, 400);
  SbString title("Offscreen Rendering");
  glutwin = glutCreateWindow(title.getString());
  glutDisplayFunc(expose_cb);
  glutReshapeFunc(reshape_cb);
 
  // start main loop processing (with an idle callback)

  glutIdleFunc(idle_cb);
  glutMainLoop();

  root->unref();
  delete scenemanager;
  return 0;
}
