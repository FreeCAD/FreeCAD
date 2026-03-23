/* Simple example that demonstrates how to render with Coin3D and SDL3.
 *
 * Note: This example uses SDL3, so you do not need to have any of the
 * SoGUI libraries installed.
 */

#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <Inventor/SoDB.h>
#include <Inventor/SoSceneManager.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/actions/SoGLRenderAction.h>

void check_error(const bool res)
{
    if (!res) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
    }
}

template <typename T>
T* check_error(T* ptr)
{
    if (!ptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
    }

    return ptr;
}

struct SceneData
{
    SoSeparator *root;
    SoPerspectiveCamera *camera;
    SoSceneManager *scene_manager;
};

SceneData create_scene()
{
    auto root = new SoSeparator;
    root->ref();

    auto camera = new SoPerspectiveCamera;
    camera->nearDistance = 0.01f;
    camera->farDistance = 100.0f;
    root->addChild(camera);

    SoLightModel* light_model = new SoLightModel;
    light_model->model = SoLightModel::Model::BASE_COLOR;
    root->addChild(light_model);

    SoBaseColor* base_clr = new SoBaseColor;
    base_clr->rgb = SbColor(1, 0, 0);
    root->addChild(base_clr);

    root->addChild(new SoCone);

    auto scene_manager = new SoSceneManager;
    scene_manager->setBackgroundColor(SbColor(0, 1, 0));
    scene_manager->setWindowSize({800, 600});
    scene_manager->activate();
    scene_manager->setSceneGraph(root);

    camera->viewAll(root, scene_manager->getViewportRegion());

    return {root, camera, scene_manager};
}

int main(int, char**)
{
    check_error(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));

    SDL_Window* window = check_error(SDL_CreateWindow("Window", 800, 600, SDL_WINDOW_OPENGL));
    check_error(SDL_GL_CreateContext(window));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    SoDB::init();

    SceneData scene_data = create_scene();
    SoSeparator* root = scene_data.root;
    SoPerspectiveCamera* camera = scene_data.camera;
    SoSceneManager* scene_manager = scene_data.scene_manager;

    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
            }
        }

        auto keys = SDL_GetKeyboardState(nullptr);

        SbVec3f position = camera->position.getValue();
        float dt = 20.0f/1000;
        if (keys[SDL_SCANCODE_W]) {
            camera->position.setValue(position + SbVec3f( 0, 0, -1) * dt);
        } else if (keys[SDL_SCANCODE_S]) {
            camera->position.setValue(position + SbVec3f( 0, 0,  1) * dt);
        }
        if (keys[SDL_SCANCODE_A]) {
            camera->position.setValue(position + SbVec3f(-1, 0,  0) * dt);
        } else if (keys[SDL_SCANCODE_D]) {
            camera->position.setValue(position + SbVec3f( 1, 0,  0) * dt);
        }

        scene_manager->render();
        check_error(SDL_GL_SwapWindow(window));

        // Limit the maximum FPS to 50
        SDL_Delay(20);
    }

    root->unref();
    delete scene_manager;

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
