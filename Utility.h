#pragma once

#include <Mesh.h>
#include <vector>

struct VertFaceRelation
{
public:
	VertFaceRelation(short orgID_);
	short orgID;
	int faceID;
};
class Utility
{
public:
	static void ComputeFaceNormals(Mesh *mesh, std::vector<Point3> &normals);

	static void generateAdjacencyList(Mesh *mesh, std::vector<std::vector<int>> &adjList);
	static void generateRelationList(Mesh* mesh, const std::vector<Point3> &normals, std::vector<std::vector<int>> &adjList, std::vector<VertFaceRelation> &relations);
};