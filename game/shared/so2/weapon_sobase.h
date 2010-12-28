#ifndef SO_WEAPON_BASE_H
#define SO_WEAPON_BASE_H

#include "cbase.h"
#include "npcevent.h"
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
	float GetAccuracyModifier( void );

	// Add support for CS:S player animations
	virtual const char *GetWeaponSuffix( void ) { return ""; }	// it is up to each weapon to override this in order for CS:S player animations to work properly

	// Add weapon bob
	void AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	float CalcViewmodelBob( void );

protected:
	// Add support for CS:S player animations
	// I know there's a better way to replace these animations, but I'm extremely lazy
	DECLARE_ACTTABLE();
};

#endif