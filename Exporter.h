#pragma once

#include <istdplug.h>
#include <impexp.h>

#include <string>
#include <fstream>

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
};
