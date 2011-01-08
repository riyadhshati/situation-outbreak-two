// Character customization system

#include <vgui/IVGui.h>
#include <vgui_controls/ImagePanel.h>

using namespace vgui;

class ICustomizationPanel
{
public:
	virtual void Create( VPANEL parent ) = 0;
	virtual void Destroy( void ) = 0;
};
extern ICustomizationPanel* customizationpanel;

class CClassImagePanel : public ImagePanel
{
public:
	typedef ImagePanel BaseClass;
 
	CClassImagePanel( Panel* pParent, const char* pName );
	virtual ~CClassImagePanel();
	virtual void UpdatePlayerModel( void );
	virtual void ApplySettings( KeyValues* inResourceData );
	virtual void Paint();

public:
	char m_ModelName[128];
};
extern CUtlVector<CClassImagePanel*> g_ClassImagePanels;
