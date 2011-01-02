#ifndef SO_FRAG_GRENADE_H
#define SO_FRAG_GRENADE_H
#pragma once

class CBaseGrenade;
struct edict_t;

CBaseGrenade *SO_Fraggrenade_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer, bool combineSpawned );
bool	SO_Fraggrenade_WasPunted( const CBaseEntity *pEntity );
bool	SO_Fraggrenade_WasCreatedByCombine( const CBaseEntity *pEntity );

#endif // SO_FRAG_GRENADE_H
