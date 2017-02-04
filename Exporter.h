#pragma once

#include <istdplug.h>
#include <impexp.h>
#include <stdint.h>

#include <string>
#include <fstream>
#include <vector>
#include <Matrix3.h>

class Exporter
{
public:
	Exporter(const TCHAR* name, ExpInterface* ei, Interface* ip, BOOL suppressPrompts, DWORD options);
	bool exportALO();

protected:
	
	template<typename T>
	void write(const T &value);
	template<typename T>
	void write(const T &value, const size_t &size);

	template<typename T>
	void writeAt(const size_t& position, const T& value);
	template<typename T>
	void writeAt(const size_t& position, const T& value, const size_t& size);

	void writeMatrix(const Matrix3 &m);

	void parseSkeleton(INode* node, uint32_t parent);

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
	std::ostream m_oStream;

	std::vector<INode*> m_nodes;
};
