#pragma once

#include <istdplug.h>
#include <impexp.h>

#include <string>

class Exporter
{
public:
	
	Exporter(const TCHAR* name, ExpInterface* ei, Interface* ip, BOOL suppressPrompts, DWORD options);
	bool exportALO();

	std::wstring m_name;
	ExpInterface* m_expInterface;
	Interface* m_interface;
	bool m_suppressPrompts;
	size_t m_options;
};