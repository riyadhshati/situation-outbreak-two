#ifndef SO_WEAPON_BASE_H
#define SO_WEAPON_BASE_H

#include "weapon_hl2mpbase.h"

#ifdef CLIENT_DLL
	#define CWeaponSOBase C_WeaponSOBase
#endif

class CWeaponSOBase : public CWeaponHL2MPBase
{
public:
	DECLARE_CLASS( CWeaponSOBase, CWeaponHL2MPBase );

	CWeaponSOBase(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

public:
	virtual void Spawn( void );
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool Deploy( void );
	virtual bool Reload( void );
	virtual void ItemPostFrame( void );
	virtual void PrimaryAttack( void );
	virtual void SecondaryAttack( void );
	virtual void SetWeaponVisible( bool visible );

	// Weapon accuracy system
	virtual float GetAccuracyModifier( void );

	// Add support for CS:S player animations
	virtual const char *GetWeaponSuffix( void ) { return ""; }	// it is up to each weapon to override this in order for CS:S player animations to work properly

	// Allow players to drop their active weapons
	// Only drop weapons we're allowed to drop
	virtual bool CanDrop( void ) { return true; }

	// Add weapon bob
	virtual void AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual float CalcViewmodelBob( void );

	// Weapon reorigin system
	// http://developer.valvesoftware.com/wiki/Adding_Ironsights
	// Modified a bit from the wiki version considering our system's purpose
	virtual Vector GetIronsightPositionOffset( void ) const;
	virtual QAngle GetIronsightAngleOffset( void ) const;
	virtual float GetIronsightFOVOffset( void ) const;

	// Weapon respawn fix
	// http://developer.valvesoftware.com/wiki/Weapon_Respawn_Fix
	virtual void FallInit( void );
#ifdef GAME_DLL
	virtual void FallThink( void );	// make the weapon fall to the ground after spawning
#endif

	// Weapon scope system
	virtual bool HasScope( void ) { return false; }	// not all of our weapons have scopes (although some do)
	virtual bool UnscopeAfterShot( void ) { return false; } // by default, allow scoping while firing
	virtual bool ShouldDrawScope( void ) { return m_bIsScoped; }
	virtual bool CanScope( void );
	virtual void EnterScope( void );
	virtual void ExitScope( bool unhideWeapon = true );
	virtual float GetScopeFOV( void ) { return 45.0f; }	// this is a reasonable default value, but it should be changed depending on the specific weapon in question
	virtual bool ShouldDrawCrosshair( void ) { return !m_bIsScoped; }	// disables drawing crosshairs when scoped

protected:
	// Add support for CS:S player animations
	// I know there's a better way to replace these animations, but I'm extremely lazy
	DECLARE_ACTTABLE();

	// Weapon scope system
	CNetworkVar( bool, m_bIsScoped );
};

#endif