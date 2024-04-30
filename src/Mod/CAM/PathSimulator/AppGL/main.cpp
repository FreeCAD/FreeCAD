
//
// main.cpp
//
// simulating cnc mill operation using Sequenced Convex Subtraction (SCS) by Nigel Stewart et al.
//


#include "GlUtils.h"
#include <GLFW/glfw3.h>
#include "linmath.h"
#include "MillSimulation.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define gEyeStep (PI / 36)

MillSim::MillSimulation gMillSimulator;

GLFWwindow* glwind;

using namespace MillSim;

const char *demoCode[] = {
 "T2",
 "G0 X-0.7 Y-0.7 Z10",
 "G0 X-0.7 Y-0.7 Z1",
 "G0 X0.7 Y0.7 Z1 I0.7 J0.7 K0",
 "G0 X0.7 Y0.7 Z10",

 "G0 X-3 Y-3 Z10",
 "G0 X-3 Y-3 Z0.5",
 "G3 X3 Y3 Z0.5 I3 J3 K0",
 "G0 X3 Y3 Z10",

 "G0 X15 Y15 Z10",
 "G0 X 15 Y15 Z1.5",
 "G0 X15 Y-15 Z1.5",
 "G0 X-15 Y-15 Z1.5",
 "G0 X-15 Y15 Z1.5",
 "G0 X15 Y15 Z1.5",

 "G0 X15 Y15 Z1",
 "G0 X15 Y-15 Z1",
 "G0 X-15 Y-15 Z1",
 "G0 X-15 Y15 Z1",
 "G0 X15 Y15 Z1",

 "G0 X15 Y15 Z0.5",
 "G0 X15 Y-15 Z0.5",
 "G0 X-15 Y-15 Z0.5",
 "G0 X-15 Y15 Z0.5",
 "G0 X15 Y15 Z0.5",

 "G0 X15 Y15 Z0",
 "G0 X15 Y-15 Z0",
 "G0 X-15 Y-15 Z0",
 "G0 X-15 Y15 Z0",
 "G0 X15 Y15 Z0",

 "G0 X15 Y15 Z10",

 "T1",
 "G0 X8 Y8 Z10",
 "G0 X8 Y8 Z1.5",
 "G0 X8 Y-8 Z1.5",
 "G0 X6.1 Y-8 Z1.5",
 "G0 X6.1 Y8 Z1.5",
 "G0 X4.2 Y8 Z1.5",
 "G0 X4.2 Y-8 Z1.5",
 "G0 X2.3 Y-8 Z1.5",
 "G0 X2.3 Y8 Z1.5",
 "G0 X0.4 Y8 Z1.5",
 "G0 X0.4 Y-8 Z1.5",
 "G0 X-1.5 Y-8 Z1.5",
 "G0 X-1.5 Y8 Z1.5",
 "G0 X-3.4 Y8 Z1.5",
 "G0 X-3.4 Y-8 Z1.5",
 "G0 X-5.3 Y-8 Z1.5",
 "G0 X-5.3 Y8 Z1.5",
 "G0 X-7.2 Y8 Z1.5",
 "G0 X-7.2 Y-8 Z1.5",
 "G0 X-8 Y-8 Z1.5",
 "G0 X-8 Y8 Z1.5",
 "G0 X 8 Y8 Z1.5",
 "G0 X 8 Y-8 Z1.5",
 "G0 X-8 Y-8 Z1.5",

 "G0 X-8 Y-8 Z10",

 // taper mill motion
 "T3",
 "G0 X14.2 Y14.2 Z10",
 "G0 X14.2 Y14.2 Z1.5",
 "G0 X14.2 Y-14.2 Z1.5",
 "G0 X-14.2 Y-14.2 Z1.5",
 "G0 X-14.2 Y14.2 Z1.5",
 "G0 X14.2 Y14.2 Z1.5",
 "G0 X14.2 Y14.2 Z10",
 "G0 X0 Y0 Z10",

 // ball mill motion
    "T4",
 "G0 X12 Y12 Z10",
 "G0 X12 Y12 Z1.5",
 "G0 X12 Y-12 Z2.5",
 "G0 X-12 Y-12 Z1.5",
 "G0 X-12 Y12 Z2.5",
 "G0 X12 Y12 Z1.5",
 "G0 X12 Y12 Z10",
 "G0 X0 Y0 Z10",
};

#define NUM_DEMO_MOTIONS (sizeof(demoCode) / sizeof(char *))




EndMillFlat endMillFlat01(1, 3.175f, 16);
EndMillFlat endMillFlat02(2, 1.5f, 16);
EndMillBall endMillBall03(4, 1, 16, 4, 0.2f);
EndMillTaper endMillTaper04(3, 1, 16, 90, 0.2f);

// test section - remove!
GLuint tprogram, tmodel, tview, tprojection, tarray;

void ShowStats() {
    glDisable(GL_DEPTH_TEST);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    //glPushMatrix();
    glLoadIdentity();
    glColor3f(0.0f, 0.0f, 0.0f);
    glRasterPos2f(-1.0f, -1.0f);
    glDisable(GL_LIGHTING);
    glEnable(GL_LIGHTING);
    //glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

// add stuff here to show over simulation
void display()
{
    //ShowStats();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) 
{
    if (action != GLFW_PRESS)
        return;
    switch (key)
    {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    case GLFW_KEY_UP:
        gMillSimulator.TiltEye(gEyeStep);
        break;
    case GLFW_KEY_DOWN:
        gMillSimulator.TiltEye(- gEyeStep);
        break;
    default:
        gMillSimulator.HandleKeyPress(key);
        break;
    }
}

void mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    if (button > 2)
        return;
    int buttMask = (1 << button);
    gMillSimulator.MousePress(buttMask, action == GLFW_PRESS, (int)x, (int)y);
}


void handle_mouse_move(GLFWwindow* window)
{
    double fx, fy;
    glfwGetCursorPos(window, &fx, &fy);
    gMillSimulator.MouseMove((int)fx, (int)fy);
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}


int main(int argc, char **argv)
{
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);


    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glwind = glfwCreateWindow(800, 600, "OpenGL Triangle", NULL, NULL);
    if (!glwind)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(glwind, key_callback);
    glfwSetMouseButtonCallback(glwind, mouse_callback);

    glfwMakeContextCurrent(glwind);
    glewInit();
    glfwSwapInterval(1);

    std::cout << glGetString(GL_VERSION) << std::endl;
    //gMillSimulator.LoadGCodeFile("cam_test1.txt");
    for (int i = 0; i < NUM_DEMO_MOTIONS; i++)
    {
        gMillSimulator.AddGcodeLine(demoCode[i]);
    }
    gMillSimulator.AddTool(&endMillFlat01);
    gMillSimulator.AddTool(&endMillFlat02);
    gMillSimulator.AddTool(&endMillBall03);
    gMillSimulator.AddTool(&endMillTaper04);
    gMillSimulator.InitSimulation();
    gMillSimulator.SetBoxStock(0, 0, -8.7f, 50, 50, 8.7f);
    //gMillSimulator.SetBoxStock(-119, -124, 0.003f, 245, 234, 30);
    gMillSimulator.InitDisplay();
    
    while (!glfwWindowShouldClose(glwind))
    {
        gMillSimulator.ProcessSim((unsigned int)(glfwGetTime() * 1000));
        display();
        glfwSwapBuffers(glwind);
        glfwPollEvents();
        handle_mouse_move(glwind);
    }

    glfwDestroyWindow(glwind);

    glfwTerminate();
    exit(EXIT_SUCCESS);


    return 0;
}

