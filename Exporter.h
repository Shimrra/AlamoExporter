#pragma once

#include "ChunkWriter.hpp"

#include <istdplug.h>
#include <impexp.h>
#include <stdint.h>

#include <string>
#include <fstream>
#include <vector>

class Exporter
{
public:
	Exporter(const TCHAR* name, ExpInterface* ei, Interface* ip, BOOL suppressPrompts, DWORD options);
	bool exportALO();

protected:
	void parseSkeleton(INode* node, Matrix3 parentTM, uint32_t parent);
	

	bool exportSkeleton();
	bool exportMesh();
	bool exportLight();
	bool exportConnections();

	std::wstring m_name;
	ExpInterface* m_expInterface;
	Interface* m_interface;
	int m_suppressPrompts;
	size_t m_options;

	std::filebuf m_fileBuf;
	ChunkWriter m_writer;

	std::vector<INode*> m_nodes;
	std::vector<std::pair<uint32_t,uint32_t>> m_meshIDs;
};
