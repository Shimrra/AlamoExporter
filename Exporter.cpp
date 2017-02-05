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
			auto nodeObjectDer = reinterpret_cast<IDerivedObject *>(nodeObject); 
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
		//Billboarding TODO
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
{/*
	for(auto node: m_nodes)
	{
		if(!node->IsSubClassOf(LIGHTSCAPE_LIGHT_CLASS) && !node->IsSubClassOf(BONE_OBJ_CLASSID))
		{
			
			while (node->SuperClassID() == GEN_DERIVOB_CLASS_ID)
			{
				auto nodeObjectDer = reinterpret_cast<IDerivedObject *>(nodeObject); 
			}
		}
	}*/
	return true;
}

bool Exporter::exportLight()
{
	return true;
}

bool Exporter::exportConnections()
{
	write(0x600);
	write(static_cast<uint32_t>(20+(1<<31)));
	write(0x601);
	write(static_cast<uint32_t>(12));
	write(static_cast<byte>(0x1),1);
	write(static_cast<byte>(4),1);
	write(static_cast<uint32_t>(0));
	write(static_cast<byte>(0x4),1);
	write(static_cast<byte>(4),1);
	write(static_cast<uint32_t>(0));
	return true;
}
