#include "haunch.h"

bool GetFacePlaneNormal(const TopoDS_Face& face, gp_Dir& outNormal)
{
  GeomAdaptor_Surface adaptor(BRep_Tool::Surface(face));
  if (adaptor.GetType() == GeomAbs_Plane)
  {
    Handle(Geom_Surface) surf = BRep_Tool::Surface(face);
    Handle(Geom_Plane) plane  = Handle(Geom_Plane)::DownCast(surf);
    if (!plane.IsNull())
    {
      outNormal = plane->Pln().Axis().Direction();
      //std::cout << "Normal: " << outNormal.X() << " " << outNormal.Y() << " " << outNormal.Z() << "\n";
      return true;
    }
  }
  return false;
}

// Compare two sets of vertices for equality (within tolerance)
bool HaveSameVertices(const TopoDS_Face& face1, const TopoDS_Face& face2, float d)
{
  TopTools_IndexedMapOfShape verts1, verts2;
  TopExp::MapShapes(face1, TopAbs_VERTEX, verts1);
  TopExp::MapShapes(face2, TopAbs_VERTEX, verts2);

  const int count = verts1.Extent();
  if (count != verts2.Extent())
    return false;

  // Get face normal (they're already confirmed to be planar and parallel)
  gp_Dir normal;
  if (!GetFacePlaneNormal(face1, normal))
    return false;

  for (int i = 1; i <= count; ++i)
  {
    const gp_Pnt& p1 = BRep_Tool::Pnt(TopoDS::Vertex(verts1(i)));

    bool foundMatch = false;
    for (int j = 1; j <= count; ++j)
    {
      const gp_Pnt& p2 = BRep_Tool::Pnt(TopoDS::Vertex(verts2(j)));

      gp_Vec delta(p1, p2);
      Standard_Real normalDistance = std::abs(delta.Dot(gp_Vec(normal)));

      // Lateral difference (should be near zero if p2 is directly offset along the normal)
      gp_Vec lateral = delta - normalDistance * gp_Vec(normal);
      if (lateral.Magnitude() > 1e-4)
        continue;

      // Check if distance is within expected offset ± tolerance
      if (std::abs(normalDistance - d) < 1e-4)
      {
        foundMatch = true;
        break;
      }
    }

    if (!foundMatch)
      return false;
  }

  return true;
}

std::vector<std::pair<TopoDS_Face, gp_Dir>> CollectPlaneFaces(const TopoDS_Shape& shape)
{
    std::vector<std::pair<TopoDS_Face, gp_Dir>> planeFaces;

    for (TopExp_Explorer faceExp(shape, TopAbs_FACE); faceExp.More(); faceExp.Next())
    {
        TopoDS_Face face = TopoDS::Face(faceExp.Current());
        gp_Dir normal;
        if (GetFacePlaneNormal(face, normal)) {
            planeFaces.emplace_back(face, normal);
        }
    }

    return planeFaces;
}

void ProcessShapeFacesForParallelPlanes(const TopoDS_Shape& shape, float max_distance)
{
    std::cout << "Processing Shape...\n";

    auto planeFaces = CollectPlaneFaces(shape);

    for (size_t i = 0; i < planeFaces.size(); ++i)
    {
        for (size_t j = i + 1; j < planeFaces.size(); ++j)
        {
            const auto& [face1, normal1] = planeFaces[i];
            const auto& [face2, normal2] = planeFaces[j];

            if (normal1.IsParallel(normal2, Precision::Angular()))
            {
                gp_Pnt p1 = BRep_Tool::Surface(face1)->Value(0.0, 0.0);
                gp_Pnt p2 = BRep_Tool::Surface(face2)->Value(0.0, 0.0);
                Standard_Real distance = p1.Distance(p2);

                if (distance <= max_distance)
                {
                    if (HaveSameVertices(face1, face2, distance))
                    {
                        std::cout << "Found parallel, close faces with same vertices\n";
                    }
                }
            }
        }
    }
}

void ProcessDisplayedShapes(const Handle(AIS_InteractiveContext)& context, float max_distance)
{
    AIS_ListOfInteractive aList;
    context->DisplayedObjects(aList);

    for (AIS_ListIteratorOfListOfInteractive it(aList); it.More(); it.Next())
    {
        Handle(AIS_InteractiveObject) io = it.Value();
        Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(io);
        if (aisShape.IsNull())
            continue;

        const TopoDS_Shape& shape = aisShape->Shape();
        ProcessShapeFacesForParallelPlanes(shape, max_distance);
    }
}