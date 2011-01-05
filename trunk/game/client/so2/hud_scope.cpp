// Originally implemented by Stephen 'SteveUK' Swires for SO
// It has since been modified to support SO2
// Thanks Steve =)

#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "vgui/ISurface.h"
#include "input.h"

#include "weapon_sobase.h"
#include "c_so_player.h"

#include <vgui/IScheme.h>
#include <vgui_controls/Panel.h>

#define SCOPE_INITIAL_SIZE 256

// memdbgon must be the last include file in a .cpp file!
#include "tier0/memdbgon.h"

class CHudScope : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudScope, vgui::Panel );

public:
	CHudScope( const char *pElementName );

public:
	void Init();
	void MsgFunc_ShowScope( bf_read &msg );

protected:
	void ApplySchemeSettings(vgui::IScheme *scheme);
	void Paint( void );
	void PaintBackground( void );
	void Think( void );
	bool ShouldDraw( void );

private:
	bool m_bStopCheck;
	CHudTexture* m_pScopeArc;
	bool m_bShow;
	int SCOPE_SIZE;
};

DECLARE_HUDELEMENT( CHudScope );
DECLARE_HUD_MESSAGE( CHudScope, ShowScope );

using namespace vgui;

CHudScope::CHudScope( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudScope" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_bShow = false;
	m_bStopCheck = false;

	// Scope will not show when the player is dead
	SetHiddenBits( HIDEHUD_PLAYERDEAD );

	int screenWide, screenTall;
	GetHudSize( screenWide, screenTall );
	SetBounds( 0, 0, screenWide, screenTall );
}

void CHudScope::Init()
{
	HOOK_HUD_MESSAGE( CHudScope, ShowScope );
}

void CHudScope::Think( void )
{
	if ( ShouldDraw() )
	{
		if ( ::input->CAM_IsThirdPerson() )
		{
			::input->CAM_ToFirstPerson();

			if ( !m_bStopCheck )
				m_bStopCheck = true;
		}
	}
	else
	{
		if ( m_bStopCheck )
		{
			::input->CAM_ToThirdPerson();
			m_bStopCheck = false;
		}
	}
}

void CHudScope::PaintBackground()
{  
    SetBgColor( Color(0, 0, 0, 0) );
   
	int s_wide, s_tall;
	surface()->GetScreenSize( s_wide, s_tall );
	SCOPE_SIZE = scheme()->GetProportionalScaledValue( SCOPE_INITIAL_SIZE );
	
    SetPaintBorderEnabled( false );
	BaseClass::PaintBackground();
}

bool CHudScope::ShouldDraw()
{
	CSO_Player* pLocalPlayer = ToSOPlayer( C_BasePlayer::GetLocalPlayer() );
	if( pLocalPlayer )
	{
		CWeaponSOBase *pWeapon = dynamic_cast<CWeaponSOBase*>( pLocalPlayer->GetActiveWeapon() );
		if( pWeapon && pWeapon->ShouldDrawScope() )
			return true;
	}

	// default behaviour for legacy support
	return m_bShow;	// is this still needed?
}

void CHudScope::Paint( void )
{
	if ( !ShouldDraw() )
		return;

	int s_wide, s_tall;
	surface()->GetScreenSize( s_wide, s_tall );
	SCOPE_SIZE = scheme()->GetProportionalScaledValue( SCOPE_INITIAL_SIZE );

	int scope_width = s_tall; // force 4:3
	int scope_x = (s_wide / 2) - (scope_width / 2); // very left side of the scope

	m_pScopeArc->DrawSelf( scope_x, 0, scope_width, s_tall, Color(255, 255, 255, 255) );

	// lines on scope (actually filled rectangles now):
	const int line_width = 5;
	surface()->DrawSetColor( Color(0, 0, 0, 150) );
	surface()->DrawFilledRect( (s_wide / 2) - (line_width / 2), 0, (s_wide / 2) + (line_width / 2), s_tall ); // vertical
	surface()->DrawFilledRect( scope_x, (s_tall / 2) - (line_width / 2), scope_x + scope_width, (s_tall / 2) + (line_width / 2) ); // horizontal

	// side fillers
	if( scope_x > 0 )
	{
		surface()->DrawSetColor( 0, 0, 0, 255 );
		surface()->DrawFilledRect( 0, 0, scope_x, s_tall ); // left
		surface()->DrawFilledRect( scope_x + scope_width, 0, s_wide, s_tall );
	}
}

void CHudScope::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings(scheme);

	if ( !m_pScopeArc )
		m_pScopeArc = gHUD.GetIcon( "scope" );
}

void CHudScope::MsgFunc_ShowScope(bf_read &msg)
{
	m_bShow = msg.ReadByte();
}
