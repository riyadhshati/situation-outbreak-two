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
	void Spawn( void );
	void ItemPostFrame( void );

	// Weapon accuracy system
	float GetAccuracyModifier( void );

	// Add support for CS:S player animations
	virtual const char *GetWeaponSuffix( void ) { return ""; }	// it is up to each weapon to override this in order for CS:S player animations to work properly

	// Allow players to drop their active weapons
	// Only drop weapons we're allowed to drop
	virtual bool CanDrop( void ) { return true; }

	// Add weapon bob
	void AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	float CalcViewmodelBob( void );

	// Weapon reorigin system
	// http://developer.valvesoftware.com/wiki/Adding_Ironsights
	// Modified a bit from the wiki version considering our system's purpose
	Vector GetIronsightPositionOffset( void ) const;
	QAngle GetIronsightAngleOffset( void ) const;
	float GetIronsightFOVOffset( void ) const;

protected:
	// Add support for CS:S player animations
	// I know there's a better way to replace these animations, but I'm extremely lazy
	DECLARE_ACTTABLE();
};

#endif