// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is part of the examples of the Open CASCADE Technology software library.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE

// std
#include <filesystem>
#include <iostream>

// occt
#include "GlfwOcctView.h"
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_Handle.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Standard_Type.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>

// imgui
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <imgui_internal.h>

// glfw
#include <GLFW/glfw3.h>

// nfd
#include <nfd.h>

// other
#include "GlfwOcctView.h"
#include "haunch.h"

#ifdef _WIN32
#include <WNT_WClass.hxx>
#include <WNT_Window.hxx>
#elif defined(__APPLE__)
#include <Cocoa_LocalPool.hxx>
#include <Cocoa_Window.hxx>
#else
#include <X11/Xlib.h>
#include <Xw_Window.hxx>
#endif

namespace win_data
{
  static constexpr int DISPLAY_WIDTH  = 800;
  static constexpr int DISPLAY_HEIGHT = 600;
  static constexpr int CONTENT_WIDTH  = 512;
  static constexpr int CONTENT_HEIGHT = 512;

  struct DockWinId
  {
    static const std::string content;
    static const std::string gui;
  };

  const std::string DockWinId::content = "Content";
  const std::string DockWinId::gui     = "Gui";

  struct Content
  {
    static Graphic3d_Vec2i pos;
    static Graphic3d_Vec2i size;
  };

  Graphic3d_Vec2i Content::pos  = Graphic3d_Vec2i(0, 0);
  Graphic3d_Vec2i Content::size = Graphic3d_Vec2i(0, 0);
} // namespace win_data

namespace
{
  //! Convert GLFW mouse button into Aspect_VKeyMouse.
  static Aspect_VKeyMouse mouseButtonFromGlfw(int theButton)
  {
    switch (theButton)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
      return Aspect_VKeyMouse_LeftButton;
    case GLFW_MOUSE_BUTTON_RIGHT:
      return Aspect_VKeyMouse_RightButton;
    case GLFW_MOUSE_BUTTON_MIDDLE:
      return Aspect_VKeyMouse_MiddleButton;
    }
    return Aspect_VKeyMouse_NONE;
  }

  //! Convert GLFW key modifiers into Aspect_VKeyFlags.
  static Aspect_VKeyFlags keyFlagsFromGlfw(int theFlags)
  {
    Aspect_VKeyFlags aFlags = Aspect_VKeyFlags_NONE;
    if ((theFlags & GLFW_MOD_SHIFT) != 0) { aFlags |= Aspect_VKeyFlags_SHIFT; }
    if ((theFlags & GLFW_MOD_CONTROL) != 0) { aFlags |= Aspect_VKeyFlags_CTRL; }
    if ((theFlags & GLFW_MOD_ALT) != 0) { aFlags |= Aspect_VKeyFlags_ALT; }
    if ((theFlags & GLFW_MOD_SUPER) != 0) { aFlags |= Aspect_VKeyFlags_META; }
    return aFlags;
  }

  static void pixMapToGL(Image_PixMap& src, GLuint& dest)
  {
    const int width     = src.Width();
    const int height    = src.Height();
    const GLenum format = src.Format() == Image_Format_RGB ? GL_RGB : throw;

    if (dest == 0)
    {
      glGenTextures(1, &dest);
      glBindTexture(GL_TEXTURE_2D, dest);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, src.Data());
    }
    else
    {
      glBindTexture(GL_TEXTURE_2D, dest);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, src.Data());
    }
  }

  static Graphic3d_Vec2i cursorToLocalViewport(Graphic3d_Vec2i cursor)
  {
    int x = cursor.x() - win_data::Content::pos.x();
    x *= win_data::CONTENT_WIDTH / (float)win_data::Content::size.x();
    int y = cursor.y() - win_data::Content::pos.y();
    y *= win_data::CONTENT_HEIGHT / (float)win_data::Content::size.y();
    return Graphic3d_Vec2i(x, y);
  }
} // namespace

// ================================================================
// Function : GlfwOcctView
// Purpose  :
// ================================================================
GlfwOcctView::GlfwOcctView() {}

// ================================================================
// Function : ~GlfwOcctView
// Purpose  :
// ================================================================
GlfwOcctView::~GlfwOcctView() {}

// ================================================================
// Function : toView
// Purpose  :
// ================================================================
GlfwOcctView* GlfwOcctView::toView(GLFWwindow* theWin)
{
  return static_cast<GlfwOcctView*>(glfwGetWindowUserPointer(theWin));
}

// ================================================================
// Function : errorCallback
// Purpose  :
// ================================================================
void GlfwOcctView::errorCallback(int theError, const char* theDescription)
{
  Message::DefaultMessenger()->Send(TCollection_AsciiString("Error") + theError + ": " + theDescription, Message_Fail);
}

// ================================================================
// Function : run
// Purpose  :
// ================================================================
void GlfwOcctView::run()
{
  glfwSetErrorCallback(GlfwOcctView::errorCallback);
  NFD_Init();
  glfwInit();
  initWindow(win_data::DISPLAY_WIDTH, win_data::DISPLAY_HEIGHT, "RD");
  initViewer();
  initDemoScene();
  if (myView.IsNull()) { return; }

  myView->MustBeResized();
  myOcctWindow->Map();
  initUI();
  mainloop();
  cleanupUI();
  NFD_Quit();
  cleanup();
}

// ================================================================
// Function : initWindow
// Purpose  :
// ================================================================
void GlfwOcctView::initWindow(int theWidth, int theHeight, const char* theTitle)
{
  const bool toAskCoreProfile = true;
  if (toAskCoreProfile)
  {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  }
  myOcctWindow = new GlfwOcctWindow(theWidth, theHeight, theTitle);
  glfwSetWindowUserPointer(myOcctWindow->getGlfwWindow(), this);
  // window callback
  glfwSetWindowSizeCallback(myOcctWindow->getGlfwWindow(), GlfwOcctView::onResizeCallback);
  glfwSetFramebufferSizeCallback(myOcctWindow->getGlfwWindow(), GlfwOcctView::onFBResizeCallback);
  // mouse callback
  glfwSetScrollCallback(myOcctWindow->getGlfwWindow(), GlfwOcctView::onMouseScrollCallback);
  glfwSetMouseButtonCallback(myOcctWindow->getGlfwWindow(), GlfwOcctView::onMouseButtonCallback);
  glfwSetCursorPosCallback(myOcctWindow->getGlfwWindow(), GlfwOcctView::onMouseMoveCallback);
}

// ================================================================
// Function : initViewer
// Purpose  :
// ================================================================
void GlfwOcctView::initViewer()
{
  if (myOcctWindow.IsNull() || myOcctWindow->getGlfwWindow() == nullptr) { return; }

  // create graphic driver
  Handle(Aspect_DisplayConnection) aDisp = new Aspect_DisplayConnection();
  Handle(OpenGl_GraphicDriver) aDriver   = new OpenGl_GraphicDriver(aDisp, true);
  aDriver->ChangeOptions().ffpEnable     = false;
  aDriver->ChangeOptions().swapInterval  = 0; // no window, no swap

  // create viewer and AIS context
  Handle(V3d_Viewer) myViewer = new V3d_Viewer(aDriver);
  myContext                   = new AIS_InteractiveContext(myViewer);

  // viewer setup
  myViewer->SetDefaultLights();
  myViewer->SetLightOn();
  myViewer->SetDefaultTypeOfView(V3d_PERSPECTIVE);
  myViewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);

  // create offscreen window
  const TCollection_AsciiString aWinName("OCCT offscreen window");
  Graphic3d_Vec2i aWinSize(win_data::CONTENT_WIDTH, win_data::CONTENT_HEIGHT);
#if defined(_WIN32)
  const TCollection_AsciiString aClassName("OffscreenClass");
  // empty callback!
  Handle(WNT_WClass) aWinClass = new WNT_WClass(aClassName.ToCString(), nullptr, 0);
  Handle(WNT_Window) aWindow =
      new WNT_Window(aWinName.ToCString(), aWinClass, WS_POPUP, 64, 64, aWinSize.x(), aWinSize.y(), Quantity_NOC_BLACK);
#elif defined(__APPLE__)
  Handle(Cocoa_Window) aWindow = new Cocoa_Window(aWinName.ToCString(), 64, 64, aWinSize.x(), aWinSize.y());
#else
  Handle(Xw_Window) aWindow = new Xw_Window(aDisp, aWinName.ToCString(), 64, 64, aWinSize.x(), aWinSize.y());
#endif
  aWindow->SetVirtual(true);

  // create 3D view from offscreen window
  myView = myViewer->CreateView();
  myView->SetWindow(aWindow);
}

// ================================================================
// Function : initDemoScene
// Purpose  :
// ================================================================
void GlfwOcctView::initDemoScene()
{
  if (myContext.IsNull()) { return; }

  myView->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_GOLD, 0.08, V3d_WIREFRAME);

  TCollection_AsciiString aGlInfo;
  {
    TColStd_IndexedDataMapOfStringString aRendInfo;
    myView->DiagnosticInformation(aRendInfo, Graphic3d_DiagnosticInfo_Basic);
    for (TColStd_IndexedDataMapOfStringString::Iterator aValueIter(aRendInfo); aValueIter.More(); aValueIter.Next())
    {
      if (!aGlInfo.IsEmpty()) { aGlInfo += "\n"; }
      aGlInfo += TCollection_AsciiString("  ") + aValueIter.Key() + ": " + aValueIter.Value();
    }
  }
  // Message::DefaultMessenger()->Send(TCollection_AsciiString("OpenGL info:\n") + aGlInfo, Message_Info);
}

// ================================================================
// Function : mainloop
// Purpose  :
// ================================================================

void GlfwOcctView::mainloop()
{
  while (!glfwWindowShouldClose(myOcctWindow->getGlfwWindow()))
  {
    // glfwPollEvents() for continuous rendering (immediate return if there are no new events)
    // and glfwWaitEvents() for rendering on demand (something actually happened in the viewer)
    glfwPollEvents();
    // glfwWaitEvents();
    if (!myView.IsNull())
    {
      // render view offscreen
      FlushViewEvents(myContext, myView, true);
      if (!myView->ToPixMap(myTexture.pixMap, win_data::CONTENT_WIDTH, win_data::CONTENT_HEIGHT))
      {
        std::cerr << "View dump failed\n";
      }
      //

      // render scene
      glfwMakeContextCurrent(myOcctWindow->getGlfwWindow());
      pixMapToGL(myTexture.pixMap, myTexture.glID);
      render();
      glfwSwapBuffers(myOcctWindow->getGlfwWindow());
    }
  }
}

// ================================================================
// Function : cleanup
// Purpose  :
// ================================================================
void GlfwOcctView::cleanup()
{
  glDeleteTextures(1, &myTexture.glID);
  if (!myView.IsNull()) { myView->Remove(); }
  if (!myOcctWindow.IsNull()) { myOcctWindow->Close(); }
  glfwTerminate();
}

// ================================================================
// Function : onResize
// Purpose  :
// ================================================================
void GlfwOcctView::onResize(int theWidth, int theHeight)
{
  if (theWidth != 0 && theHeight != 0 && !myView.IsNull())
  {
    myView->Window()->DoResize();
    myView->MustBeResized();
    myView->Invalidate();
    myView->Redraw();
  }
}

// ================================================================
// Function : onMouseScroll
// Purpose  :
// ================================================================
void GlfwOcctView::onMouseScroll(double theOffsetX, double theOffsetY)
{
  if (!myView.IsNull()) { UpdateZoom(Aspect_ScrollDelta(myOcctWindow->CursorPosition(), int(theOffsetY * 8.0))); }
}

// ================================================================
// Function : onMouseButton
// Purpose  :
// ================================================================
void GlfwOcctView::onMouseButton(int theButton, int theAction, int theMods)
{
  if (myView.IsNull()) { return; }

  const Graphic3d_Vec2i aPos = cursorToLocalViewport(myOcctWindow->CursorPosition());
  if (theAction == GLFW_PRESS)
  {
    PressMouseButton(aPos, mouseButtonFromGlfw(theButton), keyFlagsFromGlfw(theMods), false);
  }
  else { ReleaseMouseButton(aPos, mouseButtonFromGlfw(theButton), keyFlagsFromGlfw(theMods), false); }
}

// ================================================================
// Function : onMouseMove
// Purpose  :
// ================================================================
void GlfwOcctView::onMouseMove(int thePosX, int thePosY)
{
  const Graphic3d_Vec2i aNewPos = cursorToLocalViewport(Graphic3d_Vec2i(thePosX, thePosY));
  if (!myView.IsNull()) { UpdateMousePosition(aNewPos, PressedMouseButtons(), LastMouseFlags(), false); }
}

void GlfwOcctView::initUI()
{
  // setup ImGui context
  const char* glsl_version = "#version 330";
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  // setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(myOcctWindow->getGlfwWindow(), true);
  ImGui_ImplOpenGL3_Init(glsl_version);
  // setup ImGui style
  ImGui::StyleColorsDark();
}

void GlfwOcctView::cleanupUI()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void GlfwOcctView::render()
{
  // clear window
  glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  //
  // feed inputs to dear imgui, start new frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  //
  // setup docking
  static ImGuiDockNodeFlags dock_space_flags = ImGuiDockNodeFlags_PassthruCentralNode;
  ImGuiWindowFlags window_flags              = ImGuiWindowFlags_NoDocking;

  ImGuiViewport* viewport = ImGui::GetMainViewport();

  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  window_flags |= ImGuiWindowFlags_MenuBar;
  window_flags |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove;
  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
  window_flags |= ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
  ImGui::Begin("Root", nullptr, window_flags);
  {
    static bool exit             = false;
    static bool show_demo_window = false;
    // Top menu bar
    if (ImGui::BeginMenuBar())
    {
      if (ImGui::BeginMenu("File"))
      {
        ImGui::MenuItem("Exit", nullptr, &exit);
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("View"))
      {
        ImGui::MenuItem("Show Demo Window", nullptr, &show_demo_window);
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
    }
    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
      ImGuiID dock_space_id = ImGui::GetID("RootDockSpace");
      ImGui::DockSpace(dock_space_id, ImVec2(0.f, 0.f), dock_space_flags);

      static auto first_time = true;
      if (first_time)
      {
        first_time = false;
        // Clear out existing layout
        ImGui::DockBuilderRemoveNode(dock_space_id);
        // Add empty node
        ImGui::DockBuilderAddNode(dock_space_id, dock_space_flags | ImGuiDockNodeFlags_DockSpace);
        // Main node should cover entire window
        ImGui::DockBuilderSetNodeSize(dock_space_id, ImGui::GetWindowSize());
        // Split root dock node
        ImGuiID content_node, gui_node;
        const float gui_size = .2f;
        ImGui::DockBuilderSplitNode(dock_space_id, ImGuiDir_Left, gui_size, &gui_node, &content_node);

        ImGui::DockBuilderDockWindow(win_data::DockWinId::content.c_str(), content_node);
        ImGui::DockBuilderDockWindow(win_data::DockWinId::gui.c_str(), gui_node);
        ImGui::DockBuilderFinish(dock_space_id);
      }
    }

    if (exit)
    {
      // ...
    }
    if (show_demo_window) { ImGui::ShowDemoWindow(); }
  }
  ImGui::End();
  ImGui::PopStyleVar(3);
  //
  // render view
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
  ImGui::Begin(win_data::DockWinId::content.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar);
  {
    ImVec2 win_pos = ImGui::GetWindowPos();
    win_data::Content::pos.SetValues(win_pos.x, win_pos.y);
    ImVec2 win_size = ImGui::GetWindowSize();
    win_data::Content::size.SetValues(win_size.x, win_size.y);

    ImGui::Image((ImTextureID)(intptr_t)myTexture.glID, win_size, ImVec2(0, 1), ImVec2(1, 0));
  }
  ImGui::End();
  ImGui::PopStyleVar();
  //
  // render UI
  ImGui::Begin(win_data::DockWinId::gui.c_str());
  {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (ImGui::Button("Load model", ImVec2(avail.x, 0)))
    {
      nfdu8char_t* filepath;
      nfdu8filteritem_t filter           = { "Geometry", "brep" };
      std::filesystem::path default_path = std::filesystem::current_path() / "../external/occt/data/occ";
      nfdresult_t result                 = NFD_OpenDialogU8(&filepath, &filter, 1, default_path.c_str());
      if (result == NFD_OKAY)
      {
        loadModel(filepath);
        NFD_FreePathU8(filepath);
      }
      else if (result == NFD_CANCEL) {}
      else { printf("Error: %s\n", NFD_GetError()); }
    }
    ImGui::Spacing();
    static float dist = 20.f;
    ImGui::DragFloat("haunch max distance", &dist, 0.5f, 1.0f, 60.f, "%.0f");
    if (ImGui::Button("Find haunches", ImVec2(avail.x, 0))) { ProcessDisplayedShapes(myContext, dist); }
  }
  ImGui::End();
  //
  // send to imgui renderer
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GlfwOcctView::loadModel(const char* filepath)
{
  TopoDS_Shape shape;
  BRep_Builder builder;
  if (!BRepTools::Read(shape, filepath, builder)) { throw std::runtime_error("Failed to read BREP file"); }
  Message::DefaultMessenger()->Send(TCollection_AsciiString("Loaded file: ") + filepath + "\n", Message_Info);

  Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
  // myContext->Display(aisShape, Standard_True);
  myContext->Display(aisShape, AIS_Shaded, 0, false);
}
