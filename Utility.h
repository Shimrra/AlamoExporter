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

//Normal generation copied from:
//http://docs.autodesk.com/3DSMAX/16/ENU/3ds-Max-SDK-Programmer-Guide/index.html?url=files/GUID-9B83270D-1D20-4613-AB35-5EE9832C162E.htm,topicNumber=d30e24073

class VNormal
{
   public:
     Point3 norm;
     DWORD smooth;
     VNormal *next;
     BOOL init;
 
     VNormal() {smooth=0;next=NULL;init=FALSE;norm=Point3(0,0,0);}
     VNormal(Point3 &n,DWORD s) {next=NULL;init=TRUE;norm=n;smooth=s;}
     ~VNormal() {delete next;}
     void AddNormal(Point3 &n,DWORD s);
     Point3 &GetNormal(DWORD s);
     void Normalize();
};

class Utility
{
public:
	static void ComputeFaceNormals(Mesh *mesh, std::vector<Point3> &normals);

	static void generateAdjacencyList(Mesh *mesh, std::vector<std::vector<int>> &adjList);
	static void generateRelationList(Mesh* mesh, const std::vector<Point3> &normals, std::vector<std::vector<int>> &adjList, std::vector<VertFaceRelation> &relations);
};