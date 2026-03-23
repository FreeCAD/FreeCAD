/* Simple example that demonstrates how to render with Coin3D and GLFW.
 * 
 * Note: This example uses GLFW, so you do not need to have any of the 
 * SoGUI libraries installed.
 */

#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoSceneManager.h>
#include <Inventor/SoOffscreenRenderer.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoRotationXYZ.h>

#include <cstdlib>
#include <functional>


#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

// ----------------------------------------------------------------------

GLFWwindow* window;
SoSceneManager* sceneManager;
SoCamera* camera;

// ----------------------------------------------------------------------

// Redraw on scenegraph changes.
void redrawCallback(void * user, SoSceneManager * manager)
{
  const SoState* state = manager->getGLRenderAction()->getState();
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);

  sceneManager->render();
  glfwSwapBuffers(window);
}

// Redraw on expose events.
void exposeCallback(void)
{
  const SoState* state = sceneManager->getGLRenderAction()->getState();
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);

  sceneManager->render();
  glfwSwapBuffers(window);
}

// Reconfigure on changes to window dimensions.
void framebufferSizeCallback(GLFWwindow* window,int w, int h)
{
  sceneManager->setWindowSize(SbVec2s(w, h));
  sceneManager->scheduleRedraw();
}

// Process the internal Coin queues when idle.
void idleCallback(void)
{
  SoDB::getSensorManager()->processTimerQueue();
  SoDB::getSensorManager()->processDelayQueue(TRUE);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  SbVec3f position = camera->position.getValue();
  if (action == GLFW_PRESS) {
    switch(key) {
      case GLFW_KEY_W:
        camera->position.setValue(position + SbVec3f(0, 0, -1));
        break;
      case GLFW_KEY_A:
      camera->position.setValue(position + SbVec3f(-1, 0, 0));
        break;
      case GLFW_KEY_S:
        camera->position.setValue(position + SbVec3f(0, 0, 1));
        break;
      case GLFW_KEY_D:
        camera->position.setValue(position + SbVec3f(1, 0, 0));
        break;
    }
  }
}

// ----------------------------------------------------------------------

SoSeparator* createScene();

std::function<void()> loop;
void main_loop() { loop(); }

int main(void)
{
    glfwSetErrorCallback([](int error, const char* description) {
      fprintf(stderr, "Error: %s\n", description);
    });

    SoDB::init();

    if (!glfwInit())
        return EXIT_FAILURE;

    // glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API); // Use for EGL on X11

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

    window = glfwCreateWindow(640, 480, "Coin3D", NULL, NULL);
    if (!window) {
      glfwTerminate();
      return EXIT_FAILURE;
    }

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
  
    glfwMakeContextCurrent(window);

    SoSeparator* root = createScene();

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    framebufferSizeCallback(window, width, height);

    glfwSetKeyCallback(window, keyCallback);

    loop = [&] {
        glfwPollEvents();

        idleCallback();
    };

    while (!glfwWindowShouldClose(window))
        main_loop();

    root->unref();
    delete sceneManager;
  
    glfwTerminate();
    return 0;
}

// ----------------------------------------------------------------------

SoSeparator* createScene()
{
    auto root = new SoSeparator;
    root->ref();

    SoPerspectiveCamera * perspectiveCamera = new SoPerspectiveCamera;
    perspectiveCamera->nearDistance = 0.01f;
    perspectiveCamera->farDistance = 100.0f;
    camera = perspectiveCamera;
    root->addChild(perspectiveCamera);

    SoLightModel * lightModel = new SoLightModel;
    lightModel->model = SoLightModel::BASE_COLOR;
    root->addChild(lightModel);

    SoBaseColor * col = new SoBaseColor;
    col->rgb = SbColor(1, 1, 0);
    root->addChild(col);

    root->addChild(new SoCone);

    sceneManager = new SoSceneManager;
    sceneManager->setRenderCallback(redrawCallback, (void *)1);
    sceneManager->setBackgroundColor(SbColor(1.0f, 1.0f, 1.0f));
    sceneManager->activate();
    sceneManager->setSceneGraph(root);

    camera->viewAll(root, sceneManager->getViewportRegion());

    return root;
}
