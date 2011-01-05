// Enable CS:S weapon ejection shells

#include "cbase.h"
#include "fx_impact.h"
#include "tempent.h"
#include "c_te_effect_dispatch.h"
#include "c_te_legacytempents.h"

void SO_EjectBrass( int shell, const CEffectData &data )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	tempents->CSEjectBrass( data.m_vOrigin, data.m_vAngles, data.m_fFlags, shell, pPlayer );
}

void SO_FX_EjectBrass_9mm_Callback( const CEffectData &data )
{
	SO_EjectBrass( CS_SHELL_9MM, data );
}
DECLARE_CLIENT_EFFECT( "EjectBrass_9mm", SO_FX_EjectBrass_9mm_Callback );

void SO_FX_EjectBrass_57_Callback( const CEffectData &data )
{
	SO_EjectBrass( CS_SHELL_57, data );
}
DECLARE_CLIENT_EFFECT( "EjectBrass_57", SO_FX_EjectBrass_57_Callback );

void SO_FX_EjectBrass_12gauge_Callback( const CEffectData &data )
{
	SO_EjectBrass( CS_SHELL_12GAUGE, data );
}
DECLARE_CLIENT_EFFECT( "EjectBrass_12gauge", SO_FX_EjectBrass_12gauge_Callback );

void SO_FX_EjectBrass_556_Callback( const CEffectData &data )
{
	SO_EjectBrass( CS_SHELL_556, data );
}
DECLARE_CLIENT_EFFECT( "EjectBrass_556", SO_FX_EjectBrass_556_Callback );

void SO_FX_EjectBrass_762nato_Callback( const CEffectData &data )
{
	SO_EjectBrass( CS_SHELL_762NATO, data );
}
DECLARE_CLIENT_EFFECT( "EjectBrass_762nato", SO_FX_EjectBrass_762nato_Callback );

void SO_FX_EjectBrass_338mag_Callback( const CEffectData &data )
{
	SO_EjectBrass( CS_SHELL_338MAG, data );
}
DECLARE_CLIENT_EFFECT( "EjectBrass_338mag", SO_FX_EjectBrass_338mag_Callback );
