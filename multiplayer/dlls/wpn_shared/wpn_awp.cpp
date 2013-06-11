/***
*
*   Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*   This product contains software technology licensed from Id
*   Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*   All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum awp_e
{
    AWP_IDLE,
    AWP_SHOOT1,
    AWP_SHOOT2,
    AWP_SHOOT3,
    AWP_RELOAD,
    AWP_DRAW
};

#define AWP_MAX_SPEED      210
#define AWP_MAX_SPEED_ZOOM 150
#define AWP_RELOAD_TIME    2.5

LINK_ENTITY_TO_CLASS( weapon_awp, CAWP );

void CAWP::Spawn( void )
{
    pev->classname = MAKE_STRING( "weapon_awp" );

    Precache();
    m_iId = WEAPON_AWP;
    SET_MODEL( edict(), "models/w_awp.mdl" );

    m_iDefaultAmmo = AWP_DEFAULT_GIVE;
    FallInit();
}

void CAWP::Precache( void )
{
    PRECACHE_MODEL( "models/v_awp.mdl");
    PRECACHE_MODEL( "models/w_awp.mdl");

    PRECACHE_SOUND( "weapons/awp1.wav"        );
    PRECACHE_SOUND( "weapons/boltpull1.wav"   );
    PRECACHE_SOUND( "weapons/boltup.wav"      );
    PRECACHE_SOUND( "weapons/boltdown.wav"    );
    PRECACHE_SOUND( "weapons/zoom.wav"        );
    PRECACHE_SOUND( "weapons/awp_deploy.wav"  );
    PRECACHE_SOUND( "weapons/awp_clipin.wav"  );
    PRECACHE_SOUND( "weapons/awp_clipout.wav" );

    m_iShellId = m_iShell = PRECACHE_MODEL( "models/rshell_big.mdl" );
    m_usFireAWP = PRECACHE_EVENT( 1, "events/awp.sc" );
}

int CAWP::GetItemInfo( ItemInfo *p )
{
    p->pszName   = STRING( pev->classname );
    p->pszAmmo1  = "338Magnum";
    p->iMaxAmmo1 = _338MAGNUM_MAX_CARRY;
    p->pszAmmo2  = NULL;
    p->iMaxAmmo2 = -1;
    p->iMaxClip  = AWP_MAX_CLIP;
    p->iSlot     = 0;
    p->iPosition = 2;
    p->iId       = m_iId = WEAPON_AWP;
    p->iFlags    = 0;
    p->iWeight   = AWP_WEIGHT;

    return 1;
}

int CAWP::iItemSlot( void )
{
    return PRIMARY_WEAPON_SLOT;
}

BOOL CAWP::Deploy( void )
{
    if( DefaultDeploy( "models/v_awp.mdl", "models/p_awp.mdl", AWP_DRAW, "rifle", UseDecrement() != FALSE ) )
    {
        m_flNextPrimaryAttack   = m_pPlayer->m_flNextAttack = GetNextAttackDelay( 1.45 );
        m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;

        return TRUE;
    }

    return FALSE;
}

void CAWP::SecondaryAttack( void )
{
    switch( m_pPlayer->m_iFOV )
    {
    case 90: m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 40; break;
    case 40: m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 10; break;
    default: m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 90; break;
    }

    // TODO: Implement me.
    // if( TheBots )
    // {
    //     TheBots->OnEvent( EVENT_WEAPON_ZOOMED, m_pPlayer, NULL );
    // }

    m_pPlayer->ResetMaxSpeed();

    EMIT_SOUND( m_pPlayer->edict(), CHAN_ITEM, "weapons/zoom.wav", 0.2, 2.4 );

    m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CAWP::PrimaryAttack( void )
{
    if( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
    {
        AWPFire( 0.85, 1.45, FALSE );
    }
    else if( m_pPlayer->pev->velocity.Length2D() > 140 )
    {
        AWPFire( 0.25, 1.45, FALSE );
    }
    else if( m_pPlayer->pev->velocity.Length2D() > 10 )
    {
        AWPFire( 0.1, 1.45, FALSE );
    }
    else if( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
    {
        AWPFire( 0.0, 1.45, FALSE );
    }
    else
    {
        AWPFire( 0.001, 1.45, FALSE );
    }
}

void CAWP::AWPFire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
    if( m_pPlayer->pev->fov != 90 )
    {
        m_pPlayer->m_bResumeZoom = true;
        m_pPlayer->m_iLastZoom   = m_pPlayer->m_iFOV;
        m_pPlayer->m_iFOV        = m_pPlayer->pev->fov = 90;
    }
    else
    {
        flCycleTime += 0.08;
    }

    if( m_iClip <= 0 )
    {
        if( m_fFireOnEmpty )
        {
            PlayEmptySound();
            m_flNextPrimaryAttack = GetNextAttackDelay( 0.2 );
        }

        // TODO: Implement me.
        // if( TheBots )
        // {
        //     TheBots->OnEvent( EVENT_WEAPON_FIRED_ON_EMPTY, m_pPlayer, NULL );
        // }

        return;
    }

    m_iClip--;

    m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
    m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

    UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

    m_pPlayer->m_flEjectBrass  = gpGlobals->time + 0.55;
    m_pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
    m_pPlayer->m_iWeaponFlash  = NORMAL_GUN_FLASH;

    Vector vecDir = FireBullets3( m_pPlayer->GetGunPosition(), gpGlobals->v_forward, flSpread,
        AWP_DISTANCE, AWP_PENETRATION, BULLET_PLAYER_338MAG, AWP_DAMAGE, AWP_RANGE_MODIFER, m_pPlayer->pev, TRUE, m_pPlayer->random_seed );

    int flags;

#if defined( CLIENT_WEAPONS )
    flags = FEV_NOTHOST;
#else
    flags = 0;
#endif

    PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireAWP, 0, ( float* )&g_vecZero, ( float * )&g_vecZero, vecDir.x, vecDir.y, 
        ( int )( m_pPlayer->pev->punchangle.x * 100 ), ( int )(m_pPlayer->pev->punchangle.x * 100 ), FALSE, FALSE);

    m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay( flCycleTime );

    if( !m_iClip && m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] <= 0 )
    {
        m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
    }

    m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
    m_pPlayer->pev->punchangle.x -= 2;
}

void CAWP::Reload( void )
{
    if( m_pPlayer->ammo_338mag <= 0 )
    {
        return;
    } 

    if( DefaultReload( AWP_MAX_CLIP, AWP_RELOAD, AWP_RELOAD_TIME ) )
    {
        m_pPlayer->SetAnimation( PLAYER_RELOAD );

        if( m_pPlayer->pev->fov != 90 )
        {
            m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 10;
            SecondaryAttack();
        }
    }
}

void CAWP::WeaponIdle( void )
{
    ResetEmptySound();
    m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

    if( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() && m_iClip )
    {
        m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60;
        SendWeaponAnim( AWP_IDLE, UseDecrement() != FALSE );
    }
}

BOOL UseDecrement( void )
{
    #if defined( CLIENT_WEAPONS )
        return TRUE;
    #else
        return FALSE;
    #endif
}

float CAWP::GetMaxSpeed( void )
{
    return m_pPlayer->m_iFOV == 90 ? AWP_MAX_SPEED : AWP_MAX_SPEED_ZOOM;
}