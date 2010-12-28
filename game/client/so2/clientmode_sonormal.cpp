#include "cbase.h"
#include "clientmode_sonormal.h"
#include "vgui_int.h"
#include "hud.h"
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include "iinput.h"
#include "hl2mpclientscoreboard.h"
#include "hl2mptextwindow.h"
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
vgui::HScheme g_hVGuiCombineScheme = 0;

// Instance the singleton and expose the interface to it.
IClientMode *GetClientModeNormal()
{
	static ClientModeSONormal g_ClientModeNormal;
	return &g_ClientModeNormal;
}

ClientModeSONormal* GetClientModeSONormal()
{
	Assert( dynamic_cast< ClientModeSONormal* >( GetClientModeNormal() ) );

	return static_cast< ClientModeSONormal* >( GetClientModeNormal() );
}

//-----------------------------------------------------------------------------
// Purpose: this is the viewport that contains all the hud elements
//-----------------------------------------------------------------------------
class CHudViewport : public CBaseViewport
{
private:
	DECLARE_CLASS_SIMPLE( CHudViewport, CBaseViewport );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		gHUD.InitColors( pScheme );

		SetPaintBackgroundEnabled( false );
	}

	virtual IViewPortPanel *CreatePanelByName( const char *szPanelName );
};

int ClientModeSONormal::GetDeathMessageStartHeight( void )
{
	int x = YRES(2);

	IViewPortPanel *spectator = gViewPortInterface->FindPanelByName( PANEL_SPECGUI );

	//TODO: Link to actual height of spectator bar
	if ( spectator && spectator->IsVisible() )
	{
		x += YRES(52);
	}

	return x;
}

IViewPortPanel* CHudViewport::CreatePanelByName( const char *szPanelName )
{
	IViewPortPanel* newpanel = NULL;

	if ( Q_strcmp( PANEL_SCOREBOARD, szPanelName) == 0 )
	{
		newpanel = new CHL2MPClientScoreBoardDialog( this );
		return newpanel;
	}
	else if ( Q_strcmp(PANEL_INFO, szPanelName) == 0 )
	{
		newpanel = new CHL2MPTextWindow( this );
		return newpanel;
	}
	else if ( Q_strcmp(PANEL_SPECGUI, szPanelName) == 0 )
	{
		newpanel = new CHL2MPSpectatorGUI( this );	
		return newpanel;
	}

	return BaseClass::CreatePanelByName( szPanelName ); 
}

//-----------------------------------------------------------------------------
// ClientModeHLNormal implementation
//-----------------------------------------------------------------------------
ClientModeSONormal::ClientModeSONormal()
{
	m_pViewport = new CHudViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeSONormal::~ClientModeSONormal()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeSONormal::Init()
{
	BaseClass::Init();

	// Load up the combine control panel scheme
	g_hVGuiCombineScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/CombinePanelScheme.res", "CombineScheme" );
	if (!g_hVGuiCombineScheme)
	{
		Warning( "Couldn't load combine panel scheme!\n" );
	}
}
