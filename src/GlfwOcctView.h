#pragma once

#include "GlfwOcctWindow.h"

#include <AIS_InteractiveContext.hxx>
#include <AIS_Triangulation.hxx>
#include <AIS_ViewController.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <V3d_View.hxx>

class GlfwOcctView : protected AIS_ViewController {
public:
    GlfwOcctView();

    ~GlfwOcctView();

    void run();

private:
    void initWindow(int theWidth, int theHeight, const char* theTitle);

    void initViewer();

    void initDemoScene();

    void initUI();

    void mainloop();

    void cleanup();

    void cleanupUI();

    void render();

private:
    void onResize(int theWidth, int theHeight);

    void onMouseScroll(double theOffsetX, double theOffsetY);

    void onMouseButton(int theButton, int theAction, int theMods);

    void onMouseMove(int thePosX, int thePosY);

private:
    static void errorCallback(int theError, const char* theDescription);

    static GlfwOcctView* toView(GLFWwindow* theWin);

    static void onResizeCallback(
        GLFWwindow* theWin, int theWidth, int theHeight)
    {
        toView(theWin)->onResize(theWidth, theHeight);
    }

    static void onFBResizeCallback(
        GLFWwindow* theWin, int theWidth, int theHeight)
    {
        toView(theWin)->onResize(theWidth, theHeight);
    }

    static void onMouseScrollCallback(
        GLFWwindow* theWin, double theOffsetX, double theOffsetY)
    {
        toView(theWin)->onMouseScroll(theOffsetX, theOffsetY);
    }

    static void onMouseButtonCallback(
        GLFWwindow* theWin, int theButton, int theAction, int theMods)
    {
        toView(theWin)->onMouseButton(theButton, theAction, theMods);
    }

    static void onMouseMoveCallback(
        GLFWwindow* theWin, double thePosX, double thePosY)
    {
        toView(theWin)->onMouseMove((int)thePosX, (int)thePosY);
    }

private:
    void importObj(const std::string& filename);

    Handle(GlfwOcctWindow) myOcctWindow;
    Handle(V3d_View) myView;
    Handle(AIS_InteractiveContext) myContext;
    Handle(AIS_Triangulation) myImported;
    bool mNeedShow = false;

    struct {
        Image_PixMap pixMap;
        uint32_t glID = 0;
    } myTexture;

    bool mIsLearning = false;
    std::vector<TopTools_IndexedMapOfShape> uniqueVerts;
};