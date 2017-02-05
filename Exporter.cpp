#include "Exporter.h"

#include "Max.h"

#include <iosfwd>
#include <lslights.h>
#include <Esent.h>
#include <modstack.h>

Exporter::Exporter(const TCHAR* name, ExpInterface* ei, Interface* ip, BOOL suppressPrompts, DWORD options)
	: m_name(name)
	, m_expInterface(ei)
	, m_interface(ip)
	, m_suppressPrompts(suppressPrompts)
	, m_options(options)
	, m_oStream(&m_fileBuf)
{
	m_fileBuf.open(m_name, std::ios::out | std::ios::binary);
}


template <typename T>
void Exporter::write(const T &value)
{
	write(value, sizeof(value));
}
template <typename T>
void Exporter::write(const T &value, const size_t &size)
{
	m_oStream.write(reinterpret_cast<const char*>(&value), size);
}
template <typename T>
void Exporter::writeAt(const size_t& position, const T& value)
{
	writeAt(position, value, sizeof(value));
}
template <typename T>
void Exporter::writeAt(const size_t& position, const T& value, const size_t& size)
{
	auto curPos = m_oStream.tellp();
	m_oStream.seekp(position);
	write(value, size);
	m_oStream.seekp(curPos);
}

void Exporter::writeMatrix(const Matrix3& m)
{
	Point3 row;
	for (int i = 0; i < 3; ++i)
	{
		row = m.GetRow(i);
		write(row.x);
		write(row.y);
		write(row.z);
		write(0.f);
	}
}

bool Exporter::exportALO()
{
	if(!m_fileBuf.is_open()) return false;

	return exportSkeleton() && exportMesh() && exportLight() && exportConnections();
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
		write(0x202);
		write(uint32_t());
		
		//Bone name
		auto boneStart = m_oStream.tellp();
		write(0x203);
		write(uint32_t());
		
		//Converting the name from wchar_t to regular char*
		const wchar_t *name;
		if(!node->GetObjectRef())
		{
			name = L"Root";
		}
		else
		{
			name = node->GetName();
		}
		char nameMB[1024];
		WideCharToMultiByte(CP_ACP, 0, name, -1, nameMB, wcslen(name), nullptr,nullptr);
		write(nameMB, wcslen(name));
		write(short(0),1);
		writeAt(4+boneStart, static_cast<uint32_t>(m_oStream.tellp()-boneStart-8));

		//Bone Data (v2)
		write(0x206);
		write(60);
		write(parent);
		write(static_cast<uint32_t>(!node->IsHidden()));
		//TODO Billboarding TODO
		write(0);

		//Get TM for node
		
		if(!node->GetObjectRef())
		{
			writeMatrix(node->GetObjectTM(0));
		}
		else
		{
			/*Matrix3 matrix(1);
			Point3 pos = node->GetObjOffsetPos();
			matrix.PreTranslate(pos);
			Quat quat = node->GetObjOffsetRot();
			PreRotateMatrix(matrix, quat);
			ScaleValue scaleValue = node->GetObjOffsetScale();
			ApplyScaling(matrix, scaleValue);*/
			writeMatrix(node->GetObjTMBeforeWSM(1));
			//writeMatrix(node->GetObjectTM(1));
		}

		writeAt(-4+boneStart, static_cast<uint32_t>(m_oStream.tellp()-boneStart)+(1<<31));
	}

	for (int i=0;i<node->NumberOfChildren();++i)
	{
		parseSkeleton(node->GetChildNode(i), newParentID);
	}

}

bool Exporter::exportSkeleton()
{
	//Skeleton container with placeholder size
	write(0x200);
	write(uint32_t());
	
	//Bone count container
	write(0x201);
	write(uint32_t(128));
	auto boneCountPosition = m_oStream.tellp();
	byte emptySpace[128];
	std::fill(&emptySpace[0], &emptySpace[127], 0);
	write(emptySpace);

	auto root = m_interface->GetRootNode();
	parseSkeleton(root, 0xffffffff);
	writeAt(boneCountPosition, static_cast<uint32_t>(m_nodes.size()));
	
	writeAt(4, static_cast<uint32_t>(m_oStream.tellp())-8 + (1<<31));
	
	
	return true;
}

bool Exporter::exportMesh()
{
	uint32_t meshID = 0;
	for(int i=1;i<m_nodes.size();++i)
	{
		auto node = m_nodes[i];
		auto nodeObject = node->GetObjectRef();
		if(node->IsSubClassOf(LIGHTSCAPE_LIGHT_CLASS) || node->IsSubClassOf(BONE_OBJ_CLASSID))
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
		write(0x400);
		auto meshPos = static_cast<uint32_t>(m_oStream.tellp());
		write(uint32_t());
		write(0x401);
		auto name = node->GetName();
		char nameMB[1024];
		WideCharToMultiByte(CP_ACP, 0, name, -1, nameMB, wcslen(name), nullptr,nullptr);
		write(static_cast<uint32_t>(wcslen(name)+1));
		write(nameMB, wcslen(name));
		write(short(0),1);

		//Write mesh information
		write(0x402);
		write(128);
		write(1);
		//TODO Might be local bbox? TODO
		auto tMatrix = node->GetObjTMBeforeWSM(0);
		auto bbox = mesh.getBoundingBox(&tMatrix);
		meshObject->GetWorldBoundBox(0, node, &m_interface->GetActiveViewExp(), bbox);
		write(bbox.Min().x);
		write(bbox.Min().y);
		write(bbox.Min().z);
		write(bbox.Max().x);
		write(bbox.Max().y);
		write(bbox.Max().z);
		write(0);
		write(static_cast<uint32_t>(node->IsHidden()));
		//TODO CollisionEnabled TODO
		write(0);
		byte emptySpace[88];
		std::fill(&emptySpace[0], &emptySpace[87], 0);
		write(emptySpace);

		//TODO Write sub-mesh material information TODO
		write(0x10100);
		write(static_cast<uint32_t>(21+(1<<31)));
		write(0x10101);
		write(13);
		write("alDefault.fx", 12);
		write(short(0),1);

		//Write sub-mesh data
		write(0x10000);
		auto subMeshPos = static_cast<uint32_t>(m_oStream.tellp());
		write(uint32_t());

		//Mesh information
		write(0x10001);
		write(128);
		write(mesh.getNumVerts());
		write(mesh.getNumFaces());
		byte emptySpace2[120];
		std::fill(&emptySpace2[0], &emptySpace2[119], 0);
		write(emptySpace2);
		//TODO Vertex format Find out what the fuck that is supposed to be TODO
		write(0x10002);
		write(13);
		write("alD3dVertNU2", 12);
		write(short(0),1);
		//Index buffer
		write(0x10004);
		write(mesh.getNumFaces()*6);
		for(int j=0;j<mesh.getNumFaces();++j)
		{
			auto &face = mesh.faces[j];
			write(static_cast<short>(face.getVert(0)));
			write(static_cast<short>(face.getVert(1)));
			write(static_cast<short>(face.getVert(2)));
		}
		//Vertex buffer
		write(0x10007);
		write(mesh.getNumVerts()*144);
		for(int j=0;j<mesh.getNumVerts();++j)
		{
			auto &vert = mesh.verts[j];
			write(vert.x);
			write(vert.y);
			write(vert.z);

			/*auto &normal = mesh.;
			write(normal.x);
			write(normal.y);
			write(normal.z);*/

			//Texture coordinates
			//TODO test this. I'm literally guessing half of this shit TODO
			auto &texCoords = mesh.getTVert(j);
			write(texCoords.x);
			write(0.f);
			write(texCoords.y);
			write(0.f);
			write(texCoords.z);
			write(0.f);
			write(1.f);
			write(0.f);
			//REGION Begin march of the useless data fields
			//TODO tangent and binormal. Check if this works. all 0 in example. why. TODO
			write(0);
			write(0);
			write(0);
			write(0);
			write(0);
			write(0);
			//Color. What. The. Fuck. Is. Wrong. With. This. Lazy. Format.
			//Plus it's all 1 in the example
			write(1.f);
			write(1.f);
			write(1.f);
			write(1.f);
			//Unused. as a float4. Yeah I can't even anymore. In example it's all 1 for an used vertex.
			write(1.f);
			write(1.f);
			write(1.f);
			write(1.f);
			//TODO Bone Indices. Use in animation TODO
			write(0);
			write(0);
			write(0);
			write(0);
			//Bone Weight. 1,0,0,0 expected, yet another useless field. Why do half of these even exist
			write(0);
			write(0);
			write(0);
			write(0);
		}
		
		auto length = static_cast<uint32_t>(m_oStream.tellp())-subMeshPos - 4 + (1<<31);
		writeAt(subMeshPos, length);

		writeAt(meshPos, static_cast<uint32_t>(m_oStream.tellp())-meshPos - 4 + (1<<31));

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
	
	write(0x600);

	auto connPos = m_oStream.tellp();
	write(uint32_t());
	write(0x601);
	write(static_cast<uint32_t>(12));
	write(static_cast<byte>(0x1),1);
	write(static_cast<byte>(4),1);
	write(static_cast<uint32_t>(m_meshIDs.size()));
	write(static_cast<byte>(0x4),1);
	write(static_cast<byte>(4),1);
	write(static_cast<uint32_t>(0));
	
	for(auto pair: m_meshIDs)
	{
		write(0x602);
		write(12);
		write(static_cast<byte>(0x2),1);
		write(static_cast<byte>(4),1);
		write(pair.first);
		write(static_cast<byte>(0x3),1);
		write(static_cast<byte>(4),1);
		write(pair.second);
	}
	
	writeAt(connPos, static_cast<uint32_t>(m_oStream.tellp()-connPos) - 4 + (1<<31));
	
	return true;
}
