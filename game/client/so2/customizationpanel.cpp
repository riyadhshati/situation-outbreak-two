// Character customization system

#include "cbase.h"
#include "customizationpanel.h"
#include "so_player_shared.h"

#include <vgui_controls/Frame.h>

ConVar cl_showcustomizationpanel( "cl_showcustomizationpanel", "0", FCVAR_CLIENTDLL, "Sets the state of the customization panel" );

CON_COMMAND( ToggleCustomizationPanel, "Toggles the customization panel" )
{
	cl_showcustomizationpanel.SetValue( !cl_showcustomizationpanel.GetInt() );
};

extern ConVar cl_playermodel_headgear;
extern ConVar cl_playermodel_glasses;
extern ConVar cl_playermodel_commdevice;

class CCustomizationPanel : public Frame
{
public:
	DECLARE_CLASS_SIMPLE( CCustomizationPanel, Frame ); 
 
	CCustomizationPanel( VPANEL parent );
	~CCustomizationPanel() {};

	Panel *CreateControlByName( const char *controlName );
 
protected:
	virtual void OnTick( void );
	virtual void OnCommand( const char* pcCommand );
};

// Constuctor: Initializes the Panel
CCustomizationPanel::CCustomizationPanel( VPANEL parent ) : BaseClass( NULL, "CustomizationPanel" )
{
	SetParent( parent );
 
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );
 
	SetProportional( true );
	SetTitleBarVisible( true );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( true );
	SetSizeable( false );
	SetMoveable( true );
	SetVisible( true );
 
	SetScheme( scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme") );
 
	LoadControlSettings( "resource/ui/customizationpanel.res" );
 
	ivgui()->AddTickSignal( GetVPanel(), 100 );
}

Panel* CCustomizationPanel::CreateControlByName( const char* controlName )
{
	if ( V_stricmp(controlName, "ClassImagePanel") == 0 )
		return new CClassImagePanel( NULL, controlName );
 
	return BaseClass::CreateControlByName( controlName );
}

void CCustomizationPanel::OnTick()
{
	SetVisible( cl_showcustomizationpanel.GetBool() );

	BaseClass::OnTick();
}

void CCustomizationPanel::OnCommand( const char* pcCommand )
{
	if ( !Q_stricmp(pcCommand, "close") )
	{
		cl_showcustomizationpanel.SetValue( 0 );
	}
	else if ( !Q_stricmp(pcCommand, "headgear_button_increment") )
	{
		int currentValue = cl_playermodel_headgear.GetInt();
		if ( currentValue < BODYGROUP_HEADGEAR_MAX )
			cl_playermodel_headgear.SetValue( currentValue + 1 );
	}
	else if ( !Q_stricmp(pcCommand, "headgear_button_decrement") )
	{
		int currentValue = cl_playermodel_headgear.GetInt();
		if ( currentValue > 0 )
			cl_playermodel_headgear.SetValue( currentValue - 1 );
	}
	else if ( !Q_stricmp(pcCommand, "glasses_button_increment") )
	{
		int currentValue = cl_playermodel_glasses.GetInt();
		if ( currentValue < BODYGROUP_GLASSES_MAX )
			cl_playermodel_glasses.SetValue( currentValue + 1 );
	}
	else if ( !Q_stricmp(pcCommand, "glasses_button_decrement") )
	{
		int currentValue = cl_playermodel_glasses.GetInt();
		if ( currentValue > 0 )
			cl_playermodel_glasses.SetValue( currentValue - 1 );
	}
	else if ( !Q_stricmp(pcCommand, "commdevice_button_increment") )
	{
		int currentValue = cl_playermodel_commdevice.GetInt();
		if ( currentValue < BODYGROUP_COMMDEVICE_MAX )
			cl_playermodel_commdevice.SetValue( currentValue + 1 );
	}
	else if ( !Q_stricmp(pcCommand, "commdevice_button_decrement") )
	{
		int currentValue = cl_playermodel_commdevice.GetInt();
		if ( currentValue > 0 )
			cl_playermodel_commdevice.SetValue( currentValue - 1 );
	}

	BaseClass::OnTick();
}

class CCustomizationPanelInterface : public ICustomizationPanel
{
private:
	CCustomizationPanel *CustomizationPanel;

public:
	CCustomizationPanelInterface()
	{
		CustomizationPanel = NULL;
	}

	void Create( VPANEL parent )
	{
		CustomizationPanel = new CCustomizationPanel( parent );
	}

	void Destroy()
	{
		if ( CustomizationPanel )
		{
			CustomizationPanel->SetParent( (Panel *)NULL );
			delete CustomizationPanel;
		}
	}
};
static CCustomizationPanelInterface g_CustomizationPanel;
ICustomizationPanel* customizationpanel = (ICustomizationPanel*)&g_CustomizationPanel;

CUtlVector<CClassImagePanel*> g_ClassImagePanels;

CClassImagePanel::CClassImagePanel( Panel* pParent, const char* pName ) : ImagePanel( pParent, pName )
{
	g_ClassImagePanels.AddToTail( this );
	m_ModelName[0] = 0;
}
 
CClassImagePanel::~CClassImagePanel()
{
	g_ClassImagePanels.FindAndRemove( this );
}

void CClassImagePanel::UpdatePlayerModel( void )
{
	const ConVar *cl_playermodel = cvar->FindVar( "cl_playermodel" );
	if ( cl_playermodel )
	{
		const char* pName = cl_playermodel->GetString();
		if ( pName )
			V_strncpy( m_ModelName, pName, sizeof(m_ModelName) );
	}
}

void CClassImagePanel::ApplySettings( KeyValues* inResourceData )
{
	UpdatePlayerModel();
 
	BaseClass::ApplySettings( inResourceData );
}
 
void CClassImagePanel::Paint()
{
	UpdatePlayerModel();

	BaseClass::Paint();
}
