#pragma once

#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_Handle.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <TopAbs_ShapeEnum.hxx>

#include <AIS_InteractiveContext.hxx>
#include <BRep_Tool.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_Surface.hxx>
#include <Standard_Type.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>

bool GetFacePlaneNormal(const TopoDS_Face& face, gp_Dir& outNormal);
bool HaveSameVertices(const TopoDS_Face& face1, const TopoDS_Face& face2, float d);
std::vector<std::pair<TopoDS_Face, gp_Dir>> CollectPlaneFaces(const TopoDS_Shape& shape);
void ProcessShapeFacesForParallelPlanes(const TopoDS_Shape& shape, float max_distance);
void ProcessDisplayedShapes(const Handle(AIS_InteractiveContext)& context, float max_distance);