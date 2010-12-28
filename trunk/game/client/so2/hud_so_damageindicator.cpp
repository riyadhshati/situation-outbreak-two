// SO2 - James
// Full-screen damage indicator
// Uses images depicting blood splatter of varying intensity to give the player a rough idea of how much health they have

#include "cbase.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include "hudelement.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INIT_BLOOD_SPLATER_NUM -1
#define NUM_STAGES 4	// remember to change this number if we add more stages

ConVar so_client_heartbeat_sounds( "so_client_heartbeat_sounds", "1", FCVAR_CLIENTDLL, "Enable heartbeat sounds when health is low" );

class CHudSODamageIndicator : public CHudElement, public Panel
{
	DECLARE_CLASS_SIMPLE( CHudSODamageIndicator, Panel );

public:
	CHudSODamageIndicator( const char *pElementName );

	void Init( void );
	void VidInit( void );
	void Reset( void );
	bool ShouldDraw( void );

private:
	void Paint();
	void DrawFullscreenDamageIndicator();
	void HandleHeartbeatSounds();
	void ApplySchemeSettings( IScheme *pScheme );

private:
	CPanelAnimationVar( Color, m_DmgFullscreenColor, "DmgFullscreenColor", "255 255 255 255" );

	CMaterialReference m_BloodSplatterMaterials[NUM_STAGES];

	void UpdateBloodSplatterToDisplay( void );
	int m_iBloodSplatter, m_iPlayerHealth;
	bool m_bHealthHasChangedRecently;
};

DECLARE_HUDELEMENT( CHudSODamageIndicator );

static const char* kBloodSplatter[NUM_STAGES] =	// more stages can be added here (remember to change NUM_STAGES too)
{
	"VGUI/bloodsplatter/stage1",
	"VGUI/bloodsplatter/stage2",
	"VGUI/bloodsplatter/stage3",
	"VGUI/bloodsplatter/stage4"
};

CHudSODamageIndicator::CHudSODamageIndicator( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudSODamageIndicator" )
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT | HIDEHUD_WEAPONSELECTION );

	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	for ( int i = 0; i < NUM_STAGES; i++ )
		m_BloodSplatterMaterials[i].Init( kBloodSplatter[i], TEXTURE_GROUP_VGUI );
}

void CHudSODamageIndicator::Init()
{
	Reset();
}

void CHudSODamageIndicator::VidInit()
{
	Reset();
}

void CHudSODamageIndicator::Reset( void )
{
	m_iBloodSplatter = INIT_BLOOD_SPLATER_NUM;
	m_iPlayerHealth = 0;
	m_bHealthHasChangedRecently = false;
}

bool CHudSODamageIndicator::ShouldDraw( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && ((pPlayer->GetTeamNumber() == TEAM_SPECTATOR) || !pPlayer->IsAlive()) )
		return false;	// don't do anything here if we're spectating or not alive

	// Refresh some stuff before we check whether or not to draw (kind of a hack, but oh well)
	UpdateBloodSplatterToDisplay();
	HandleHeartbeatSounds();

	if ( (m_iBloodSplatter <= INIT_BLOOD_SPLATER_NUM) || (m_iBloodSplatter >= NUM_STAGES) )
		return false;	// out of bounds; will crash the game; do not show any blood splatter

	return CHudElement::ShouldDraw();
}

// Determines what stage of blood splatter to use based on the player's current health
void CHudSODamageIndicator::UpdateBloodSplatterToDisplay( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
	{
		m_iBloodSplatter = INIT_BLOOD_SPLATER_NUM;	// can't find local player; don't show any blood splatter
		return;
	}

	int savedPlayerHealth = pPlayer->GetHealth();
	if ( savedPlayerHealth == m_iPlayerHealth )
		return;	// the player's health hasn't changed since we last checked, so there's nothing else that needs to be done here

	m_bHealthHasChangedRecently = true;	// remember the fact that our health has changed recently
	m_iPlayerHealth = savedPlayerHealth;	// save the player's new health value to our class variable

	if ( m_iPlayerHealth <= 0 )
		m_iBloodSplatter = 3;
	else if ( m_iPlayerHealth < 25 )
		m_iBloodSplatter = 2;
	else if ( m_iPlayerHealth < 50 )
		m_iBloodSplatter = 1;
	else if ( m_iPlayerHealth < 75 )
		m_iBloodSplatter = 0;
	else
		m_iBloodSplatter = INIT_BLOOD_SPLATER_NUM;	// we must be pretty healthy, so don't show any blood splatter
}

void CHudSODamageIndicator::HandleHeartbeatSounds( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( so_client_heartbeat_sounds.GetBool() )
	{
		if ( m_iPlayerHealth > 0 )
		{
			CLocalPlayerFilter filter;

			if ( m_iPlayerHealth < 25 )
			{
				pPlayer->StopSound( "Heartbeat.Slow" );
				pPlayer->EmitSound( filter, -1, "Heartbeat.Fast" );
			}
			else if ( m_iPlayerHealth < 50 )
			{
				pPlayer->StopSound( "Heartbeat.Fast" );
				pPlayer->EmitSound( filter, -1, "Heartbeat.Slow" );
			}
			else
			{
				pPlayer->StopSound( "Heartbeat.Fast" );
				pPlayer->StopSound( "Heartbeat.Slow" );
			}
		}
		else
		{
			pPlayer->StopSound( "Heartbeat.Fast" );
			pPlayer->StopSound( "Heartbeat.Slow" );
		}
	}
	else
	{
		pPlayer->StopSound( "Heartbeat.Fast" );
		pPlayer->StopSound( "Heartbeat.Slow" );
	}
}

void CHudSODamageIndicator::DrawFullscreenDamageIndicator()
{
	CMatRenderContextPtr pRenderContext( materials );
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_BloodSplatterMaterials[m_iBloodSplatter] );
	if ( !pMesh )
		return;	// don't let bad things happen...

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	int r = m_DmgFullscreenColor[0], g = m_DmgFullscreenColor[1], b = m_DmgFullscreenColor[2], a = m_DmgFullscreenColor[3];
	float wide = GetWide(), tall = GetTall();

	meshBuilder.Color4ub( r, g, b, a );
	meshBuilder.TexCoord2f( 0, 0, 0 );
	meshBuilder.Position3f( 0.0f, 0.0f, 0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( r, g, b, a );
	meshBuilder.TexCoord2f( 0, 1, 0 );
	meshBuilder.Position3f( wide, 0.0f, 0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( r, g, b, a );
	meshBuilder.TexCoord2f( 0, 1, 1 );
	meshBuilder.Position3f( wide, tall, 0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( r, g, b, a );
	meshBuilder.TexCoord2f( 0, 0, 1 );
	meshBuilder.Position3f( 0.0f, tall, 0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	if ( m_bHealthHasChangedRecently )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudSODamageIndicatorBlink" );	// blink (quick fade out and in)
		m_bHealthHasChangedRecently = false;	// don't blink our indicator until our health has changed again
	}
}

void CHudSODamageIndicator::Paint()
{
	if ( ShouldDraw() )
		DrawFullscreenDamageIndicator();	// draw fullscreen damage indicators
}

void CHudSODamageIndicator::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetPaintBackgroundEnabled( false );

	int wide, tall;
	GetHudSize( wide, tall );
	SetSize( wide, tall );
}
