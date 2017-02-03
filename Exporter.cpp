#include "Exporter.h"

#include <iosfwd>
#include <sstream>
#include <stdint.h>

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
	m_oStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}
template <typename T>
void Exporter::write(const T &value, const size_t &size)
{
	m_oStream.write(reinterpret_cast<const char*>(&value), size);
}

bool Exporter::exportALO()
{
	if(!m_fileBuf.is_open()) return false;

	return exportSkeleton() && exportMesh() && exportLight() && exportConnections();
}

bool Exporter::exportSkeleton()
{
	uint32_t sizeTotal = 12;
	std::stringstream data;
	data << "YoDank";
	
	//Skeleton container
	write(0x200);
	write(sizeTotal);
	//We're doubling memory here, do we really want to do that? TODO
	std::string dataString(data.str());
	write(dataString, dataString.size());
	
	return true;
}

bool Exporter::exportMesh()
{
	unsigned int sizeTotal = 0;

	return true;
}

bool Exporter::exportLight()
{
	unsigned int sizeTotal = 0;

	return true;
}

bool Exporter::exportConnections()
{
	unsigned int sizeTotal = 0;

	return true;
}
