// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "w_saber.h"
#include "q_shared.h"

#define	MISSILE_PRESTEP_TIME	50

extern void laserTrapStick( gentity_t *ent, vec3_t endpos, vec3_t normal );
extern void Jedi_Decloak( gentity_t *self );

#include "../namespace_begin.h"
extern qboolean FighterIsLanded( Vehicle_t *pVeh, playerState_t *parentPS );
#include "../namespace_end.h"

/*
================
G_ReflectMissile

  Reflect the missile roughly back at it's owner
================
*/
float RandFloat(float min, float max);
void G_ReflectMissile( gentity_t *ent, gentity_t *missile, vec3_t forward ) 
{
	vec3_t	bounce_dir;
	int		i;
	float	speed;
	gentity_t	*owner = ent;
	int		isowner = 0;

	if ( ent->r.ownerNum )
	{
		owner = &g_entities[ent->r.ownerNum];
	}

	if (missile->r.ownerNum == ent->s.number)
	{ //the original owner is bouncing the missile, so don't try to bounce it back at him
		isowner = 1;
	}

	//save the original speed
	speed = VectorNormalize( missile->s.pos.trDelta );

	//if ( ent && owner && owner->NPC && owner->enemy && Q_stricmp( "Tavion", owner->NPC_type ) == 0 && Q_irand( 0, 3 ) )
	if ( &g_entities[missile->r.ownerNum] && missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART && !isowner )
	{//bounce back at them if you can
		VectorSubtract( g_entities[missile->r.ownerNum].r.currentOrigin, missile->r.currentOrigin, bounce_dir );
		VectorNormalize( bounce_dir );
	}
	else if (isowner)
	{ //in this case, actually push the missile away from me, and since we're giving boost to our own missile by pushing it, up the velocity
		vec3_t missile_dir;

		speed *= 1.5;

		VectorSubtract( missile->r.currentOrigin, ent->r.currentOrigin, missile_dir );
		VectorCopy( missile->s.pos.trDelta, bounce_dir );
		VectorScale( bounce_dir, DotProduct( forward, missile_dir ), bounce_dir );
		VectorNormalize( bounce_dir );
	}
	else
	{
		vec3_t missile_dir;

		VectorSubtract( ent->r.currentOrigin, missile->r.currentOrigin, missile_dir );
		VectorCopy( missile->s.pos.trDelta, bounce_dir );
		VectorScale( bounce_dir, DotProduct( forward, missile_dir ), bounce_dir );
		VectorNormalize( bounce_dir );
	}
	for ( i = 0; i < 3; i++ )
	{
		bounce_dir[i] += RandFloat( -0.2f, 0.2f );
	}

	VectorNormalize( bounce_dir );
	VectorScale( bounce_dir, speed, missile->s.pos.trDelta );
	missile->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( missile->r.currentOrigin, missile->s.pos.trBase );
	if ( missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART )
	{//you are mine, now!
		missile->r.ownerNum = ent->s.number;
	}
	if ( missile->s.weapon == WP_ROCKET_LAUNCHER )
	{//stop homing
		missile->think = 0;
		missile->nextthink = 0;
	}
}

void G_DeflectMissile( gentity_t *ent, gentity_t *missile, vec3_t forward ) 
{
	vec3_t	bounce_dir;
	int		i;
	float	speed;
	int		isowner = 0;
	vec3_t missile_dir;

	if (missile->r.ownerNum == ent->s.number)
	{ //the original owner is bouncing the missile, so don't try to bounce it back at him
		isowner = 1;
	}

	//save the original speed
	speed = VectorNormalize( missile->s.pos.trDelta );

	if (ent->client)
	{
		//VectorSubtract( ent->r.currentOrigin, missile->r.currentOrigin, missile_dir );
		AngleVectors(ent->client->ps.viewangles, missile_dir, 0, 0);
		VectorCopy(missile_dir, bounce_dir);
		//VectorCopy( missile->s.pos.trDelta, bounce_dir );
		VectorScale( bounce_dir, DotProduct( forward, missile_dir ), bounce_dir );
		VectorNormalize( bounce_dir );
	}
	else
	{
		VectorCopy(forward, bounce_dir);
		VectorNormalize(bounce_dir);
	}

	for ( i = 0; i < 3; i++ )
	{
		bounce_dir[i] += RandFloat( -1.0f, 1.0f );
	}

	VectorNormalize( bounce_dir );
	VectorScale( bounce_dir, speed, missile->s.pos.trDelta );
	missile->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( missile->r.currentOrigin, missile->s.pos.trBase );
	if ( missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART )
	{//you are mine, now!
		missile->r.ownerNum = ent->s.number;
	}
	if ( missile->s.weapon == WP_ROCKET_LAUNCHER )
	{//stop homing
		missile->think = 0;
		missile->nextthink = 0;
	}
}

/*
================
G_BounceMissile

================
*/
void G_BounceMissile( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );


	if ( ent->flags & FL_BOUNCE_SHRAPNEL ) 
	{
		VectorScale( ent->s.pos.trDelta, 0.25f, ent->s.pos.trDelta );
		ent->s.pos.trType = TR_GRAVITY;

		// check for stop
		if ( trace->plane.normal[2] > 0.7 && ent->s.pos.trDelta[2] < 40 ) //this can happen even on very slightly sloped walls, so changed it from > 0 to > 0.7
		{
			G_SetOrigin( ent, trace->endpos );
			ent->nextthink = level.time + 100;
			return;
		}
	}
	else if ( ent->flags & FL_BOUNCE_HALF ) 
	{
		VectorScale( ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta );
		// check for stop
		if ( trace->plane.normal[2] > 0.2 && VectorLength( ent->s.pos.trDelta ) < 40 ) 
		{
			G_SetOrigin( ent, trace->endpos );
			return;
		}
	}

	if (ent->s.weapon == WP_THERMAL)
	{ //slight hack for hit sound
		G_Sound(ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/thermal/bounce%i.wav", Q_irand(1, 2))));
	}
	else if (ent->s.weapon == WP_SABER)
	{
		G_Sound(ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/saber/bounce%i.wav", Q_irand(1, 3))));
	}
	else if (ent->s.weapon == G2_MODEL_PART)
	{
		//Limb bounce sound?
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;

	if (ent->bounceCount != -5)
	{
		ent->bounceCount--;
	}
}


/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile( gentity_t *ent ) {
	vec3_t		dir;
	vec3_t		origin;

	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );

	ent->freeAfterEvent = qtrue;

	ent->takedamage = qfalse;
	// splash damage
	if ( ent->splashDamage ) {
		if( G_RadiusDamage( ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent, 
				ent, ent->splashMethodOfDeath ) ) 
		{
			if (ent->parent)
			{
				g_entities[ent->parent->s.number].client->accuracy_hits++;
			}
			else if (ent->activator)
			{
				g_entities[ent->activator->s.number].client->accuracy_hits++;
			}
		}
	}

	trap_LinkEntity( ent );
}

void G_RunStuckMissile( gentity_t *ent )
{
	if ( ent->takedamage )
	{
		if ( ent->s.groundEntityNum >= 0 && ent->s.groundEntityNum < ENTITYNUM_WORLD )
		{
			gentity_t *other = &g_entities[ent->s.groundEntityNum];

			if ( (!VectorCompare( vec3_origin, other->s.pos.trDelta ) && other->s.pos.trType != TR_STATIONARY) || 
				(!VectorCompare( vec3_origin, other->s.apos.trDelta ) && other->s.apos.trType != TR_STATIONARY) )
			{//thing I stuck to is moving or rotating now, kill me
				G_Damage( ent, other, other, NULL, NULL, 99999, 0, MOD_CRUSH );
				return;
			}
		}
	}
	// check think function
	G_RunThink( ent );
}

/*
================
G_BounceProjectile
================
*/
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout ) {
	vec3_t v, newv;
	float dot;

	VectorSubtract( impact, start, v );
	dot = DotProduct( v, dir );
	VectorMA( v, -2*dot, dir, newv );

	VectorNormalize(newv);
	VectorMA(impact, 8192, newv, endout);
}


//-----------------------------------------------------------------------------
gentity_t *CreateMissile( vec3_t org, vec3_t dir, float vel, int life, 
							gentity_t *owner, qboolean altFire)
//-----------------------------------------------------------------------------
{
	gentity_t	*missile;

	missile = G_Spawn();
	
	missile->nextthink = level.time + life;
	missile->think = G_FreeEntity;
	missile->s.eType = ET_MISSILE;
	missile->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	missile->parent = owner;
	missile->r.ownerNum = owner->s.number;

	if (altFire)
	{
		missile->s.eFlags |= EF_ALT_FIRING;
	}

	missile->s.pos.trType = TR_LINEAR;
	missile->s.pos.trTime = level.time;// - MISSILE_PRESTEP_TIME;	// NOTENOTE This is a Quake 3 addition over JK2
	missile->target_ent = NULL;

	SnapVector(org);
	VectorCopy( org, missile->s.pos.trBase );
	VectorScale( dir, vel, missile->s.pos.trDelta );
	VectorCopy( org, missile->r.currentOrigin);
	SnapVector(missile->s.pos.trDelta);

#ifdef PORTALS
	//Raz: Save the velocity for portals reorienting projectiles
	missile->genericValue1 = *(int*)&vel;
	missile->genericValue2 = life;
#endif

	return missile;
}
#ifdef PORTALS

#ifdef _DEBUG
	extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);
#endif
trace_t *RealTrace( gentity_t *ent, float dist )
{
static	trace_t tr;
		vec3_t	start, end;

	//Get start
	VectorCopy( ent->client->ps.origin, start );
	start[2] += ent->client->ps.viewheight; //36.0f;

	//Get end
	AngleVectors( ent->client->ps.viewangles, end, NULL, NULL );
	VectorMA( start, dist ? dist : 16384.0f, end, end );

	//Trace
	trap_Trace( &tr, start, NULL, NULL, end, ent->s.number, 1184515/*MASK_OPAQUE|CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_ITEM|CONTENTS_CORPSE*/ );

	#ifdef _DEBUG
		G_TestLine( start, tr.endpos, 0xFF, 7500 );
	#endif

	return &tr;
}

static ID_INLINE void CrossProductS( const vec3_t v1, const vec3_t v2, vec3_t cross ) {
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = -v1[0]*v2[2] + v1[2]*v2[0];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

#define VEC3(x) (x[0], x[1], x[2])
void AM_Test( gentity_t *ent )
{
	gentity_t	*obj = G_Spawn();
	trace_t		*tr = RealTrace( ent, 0.0f );
	vec3_t		forward, up;

	VectorSet(up, 0, 0, 1);

	obj->classname = "jp_portal";
	VectorCopy( tr->endpos, obj->s.origin );
	obj->s.userInt2 = atoi( ConcatArgs(1) );
	VectorCopy( tr->plane.normal, obj->s.boneAngles1 );

	AngleVectors(ent->client->ps.viewangles,forward,NULL,NULL);
	CrossProduct(up,tr->plane.normal,obj->s.boneAngles2);
	if(VectorLength(obj->s.boneAngles2) == 0.0)
		VectorSet(obj->s.boneAngles2,1,0,0);
	VectorNormalize(obj->s.boneAngles2);
	CrossProduct(tr->plane.normal,obj->s.boneAngles2,obj->s.boneAngles3);
	VectorNormalize(obj->s.boneAngles3);

	VectorMA(tr->endpos,64.0,obj->s.boneAngles2,up);
	G_TestLine( tr->endpos, up, 0x00, 7500 );
	VectorMA(tr->endpos, 64.0, obj->s.boneAngles3,up);
	G_TestLine( tr->endpos, up, 0x77, 7500 );

	VectorMA(tr->endpos, 64.0, tr->plane.normal,up);
	G_TestLine( tr->endpos, up, 2, 7500 );

	G_CallSpawn( obj );


	return;
}

void MatrixMultiply3f(float m[3][3], vec3_t vec, vec3_t result)
{
	result[0] = m[0][0]*vec[0] + m[0][1]*vec[1] + m[0][2]*vec[2];
	result[1] = m[1][0]*vec[0] + m[1][1]*vec[1] + m[1][2]*vec[2];
	result[2] = m[2][0]*vec[0] + m[2][1]*vec[1] + m[2][2]*vec[2];
}

void MatrixIdentity(float m[3][3])
{
	VectorSet(m[0],1,0,0);
	VectorSet(m[1],0,1,0);
	VectorSet(m[2],0,0,1);
}


void jp_portal_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
//	Com_Printf( "jp_portal: touch() at %i\n", level.time );
	//Remove the touch function for a while to avoid flooding
	self->genericValue1 = level.time;
	self->touch = NULL;
	if ( self->s.health != -1 )
	{
		vec3_t tpPos;
		vec3_t sourceVelocity, destVelocity;
		vec3_t sourceNormal, destNormal;
		float speed;

		//Fetch normals
		VectorCopy( self->s.boneAngles1, sourceNormal );
		VectorCopy( g_entities[self->s.health].s.boneAngles1, destNormal );

		//Set position to teleport to
		VectorCopy( g_entities[self->s.health].s.origin, tpPos );
		VectorMA( tpPos, 32.0f, destNormal, tpPos );

		//BEGIN position
		if ( other->client )
		{
			VectorCopy( tpPos, other->client->ps.origin );
			other->client->ps.origin[2] += 1.0f;
			other->client->ps.eFlags ^= EF_TELEPORT_BIT;
			SetClientViewAngle( other, other->client->ps.viewangles );
			BG_PlayerStateToEntityState( &other->client->ps, &other->s, qtrue );
			VectorCopy( other->client->ps.origin, other->r.currentOrigin );
		}
		else
		{
			VectorCopy( tpPos, other->s.pos.trBase );
		}
		//END position

		//BEGIN velocity
		if ( other->client )
		{	
			float reflectFactor;
			vec3_t exitVector;
			vec3_t velNormal;
			float length;

			vec3_t debugLine;

			VectorNormalize2(other->client->ps.velocity,velNormal);
			reflectFactor = 2 * DotProduct(sourceNormal, velNormal);

			VectorCopy( other->client->ps.velocity, sourceVelocity );
			length = VectorLength(sourceVelocity);

			exitVector[0] = velNormal[0] - reflectFactor * sourceNormal[0];
			exitVector[1] = velNormal[1] - reflectFactor * sourceNormal[1];
			exitVector[2] = velNormal[2] - reflectFactor * sourceNormal[2];

			VectorMA(self->s.origin, -128.0, velNormal,debugLine);
			G_TestLine( debugLine, self->s.origin, 0xF0, 7500 );

			MatrixMultiply3f(self->portal_matrix,exitVector,destVelocity);

			VectorMA(g_entities[self->s.health].s.origin, 128.0, destVelocity,debugLine);
			G_TestLine( g_entities[self->s.health].s.origin, debugLine, 0x0F, 7500 );

			VectorScale(destVelocity,length,destVelocity);
		}
		else
		{
			float velocity = *(float*)&other->genericValue1;
		//	VectorScale( destNormal, velocity, other->s.pos.trDelta );
			VectorMA( other->s.pos.trBase, velocity, destNormal, other->s.pos.trDelta );
			speed = velocity;
			other->s.pos.trTime = level.time;
			other->nextthink = level.time + other->genericValue2;
		}

		//VectorSet( destVelocity, destNormal[0]*speed, destNormal[1]*speed, destNormal[2]*speed );
		if ( other->client )
		{
			VectorCopy( destVelocity, other->client->ps.velocity );
			other->client->ps.pm_flags |= PMF_DUCKED;
			other->s.solid = 0;
		}
		else
		{
			VectorCopy( destVelocity, other->s.pos.trDelta );
			VectorCopy( tpPos, other->r.currentOrigin );
		}
		//END velocity

		g_entities[self->s.health].genericValue1 = level.time;
		g_entities[self->s.health].touch = NULL;
	}
	return;
}

void jp_portal_think( gentity_t *self )
{
//	G_PlayEffectID( G_EffectIndex( self->genericValue2 ? "env/portal2" : "env/portal1" ), self->s.origin, self->s.boneAngles1 );
	self->nextthink = level.time + self->count;

	//If it's been 100 milliseconds since the last touch, activate it again
	if ( self->genericValue1 < level.time - 100 )
		self->touch = jp_portal_touch;
	return;
}

void SP_jp_portal( gentity_t *ent )
{
	gentity_t *found = NULL, *from = NULL;

//	G_SpawnInt( "source", "1", &ent->userInt2 );
	ent->s.health = -1; //no linked portal

	while ( (found = G_Find( from, FOFS(classname), "jp_portal" )) )
	{//search for other portals, so we can remove our old 'self' or link with partners
		if ( found == ent )
		{//Found ourself
			from = ent;
			continue;
		}
		if ( found->s.userInt2 == ent->s.userInt2 )
		{//Found our old portal
			G_FreeEntity( found );
		}
		else
		{//Found a portal to link with
			float mat[3][3];
			float mat_inv[3][3]; 
			ent->s.health = found->s.number;
			found->s.health = ent->s.number;
			from = found;

			mat[0][0] = found->s.boneAngles2[0];  
			mat[1][0] = found->s.boneAngles2[1]; 
			mat[2][0] = found->s.boneAngles2[2]; 
			mat[0][1] = found->s.boneAngles3[0];  
			mat[1][1] = found->s.boneAngles3[1]; 
			mat[2][1] = found->s.boneAngles3[2]; 
			mat[0][2] = found->s.boneAngles1[0]; 
			mat[1][2] = found->s.boneAngles1[1]; 
			mat[2][2] = found->s.boneAngles1[2]; 
			
			mat_inv[0][0] = ent->s.boneAngles2[0];  
			mat_inv[0][1] = ent->s.boneAngles2[1]; 
			mat_inv[0][2] = ent->s.boneAngles2[2]; 
			mat_inv[1][0] = ent->s.boneAngles3[0];  
			mat_inv[1][1] = ent->s.boneAngles3[1]; 
			mat_inv[1][2] = ent->s.boneAngles3[2]; 
			mat_inv[2][0] = ent->s.boneAngles1[0]; 
			mat_inv[2][1] = ent->s.boneAngles1[1]; 
			mat_inv[2][2] = ent->s.boneAngles1[2]; 

			/*if(VectorLength(mat_inv[0]) == 0.0 || VectorLength(mat_inv[1]) == 0.0 || VectorLength(mat_inv[2]) == 0.0)
				MatrixIdentity(mat_inv);

			if(VectorLength(mat[0]) == 0.0 || VectorLength(mat[1]) == 0.0 || VectorLength(mat[2]) == 0.0)
				MatrixIdentity(mat);*/

			MatrixMultiply(mat,mat_inv,ent->portal_matrix);

			mat[0][0] = ent->s.boneAngles2[0];  
			mat[1][0] = ent->s.boneAngles2[1]; 
			mat[2][0] = ent->s.boneAngles2[2]; 
			mat[0][1] = ent->s.boneAngles3[0];  
			mat[1][1] = ent->s.boneAngles3[1]; 
			mat[2][1] = ent->s.boneAngles3[2]; 
			mat[0][2] = ent->s.boneAngles1[0]; 
			mat[1][2] = ent->s.boneAngles1[1]; 
			mat[2][2] = ent->s.boneAngles1[2]; 
			
			mat_inv[0][0] = found->s.boneAngles2[0];  
			mat_inv[0][1] = found->s.boneAngles2[1]; 
			mat_inv[0][2] = found->s.boneAngles2[2]; 
			mat_inv[1][0] = found->s.boneAngles3[0];  
			mat_inv[1][1] = found->s.boneAngles3[1]; 
			mat_inv[1][2] = found->s.boneAngles3[2]; 
			mat_inv[2][0] = found->s.boneAngles1[0]; 
			mat_inv[2][1] = found->s.boneAngles1[1]; 
			mat_inv[2][2] = found->s.boneAngles1[2]; 

			MatrixMultiply(mat,mat_inv,found->portal_matrix);

		}
	}

	ent->s.eType = ET_SPECIAL;
	ent->s.userInt1 = 1; // is a jp_portal
	ent->r.contents = CONTENTS_TRIGGER|CONTENTS_CORPSE;   //actually solid, helps projectiles trigger us.
	ent->clipmask = MASK_SOLID;
	VectorSet( ent->r.mins, -16.0f, -16.0f, 0.0f );
	VectorSet( ent->r.maxs, 16.0f, 16.0f, 32.0f );

	G_SetOrigin( ent, ent->s.origin );

	ent->think		= jp_portal_think;
	ent->touch		= jp_portal_touch;
	ent->nextthink	= level.time + ent->count;

	trap_LinkEntity( ent );

	return;
}

#endif
void G_MissileBounceEffect( gentity_t *ent, vec3_t org, vec3_t dir )
{
	//FIXME: have an EV_BOUNCE_MISSILE event that checks the s.weapon and does the appropriate effect
	switch( ent->s.weapon )
	{
	case WP_BOWCASTER:
		G_PlayEffectID( G_EffectIndex("bowcaster/deflect"), ent->r.currentOrigin, dir );
		break;
	case WP_BLASTER:
	case WP_BRYAR_PISTOL:
		G_PlayEffectID( G_EffectIndex("blaster/deflect"), ent->r.currentOrigin, dir );
		break;
	default:
		{
			gentity_t *te = G_TempEntity( org, EV_SABER_BLOCK );
			VectorCopy(org, te->s.origin);
			VectorCopy(dir, te->s.angles);
			te->s.eventParm = 0;
			te->s.weapon = 0;//saberNum
			te->s.legsAnim = 0;//bladeNum
		}
		break;
	}
}

/*
================
G_MissileImpact
================
*/
// Makermod fix: rocket circle by spior
int maxRocketsPerLoop = 0;

void WP_SaberBlockNonRandom( gentity_t *self, vec3_t hitloc, qboolean missileBlock );
void G_MissileImpact( gentity_t *ent, trace_t *trace ) {
	gentity_t		*other;
	qboolean		hitClient = qfalse;
	qboolean		isKnockedSaber = qfalse;

	other = &g_entities[trace->entityNum];

	// check for bounce
	if ( !other->takedamage &&
		(ent->bounceCount > 0 || ent->bounceCount == -5) &&
		( ent->flags & ( FL_BOUNCE | FL_BOUNCE_HALF ) ) ) {
		G_BounceMissile( ent, trace );
		G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
		return;
	}
	else if (ent->neverFree && ent->s.weapon == WP_SABER && (ent->flags & FL_BOUNCE_HALF))
	{ //this is a knocked-away saber
		if (ent->bounceCount > 0 || ent->bounceCount == -5)
		{
			G_BounceMissile( ent, trace );
			G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
			return;
		}

		isKnockedSaber = qtrue;
	}
	
	// I would glom onto the FL_BOUNCE code section above, but don't feel like risking breaking something else
	if ( (!other->takedamage && (ent->bounceCount > 0 || ent->bounceCount == -5) && ( ent->flags&(FL_BOUNCE_SHRAPNEL) ) ) || ((trace->surfaceFlags&SURF_FORCEFIELD)&&!ent->splashDamage&&!ent->splashRadius&&(ent->bounceCount > 0 || ent->bounceCount == -5)) ) 
	{
		G_BounceMissile( ent, trace );

		if ( ent->bounceCount < 1 )
		{
			ent->flags &= ~FL_BOUNCE_SHRAPNEL;
		}
		return;
	}

	/*
	if ( !other->takedamage && ent->s.weapon == WP_THERMAL && !ent->alt_fire )
	{//rolling thermal det - FIXME: make this an eFlag like bounce & stick!!!
		//G_BounceRollMissile( ent, trace );
		if ( ent->owner && ent->owner->s.number == 0 ) 
		{
			G_MissileAddAlerts( ent );
		}
		//gi.linkentity( ent );
		return;
	}
	*/

	if ((other->r.contents & CONTENTS_LIGHTSABER) && !isKnockedSaber)
	{ //hit this person's saber, so..
		gentity_t *otherOwner = &g_entities[other->r.ownerNum];

		if (otherOwner->takedamage && otherOwner->client && otherOwner->client->ps.duelInProgress &&
			otherOwner->client->ps.duelIndex != ent->r.ownerNum)
		{
			goto killProj;
		}
	}
	else if (!isKnockedSaber)
	{
		if (other->takedamage && other->client && other->client->ps.duelInProgress &&
			other->client->ps.duelIndex != ent->r.ownerNum)
		{
			goto killProj;
		}
	}

	if (other->flags & FL_DMG_BY_HEAVY_WEAP_ONLY)
	{
		if (ent->methodOfDeath != MOD_REPEATER_ALT &&
			ent->methodOfDeath != MOD_ROCKET &&
			ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
			ent->methodOfDeath != MOD_ROCKET_HOMING &&
			ent->methodOfDeath != MOD_THERMAL &&
			ent->methodOfDeath != MOD_THERMAL_SPLASH &&
			ent->methodOfDeath != MOD_TRIP_MINE_SPLASH &&
			ent->methodOfDeath != MOD_TIMED_MINE_SPLASH &&
			ent->methodOfDeath != MOD_DET_PACK_SPLASH &&
			ent->methodOfDeath != MOD_VEHICLE &&
			ent->methodOfDeath != MOD_CONC &&
			ent->methodOfDeath != MOD_CONC_ALT &&
			ent->methodOfDeath != MOD_SABER &&
			ent->methodOfDeath != MOD_TURBLAST)
		{
			vec3_t fwd;

			if (trace)
			{
				VectorCopy(trace->plane.normal, fwd);
			}
			else
			{ //oh well
				AngleVectors(other->r.currentAngles, fwd, NULL, NULL);
			}

			G_DeflectMissile(other, ent, fwd);
			G_MissileBounceEffect(ent, ent->r.currentOrigin, fwd);
			return;
		}
	}

	if ((other->flags & FL_SHIELDED) &&
		ent->s.weapon != WP_ROCKET_LAUNCHER &&
		ent->s.weapon != WP_THERMAL &&
		ent->s.weapon != WP_TRIP_MINE &&
		ent->s.weapon != WP_DET_PACK &&
		ent->s.weapon != WP_DEMP2 &&
		ent->s.weapon != WP_EMPLACED_GUN &&
		ent->methodOfDeath != MOD_REPEATER_ALT &&
		ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH && 
		ent->methodOfDeath != MOD_TURBLAST &&
		ent->methodOfDeath != MOD_VEHICLE &&
		ent->methodOfDeath != MOD_CONC &&
		ent->methodOfDeath != MOD_CONC_ALT &&
		!(ent->dflags&DAMAGE_HEAVY_WEAP_CLASS) )
	{
		vec3_t fwd;

		if (other->client)
		{
			AngleVectors(other->client->ps.viewangles, fwd, NULL, NULL);
		}
		else
		{
			AngleVectors(other->r.currentAngles, fwd, NULL, NULL);
		}

		G_DeflectMissile(other, ent, fwd);
		G_MissileBounceEffect(ent, ent->r.currentOrigin, fwd);
		return;
	}
		
	if (other->takedamage && other->client &&
		ent->s.weapon != WP_ROCKET_LAUNCHER &&
		ent->s.weapon != WP_THERMAL &&
		ent->s.weapon != WP_TRIP_MINE &&
		ent->s.weapon != WP_DET_PACK &&
		ent->s.weapon != WP_DEMP2 &&
		ent->methodOfDeath != MOD_REPEATER_ALT &&
		ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
		ent->methodOfDeath != MOD_CONC &&
		ent->methodOfDeath != MOD_CONC_ALT &&
		other->client->ps.saberBlockTime < level.time &&
		!isKnockedSaber &&
		WP_SaberCanBlock(other, ent->r.currentOrigin, 0, 0, qtrue, 0))
	{ //only block one projectile per 200ms (to prevent giant swarms of projectiles being blocked)
		vec3_t fwd;
		gentity_t *te;
		int otherDefLevel = other->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE];

		te = G_TempEntity( ent->r.currentOrigin, EV_SABER_BLOCK );
		VectorCopy(ent->r.currentOrigin, te->s.origin);
		VectorCopy(trace->plane.normal, te->s.angles);
		te->s.eventParm = 0;
		te->s.weapon = 0;//saberNum
		te->s.legsAnim = 0;//bladeNum

		/*if (other->client->ps.velocity[2] > 0 ||
			other->client->pers.cmd.forwardmove ||
			other->client->pers.cmd.rightmove)
			*/
		if (other->client->ps.velocity[2] > 0 ||
			other->client->pers.cmd.forwardmove < 0) //now we only do it if jumping or running backward. Should be able to full-on charge.
		{
			otherDefLevel -= 1;
			if (otherDefLevel < 0)
			{
				otherDefLevel = 0;
			}
		}

		AngleVectors(other->client->ps.viewangles, fwd, NULL, NULL);
		if (otherDefLevel == FORCE_LEVEL_1)
		{
			//if def is only level 1, instead of deflecting the shot it should just die here
		}
		else if (otherDefLevel == FORCE_LEVEL_2)
		{
			G_DeflectMissile(other, ent, fwd);
		}
		else
		{
			G_ReflectMissile(other, ent, fwd);
		}
		other->client->ps.saberBlockTime = level.time + (350 - (otherDefLevel*100)); //200;

		//For jedi AI
		other->client->ps.saberEventFlags |= SEF_DEFLECTED;

		if (otherDefLevel == FORCE_LEVEL_3)
		{
			other->client->ps.saberBlockTime = 0; //^_^
		}

		if (otherDefLevel == FORCE_LEVEL_1)
		{
			goto killProj;
		}
		return;
	}
	else if ((other->r.contents & CONTENTS_LIGHTSABER) && !isKnockedSaber)
	{ //hit this person's saber, so..
		gentity_t *otherOwner = &g_entities[other->r.ownerNum];

		if (otherOwner->takedamage && otherOwner->client &&
			ent->s.weapon != WP_ROCKET_LAUNCHER &&
			ent->s.weapon != WP_THERMAL &&
			ent->s.weapon != WP_TRIP_MINE &&
			ent->s.weapon != WP_DET_PACK &&
			ent->s.weapon != WP_DEMP2 &&
			ent->methodOfDeath != MOD_REPEATER_ALT &&
			ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
			ent->methodOfDeath != MOD_CONC &&
			ent->methodOfDeath != MOD_CONC_ALT /*&&
			otherOwner->client->ps.saberBlockTime < level.time*/)
		{ //for now still deflect even if saberBlockTime >= level.time because it hit the actual saber
			vec3_t fwd;
			gentity_t *te;
			int otherDefLevel = otherOwner->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE];

			//in this case, deflect it even if we can't actually block it because it hit our saber
			//WP_SaberCanBlock(otherOwner, ent->r.currentOrigin, 0, 0, qtrue, 0);
			if (otherOwner->client && otherOwner->client->ps.weaponTime <= 0)
			{
				WP_SaberBlockNonRandom(otherOwner, ent->r.currentOrigin, qtrue);
			}

			te = G_TempEntity( ent->r.currentOrigin, EV_SABER_BLOCK );
			VectorCopy(ent->r.currentOrigin, te->s.origin);
			VectorCopy(trace->plane.normal, te->s.angles);
			te->s.eventParm = 0;
			te->s.weapon = 0;//saberNum
			te->s.legsAnim = 0;//bladeNum

			/*if (otherOwner->client->ps.velocity[2] > 0 ||
				otherOwner->client->pers.cmd.forwardmove ||
				otherOwner->client->pers.cmd.rightmove)*/
			if (otherOwner->client->ps.velocity[2] > 0 ||
				otherOwner->client->pers.cmd.forwardmove < 0) //now we only do it if jumping or running backward. Should be able to full-on charge.
			{
				otherDefLevel -= 1;
				if (otherDefLevel < 0)
				{
					otherDefLevel = 0;
				}
			}

			AngleVectors(otherOwner->client->ps.viewangles, fwd, NULL, NULL);

			if (otherDefLevel == FORCE_LEVEL_1)
			{
				//if def is only level 1, instead of deflecting the shot it should just die here
			}
			else if (otherDefLevel == FORCE_LEVEL_2)
			{
				G_DeflectMissile(otherOwner, ent, fwd);
			}
			else
			{
				G_ReflectMissile(otherOwner, ent, fwd);
			}
			otherOwner->client->ps.saberBlockTime = level.time + (350 - (otherDefLevel*100));//200;

			//For jedi AI
			otherOwner->client->ps.saberEventFlags |= SEF_DEFLECTED;

			if (otherDefLevel == FORCE_LEVEL_3)
			{
				otherOwner->client->ps.saberBlockTime = 0; //^_^
			}

			if (otherDefLevel == FORCE_LEVEL_1)
			{
				goto killProj;
			}
			return;
		}
	}

	// check for sticking
	if ( !other->takedamage && ( ent->s.eFlags & EF_MISSILE_STICK ) ) 
	{
		laserTrapStick( ent, trace->endpos, trace->plane.normal );
		G_AddEvent( ent, EV_MISSILE_STICK, 0 );
		return;
	}

	// impact damage
	if (other->takedamage && !isKnockedSaber) {
		// FIXME: wrong damage direction?
		if ( ent->damage ) {
			vec3_t	velocity;
			qboolean didDmg = qfalse;

			if( LogAccuracyHit( other, &g_entities[ent->r.ownerNum] ) ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
				hitClient = qtrue;
			}
			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
			if ( VectorLength( velocity ) == 0 ) {
				velocity[2] = 1;	// stepped on a grenade
			}

			if (ent->s.weapon == WP_BOWCASTER || ent->s.weapon == WP_FLECHETTE ||
				ent->s.weapon == WP_ROCKET_LAUNCHER)
			{
				if (ent->s.weapon == WP_FLECHETTE && (ent->s.eFlags & EF_ALT_FIRING))
				{
					ent->think(ent);
				}
				else
				{
					G_Damage (other, ent, &g_entities[ent->r.ownerNum], velocity,
						/*ent->s.origin*/ent->r.currentOrigin, ent->damage, 
						DAMAGE_HALF_ABSORB, ent->methodOfDeath);
					didDmg = qtrue;
				}
			}
			else
			{
				G_Damage (other, ent, &g_entities[ent->r.ownerNum], velocity,
					/*ent->s.origin*/ent->r.currentOrigin, ent->damage, 
					0, ent->methodOfDeath);
				didDmg = qtrue;
			}

			if (didDmg && other && other->client)
			{ //What I'm wondering is why this isn't in the NPC pain funcs. But this is what SP does, so whatever.
				class_t	npc_class = other->client->NPC_class;

				// If we are a robot and we aren't currently doing the full body electricity...
				if ( npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE ||
					   npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 || npc_class == CLASS_REMOTE ||
					   npc_class == CLASS_MARK1 || npc_class == CLASS_MARK2 || //npc_class == CLASS_PROTOCOL ||//no protocol, looks odd
					   npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST || npc_class == CLASS_SENTRY )
				{
					// special droid only behaviors
					if ( other->client->ps.electrifyTime < level.time + 100 )
					{
						// ... do the effect for a split second for some more feedback
						other->client->ps.electrifyTime = level.time + 450;
					}
					//FIXME: throw some sparks off droids,too
				}
			}
		}

		if ( ent->s.weapon == WP_DEMP2 )
		{//a hit with demp2 decloaks people, disables ships
			if ( other && other->client && other->client->NPC_class == CLASS_VEHICLE )
			{//hit a vehicle
				if ( other->m_pVehicle //valid vehicle ent
					&& other->m_pVehicle->m_pVehicleInfo//valid stats
					&& (other->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER//always affect speeders
						||(other->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER && ent->classname && Q_stricmp("vehicle_proj", ent->classname ) == 0) )//only vehicle ion weapons affect a fighter in this manner
					&& !FighterIsLanded( other->m_pVehicle , &other->client->ps )//not landed
					&& !(other->spawnflags&2) )//and not suspended
				{//vehicles hit by "ion cannons" lose control
					if ( other->client->ps.electrifyTime > level.time )
					{//add onto it
						//FIXME: extern the length of the "out of control" time?
						other->client->ps.electrifyTime += Q_irand(200,500);
						if ( other->client->ps.electrifyTime > level.time + 4000 )
						{//cap it
							other->client->ps.electrifyTime = level.time + 4000;
						}
					}
					else
					{//start it
						//FIXME: extern the length of the "out of control" time?
						other->client->ps.electrifyTime = level.time + Q_irand(200,500);
					}
				}
			}
			else if ( other && other->client && other->client->ps.powerups[PW_CLOAKED] )
			{
				Jedi_Decloak( other );
				if ( ent->methodOfDeath == MOD_DEMP2_ALT )
				{//direct hit with alt disables cloak forever
					//permanently disable the saboteur's cloak
					other->client->cloakToggleTime = Q3_INFINITE;
				}
				else
				{//temp disable
					other->client->cloakToggleTime = level.time + Q_irand( 3000, 10000 );
				}
			}
		}
	}
killProj:
	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if ( other->takedamage && other->client && !isKnockedSaber ) {
		G_AddEvent( ent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
		ent->s.otherEntityNum = other->s.number;
	} else if( trace->surfaceFlags & SURF_METALSTEPS ) {
		G_AddEvent( ent, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );
	} else if (ent->s.weapon != G2_MODEL_PART && !isKnockedSaber) {
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
	}

	if (!isKnockedSaber)
	{
		ent->freeAfterEvent = qtrue;

		// change over to a normal entity right at the point of impact
		ent->s.eType = ET_GENERAL;
	}

	SnapVectorTowards( trace->endpos, ent->s.pos.trBase );	// save net bandwidth

	G_SetOrigin( ent, trace->endpos );

	ent->takedamage = qfalse;
	// splash damage (doesn't apply to person directly hit)
	maxRocketsPerLoop = 0;			// Makermod fix: rocket circle
	if ( ent->splashDamage ) {
		if( G_RadiusDamage( trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius, 
			other, ent, ent->splashMethodOfDeath ) ) {
			if( !hitClient 
				&& g_entities[ent->r.ownerNum].client ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
			}
		}
	}

	if (ent->s.weapon == G2_MODEL_PART)
	{
		ent->freeAfterEvent = qfalse; //it will free itself
	}

	trap_LinkEntity( ent );
}

/*
================
G_RunMissile
================
*/
void G_RunMissile( gentity_t *ent ) {
	vec3_t		origin, groundSpot;
	trace_t		tr;
	int			passent;
	qboolean	isKnockedSaber = qfalse;

	if (ent->neverFree && ent->s.weapon == WP_SABER && (ent->flags & FL_BOUNCE_HALF))
	{
		isKnockedSaber = qtrue;
		ent->s.pos.trType = TR_GRAVITY;
	}

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// if this missile bounced off an invulnerability sphere
	if ( ent->target_ent ) {
		passent = ent->target_ent->s.number;
	}
	else {
		// ignore interactions with the missile owner
		if ( (ent->r.svFlags&SVF_OWNERNOTSHARED) 
			&& (ent->s.eFlags&EF_JETPACK_ACTIVE) )
		{//A vehicle missile that should be solid to its owner
			//I don't care about hitting my owner
			passent = ent->s.number;
		}
		else
		{
			passent = ent->r.ownerNum;
		}
	}
	// trace a line from the previous position to the current position
	if (d_projectileGhoul2Collision.integer)
	{
		trap_G2Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );

		if (tr.fraction != 1.0 && tr.entityNum < ENTITYNUM_WORLD)
		{
			gentity_t *g2Hit = &g_entities[tr.entityNum];

			if (g2Hit->inuse && g2Hit->client && g2Hit->ghoul2)
			{ //since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				g2Hit->client->g2LastSurfaceHit = tr.surfaceFlags;
				g2Hit->client->g2LastSurfaceTime = level.time;
			}

			if (g2Hit->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}
	}
	else
	{
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask );
	}

	if ( tr.startsolid || tr.allsolid ) {
		// make sure the tr.entityNum is set to the entity we're stuck in
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask );
		tr.fraction = 0;
	}
	else {
		VectorCopy( tr.endpos, ent->r.currentOrigin );
	}

	if (ent->passThroughNum && tr.entityNum == (ent->passThroughNum-1))
	{
		VectorCopy( origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
		goto passthrough;
	}

	trap_LinkEntity( ent );

	if (ent->s.weapon == G2_MODEL_PART && !ent->bounceCount)
	{
		vec3_t lowerOrg;
		trace_t trG;

		VectorCopy(ent->r.currentOrigin, lowerOrg);
		lowerOrg[2] -= 1;
		trap_Trace( &trG, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, lowerOrg, passent, ent->clipmask );

		VectorCopy(trG.endpos, groundSpot);

		if (!trG.startsolid && !trG.allsolid && trG.entityNum == ENTITYNUM_WORLD)
		{
			ent->s.groundEntityNum = trG.entityNum;
		}
		else
		{
			ent->s.groundEntityNum = ENTITYNUM_NONE;
		}
	}

	if ( tr.fraction != 1) {
		// never explode or bounce on sky
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// If grapple, reset owner
			if (ent->parent && ent->parent->client && ent->parent->client->hook == ent) {
				ent->parent->client->hook = NULL;
			}

			if ((ent->s.weapon == WP_SABER && ent->isSaberEntity) || isKnockedSaber)
			{
				G_RunThink( ent );
				return;
			}
			else if (ent->s.weapon != G2_MODEL_PART)
			{
				G_FreeEntity( ent );
				return;
			}
		}

#if 0 //will get stomped with missile impact event...
		if (ent->s.weapon > WP_NONE && ent->s.weapon < WP_NUM_WEAPONS &&
			(tr.entityNum < MAX_CLIENTS || g_entities[tr.entityNum].s.eType == ET_NPC))
		{ //player or NPC, try making a mark on him
			/*
			gentity_t *evEnt = G_TempEntity(ent->r.currentOrigin, EV_GHOUL2_MARK);

			evEnt->s.owner = tr.entityNum; //the entity the mark should be placed on
			evEnt->s.weapon = ent->s.weapon; //the weapon used (to determine mark type)
			VectorCopy(ent->r.currentOrigin, evEnt->s.origin); //the point of impact

			//origin2 gets the predicted trajectory-based position.
			BG_EvaluateTrajectory( &ent->s.pos, level.time, evEnt->s.origin2 );

			//If they are the same, there will be problems.
			if (VectorCompare(evEnt->s.origin, evEnt->s.origin2))
			{
				evEnt->s.origin2[2] += 2; //whatever, at least it won't mess up.
			}
			*/
			//ok, let's try adding it to the missile ent instead (tempents bad!)
			G_AddEvent(ent, EV_GHOUL2_MARK, 0);

			//copy current pos to s.origin, and current projected traj to origin2
			VectorCopy(ent->r.currentOrigin, ent->s.origin);
			BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->s.origin2 );

			//the index for whoever we are hitting
			ent->s.otherEntityNum = tr.entityNum;

			if (VectorCompare(ent->s.origin, ent->s.origin2))
			{
				ent->s.origin2[2] += 2.0f; //whatever, at least it won't mess up.
			}
		}
#else
		if (ent->s.weapon > WP_NONE && ent->s.weapon < WP_NUM_WEAPONS &&
			(tr.entityNum < MAX_CLIENTS || g_entities[tr.entityNum].s.eType == ET_NPC))
		{ //player or NPC, try making a mark on him
			//copy current pos to s.origin, and current projected traj to origin2
			VectorCopy(ent->r.currentOrigin, ent->s.origin);
			BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->s.origin2 );

			if (VectorCompare(ent->s.origin, ent->s.origin2))
			{
				ent->s.origin2[2] += 2.0f; //whatever, at least it won't mess up.
			}
		}
#endif

		G_MissileImpact( ent, &tr );

		if (tr.entityNum == ent->s.otherEntityNum)
		{ //if the impact event other and the trace ent match then it's ok to do the g2 mark
			ent->s.trickedentindex = 1;
		}

		if ( ent->s.eType != ET_MISSILE && ent->s.weapon != G2_MODEL_PART )
		{
			return;		// exploded
		}
	}

passthrough:
	if ( ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags&EF_MISSILE_STICK) )
	{//stuck missiles should check some special stuff
		G_RunStuckMissile( ent );
		return;
	}

	if (ent->s.weapon == G2_MODEL_PART)
	{
		if (ent->s.groundEntityNum == ENTITYNUM_WORLD)
		{
			ent->s.pos.trType = TR_LINEAR;
			VectorClear(ent->s.pos.trDelta);
			ent->s.pos.trTime = level.time;

			VectorCopy(groundSpot, ent->s.pos.trBase);
			VectorCopy(groundSpot, ent->r.currentOrigin);

			if (ent->s.apos.trType != TR_STATIONARY)
			{
				ent->s.apos.trType = TR_STATIONARY;
				ent->s.apos.trTime = level.time;

				ent->s.apos.trBase[ROLL] = 0;
				ent->s.apos.trBase[PITCH] = 0;
			}
		}
	}

	// check think function after bouncing
	G_RunThink( ent );
}


//=============================================================================




