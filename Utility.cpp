#include "Utility.h"
#include "Max.h"

#include <VertexNormal.h>

#include <Esent.h>

VertFaceRelation::VertFaceRelation(short orgID_)
	:orgID(orgID_)
	,faceID(-1)
{
}

// Add a normal to the list if the smoothing group bits overlap,
// otherwise create a new vertex normal in the list
void VNormal::AddNormal(Point3 &n,DWORD s) {
   if (!(s&smooth) && init) {
     if (next) next->AddNormal(n,s);
     else {
      next = new VNormal(n,s);
     }
   }
   else {
     norm += n;
     smooth |= s;
     init = TRUE;
   }
}
 
// Retrieves a normal if the smoothing groups overlap or there is
// only one in the list
Point3 &VNormal::GetNormal(DWORD s)
{
   if (smooth&s || !next) return norm;
   else return next->GetNormal(s); 
}
 
// Normalize each normal in the list
void VNormal::Normalize() {
   VNormal *ptr = next, *prev = this;
   while (ptr)
   {
     if (ptr->smooth&smooth) {
      norm += ptr->norm;
      prev->next = ptr->next;
       delete ptr;
      ptr = prev->next;
     }
     else {
      prev = ptr;
      ptr = ptr->next;
     }
   }
   norm = ::Normalize(norm);
   if (next) next->Normalize();
}

void Utility::ComputeFaceNormals(Mesh *mesh, std::vector<Point3> &normals)
{
   normals.resize(mesh->getNumFaces());
   Face *face; 
   Point3 *verts;
   Point3 v0, v1, v2;
   face = mesh->faces; 
   verts = mesh->verts;

   for (int i = 0; i < mesh->getNumFaces(); ++i, ++face) 
   {
     // Calculate the surface normal
     v0 = verts[face->v[0]];
     v1 = verts[face->v[1]];
     v2 = verts[face->v[2]];
     normals[i] = Normalize(CrossProd((v1-v0),(v2-v1)));
	 mesh->setFaceNormal(i, normals[i]);
   }
}

void Utility::generateAdjacencyList(Mesh *mesh, std::vector<std::vector<int>> &adjList)
{
	adjList.resize(mesh->getNumVerts());
	int id=0;
	int numverts = mesh->getNumVerts();
	int numfaces = mesh->getNumFaces();
	for(int i=0;i<mesh->getNumFaces();++i)
	{
		auto &face = mesh->faces[i];
		for(int j=0;j<3;++j)
		{
			int vert = face.getVert(j);
			adjList[face.getVert(j)].push_back(i);
		}
	}
}

void Utility::generateRelationList(Mesh* mesh, const std::vector<Point3> &normals, std::vector<std::vector<int>> &adjList, std::vector<VertFaceRelation>& relations)
{	
	for(int i = 0;i<adjList.size();++i)
	{
		for (int j = 0; j < adjList[i].size(); ++j)
		{
			int matches = 0;
			for(int k = j+1; k<adjList[i].size(); ++k)
			{
				if(normals[adjList[i][k]].Equals(normals[adjList[i][j]]))
				{
					std::swap(adjList[i][j+1+matches], adjList[i][k]);
					++matches;
				}
			}
			j+=matches;
			relations.push_back(VertFaceRelation(i));
		}
	}
}
