#include "Exporter.h"

#include "Max.h"

#include <iosfwd>
#include <lslights.h>
#include <Esent.h>
#include <modstack.h>
#include "Utility.h"

Exporter::Exporter(const TCHAR* name, ExpInterface* ei, Interface* ip, BOOL suppressPrompts, DWORD options)
	: m_name(name)
	, m_expInterface(ei)
	, m_interface(ip)
	, m_suppressPrompts(suppressPrompts)
	, m_options(options)
	, m_writer(&m_fileBuf)
{
	m_fileBuf.open(m_name, std::ios::out | std::ios::binary);
}

bool Exporter::exportALO()
{
	if(!m_fileBuf.is_open()) return false;

	return exportSkeleton() && exportMesh() && exportLight() && exportConnections() && m_writer.isEmpty();
}

void Exporter::parseSkeleton(INode* node, uint32_t parent)
{
	auto nodeObject = node->GetObjectRef();
	uint32_t newParentID = m_nodes.size();
	if(nodeObject)
	{
		while (nodeObject->SuperClassID() == GEN_DERIVOB_CLASS_ID)
		{
			auto nodeObjectDer = reinterpret_cast<IDerivedObject*>(nodeObject); 
			nodeObject = nodeObjectDer->GetObjRef();
		}

		//Check if this is an object we can use, if not, we don't add it and continue on with the last valid parent
		if(nodeObject->CanConvertToType(triObjectClassID) || nodeObject->IsSubClassOf(LIGHTSCAPE_LIGHT_CLASS) || nodeObject->IsSubClassOf(BONE_OBJ_CLASSID))
		{
			m_nodes.push_back(node);
		}
		else
		{
			newParentID = parent;
		}
	}
	else
	{
		m_nodes.push_back(node);
	}

	if(newParentID != parent)
	{
		m_writer.beginChunk(0x202);
		
		//Bone name
		m_writer.beginChunk(0x203);
		const wchar_t *name;
		if(!node->GetObjectRef())
		{
			name = L"Root";
		}
		else
		{
			name = node->GetName();
		}
		m_writer.writeName(name);
		m_writer.endChunk();

		//Bone Data (v2)
		m_writer.beginChunk(0x206);
		m_writer.write(parent);
		m_writer.write(static_cast<uint32_t>(!node->IsHidden()));
		//TODO Billboarding TODO
		m_writer.write(0);
		//Get TM for node
		if(!node->GetObjectRef())
		{
			m_writer.writeMatrix(node->GetNodeTM(0));
		}
		else
		{
			auto nodeTM = node->GetObjTMAfterWSM(0);
			auto parentTM = node->GetParentNode()->GetObjTMAfterWSM(0);
			m_writer.writeMatrix(nodeTM*Inverse(parentTM));
			//m_writer.writeMatrix(node->GetObjTMBeforeWSM(0));
		}
		m_writer.endChunk();
		m_writer.endChunk();
	}

	for (int i=0;i<node->NumberOfChildren();++i)
	{
		parseSkeleton(node->GetChildNode(i), newParentID);
	}

}

bool Exporter::exportSkeleton()
{
	//Skeleton container with placeholder size
	m_writer.beginChunk(0x200);
		
	//TODO Bone count container correct value TODO
	m_writer.beginChunk(0x201);
	auto boneCountPosition = m_writer.tellp();
	m_writer.fillBytes(128);
	m_writer.endChunk();

	auto root = m_interface->GetRootNode();
	parseSkeleton(root, 0xffffffff);
	m_writer.writeAt(boneCountPosition, static_cast<uint32_t>(m_nodes.size()));

	m_writer.endChunk();
	
	
	return true;
}

bool Exporter::exportMesh()
{
	uint32_t meshID = 0;
	for(int i=1;i<m_nodes.size();++i)
	{
		auto node = m_nodes[i];
		auto nodeObject = node->GetObjectRef();
		if(nodeObject->IsSubClassOf(LIGHTSCAPE_LIGHT_CLASS) || nodeObject->IsSubClassOf(BONE_OBJ_CLASSID))
		{
			continue;
		}

		if(nodeObject->SuperClassID() == GEN_DERIVOB_CLASS_ID)
		{
			auto originalObject = nodeObject;
			while (originalObject->SuperClassID() == GEN_DERIVOB_CLASS_ID)
			{
				auto nodeObjectDer = reinterpret_cast<IDerivedObject*>(originalObject); 
				originalObject = nodeObjectDer->GetObjRef();
					
			}
			nodeObject = originalObject;
		}

		//We already know we can convert this, otherwise it'd not be in m_nodes
		//Btw is this persistent? Need to delete it in that case TODO
		auto meshObject = reinterpret_cast<TriObject*>(nodeObject->ConvertToType(0, triObjectClassID));
		auto mesh = meshObject->GetMesh();

		//Write header for mesh (base+name)
		m_writer.beginChunk(0x400);

		m_writer.beginChunk(0x401);
		m_writer.writeName(node->GetName());
		m_writer.endChunk();

		//Write mesh information
		m_writer.beginChunk(0x402);
		m_writer.write(1);
		//TODO Might be local bbox? TODO
		auto tMatrix = node->GetObjTMBeforeWSM(0);
		auto bbox = mesh.getBoundingBox(&tMatrix);
		meshObject->GetWorldBoundBox(0, node, &m_interface->GetActiveViewExp(), bbox);
		m_writer.write(bbox.Min().x);
		m_writer.write(bbox.Min().y);
		m_writer.write(bbox.Min().z);
		m_writer.write(bbox.Max().x);
		m_writer.write(bbox.Max().y);
		m_writer.write(bbox.Max().z);
		m_writer.write(0);
		m_writer.write(static_cast<uint32_t>(node->IsHidden()));
		//TODO CollisionEnabled TODO
		m_writer.write(0);
		m_writer.fillBytes(88);
		m_writer.endChunk();

		//TODO Write sub-mesh material information TODO
		m_writer.beginChunk(0x10100);
		m_writer.beginChunk(0x10101);
		m_writer.writeName("alDefault.fx");
		m_writer.endChunk();
		m_writer.endChunk();

		//Write sub-mesh data
		m_writer.beginChunk(0x10000);
		
		//Please note that petro saves vertex normals per face in the vertices, which means we need to duplicate vertices if they're in multiple faces
		std::vector<Point3> normals;
		Utility::ComputeFaceNormals(&mesh, normals);

		//Rough algorithm: 
		//>Get vertices with multiple faces (adjList)
		//>Check for same normals
		//>Make table that projects file vertex id -> original vertex id in mesh + face id of face that fills that vertex
		//>Check for each face for each vertex where the next free+same vertex is
		std::vector<std::vector<int>> adjList;
		Utility::generateAdjacencyList(&mesh, adjList);
		std::vector<VertFaceRelation> relations;
		Utility::generateRelationList(&mesh, normals, adjList, relations);

		//Mesh information
		m_writer.beginChunk(0x10001);
		m_writer.write(static_cast<uint32_t>(relations.size()));
		m_writer.write(mesh.getNumFaces());
		m_writer.fillBytes(120);
		m_writer.endChunk();

		//TODO Vertex format Find out what the fuck that is supposed to be TODO
		m_writer.beginChunk(0x10002);
		m_writer.write("alD3dVertNU2", 12);
		m_writer.endChunk();

		//Vertex buffer

		//TODO look into different normal exception handling to allow more vertices TODO
		if(relations.size() > USHRT_MAX)
		{
			return false;
		}
		

		m_writer.beginChunk(0x10007);

		auto vertPos = m_writer.tellp();
		for(int j=0;j<relations.size();++j)
		{
			auto &vert = mesh.verts[relations[j].orgID];
			m_writer.write(vert.x);
			m_writer.write(vert.y);
			m_writer.write(vert.z);

			//Placeholder for normals
			m_writer.writeValues(0.f,3);

			//Texture coordinates
			//TODO test this. I'm literally guessing half of this shit TODO
			auto &texCoords = mesh.getTVert(relations[j].orgID);
			m_writer.write(texCoords.x);
			m_writer.write(0.f);
			m_writer.write(texCoords.y);
			m_writer.write(0.f);
			m_writer.write(texCoords.z);
			m_writer.write(0.f);
			m_writer.write(1.f);
			m_writer.write(0.f);
			//REGION Begin march of the useless data fields
			//TODO tangent and binormal. Check if this works. all 0 in example. why. TODO
			m_writer.writeValues(0.f,6);
			//Color. What. The. Fuck. Is. Wrong. With. This. Lazy. Format.
			//Plus it's all 1 in the example
			m_writer.writeValues(1.f,4);
			//Unused. as a float4. Yeah I can't even anymore, format of 4 is ignored with vert/norm anyway. Also, in example it's all 1 for an used vertex.
			m_writer.writeValues(1.f,4);
			//TODO Bone Indices. Use in animation TODO
			m_writer.writeValues(0,4);
			//Bone Weight. 1,0,0,0 expected, yet another useless field. Why do half of these even exist
			m_writer.write(1.f);
			m_writer.writeValues(0.f,3);	
		}
		m_writer.endChunk();

		//Index buffer
		m_writer.beginChunk(0x10004);
		for(int j=0;j<mesh.getNumFaces();++j)
		{
			auto &face = mesh.faces[j];
			for(int k=0;k<3;++k)
			{
				int idx = 0;
				for (int m = face.getVert(k); m < relations.size(); ++m)
				{
					if(face.getVert(k) == relations[m].orgID)
					{
						idx=m;
						break;
					}
				}
				while(relations[idx].faceID != -1 && !normals[relations[idx].faceID].Equals(normals[j]))
				{
					++idx;
				}
				if(relations[idx].faceID == -1)
				{
					relations[idx].faceID = j;
					m_writer.writeAt(vertPos + idx*144 + 12, normals[j].x);
					m_writer.writeAt(vertPos + idx*144 + 16, normals[j].y);
					m_writer.writeAt(vertPos + idx*144 + 20, normals[j].z);
				}
				m_writer.write(static_cast<unsigned short>(idx));
			}
		}
		m_writer.endChunk();

		//0x10000
		m_writer.endChunk();
		//0x400
		m_writer.endChunk();

		m_meshIDs.push_back(std::make_pair(meshID,i));
		++meshID;
		meshObject->DeleteThis();
	}
	return true;
}

bool Exporter::exportLight()
{
	return true;
}

bool Exporter::exportConnections()
{
	m_writer.beginChunk(0x600);

	m_writer.beginChunk(0x601);
	m_writer.beginMiniChunk(1);
	m_writer.write(static_cast<uint32_t>(m_meshIDs.size()));
	m_writer.endMiniChunk();
	m_writer.beginMiniChunk(4);
	m_writer.write(static_cast<uint32_t>(0));
	m_writer.endMiniChunk();
	m_writer.endChunk();
	
	for(auto pair: m_meshIDs)
	{
		m_writer.beginChunk(0x602);
		m_writer.beginMiniChunk(2);
		m_writer.write(pair.first);
		m_writer.endMiniChunk();
		
		m_writer.beginMiniChunk(3);
		m_writer.write(pair.second);
		m_writer.endMiniChunk();
		m_writer.endChunk();
	}

	m_writer.endChunk();
	return true;
}