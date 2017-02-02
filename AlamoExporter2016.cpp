//**************************************************************************/
// Copyright (c) 1998-2007 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: Appwizard generated plugin
// AUTHOR: 
//***************************************************************************/

#include "AlamoExporter2016.h"
#include "Exporter.h"


#define AlamoExporter2016_CLASS_ID	Class_ID(0xcea82c41, 0xa7b5c9ab)

class AlamoExporter2016 : public SceneExport {
public:
	//Constructor/Destructor
	AlamoExporter2016();
	~AlamoExporter2016();

	int				ExtCount();					// Number of extensions supported
	const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
	const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
	const TCHAR *	AuthorName();				// ASCII Author name
	const TCHAR *	CopyrightMessage();			// ASCII Copyright message
	const TCHAR *	OtherMessage1();			// Other message #1
	const TCHAR *	OtherMessage2();			// Other message #2
	unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
	void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box

	BOOL SupportsOptions(int ext, DWORD options);
	int  DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0);
};



class AlamoExporter2016ClassDesc : public ClassDesc2 
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) 		{ return new AlamoExporter2016(); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID SuperClassID() 				{ return SCENE_EXPORT_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return AlamoExporter2016_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() 			{ return _T("AlamoExporter2016"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle
	

};


ClassDesc2* GetAlamoExporter2016Desc() { 
	static AlamoExporter2016ClassDesc AlamoExporter2016Desc;
	return &AlamoExporter2016Desc; 
}





INT_PTR CALLBACK AlamoExporter2016OptionsDlgProc(HWND hWnd,UINT message,WPARAM,LPARAM lParam) {
	static AlamoExporter2016* imp = nullptr;

	switch(message) {
		case WM_INITDIALOG:
			imp = (AlamoExporter2016 *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));
			return TRUE;

		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return 1;
	}
	return 0;
}


//--- AlamoExporter2016 -------------------------------------------------------
AlamoExporter2016::AlamoExporter2016()
{

}

AlamoExporter2016::~AlamoExporter2016() 
{

}

int AlamoExporter2016::ExtCount()
{
	return 1;
}

const TCHAR *AlamoExporter2016::Ext(int /*i*/)
{		
	#pragma message(TODO("Return the 'i-th' file name extension (i.e. \"3DS\")."))
	return _T("alo");
}

const TCHAR *AlamoExporter2016::LongDesc()
{
	return _T("Alamo Object (Petroglyph)");
}
	
const TCHAR *AlamoExporter2016::ShortDesc() 
{			
	return _T("Alamo");
}

const TCHAR *AlamoExporter2016::AuthorName()
{			
	#pragma message(TODO("Return ASCII Author name"))
	return _T("Shim");
}

const TCHAR *AlamoExporter2016::CopyrightMessage() 
{	
	return _T("");
}

const TCHAR *AlamoExporter2016::OtherMessage1() 
{		
	//TODO: Return Other message #1 if any
	return _T("");
}

const TCHAR *AlamoExporter2016::OtherMessage2() 
{		
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int AlamoExporter2016::Version()
{				
	#pragma message(TODO("Return Version number * 100 (i.e. v3.01 = 301)"))
	return 100;
}

void AlamoExporter2016::ShowAbout(HWND /*hWnd*/)
{			
	// Optional
}

BOOL AlamoExporter2016::SupportsOptions(int /*ext*/, DWORD /*options*/)
{
	#pragma message(TODO("Decide which options to support.  Simply return true for each option supported by each Extension the exporter supports."))
	return TRUE;
}


int	AlamoExporter2016::DoExport(const TCHAR* name, ExpInterface* ei, Interface* ip, BOOL suppressPrompts, DWORD options)
{
	if(!suppressPrompts)
	{
		DialogBoxParam(hInstance, 
				MAKEINTRESOURCE(IDD_PANEL), 
				GetActiveWindow(), 
				AlamoExporter2016OptionsDlgProc, (LPARAM)this);	
	}

	Exporter exporter(name,ei,ip,suppressPrompts,options);
	
	return exporter.exportALO();
}


