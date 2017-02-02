#include "Exporter.h"

Exporter::Exporter(const TCHAR* name, ExpInterface* ei, Interface* ip, BOOL suppressPrompts, DWORD options)
	: m_name(name)
	, m_expInterface(ei)
	, m_interface(ip)
	, m_suppressPrompts(suppressPrompts)
	, m_options(options)
{
}

bool Exporter::exportALO()
{
	return true;
}
