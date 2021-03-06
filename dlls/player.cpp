/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
/*

===== player.cpp ========================================================

functions dealing with the player

*/

#include "extdll.h"
#include "util.h"

#include "cbase.h"
#include "player.h"
#include "trains.h"
#include "nodes.h"
#include "weapons.h"
#include "soundent.h"
#include "monsters.h"
#include "shake.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"
#include "hltv.h"
#include "pm_shared.h"
#include "client.h"
#include "career_tasks.h"
#include "training_gamerules.h"
#include "studio.h"

// #define DUCKFIX

extern DLL_GLOBAL ULONG     g_ulModelIndexPlayer;
extern DLL_GLOBAL BOOL      g_fGameOver;
extern DLL_GLOBAL   BOOL    g_fDrawLines;
int gEvilImpulse101;
extern DLL_GLOBAL int       g_iSkillLevel, gDisplayTitle;

BOOL gInitHUD = TRUE;

extern void CopyToBodyQue(entvars_t* pev);
extern void respawn(entvars_t *pev, BOOL fCopyCorpse);
extern Vector VecBModelOrigin(entvars_t *pevBModel);
extern edict_t *EntSelectSpawnPoint(CBaseEntity *pPlayer);

// the world node graph
extern CGraph   WorldGraph;

// CS
extern WeaponStruct g_weaponStruct[MAX_WEAPONS];

#define TRAIN_ACTIVE    0x80
#define TRAIN_NEW       0xc0
#define TRAIN_OFF       0x00
#define TRAIN_NEUTRAL   0x01
#define TRAIN_SLOW      0x02
#define TRAIN_MEDIUM    0x03
#define TRAIN_FAST      0x04
#define TRAIN_BACK      0x05

#define FLASH_DRAIN_TIME     1.2 //100 units/3 minutes
#define FLASH_CHARGE_TIME    0.2 // 100 units/20 seconds  (seconds per unit)

AutoBuyInfoStruct g_autoBuyInfo[] =
{
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_RIFLE      , "galil" , "weapon_galil"        },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_RIFLE      , "ak47"  , "weapon_ak47"         },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SNIPERRIFLE, "scout" , "weapon_scout"        },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_RIFLE      , "sg552" , "weapon_sg552"        },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SNIPERRIFLE, "awp"   , "weapon_awp"          },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SNIPERRIFLE, "g3sg1" , "weapon_g3sg1"        },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_RIFLE      , "famas" , "weapon_famas"        },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_RIFLE      , "m4a1"  , "weapon_m4a1"         },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_RIFLE      , "aug"   , "weapon_aug"          },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SNIPERRIFLE, "sg550" , "weapon_sg550"        },
	{ AUTOBUYCLASS_SECONDARY | AUTOBUYCLASS_PISTOL   , "glock" , "weapon_glock18"      },
	{ AUTOBUYCLASS_SECONDARY | AUTOBUYCLASS_PISTOL   , "usp"   , "weapon_usp"          },
	{ AUTOBUYCLASS_SECONDARY | AUTOBUYCLASS_PISTOL   , "p228"  , "weapon_p228"         },
	{ AUTOBUYCLASS_SECONDARY | AUTOBUYCLASS_PISTOL   , "deagle", "weapon_deagle"       },
	{ AUTOBUYCLASS_SECONDARY | AUTOBUYCLASS_PISTOL   , "elites", "weapon_elite"        },
	{ AUTOBUYCLASS_SECONDARY | AUTOBUYCLASS_PISTOL   , "fn57"  , "weapon_fiveseven"    },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SHOTGUN    , "m3"    , "weapon_m3"           },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SHOTGUN    , "xm1014", "weapon_xm1014"       },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SMG        , "mac10" , "weapon_mac10"        },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SMG        , "tmp"   , "weapon_tmp"          },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SMG        , "mp5"   , "weapon_mp5navy"      },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SMG        , "ump45" , "weapon_ump45"        },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SMG        , "p90"   , "weapon_p90"          },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_MACHINEGUN , "m249"  , "weapon_m249"         },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_AMMO       , "primammo", "primammo"          },
	{ AUTOBUYCLASS_SECONDARY | AUTOBUYCLASS_AMMO     , "secammo" , "secammo"           },
	{ AUTOBUYCLASS_ARMOR                             , "vest"    , "item_kevlar"       },
	{ AUTOBUYCLASS_ARMOR                             , "vesthelm", "item_assaultsuit"  },
	{ AUTOBUYCLASS_GRENADE                           , "flash"   , "weapon_flashbang"  },
	{ AUTOBUYCLASS_GRENADE                           , "hegren"  , "weapon_hegrenade"  },
	{ AUTOBUYCLASS_GRENADE                           , "sgren"   , "weapon_smokegrenade"},
	{ AUTOBUYCLASS_NIGHTVISION                       , "nvgs"    , "nvgs"              },
	{ AUTOBUYCLASS_DEFUSER                           , "defuser" , "defuser"           },
	{ AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SHIELD     , "shield"  , "shield"            },
	{ 0, NULL, NULL }
};

// Global Savedata for player
TYPEDESCRIPTION CBasePlayer::m_playerSaveData[] =
{
	DEFINE_FIELD(CBasePlayer, m_flFlashLightTime, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_iFlashBattery, FIELD_INTEGER),

	DEFINE_FIELD(CBasePlayer, m_afButtonLast, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_afButtonPressed, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_afButtonReleased, FIELD_INTEGER),

	DEFINE_ARRAY(CBasePlayer, m_rgItems, FIELD_INTEGER, MAX_ITEMS),
	DEFINE_FIELD(CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER),

	DEFINE_FIELD(CBasePlayer, m_flTimeStepSound, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_flTimeWeaponIdle, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_flSwimTime, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_flDuckTime, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_flWallJumpTime, FIELD_TIME),

	DEFINE_FIELD(CBasePlayer, m_flSuitUpdate, FIELD_TIME),
	DEFINE_ARRAY(CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER, CSUITPLAYLIST),
	DEFINE_FIELD(CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER),
	DEFINE_ARRAY(CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER, CSUITNOREPEAT),
	DEFINE_ARRAY(CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_TIME, CSUITNOREPEAT),
	DEFINE_FIELD(CBasePlayer, m_lastDamageAmount, FIELD_INTEGER),

	DEFINE_ARRAY(CBasePlayer, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES),
	DEFINE_FIELD(CBasePlayer, m_pActiveItem, FIELD_CLASSPTR),
	DEFINE_FIELD(CBasePlayer, m_pLastItem, FIELD_CLASSPTR),

	DEFINE_ARRAY(CBasePlayer, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS),
	DEFINE_FIELD(CBasePlayer, m_idrowndmg, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_idrownrestored, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_tSneaking, FIELD_TIME),

	DEFINE_FIELD(CBasePlayer, m_iTrain, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_flFallVelocity, FIELD_FLOAT),
	DEFINE_FIELD(CBasePlayer, m_iTargetVolume, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iWeaponVolume, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iExtraSoundTypes, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iWeaponFlash, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_fLongJump, FIELD_BOOLEAN),
	DEFINE_FIELD(CBasePlayer, m_fInitHUD, FIELD_BOOLEAN),
	DEFINE_FIELD(CBasePlayer, m_tbdPrev, FIELD_TIME),

	DEFINE_FIELD(CBasePlayer, m_pTank, FIELD_EHANDLE),
	DEFINE_FIELD(CBasePlayer, m_iHideHUD, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iFOV, FIELD_INTEGER),

	//DEFINE_FIELD( CBasePlayer, m_fDeadTime           , FIELD_FLOAT                       ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_fGameHUDInitialized , FIELD_INTEGER                     ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_flStopExtraSoundTime, FIELD_TIME                        ),
	//DEFINE_FIELD( CBasePlayer, m_fKnownItem          , FIELD_INTEGER                     ), // reset to zero on load
	//DEFINE_FIELD( CBasePlayer, m_iPlayerSound        , FIELD_INTEGER                     ), // Don't restore, set in Precache()
	//DEFINE_FIELD( CBasePlayer, m_pentSndLast         , FIELD_EDICT                       ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flSndRoomtype       , FIELD_FLOAT                       ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flSndRange          , FIELD_FLOAT                       ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fNewAmmo            , FIELD_INTEGER                     ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flgeigerRange       , FIELD_FLOAT                       ), // Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_flgeigerDelay       , FIELD_FLOAT                       ), // Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_igeigerRangePrev    , FIELD_FLOAT                       ), // Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_iStepLeft           , FIELD_INTEGER                     ), // Don't need to restore
	//DEFINE_ARRAY( CBasePlayer, m_szTextureName       , FIELD_CHARACTER, CBTEXTURENAMEMAX ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_chTextureType       , FIELD_CHARACTER                   ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fNoPlayerSound      , FIELD_BOOLEAN                     ), // Don't need to restore, debug
	//DEFINE_FIELD( CBasePlayer, m_iUpdateTime         , FIELD_INTEGER                     ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_iClientHealth       , FIELD_INTEGER                     ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientBattery      , FIELD_INTEGER                     ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientHideHUD      , FIELD_INTEGER                     ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fWeapon             , FIELD_BOOLEAN                     ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames  , FIELD_INTEGER                     ), // Don't restore, depends on server message after spawning and only matters in multiplayer
	//DEFINE_FIELD( CBasePlayer, m_vecAutoAim          , FIELD_VECTOR                      ), // Don't save/restore - this is recomputed
	//DEFINE_ARRAY( CBasePlayer, m_rgAmmoLast          , FIELD_INTEGER, MAX_AMMO_SLOTS     ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fOnTarget           , FIELD_BOOLEAN                     ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames  , FIELD_INTEGER                     ), // Don't need to restore
};

int giPrecacheGrunt = 0;

int gmsgCurWeapon        = 0;
int gmsgGeigerRange      = 0;
int gmsgFlashlight       = 0;
int gmsgFlashBattery     = 0;
int gmsgHealth           = 0;
int gmsgDamage           = 0;
int gmsgBattery          = 0;
int gmsgTrain            = 0;
int gmsgHudText          = 0;
int gmsgSayText          = 0;
int gmsgTextMsg          = 0;
int gmsgWeaponList       = 0;
int gmsgResetHUD         = 0;
int gmsgInitHUD          = 0;
int gmsgViewMode         = 0;
int gmsgShowGameTitle    = 0;
int gmsgDeathMsg         = 0;
int gmsgScoreAttrib      = 0;
int gmsgScoreInfo        = 0;
int gmsgTeamInfo         = 0;
int gmsgTeamScore        = 0;
int gmsgGameMode         = 0;
int gmsgMOTD             = 0;
int gmsgServerName       = 0;
int gmsgAmmoPickup       = 0;
int gmsgWeapPickup       = 0;
int gmsgItemPickup       = 0;
int gmsgHideWeapon       = 0;
int gmsgSetFOV           = 0;
int gmsgShowMenu         = 0;
int gmsgShake            = 0;
int gmsgFade             = 0;
int gmsgAmmoX            = 0;
int gmsgSendAudio        = 0;
int gmsgRoundTime        = 0;
int gmsgMoney            = 0;
int gmsgArmorType        = 0;
int gmsgBlinkAcct        = 0;
int gmsgStatusValue      = 0;
int gmsgStatusText       = 0;
int gmsgStatusIcon       = 0;
int gmsgBarTime          = 0;
int gmsgReloadSound      = 0;
int gmsgCrosshair        = 0;
int gmsgNVGToggle        = 0;
int gmsgRadar            = 0;
int gmsgSpectator        = 0;
int gmsgVGUIMenu         = 0;
int gmsgTutorText        = 0;
int gmsgTutorLine        = 0;
int gmsgTutorState       = 0;
int gmsgTutorClose       = 0;
int gmsgAllowSpec        = 0;
int gmsgBombDrop         = 0;
int gmsgBombPickup       = 0;
int gmsgSendCorpse       = 0;
int gmsgHostagePos       = 0;
int gmsgHostageK         = 0;
int gmsgHLTV             = 0;
int gmsgSpecHealth       = 0;
int gmsgForceCam         = 0;
int gmsgADStop           = 0;
int gmsgReceiveW         = 0;
int gmsgCZCareer         = 0;
int gmsgCZCareerHUD      = 0;
int gmsgShadowIdx        = 0;
int gmsgTaskTime         = 0;
int gmsgScenarioIcon     = 0;
int gmsgBotVoice         = 0;
int gmsgBuyClose         = 0;
int gmsgSpecHealth2      = 0;
int gmsgBarTime2         = 0;
int gmsgItemStatus       = 0;
int gmsgLocation         = 0;
int gmsgBotProgress      = 0;
int gmsgBrass            = 0;
int gmsgFog              = 0;
int gmsgShowTimer        = 0;
int gmsgHudTextArgs      = 0;
int gmsgLogo             = 0;

// CS
void LinkUserMessages(void)  // Last check: 2013, May 29
{
	if (!gmsgCurWeapon)
	{
		gmsgCurWeapon     = REG_USER_MSG("CurWeapon", 3);
		gmsgGeigerRange   = REG_USER_MSG("Geiger", 1);
		gmsgFlashlight    = REG_USER_MSG("Flashlight", 2);
		gmsgFlashBattery  = REG_USER_MSG("FlashBat", 1);
		gmsgHealth        = REG_USER_MSG("Health", 1);
		gmsgDamage        = REG_USER_MSG("Damage", 12);
		gmsgBattery       = REG_USER_MSG("Battery", 2);
		gmsgTrain         = REG_USER_MSG("Train", 1);
		gmsgHudText       = REG_USER_MSG("HudTextPro", -1);
		/* not used    */   REG_USER_MSG("HudText", -1);
		gmsgSayText       = REG_USER_MSG("SayText", -1);
		gmsgTextMsg       = REG_USER_MSG("TextMsg", -1);
		gmsgWeaponList    = REG_USER_MSG("WeaponList", -1);
		gmsgResetHUD      = REG_USER_MSG("ResetHUD", 0);
		gmsgInitHUD       = REG_USER_MSG("InitHUD", 0);
		gmsgViewMode      = REG_USER_MSG("ViewMode", 0);
		gmsgShowGameTitle = REG_USER_MSG("GameTitle", 1);
		gmsgDeathMsg      = REG_USER_MSG("DeathMsg", -1);
		gmsgScoreAttrib   = REG_USER_MSG("ScoreAttrib", 2);
		gmsgScoreInfo     = REG_USER_MSG("ScoreInfo", 9);
		gmsgTeamInfo      = REG_USER_MSG("TeamInfo", -1);
		gmsgTeamScore     = REG_USER_MSG("TeamScore", -1);
		gmsgGameMode      = REG_USER_MSG("GameMode", 1);
		gmsgMOTD          = REG_USER_MSG("MOTD", -1);
		gmsgServerName    = REG_USER_MSG("ServerName", -1);
		gmsgAmmoPickup    = REG_USER_MSG("AmmoPickup", 2);
		gmsgWeapPickup    = REG_USER_MSG("WeapPickup", 1);
		gmsgItemPickup    = REG_USER_MSG("ItemPickup", -1);
		gmsgHideWeapon    = REG_USER_MSG("HideWeapon", 1);
		gmsgSetFOV        = REG_USER_MSG("SetFOV", 1);
		gmsgShowMenu      = REG_USER_MSG("ShowMenu", -1);
		gmsgShake         = REG_USER_MSG("ScreenShake", sizeof(ScreenShake));
		gmsgFade          = REG_USER_MSG("ScreenFade", sizeof(ScreenFade));
		gmsgAmmoX         = REG_USER_MSG("AmmoX", 2);
		gmsgSendAudio     = REG_USER_MSG("SendAudio", -1);
		gmsgRoundTime     = REG_USER_MSG("RoundTime", 2);
		gmsgMoney         = REG_USER_MSG("Money", 5);
		gmsgArmorType     = REG_USER_MSG("ArmorType", 1);
		gmsgBlinkAcct     = REG_USER_MSG("BlinkAcct", 1);
		gmsgStatusValue   = REG_USER_MSG("StatusValue", -1);
		gmsgStatusText    = REG_USER_MSG("StatusText", -1);
		gmsgStatusIcon    = REG_USER_MSG("StatusIcon", -1);
		gmsgBarTime       = REG_USER_MSG("BarTime", 2);
		gmsgReloadSound   = REG_USER_MSG("ReloadSound", 2);
		gmsgCrosshair     = REG_USER_MSG("Crosshair", 1);
		gmsgNVGToggle     = REG_USER_MSG("NVGToggle", 1);
		gmsgRadar         = REG_USER_MSG("Radar", 7);
		gmsgSpectator     = REG_USER_MSG("Spectator", 2);
		gmsgVGUIMenu      = REG_USER_MSG("VGUIMenu", -1);
		gmsgTutorText     = REG_USER_MSG("TutorText", -1);
		gmsgTutorLine     = REG_USER_MSG("TutorLine", -1);
		gmsgTutorState    = REG_USER_MSG("TutorState", -1);
		gmsgTutorClose    = REG_USER_MSG("TutorClose", -1);
		gmsgAllowSpec     = REG_USER_MSG("AllowSpec", 1);
		gmsgBombDrop      = REG_USER_MSG("BombDrop", 7);
		gmsgBombPickup    = REG_USER_MSG("BombPickup", 0);
		gmsgSendCorpse    = REG_USER_MSG("ClCorpse", -1);
		gmsgHostagePos    = REG_USER_MSG("HostagePos", 8);
		gmsgHostageK      = REG_USER_MSG("HostageK", 1);
		gmsgHLTV          = REG_USER_MSG("HLTV", 2);
		gmsgSpecHealth    = REG_USER_MSG("SpecHealth", 1);
		gmsgForceCam      = REG_USER_MSG("ForceCam", 3);
		gmsgADStop        = REG_USER_MSG("ADStop", 0);
		gmsgReceiveW      = REG_USER_MSG("ReceiveW", 1);
		gmsgCZCareer      = REG_USER_MSG("CZCareer", -1);
		gmsgCZCareerHUD   = REG_USER_MSG("CZCareerHUD", -1);
		gmsgShadowIdx     = REG_USER_MSG("ShadowIdx", 4);
		gmsgTaskTime      = REG_USER_MSG("TaskTime", 4);
		gmsgScenarioIcon  = REG_USER_MSG("Scenario", -1);
		gmsgBotVoice      = REG_USER_MSG("BotVoice", 2);
		gmsgBuyClose      = REG_USER_MSG("BuyClose", 0);
		gmsgSpecHealth2   = REG_USER_MSG("SpecHealth2", 2);
		gmsgBarTime2      = REG_USER_MSG("BarTime2", 4);
		gmsgItemStatus    = REG_USER_MSG("ItemStatus", 1);
		gmsgLocation      = REG_USER_MSG("Location", -1);
		gmsgBotProgress   = REG_USER_MSG("BotProgress", -1);
		gmsgBrass         = REG_USER_MSG("Brass", -1);
		gmsgFog           = REG_USER_MSG("Fog", 7);
		gmsgShowTimer     = REG_USER_MSG("ShowTimer", 0);
		gmsgHudTextArgs   = REG_USER_MSG("HudTextArgs", -1);
	}
}

LINK_ENTITY_TO_CLASS(player, CBasePlayer);



const char *GetCSModelName(int item_id)  // Last check : 2014, November 17.
{
	const char *modelName = NULL;

	switch (item_id)
	{
		case WEAPON_AK47        : modelName = "models/w_ak47.mdl"        ; break;
		case WEAPON_GALIL       : modelName = "models/w_galil.mdl"       ; break;
		case WEAPON_FAMAS       : modelName = "models/w_famas.mdl"       ; break;
		case WEAPON_AWP         : modelName = "models/w_awp.mdl"         ; break;
		case WEAPON_DEAGLE      : modelName = "models/w_deagle.mdl"      ; break;
		case WEAPON_G3SG1       : modelName = "models/w_g3sg1.mdl"       ; break;
		case WEAPON_SG550       : modelName = "models/w_sg550.mdl"       ; break;
		case WEAPON_GLOCK18     : modelName = "models/w_glock18.mdl"     ; break;
		case WEAPON_M249        : modelName = "models/w_m249.mdl"        ; break;
		case WEAPON_M3          : modelName = "models/w_m3.mdl"          ; break;
		case WEAPON_M4A1        : modelName = "models/w_m4a1.mdl"        ; break;
		case WEAPON_MP5N        : modelName = "models/w_mp5.mdl"         ; break;
		case WEAPON_SG552       : modelName = "models/w_sg552.mdl"       ; break;
		case WEAPON_AUG         : modelName = "models/w_aug.mdl"         ; break;
		case WEAPON_TMP         : modelName = "models/w_tmp.mdl"         ; break;
		case WEAPON_USP         : modelName = "models/w_usp.mdl"         ; break;
		case WEAPON_ELITE       : modelName = "models/w_elite.mdl"       ; break;
		case WEAPON_FIVESEVEN   : modelName = "models/w_fiveseven.mdl"   ; break;
		case WEAPON_P90         : modelName = "models/w_p90.mdl"         ; break;
		case WEAPON_UMP45       : modelName = "models/w_ump45.mdl"       ; break;
		case WEAPON_MAC10       : modelName = "models/w_mac10.mdl"       ; break;
		case WEAPON_P228        : modelName = "models/w_p228.mdl"        ; break;
		case WEAPON_SCOUT       : modelName = "models/w_scout.mdl"       ; break;
		case WEAPON_KNIFE       : modelName = "models/w_knife.mdl"       ; break;
		case WEAPON_FLASHBANG   : modelName = "models/w_flashbang.mdl"   ; break;
		case WEAPON_HEGRENADE   : modelName = "models/w_hegrenade.mdl"   ; break;
		case WEAPON_SMOKEGRENADE: modelName = "models/w_smokegrenade.mdl"; break;
		case WEAPON_XM1014      : modelName = "models/w_xm1014.mdl"      ; break;
		case WEAPON_C4          : modelName = "models/w_backpack.mdl"    ; break;
		case WEAPON_SHIELDGUN   : modelName = "models/w_shield.mdl"      ; break;
		default: ALERT(at_console, "CBasePlayer::PackDeadPlayerItems(): Unhandled item- not creating weaponbox\n");
	}

	return modelName;
}

void packPlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem, bool packAmmo)  // Last check: 2013, November 18.
{
	if (pItem)
	{
		const char *modelName = GetCSModelName(pItem->m_iId);

		if (modelName)
		{
			CWeaponBox *pWeaponBox = (CWeaponBox *)CBaseEntity::Create("weaponbox", pPlayer->pev->origin, pPlayer->pev->angles, pPlayer->edict());

			pWeaponBox->pev->angles.x = 0;
			pWeaponBox->pev->angles.y = 0;
			pWeaponBox->pev->velocity = pPlayer->pev->velocity * 0.75;

			pWeaponBox->SetThink(&CWeaponBox::Kill);
			pWeaponBox->pev->nextthink = gpGlobals->time + 300;

			pWeaponBox->PackWeapon(pItem);

			if (packAmmo)
			{
				pWeaponBox->PackAmmo(MAKE_STRING(pItem->pszAmmo1()), pPlayer->m_rgAmmo[pItem->PrimaryAmmoIndex()]);
			}

			SET_MODEL(pWeaponBox->edict(), modelName);
		}
	}
}

void SendItemStatus(CBasePlayer *pPlayer) // Last check : 2014, November 17.
{
	int itemStatus = 0;

	if (pPlayer->m_bHasNightVision)
	{
		itemStatus |= ITEMSTATE_HASNIGHTVISION;
	}

	if (pPlayer->m_bHasDefuser)
	{
		itemStatus |= ITEMSTATE_HASDEFUSER;
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgItemStatus, NULL, pPlayer->edict());
		WRITE_BYTE(itemStatus);
	MESSAGE_END();
}

void CBasePlayer::AddAccount(int amount, bool bTrackChange) // Last check : 2013, June 13.
{
	m_iAccount += amount;

	if (m_iAccount < 0)
	{
		m_iAccount = 0;
	}
	else if (m_iAccount > 16000)
	{
		m_iAccount = 16000;
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgMoney, NULL, edict());
		WRITE_LONG(m_iAccount);
		WRITE_BYTE(bTrackChange);
	MESSAGE_END();
}

void CBasePlayer::AddAutoBuyData(const char *string) // Last check : 2013, June 13.
{
	int length = strlen(m_autoBuyString);

	if (length < sizeof(m_autoBuyString))
	{
		if (length > 0)
		{
			m_autoBuyString[length] = ' ';
		}

		strncat(m_autoBuyString, string, sizeof(m_autoBuyString) - length);
	}
}

/**
 * Add a weapon to the player (Item == Weapon == Selectable Object)
 */
int CBasePlayer::AddPlayerItem(CBasePlayerItem *pItem) // Last check : 2013, June 13.
{
	CBasePlayerItem *pInsert = m_rgpPlayerItems[pItem->iItemSlot()];

	while (pInsert)
	{
		if (FClassnameIs(pInsert->pev, STRING(pItem->pev->classname)))
		{
			if (pItem->AddDuplicate(pInsert))
			{
				g_pGameRules->PlayerGotWeapon(this, pItem);

				pItem->CheckRespawn();
				pItem->UpdateItemInfo();

				if (m_pActiveItem)
				{
					m_pActiveItem->UpdateItemInfo();
				}

				pItem->Kill();
			}
			else if (gEvilImpulse101)
			{
				pItem->Kill();
			}

			return FALSE;
		}

		pInsert = pInsert->m_pNext;
	}

	if (pItem->AddToPlayer(this))
	{
		g_pGameRules->PlayerGotWeapon(this, pItem);

		if (pItem->iItemSlot() == PRIMARY_WEAPON_SLOT)
		{
			m_bHasPrimary = true;
		}

		pItem->CheckRespawn();

		pItem->m_pNext = m_rgpPlayerItems[pItem->iItemSlot()];
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem;

		if (HasShield())
		{
			pev->gamestate = 0;
		}

		if (g_pGameRules->FShouldSwitchWeapon(this, pItem) && !m_bShieldDrawn)
		{
			SwitchWeapon(pItem);
		}

		return TRUE;
	}
	else if (gEvilImpulse101)
	{
		pItem->Kill();
	}

	return FALSE;
}

void CBasePlayer::AddPoints(int score, BOOL bAllowNegativeScore) // Last check : 2013, June 13.
{
	if (score < 0)
	{
		if (!bAllowNegativeScore)
		{
			if (pev->frags < 0)
			{
				return;
			}

			if (-score > pev->frags)
			{
				score = -pev->frags;
			}
		}
	}

	pev->frags += score;

	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	WRITE_BYTE(entindex());
	WRITE_SHORT(pev->frags);
	WRITE_SHORT(m_iDeaths);
	WRITE_SHORT(0);
	WRITE_SHORT(m_iTeam);
	MESSAGE_END();
}

void CBasePlayer::AddPointsToTeam(int score, BOOL bAllowNegativeScore) // Last check : 2013, June 13.
{
	int index = entindex();

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex(i);

		if (pPlayer && i != index)
		{
			if (g_pGameRules->PlayerRelationship(this, pPlayer) == GR_TEAMMATE)
			{
				pPlayer->AddPoints(score, bAllowNegativeScore);
			}
		}
	}
}

int CBasePlayer::AmmoInventory(int iAmmoIndex) // Last check : 2013, June 6.
{
	if (iAmmoIndex == -1)
	{
		return -1;
	}

	return m_rgAmmo[iAmmoIndex];
}

void CBasePlayer::AutoBuy(void) // Last check : 2013, June 6.
{
	/*bool boughtPrimary   = false;
	bool boughtSecondary = false;

	const char *weapon = NULL;
	char string[sizeof(m_autoBuyString)];

	weapon = PickFlashKillWeaponString();

	if ((weapon = PickFlashKillWeaponString()))
	{
		ParseAutoBuyString(weapon, boughtPrimary, boughtSecondary);
	}

	if ((weapon = PickGrenadeKillWeaponString()))
	{
		ParseAutoBuyString(weapon, boughtPrimary, boughtSecondary);
	}

	if ((weapon = PickPrimaryCareerTaskWeapon()))
	{
		strcpy(string, weapon);

		PrioritizeAutoBuyString(string, m_autoBuyString);
		ParseAutoBuyString(string, boughtPrimary, boughtSecondary);
	}

	if ((weapon = PickSecondaryCareerTaskWeapon()))
	{
		strcpy(string, weapon);

		PrioritizeAutoBuyString(string, m_autoBuyString);
		ParseAutoBuyString(string, boughtPrimary, boughtSecondary);
	}

	if (m_autoBuyString[0])
	{
		ParseAutoBuyString(m_autoBuyString, boughtPrimary, boughtSecondary);
	}

	if ((weapon = PickFlashKillWeaponString()))
	{
		ParseAutoBuyString(weapon, boughtPrimary, boughtSecondary);
	}*/

	// TODO: Implement me.
	// if (TheTutor)
	// {
	//     TheTutor->OnEvent(VENT_PLAYER_LEFT_BUY_ZONE, NULL, NULL);
	// }
}

void CBasePlayer::InitRebuyData(const char *string) // Last check : 2014, November 17.
{
	if (!string)
	{
		return;
	}

	int length = strlen(string);

	if (length <= MAX_REBUY_LENGTH)
	{
		if (m_rebuyString)
		{
			delete m_rebuyString;
			m_rebuyString = NULL;
		}

		m_rebuyString = new char[length + 1];
		strcpy(m_rebuyString, string);

		m_rebuyString[length] = '\0';
	}
}

void CBasePlayer::ParseAutoBuyString(const char *string, bool &boughtPrimary, bool &boughtSecondary)  // Last check : 2014, November 18.
{
	char command[32];
	const char *c = string;

	if (!string && !*string)
	{
		return;
	}

	while (*c != NULL)
	{
		int i = 0;

		while (*c && *c != ' ' && i < sizeof(command))
		{
			command[i] = *c;
			++c;
			++i;
		}

		if (*c == ' ')
		{
			++c;
		}

		command[i] = 0;
		i = 0;

		while (command[i] != 0)
		{
			if (command[i] == ' ')
			{
				command[i] = 0;
				break;
			}

			++i;
		}

		if (!strlen(command))
		{
			continue;
		}

		AutoBuyInfoStruct *commandInfo = GetAutoBuyCommandInfo(command);

		if (ShouldExecuteAutoBuyCommand(commandInfo, boughtPrimary, boughtSecondary))
		{
			ClientCommand(commandInfo->m_command);
			PostAutoBuyCommandProcessing(commandInfo, boughtPrimary, boughtSecondary);
		}
	}
}

void CBasePlayer::PostAutoBuyCommandProcessing(const AutoBuyInfoStruct *commandInfo, bool &boughtPrimary, bool &boughtSecondary)
{
	if (!commandInfo)
	{
		return;
	}

	CBasePlayerWeapon *pPrimary   = (CBasePlayerWeapon *)m_rgpPlayerItems[PRIMARY_WEAPON_SLOT];
	CBasePlayerWeapon *pSecondary = (CBasePlayerWeapon *)m_rgpPlayerItems[PISTOL_SLOT];

	if (pPrimary && !stricmp(STRING(pPrimary->pev->classname), commandInfo->m_classname))
	{
		boughtPrimary = true;
	}
	else if ((commandInfo->m_class & AUTOBUYCLASS_SHIELD) && HasShield())
	{
		boughtPrimary = true;
	}
	else if (pSecondary && !stricmp(STRING(pSecondary->pev->classname), commandInfo->m_classname))
	{
		boughtSecondary = true;
	}
}

bool CBasePlayer::ShouldExecuteAutoBuyCommand(const AutoBuyInfoStruct *commandInfo, bool boughtPrimary, bool boughtSecondary)
{
	if (!commandInfo)
	{
		return false;
	}

	if (boughtPrimary && FBitSet(commandInfo->m_class, AUTOBUYCLASS_PRIMARY) && !FBitSet(commandInfo->m_class, AUTOBUYCLASS_AMMO))
	{
		return false;
	}

	if (boughtSecondary && FBitSet(commandInfo->m_class, AUTOBUYCLASS_SECONDARY) && !FBitSet(commandInfo->m_class, AUTOBUYCLASS_AMMO))
	{
		return false;
	}

	return true;
}



void CBasePlayer::Blind(float duration, float holdTime, float fadeTime, int alpha) // Last check : 2014, November 11.
{
	m_blindUntilTime = gpGlobals->time + duration;
	m_blindStartTime = gpGlobals->time;
	m_blindHoldTime = holdTime;
	m_blindFadeTime = fadeTime;
	m_blindAlpha = alpha;
};

bool CBasePlayer::CanPlayerBuy(bool display) // Last check: 2013, September 13.
{
	if (!g_pGameRules->IsMultiplayer())
	{
		return CHalfLifeTraining::PlayerCanBuy(this);
	}

	if (pev->deadflag != DEAD_NO || !(m_signals.GetState() & SIGNAL_BUY))
	{
		return false;
	}

	int buyTime = (int)CVAR_GET_FLOAT("mp_buytime") * 60;

	if (buyTime < 15)
	{
		buyTime = 15;
		CVAR_SET_FLOAT("mp_buytime", 1 / (60 / 15));
	}

	if (gpGlobals->time - g_pGameRules->m_fRoundCount > buyTime)
	{
		if (display)
		{
			ClientPrint(pev, HUD_PRINTCENTER, "#Cant_buy", UTIL_dtos1(buyTime));
		}

		return false;
	}

	if (m_bIsVIP)
	{
		if (display)
		{
			ClientPrint(pev, HUD_PRINTCENTER, "#VIP_cant_buy");
		}

		return false;
	}

	if (g_pGameRules->m_bCTCantBuy && m_iTeam == CT)
	{
		if (display == true)
		{
			ClientPrint(pev, HUD_PRINTCENTER, "#CT_cant_buy");
		}

		return false;
	}

	if (g_pGameRules->m_bTCantBuy && m_iTeam == TERRORIST)
	{
		if (display)
		{
			ClientPrint(pev, HUD_PRINTCENTER, "#Terrorist_cant_buy");
		}

		return false;
	}

	return true;
}


void CBasePlayer::CalculatePitchBlend(void) // Last check: 2013, September 13.
{
	float pitch = (int)pev->angles[0] * 3;
	float blend_pitch;

	if (pitch <= -45)
	{
		blend_pitch = 255;
	}
	else if (pitch < 45)
	{
		blend_pitch = (45.0 - pitch) * 255 / 90;
	}
	else
	{
		blend_pitch = 0;
	}
		
	pev->blending[1] = (int)blend_pitch;

	m_flPitch = blend_pitch;
}

void CBasePlayer::CalculateYawBlend(void) // Last check: 2013, September 13.
{
	StudioEstimateGait();

	float yaw = pev->angles[1] - m_flGaityaw;

	if (yaw < -180)
	{
		yaw += 360;
	}
	else if (yaw > 180)
	{
		yaw -= 360;
	}

	if (m_flGaitMovement != 0)
	{
		if (yaw > 120)
		{
			m_flGaityaw -= 180;
			m_flGaitMovement = -m_flGaitMovement;
			yaw -= 180;
		}
		else if (yaw < -120)
		{
			m_flGaityaw += 180;
			m_flGaitMovement = -m_flGaitMovement;
			yaw += 180;
		}
	}

	float blend_yaw = (yaw / 90) * 128.0 + 127.0;

	if (blend_yaw > 255)
	{
		blend_yaw = 255;
	}

	if (blend_yaw < 0)
	{
		blend_yaw = 0;
	}

	blend_yaw = 255.0 - blend_yaw;
	pev->blending[0] = (int)blend_yaw;
	m_flYaw = blend_yaw;
}

void CBasePlayer::StudioEstimateGait(void) // Last check: 2013, September 13.
{
	float dt = gpGlobals->frametime;

	if (dt < 0)
	{
		dt = 0;
	}
	else if (dt > 1)
	{
		dt = 1;
	}

	if (dt == 0)
	{
		m_flGaitMovement = 0;
		return;
	}

	Vector velocity = pev->origin - m_prevgaitorigin;

	m_prevgaitorigin = pev->origin;
	m_flGaitMovement = velocity.Length();
		
	if (dt <= 0 || m_flGaitMovement / dt < 5)
	{
		m_flGaitMovement = 0;
		velocity.x = velocity.y = 0;
	}

	if (velocity.x == 0 && velocity.y == 0)
	{
		float flYawDiff = pev->angles[1] - m_flGaityaw;
		float flYaw = flYawDiff;

		flYawDiff = flYawDiff - (int)(flYawDiff / 360) * 360;

		if (flYawDiff > 180)
		{
			flYawDiff -= 360;
		}

		if (flYawDiff < -180)
		{
			flYawDiff += 360;
		}

		if (flYaw < -180)
		{
			flYaw += 360;
		}
		else if (flYaw > 180)
		{
			flYaw -= 360;
		}

		if (flYaw > -5 && flYaw < 5)
		{
			m_flYawModifier = 0.05;
		}

		if (flYaw < -90 || flYaw > 90)
		{
			m_flYawModifier = 3.5;
		}

		if (dt < 0.25)
		{
			flYawDiff *= dt * m_flYawModifier;
		}
		else
		{
			flYawDiff *= dt;
		}

		if (abs(flYawDiff) < 0.1)
		{
			flYawDiff = 0;
		}

		m_flGaityaw += flYawDiff;
		m_flGaityaw -= (int)(m_flGaityaw / 360) * 360;
		m_flGaitMovement = 0;
	}
	else
	{
		m_flGaityaw = (atan2(velocity.y, velocity.x) * 180 / M_PI);

		if (m_flGaityaw > 180)
		{
			m_flGaityaw = 180;
		}

		if (m_flGaityaw < -180)
		{
			m_flGaityaw = -180;
		}
	}
}

void CBasePlayer::StudioProcessGait(void) // Last check: 2013, September 14.
{
	float dt = max(0, gpGlobals->frametime);

	if (dt > 1)
	{
		dt = 1;
	}

	CalculateYawBlend();
	CalculatePitchBlend();

	void *pmodel = GET_MODEL_PTR(ENT(pev));

	if (pmodel)
	{
		studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
		mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + pev->gaitsequence;

		if (pseqdesc->linearmovement[0] > 0)
		{
			m_flGaitframe += (m_flGaitMovement / pseqdesc->linearmovement[0]) * pseqdesc->numframes;
		}
		else
		{
			m_flGaitframe += pseqdesc->fps * dt * pev->framerate;
		}

		m_flGaitframe -= (m_flGaitframe / pseqdesc->numframes) * pseqdesc->numframes;

		if (m_flGaitframe < 0)
		{
			m_flGaitframe += pseqdesc->numframes;
		}
	}
}

bool CBasePlayer::CanAffordArmor(void) // Last check: 2013, September 14.
{
	if (m_iKevlar != 1)
	{
		return m_iAccount >= 650;
	}

	if (pev->armorvalue < 100)
	{
		return m_iAccount >= 650;
	}
	
	return m_iAccount >= 350;
}

bool CBasePlayer::CanAffordDefuseKit(void) // Last check: 2013, September 14.
{
	return m_iAccount >= 200;
}

bool CBasePlayer::CanAffordGrenade(void) // Last check: 2013, September 14.
{
	return m_iAccount >= 200;
}

bool CBasePlayer::CanAffordPrimary(void) // Last check: 2013, September 14.
{
	if (m_iTeam != CT && m_iTeam != TERRORIST)
	{
		return false;
	}

	for (size_t i = 0; i < MAX_WEAPONS; ++i)
	{
		WeaponStruct *weapon = &g_weaponStruct[i];

		if ((weapon->m_side & m_iTeam) && weapon->m_slot == PRIMARY_WEAPON_SLOT && m_iAccount >= weapon->m_price)
		{
			return true;
		}
	}

	return false;
}

bool CBasePlayer::CanAffordPrimaryAmmo(void) // Last check: 2013, September 14.
{
	CBasePlayerItem *pItem = m_rgpPlayerItems[PRIMARY_WEAPON_SLOT];

	if (pItem)
	{
		for (size_t i = 0; i < MAX_WEAPONS; ++i)
		{
			WeaponStruct *weapon = &g_weaponStruct[i];

			if (weapon->m_type == pItem->m_iId && m_iAccount >= weapon->m_ammoPrice)
			{
				return true;
			}
		}
	}

	return false;
}

bool CBasePlayer::CanAffordSecondaryAmmo(void) // Last check: 2013, September 14.
{
	CBasePlayerItem *pItem = m_rgpPlayerItems[PISTOL_SLOT];

	if (pItem)
	{
		for (size_t i = 0; i < MAX_WEAPONS; ++i)
		{
			WeaponStruct *weapon = &g_weaponStruct[i];

			if (weapon->m_type == pItem->m_iId && m_iAccount >= weapon->m_ammoPrice)
			{
				return true;
			}
		}
	}

	return false;
}


bool CBasePlayer::NeedsArmor(void)  // Last check: 2013, September 14.
{
	return m_iKevlar && pev->armorvalue < 50;
}

bool CBasePlayer::NeedsDefuseKit(void)  // Last check: 2013, September 14.
{
	return !m_bHasDefuser && m_iTeam == CT && g_pGameRules->m_bMapHasBombTarget;
}

bool CBasePlayer::NeedsGrenade(void)  // Last check: 2013, September 14.
{
	int ammoIndex;

	if ((ammoIndex = GetAmmoIndex("HEGrenade")) > -1 && m_rgAmmo[ammoIndex])
	{
		return false;
	}

	if ((ammoIndex = GetAmmoIndex("Flashbang")) > -1 && m_rgAmmo[ammoIndex])
	{
		return false;
	}

	if ((ammoIndex = GetAmmoIndex("SmokeGrenade")) > -1 && m_rgAmmo[ammoIndex])
	{
		return false;
	}

	return true;
}

bool CBasePlayer::NeedsPrimaryAmmo(void)  // Last check: 2013, September 14.
{
	CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_rgpPlayerItems[PRIMARY_WEAPON_SLOT];

	return pWeapon && pWeapon->m_iId != WEAPON_SHIELDGUN && m_rgAmmo[pWeapon->m_iPrimaryAmmoType] < pWeapon->iMaxAmmo1();
}

bool CBasePlayer::NeedsSecondaryAmmo(void)  // Last check: 2013, September 14.
{
	CBasePlayerWeapon* pWeapon = (CBasePlayerWeapon *)m_rgpPlayerItems[PISTOL_SLOT];

	return pWeapon && m_rgAmmo[pWeapon->m_iPrimaryAmmoType] < pWeapon->iMaxAmmo1();
}


void CBasePlayer::CheckPowerups(entvars_s *pev) // Last check: 2013, November 17.
{
	if (pev->health > 0)
	{
		pev->modelindex = m_modelIndexPlayer;
	}
}

void CBasePlayer::ClearAutoBuyData(void) // Last check: 2013, November 17.
{
	m_autoBuyString[0] = '\0';
}

const char *BotArgs[4];
bool UseBotArgs = false;

void CBasePlayer::ClientCommand(const char *cmd, const char *arg1, const char *arg2, const char *arg3) // Last check: 2013, November 17.
{
	UseBotArgs = true;

	BotArgs[0] = cmd;
	BotArgs[1] = arg1;
	BotArgs[2] = arg2;
	BotArgs[3] = arg3;

	::ClientCommand(edict());

	UseBotArgs = false;
}

void CBasePlayer::Disappear(void) // Last check: 2013, November 17.
{
	if (m_pTank)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));

	if (pSound)
	{
		pSound->Reset();
	}

	m_fSequenceFinished = TRUE;

	pev->modelindex = m_modelIndexPlayer;
	pev->view_ofs   = Vector(0, 0, -8);
	pev->deadflag   = DEAD_DYING;
	pev->solid      = SOLID_NOT;
	pev->flags     &= FL_ONGROUND;

	m_iClientHealth = 0;

	MESSAGE_BEGIN(MSG_ONE, gmsgHealth, NULL, edict());
		WRITE_BYTE(m_iClientHealth);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, edict());
		WRITE_BYTE(0);
		WRITE_BYTE(0xFF);
		WRITE_BYTE(0xFF);
	MESSAGE_END();

	SendFOV(0);

	g_pGameRules->CheckWinConditions();

	m_bNotKilled = false;

	if (m_bHasC4)
	{
		DropPlayerItem("weapon_c4");
		SetProgressBarTime(0);
	}
	else if (m_bHasDefuser)
	{
		m_bHasDefuser = false;
		pev->body = 0;

		GiveNamedItem("item_thighpack");

		MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, edict());
			WRITE_BYTE(STATUSICON_HIDE);
			WRITE_STRING("defuser");
		MESSAGE_END();

		SendItemStatus(this);
		SetProgressBarTime(0);
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, edict());
		WRITE_BYTE(STATUSICON_HIDE);
		WRITE_STRING("buyzone");
	MESSAGE_END();

	if (m_iMenu >= Menu_Buy)
	{
		if (m_iMenu <= Menu_BuyItem)
		{
			CLIENT_COMMAND(ENT(pev), "slot10\n");
		}
		else if (m_iMenu == Menu_ClientBuy)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgBuyClose, NULL, edict());
			MESSAGE_END();
		}
	}

	SetThink(&CBasePlayer::PlayerDeathThink);

	pev->nextthink = gpGlobals->time + 0.1;
	pev->angles.x  = 0;
	pev->angles.z  = 0;
}

AutoBuyInfoStruct *CBasePlayer::GetAutoBuyCommandInfo(const char *command)  // Last check: 2013, November 17.
{
	size_t i = 0;
	AutoBuyInfoStruct *info = NULL;

	do
	{
		info = &(g_autoBuyInfo[i++]);

		if (stricmp(info->m_command, command) == 0)
		{
			return info;
		}

	} while (info->m_class);

	return NULL;
}

CBasePlayer *CBasePlayer::GetNextRadioRecipient(CBasePlayer *pStartPlayer) // Last check: 2013, November 17.
{
	CBaseEntity *pEntity = NULL;

	while ((pEntity = (CBasePlayer *)UTIL_FindEntityByClassname(pStartPlayer, "player")))
	{
		if (FNullEnt(pEntity->edict()))
		{
			return NULL;
		}

		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);

		if (pEntity->IsPlayer())
		{
			if (!pEntity->IsDormant() && pPlayer && pPlayer->m_iTeam == m_iTeam)
			{
				return pPlayer;
			}
		}
		else if (pPlayer)
		{
			if (pPlayer->pev->iuser1 <= OBS_CHASE_LOCKED || pPlayer->pev->iuser1 == OBS_IN_EYE)
			{
				if (!FNullEnt(pPlayer->m_hObserverTarget->edict()))
				{
					CBasePlayer *ptarget = (CBasePlayer *)((CBaseEntity *)pPlayer->m_hObserverTarget);

					if (ptarget && ptarget->m_iTeam == m_iTeam)
					{
						return pPlayer;
					}
				}
			}
		}
	}

	return NULL;
}

void CBasePlayer::HandleSignals(void) // Last check: 2013, November 17.
{
	if (g_pGameRules->IsMultiplayer())
	{
		if (!g_pGameRules->m_bMapHasBuyZone)
		{
			if (m_iTeam == TERRORIST || m_iTeam == CT)
			{
				CBaseEntity *pEntity = NULL;
				const char *classname = (m_iTeam == TERRORIST) ? "info_player_deathmatch" : "info_player_start";

				while ((pEntity = UTIL_FindEntityByClassname(pEntity, classname)) != NULL)
				{
					if ((pEntity->pev->origin - pev->origin).Length() < 200)
					{
						m_signals.Signal(SIGNAL_BUY);
					}
				}
			}
		}

		if (!g_pGameRules->m_bMapHasBombZone)
		{
			CBaseEntity *pEntity = NULL;

			while ((pEntity = UTIL_FindEntityByClassname(pEntity, "info_bomb_target")) != NULL)
			{
				if ((pEntity->pev->origin - pev->origin).Length() <= 256)
				{
					m_signals.Signal(SIGNAL_BOMB);
				}
			}
		}

		if (!g_pGameRules->m_bMapHasRescueZone)
		{
			CBaseEntity *pEntity = NULL;

			while ((pEntity = UTIL_FindEntityByClassname(pEntity, "info_hostage_rescue")) != NULL)
			{
				if ((pEntity->pev->origin - pev->origin).Length() <= 256)
				{
					m_signals.Signal(SIGNAL_RESCUE);
				}
			}
		}
	}

	int signalSave    = m_signals.GetSignal();
	int signalChanged = m_signals.GetState() ^ m_signals.GetSignal();

	m_signals.Update();

	if (signalChanged & SIGNAL_BUY)
	{
		if (signalSave & SIGNAL_BUY)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
				WRITE_BYTE(STATUSICON_SHOW);
				WRITE_STRING("buyzone");
				WRITE_BYTE(0);
				WRITE_BYTE(160);
				WRITE_BYTE(0);
			MESSAGE_END();
		}
		else
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
				WRITE_BYTE(STATUSICON_HIDE);
				WRITE_STRING("buyzone");
			MESSAGE_END();

			if (m_iMenu >= Menu_Buy)
			{
				if (m_iMenu <= Menu_BuyItem)
				{
					CLIENT_COMMAND(ENT(pev), "slot10\n");
				}
				else if (m_iMenu == Menu_ClientBuy)
				{
					MESSAGE_BEGIN(MSG_ONE, gmsgBuyClose, NULL, pev);
					MESSAGE_END();
				}
			}
		}
	}

	if (signalChanged & SIGNAL_BOMB)
	{
		if (signalSave & SIGNAL_BOMB)
		{
			if (m_bHasC4 && !(m_flDisplayHistory & Hint_you_are_in_targetzone))
			{
				m_flDisplayHistory |= Hint_you_are_in_targetzone;
				HintMessage("#Hint_you_are_in_targetzone");
			}

			SetBombIcon(TRUE);
		}
		else
		{
			SetBombIcon(FALSE);
		}
	}

	if (signalChanged & SIGNAL_RESCUE)
	{
		if (signalSave & SIGNAL_RESCUE)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
				WRITE_BYTE(STATUSICON_SHOW);
				WRITE_STRING("rescue");
				WRITE_BYTE(0);
				WRITE_BYTE(160);
				WRITE_BYTE(0);
			MESSAGE_END();

			if (m_iTeam == CT && !(m_flDisplayHistory & Hint_hostage_rescue_zone))
			{
				m_flDisplayHistory |= Hint_hostage_rescue_zone;
				HintMessage("#Hint_hostage_rescue_zone");
			}
		}
		else
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
				WRITE_BYTE(STATUSICON_HIDE);
				WRITE_STRING("rescue");
			MESSAGE_END();

			if (m_iMenu >= Menu_Buy)
			{
				if (m_iMenu <= Menu_BuyItem)
				{
					CLIENT_COMMAND(ENT(pev), "slot10\n");
				}
				else if (m_iMenu == Menu_ClientBuy)
				{
					MESSAGE_BEGIN(MSG_ONE, gmsgBuyClose, NULL, pev);
					MESSAGE_END();
				}
			}
		}
	}

	if (signalChanged & SIGNAL_ESCAPE)
	{
		if (signalSave & SIGNAL_ESCAPE)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
				WRITE_BYTE(STATUSICON_SHOW);
				WRITE_STRING("escape");
				WRITE_BYTE(0);
				WRITE_BYTE(160);
				WRITE_BYTE(0);
			MESSAGE_END();

			if (m_iTeam == CT && !(m_flDisplayHistory & Hint_terrorist_escape_zone))
			{
				m_flDisplayHistory |= Hint_terrorist_escape_zone;
				HintMessage("#Hint_terrorist_escape_zone", TRUE);
			}
		}
		else
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
				WRITE_BYTE(STATUSICON_HIDE);
				WRITE_STRING("escape");
			MESSAGE_END();

			if (m_iMenu >= Menu_Buy)
			{
				if (m_iMenu <= Menu_BuyItem)
				{
					CLIENT_COMMAND(ENT(pev), "slot10\n");
				}
				else if (m_iMenu == Menu_ClientBuy)
				{
					MESSAGE_BEGIN(MSG_ONE, gmsgBuyClose, NULL, pev);
					MESSAGE_END();
				}
			}
		}
	}

	if (signalChanged & SIGNAL_VIPSAFETY)
	{
		if (signalSave & SIGNAL_VIPSAFETY)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
				WRITE_BYTE(STATUSICON_SHOW);
				WRITE_STRING("vipsafety");
				WRITE_BYTE(0);
				WRITE_BYTE(160);
				WRITE_BYTE(0);
			MESSAGE_END();

			if (m_iTeam == CT && !(m_flDisplayHistory & Hint_ct_vip_zone))
			{
				m_flDisplayHistory |= Hint_ct_vip_zone;
				HintMessage("#Hint_ct_vip_zone");
			}
			else if (m_iTeam == TERRORIST && !(m_flDisplayHistory & Hint_terrorist_vip_zone))
			{
				m_flDisplayHistory |= Hint_terrorist_vip_zone;
				HintMessage("#Hint_terrorist_vip_zone");
			}
		}
		else
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
				WRITE_BYTE(STATUSICON_HIDE);
				WRITE_STRING("vipsafety");
			MESSAGE_END();

			if (m_iMenu >= Menu_Buy)
			{
				if (m_iMenu <= Menu_BuyItem)
				{
					CLIENT_COMMAND(ENT(pev), "slot10\n");
				}
				else if (m_iMenu == Menu_ClientBuy)
				{
					MESSAGE_BEGIN(MSG_ONE, gmsgBuyClose, NULL, pev);
					MESSAGE_END();
				}
			}
		}
	}
}

bool CBasePlayer::HintMessage(const char *pMessage, BOOL bDisplayIfPlayerDead, BOOL bOverride)  // Last check: 2013, November 17.
{
	if (!bDisplayIfPlayerDead && !IsAlive())
	{
		return false;
	}

	if (bOverride || m_bShowHints)
	{
		return m_hintMessageQueue.AddMessage(pMessage, 6.0, true, NULL);
	}

	return true;
}

void CBasePlayer::HostageUsed(void)  // Last check: 2013, November 17.
{
	if (m_flDisplayHistory & Hint_lead_hostage_to_rescue_point)
	{
		return;
	}

	if (m_iTeam == TERRORIST)
	{
		HintMessage("#Hint_use_hostage_to_stop_him");
	}
	else if (m_iTeam == CT)
	{
		HintMessage("#Hint_lead_hostage_to_rescue_point");
	}

	m_flDisplayHistory |= Hint_lead_hostage_to_rescue_point;
}

BOOL CBasePlayer::IsArmored(int nHitGroup)  // Last check: 2013, November 17.
{
	if (!m_iKevlar)
	{
		return FALSE;
	}

	switch (nHitGroup)
	{
		case HITGROUP_HEAD:
		{
			return m_iKevlar == 2;
		}
		case HITGROUP_GENERIC:
		case HITGROUP_CHEST:
		case HITGROUP_STOMACH:
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CBasePlayer::IsBombGuy(void)  // Last check: 2013, November 17.
{
	return g_pGameRules->IsMultiplayer() && m_bHasC4;
}

bool CBasePlayer::IsHittingShield(const Vector &vecDirection, TraceResult *ptr) // Last check: 2013, November 17.
{
	if (!HasShield())
	{
		return false;
	}

	if (ptr->iHitgroup == HITGROUP_SHIELD)
	{
		return true;
	}

	if (!m_bShieldDrawn)
	{
		return false;
	}

	UTIL_MakeVectors(pev->angles);

	return false;
}

bool CBasePlayer::IsObservingPlayer(CBasePlayer *pTarget) // Last check: 2013, November 17.
{
	if (!pTarget || IsDormant() || FNullEnt(pTarget->edict()))
	{
		return false;
	}

	if (pev->iuser1 == OBS_IN_EYE && pev->iuser2 == pTarget->entindex())
	{
		return true;
	}

	return false;
}

void CBasePlayer::MakeVIP(void) // Last check: 2013, November 18.
{
	pev->body = 0;
	m_iModelName = MODEL_VIP;

	g_engfuncs.pfnSetClientKeyValue(entindex(), g_engfuncs.pfnGetInfoKeyBuffer(edict()), "model", "vip");

	UTIL_LogPrintf("\"%s<%i><%s><CT>\" triggered \"Became_VIP\"\n",	STRING(pev->netname), GETPLAYERUSERID(edict()), GETPLAYERAUTHID(edict()));

	m_iTeam      = CT;
	m_bIsVIP     = true;
	m_bNotKilled = false;

	g_pGameRules->m_pVIP = this;
	g_pGameRules->m_iConsecutiveVIP = 1;
}

void CBasePlayer::JoiningThink(void)  // Last check: 2013, November 17.
{
	switch (m_iJoiningState)
	{
		case SHOWLTEXT:
		{
			RemoveLevelText();

			m_iJoiningState = SHOWTEAMSELECT;

			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
				WRITE_BYTE(0);
				WRITE_STRING("defuser");
			MESSAGE_END();

			m_bHasDefuser      = false;
			m_bMissionBriefing = false;
			m_fLastMovement    = gpGlobals->time;

			MESSAGE_BEGIN(MSG_ONE, gmsgItemStatus, NULL, pev);
				WRITE_BYTE(m_bHasNightVision);
			MESSAGE_END();

			break;
		}
		case READINGLTEXT:
		{
			if (m_afButtonPressed & (IN_ATTACK | IN_ATTACK2 | IN_JUMP))
			{
				m_afButtonPressed &= ~(IN_ATTACK | IN_ATTACK2 | IN_JUMP);

				RemoveLevelText();

				m_iJoiningState = SHOWTEAMSELECT;
			}

			break;
		}
		case GETINTOGAME:
		{
			m_iIgnoreGlobalChat = IGNOREMSG_NONE;
			m_iTeamKills        = 0;
			m_iFOV              = 90;
			m_bNotKilled        = false;

			memset(&m_rebuyStruct, 0, sizeof(RebuyStruct));

			m_bIsInRebuy     = false;
			m_bJustConnected = false;
			m_fLastMovement  = gpGlobals->time;

			ResetMaxSpeed();

			m_iJoiningState = JOINED;

			if (g_pGameRules->m_bMapHasEscapeZone && m_iTeam == CT)
			{
				m_iAccount = 0;

				CheckStartMoney();
				AddAccount((int)startmoney.value, true);
			}

			if (g_pGameRules->FPlayerCanRespawn(this))
			{
				Spawn();
				g_pGameRules->CheckWinConditions();

				if (!g_pGameRules->m_fTeamCount && g_pGameRules->m_bMapHasBombTarget && !g_pGameRules->IsThereABomber() && !g_pGameRules->IsThereABomb())
				{
					g_pGameRules->GiveC4();
				}

				if (m_iTeam == TERRORIST)
				{
					g_pGameRules->m_iNumEscapers++;
				}
			}
			else
			{
				pev->deadflag = DEAD_RESPAWNABLE;

				if (pev->classname)
				{
					RemoveEntityHashValue(pev, STRING(pev->classname), CLASSNAME);
				}

				AddEntityHashValue(pev, STRING(pev->classname), CLASSNAME);

				pev->deadflag   = DEAD_RESPAWNABLE;
				pev->classname  = MAKE_STRING("player");
				pev->flags     &= (FL_PROXY | FL_FAKECLIENT);
				pev->flags     |= FL_CLIENT | FL_SPECTATOR;

				edict_t *pentSpawnSpot = g_pGameRules->GetPlayerSpawnSpot(this);
				StartObserver(pentSpawnSpot->v.origin, pentSpawnSpot->v.angles);

				g_pGameRules->CheckWinConditions();

				MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
					WRITE_BYTE(entindex());
					switch (m_iTeam)
					{
						case TERRORIST: WRITE_STRING("TERRORIST");   break;
						case CT:        WRITE_STRING("CT");          break;
						case SPECTATOR: WRITE_STRING("SPECTATOR");   break;
						default:        WRITE_STRING("UNASSIGNED");  break;
					}
				MESSAGE_END();

				MESSAGE_BEGIN(MSG_ALL, gmsgLocation);
					WRITE_BYTE(entindex());
					WRITE_STRING("");
				MESSAGE_END();

				MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
					WRITE_BYTE(entindex());
					WRITE_SHORT(pev->frags);
					WRITE_SHORT(m_iDeaths);
					WRITE_SHORT(0);
					WRITE_SHORT(m_iTeam);
				MESSAGE_END();

				if (~m_flDisplayHistory & Hint_Spec_Duck)
				{
					m_hintMessageQueue.AddMessage("#Spec_Duck", 6.0, true, NULL);
					m_flDisplayHistory |= Hint_Spec_Duck;
				}
			}

			return;
		}
	}

	if (m_pIntroCamera && m_fIntroCamTime < gpGlobals->time)
	{
		m_pIntroCamera = UTIL_FindEntityByClassname(m_pIntroCamera, "trigger_camera");

		if (!m_pIntroCamera)
		{
			m_pIntroCamera = UTIL_FindEntityByClassname(NULL, "trigger_camera");
		}

		CBaseEntity *pTarget = UTIL_FindEntityByTargetname(NULL, STRING(m_pIntroCamera->pev->target));

		if (pTarget)
		{
			pev->angles = UTIL_VecToAngles(pTarget->pev->origin - m_pIntroCamera->pev->origin).Normalize();
			pev->angles.x = -pev->angles.x;

			UTIL_SetOrigin(pev, m_pIntroCamera->pev->origin);

			pev->v_angle    = pev->angles;
			pev->velocity   = g_vecZero;
			pev->punchangle = g_vecZero;
			pev->view_ofs   = g_vecZero;
			pev->fixangle   = TRUE;

			m_fIntroCamTime = gpGlobals->time + 6.0;
		}
		else
		{
			m_pIntroCamera = NULL;
		}
	}
}

void CBasePlayer::Pain(int m_LastHitGroup, bool HasArmour)   // Last check: 2013, November 18.
{
	int rand = RANDOM_LONG(0, 2);

	switch (m_LastHitGroup)
	{
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
		{
			if (HasArmour)
			{
				EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_kevlar-1.wav", VOL_NORM, ATTN_NORM);
			}

			switch (rand)
			{
				case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_flesh-1.wav", 1, ATTN_NORM); break;
				case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_flesh-2.wav", 1, ATTN_NORM); break;
				case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_flesh-3.wav", 1, ATTN_NORM); break;
			}

			break;
		}
		case HITGROUP_HEAD:
		{
			if (m_iKevlar == 2)
			{
				EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_helmet-1.wav", 1, ATTN_NORM);
			}
			else
			{
				switch (rand)
				{
					case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/headshot1.wav", 1, ATTN_NORM); break;
					case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/headshot2.wav", 1, ATTN_NORM); break;
					case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/headshot3.wav", 1, ATTN_NORM); break;
				}
			}

			break;
		}
		default:
		{
			switch (rand)
			{
				case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_flesh-1.wav", 1, ATTN_NORM); break;
				case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_flesh-2.wav", 1, ATTN_NORM); break;
				case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_flesh-3.wav", 1, ATTN_NORM); break;
			}

			break;
		}
	}
}

// CS
void CBasePlayer::Radio(const char *msg_id, const char *msg_verbose, short pitch, bool showIcon)
{
	if (!IsPlayer() || (pev->deadflag != DEAD_NO && !IsBot()))
	{
		return;
	}

	CBaseEntity *pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
	{
		if (FNullEnt(pEntity->edict()))
		{
			break;
		}

		CBasePlayer *pTarget = GetClassPtr((CBasePlayer*)pEntity->pev);

		if (!pTarget->IsPlayer())
		{
			if (pTarget->pev->iuser1 == OBS_CHASE_LOCKED || pTarget->pev->iuser1 == OBS_CHASE_FREE || pTarget->pev->iuser1 == OBS_IN_EYE)
			{
				if (!FNullEnt(m_hObserverTarget->edict()))
					pTarget = (CBasePlayer*)((CBaseEntity*)m_hObserverTarget);
				else
					pTarget = NULL;
			}
		}
		else if (pTarget->IsDormant())
		{
			pTarget = NULL;
		}

		if (!pTarget || pTarget->m_iTeam != m_iTeam || pTarget->m_bIgnoreRadio)
		{
			continue;
		}

		MESSAGE_BEGIN(MSG_ONE, gmsgSendAudio, NULL, pTarget->edict());
		WRITE_BYTE(entindex());
		WRITE_STRING(msg_id);
		WRITE_SHORT(pitch);
		MESSAGE_END();

		if (msg_verbose)
		{
			// TODO: Implements me.
			// Place index = TheNavAreaGrid->GetPlace( pev->origin );
			// ...

			ClientPrint(pTarget->pev, HUD_PRINTRADIO, NumAsString(entindex()), "#Game_radio", STRING(pev->netname), msg_verbose);
		}

		if (showIcon)
		{
			MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pTarget->edict());
			WRITE_BYTE(TE_PLAYERATTACHMENT);
			WRITE_BYTE(entindex());
			WRITE_COORD(35);
			WRITE_SHORT(g_sModelIndexRadio);
			WRITE_SHORT(15);
			MESSAGE_END();
		}
	}
}

#define MENUBUF_LEN 51

void CBasePlayer::MenuPrint(const char *pszText) // Last check: 2013, November 18.
{
	char szBuffer[MENUBUF_LEN + 1];

	while (strlen(pszText) >= MENUBUF_LEN - 1)
	{
		strncpy(szBuffer, pszText, MENUBUF_LEN - 1);
		szBuffer[MENUBUF_LEN - 1] = '\0';
		pszText += MENUBUF_LEN - 1;

		MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pev);
			WRITE_SHORT(0xFFFF);
			WRITE_CHAR(-1);
			WRITE_BYTE(1);
			WRITE_STRING(szBuffer);
		MESSAGE_END();
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pev);
		WRITE_SHORT(0xFFFF);
		WRITE_CHAR(-1);
		WRITE_BYTE(0);
		WRITE_STRING(pszText);
	MESSAGE_END();
}

void CBasePlayer::RemoveLevelText(void)  // Last check: 2013, November 17.
{
	ResetMenu();
}

// CS
void CBasePlayer::Reset(void)
{
	pev->frags = 0.0;
	m_iDeaths  = 0;
	m_iAccount = 0;

	MESSAGE_BEGIN(MSG_ONE, gmsgMoney, NULL, ENT(pev));
	WRITE_LONG(m_iAccount);
	WRITE_BYTE(0);
	MESSAGE_END();

	m_bNotKilled = false;

	if (HasShield())
	{
		m_bOwnsShield  = false;
		m_bHasPrimary  = false;
		m_bShieldDrawn = false;

		pev->gamestate = 1;

		UpdateShieldCrosshair(true);
	}

	CheckStartMoney();

	int money = m_iAccount + startmoney.value;

	if (money < 0)
	{
		m_iAccount = 0;
	}
	else if (money > 16000)
	{
		m_iAccount = 16000;
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgMoney, NULL, edict());
	WRITE_BYTE(m_iAccount);
	WRITE_BYTE(1);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	WRITE_BYTE(entindex());
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	WRITE_SHORT(m_iTeam);
	MESSAGE_END();
}

// CS
void CBasePlayer::ResetMaxSpeed()
{
	float speed = 900.0;

	if (pev->iuser1)
	{
		speed = 900.0;
	}
	else if (g_pGameRules->IsMultiplayer() && g_pGameRules->IsFreezePeriod())
	{
		speed = 1.0;
	}
	else if (m_bIsVIP)
	{
		speed = 227.0;
	}
	else if (m_pActiveItem == NULL)
	{
		speed = 240.0;
	}
	else
	{
		speed = m_pActiveItem->GetMaxSpeed();
	}
}

void CBasePlayer::ResetMenu(void)  // Last check: 2013, November 17.
{
	MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pev);
		WRITE_SHORT(0);
		WRITE_CHAR(0);
		WRITE_BYTE(0);
		WRITE_STRING(0);
	MESSAGE_END();
}

// CS
void CBasePlayer::RoundRespawn()
{
	m_canSwitchObserverModes = true;

	if (!m_bJustKilledTeammate)
	{
		if (CVAR_GET_FLOAT("mp_tkpunish") != 0.0)
		{
			m_bJustKilledTeammate = false;
			CLIENT_COMMAND(edict(), "kill\n");
			m_bPunishedForTK = true;
		}
	}

	if (m_iMenu != Menu_ChooseAppearance)
	{
		respawn(pev, 0);

		pev->button = 0;
		pev->nextthink = -1.0;
	}

	if (m_pActiveItem)
	{
		if (m_pActiveItem->iItemSlot() == GRENADE_SLOT && m_pActiveItem->CanDeploy())
		{
			SwitchWeapon(m_pActiveItem);
		}
	}

	m_lastLocation[0] = 0;
}

// CS
void CBasePlayer::SendWeatherInfo(void)
{
	int mode = 0;

	if (UTIL_FindEntityByClassname(NULL, "env_rain") != NULL || UTIL_FindEntityByClassname(NULL, "func_rain") != NULL)
	{
		mode = 1;
	}
	else if (UTIL_FindEntityByClassname(NULL, "env_snow") != NULL || UTIL_FindEntityByClassname(NULL, "func_snow") != NULL)
	{
		mode = 2;
	}

	if (mode)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgReceiveW, NULL, ENT(pev));
		WRITE_BYTE(mode);
		MESSAGE_END();
	}
}

// CS
void CBasePlayer::SetBombIcon(BOOL bFlash)
{
	if (m_bHasC4)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, ENT(pev));
		WRITE_BYTE(bFlash + 1);
		WRITE_STRING("c4");
		WRITE_BYTE(0);
		WRITE_BYTE(160);
		WRITE_BYTE(0);
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, ENT(pev));
		WRITE_BYTE(0);
		WRITE_STRING("c4");
		MESSAGE_END();
	}

	SetScoreboardAttributes(NULL);
}

void CBasePlayer::SendFOV(int fov) // Last check: 2013, November 17.
{
	m_iFOV = m_iClientFOV = pev->fov = fov;

	MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, edict());
		WRITE_BYTE(fov);
	MESSAGE_END();
}

// CS
void CBasePlayer::SetNewPlayerModel(const char *modelName)
{
	SET_MODEL(ENT(pev), modelName);
	m_modelIndexPlayer = pev->modelindex;
}

// CS
void CBasePlayer::SetPlayerModel(BOOL HasC4)
{
	char *model = NULL;

	if (m_iTeam == CT)
	{
		switch (m_iModelName)
		{
		case MODEL_URBAN: model = "urban"; break;
		case MODEL_GSG9: model = "gsg9"; break;
		case MODEL_SAS: model = "sas"; break;
		case MODEL_GIGN: model = "gign"; break;
		case MODEL_VIP: model = "vip"; break;

		case MODEL_SPETSNAZ: if (UTIL_IsGame("czero"))
		{
			model = "vip";
			break;
		}
		default: if (!IsBot() /*|| !( model = TheBotProfiles->GetCustomSkinModelname( m_iModelName ) )*/)
		{
			model = "urban";
			break;
		}
		}
	}
	else if (m_iTeam == TERRORIST)
	{
		switch (m_iModelName)
		{
		case MODEL_TERROR: model = "terror"; break;
		case MODEL_LEET: model = "leet"; break;
		case MODEL_ARCTIC: model = "arctic"; break;
		case MODEL_GUERILLA: model = "guerilla"; break;

		case MODEL_MILITIA: if (UTIL_IsGame("czero"))
		{
			model = "militia";
			break;
		}
		default: if (!IsBot() /*|| !( model = TheBotProfiles->GetCustomSkinModelname( m_iModelName ) )*/)
		{
			model = "terror";
			break;
		}
		}
	}
	else
	{
		model = "urban";
	}

	char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer(edict());

	if (strcmp(g_engfuncs.pfnInfoKeyValue(infobuffer, "model"), model))
	{
		g_engfuncs.pfnSetClientKeyValue(entindex(), infobuffer, "model", model);
	}
}

// CS
void CBasePlayer::SetPrefsFromUserinfo(char *infobuffer)
{
	char *buffer = NULL;

	buffer = g_engfuncs.pfnInfoKeyValue(infobuffer, "_cl_autowepswitch");

	if (*buffer)
		m_iAutoWepSwitch = strtol(buffer, NULL, 10);
	else
		m_iAutoWepSwitch = 1;

	buffer = g_engfuncs.pfnInfoKeyValue(infobuffer, "_vgui_menus");

	if (*buffer)
		m_bVGUIMenus = strtol(buffer, NULL, 10) != 0;
	else
		m_bVGUIMenus = true;

	buffer = g_engfuncs.pfnInfoKeyValue(infobuffer, "_ah");

	if (*buffer)
		m_bShowHints = strtol(buffer, NULL, 10) != 0;
	else
		m_bShowHints = true;
}

// CS
void CBasePlayer::SetProgressBarTime(int time)
{
	if (time)
	{
		m_progressStart = gpGlobals->time;
		m_progressEnd = gpGlobals->time + time;
	}
	else
	{
		m_progressStart = 0.0;
		m_progressEnd = 0.0;
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgBarTime, NULL, ENT(pev));
	WRITE_SHORT(time);
	MESSAGE_END();

	CBaseEntity *pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
	{
		CBasePlayer *pWatcher = GetClassPtr((CBasePlayer*)pEntity->pev);

		if (pWatcher->pev->iuser1 == OBS_IN_EYE && pWatcher->pev->iuser2 == entindex())
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgBarTime, NULL, ENT(pWatcher->pev));
			WRITE_SHORT(time);
			MESSAGE_END();
		}
	}
}

void CBasePlayer::SetProgressBarTime2(int time, float timeElapsed)  // Last check: 2013, November 18.
{
	if (time)
	{
		m_progressStart = gpGlobals->time - timeElapsed;
		m_progressEnd   = time + m_progressStart;
	}
	else
	{
		time = 0;
		m_progressStart = 0;
		m_progressEnd   = 0;
	}

	int iTimeElapsed = (time * 100.0 / (m_progressEnd - m_progressStart));

	MESSAGE_BEGIN(MSG_ONE, gmsgBarTime2, NULL, pev);
		WRITE_SHORT(time);
		WRITE_SHORT(iTimeElapsed);
	MESSAGE_END();

	CBaseEntity *pPlayer = NULL;
	int myIndex = entindex();

	while ((pPlayer = UTIL_FindEntityByClassname(pPlayer, "player")) != NULL)
	{
		if (FNullEnt(pPlayer->edict()))
		{
			break;
		}

		CBasePlayer *player = GetClassPtr((CBasePlayer *)pPlayer->pev);

		if (player->pev->iuser1 == OBS_IN_EYE && player->pev->iuser2 == myIndex)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgBarTime, NULL, player->pev);
				WRITE_SHORT(time);
				WRITE_SHORT(iTimeElapsed);
			MESSAGE_END();
		}
	}
}

// CS
void CBasePlayer::SetScoreboardAttributes(CBasePlayer *destination)
{
	if (destination == NULL)
	{
		SetScoreboardAttributes(this);
	}
	else
	{
		int flags = 0;

		if (pev->deadflag)
			flags |= 1;
		else if (m_bHasC4)
			flags |= 2;
		else if (m_bIsVIP)
			flags |= 4;

		if (gmsgScoreAttrib)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgScoreAttrib, NULL, edict());
			WRITE_BYTE(entindex());
			WRITE_BYTE(flags);
			MESSAGE_END();
		}
	}
}

// CS
void CBasePlayer::SwitchTeam(void)
{
	char *model = NULL;
	int oldTeam = m_iTeam;
	const char *name = NULL;

	if (m_iTeam == CT)
	{
		m_iTeam = TERRORIST;

		switch (m_iModelName)
		{
		case MODEL_LEET: m_iModelName = MODEL_URBAN;    model = "urban";    break;
		case MODEL_URBAN: m_iModelName = MODEL_LEET;     model = "leet";     break;
		case MODEL_GSG9: m_iModelName = MODEL_TERROR;   model = "terror";   break;
		case MODEL_GIGN: m_iModelName = MODEL_GUERILLA; model = "guerilla"; break;
		case MODEL_SAS: m_iModelName = MODEL_ARCTIC;   model = "arctic";   break;

		case MODEL_SPETSNAZ: if (UTIL_IsGame("czero"))
		{
			m_iModelName = MODEL_MILITIA; model = "militia";
			break;
		}
		default: if (!IsBot() /*|| !( model = TheBotProfiles->GetCustomSkinModelname( m_iModelName ) )*/)
		{
			m_iModelName = MODEL_LEET; model = "terror";
			break;
		}
		}

		char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer(edict());
		g_engfuncs.pfnSetClientKeyValue(entindex(), infobuffer, "model", model);
	}
	else if (m_iTeam == TERRORIST)
	{
		m_iTeam = CT;

		switch (m_iModelName)
		{
		case MODEL_TERROR: m_iModelName = MODEL_GSG9;  model = "gsg9";  break;
		case MODEL_LEET: m_iModelName = MODEL_URBAN; model = "urban"; break;
		case MODEL_ARCTIC: m_iModelName = MODEL_SAS;   model = "sas";   break;
		case MODEL_GUERILLA: m_iModelName = MODEL_GIGN;  model = "gign";  break;

		case MODEL_MILITIA: if (UTIL_IsGame("czero"))
		{
			m_iModelName = MODEL_SPETSNAZ; model = "spetsnaz";
			break;
		}
		default: if (!IsBot() /*|| !( model = TheBotProfiles->GetCustomSkinModelname( m_iModelName ) )*/)
		{
			m_iModelName = MODEL_URBAN; model = "urban";
			break;
		}
		}

		char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer(edict());
		g_engfuncs.pfnSetClientKeyValue(entindex(), infobuffer, "model", model);
	}

	MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
	WRITE_BYTE(entindex());
	WRITE_STRING(GetTeam(m_iTeam));
	MESSAGE_END();

	// TODO: Implement me.
	// TheBots->OnEvent( EVENT_PLAYER_CHANGED_TEAM, this, NULL );

	UpdateLocation(true);

	if (m_iTeam)
	{
		SetScoreboardAttributes(this);
	}

	name = "<unconnected>";

	if (pev->netname)
	{
		name = STRING(pev->netname);

		if (!*name)
		{
			name = "<unconnected>";
		}
	}

	if (m_iTeam == TERRORIST)
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#Game_join_terrorist_auto", name);
	else
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#Game_join_ct_auto", name);

	if (m_bHasDefuser)
	{
		m_bHasDefuser = false;
		pev->body = 0;

		MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, edict());
		WRITE_BYTE(0);
		WRITE_STRING("defuser");
		MESSAGE_END();

		MESSAGE_BEGIN(MSG_ONE, gmsgItemStatus, NULL, edict());
		WRITE_BYTE((int)m_bHasNightVision | 2 * m_bHasDefuser);
		MESSAGE_END();

		SetProgressBarTime(0);

		for (int i = 0; i < MAX_ITEM_TYPES; i++)
		{
			m_pActiveItem = m_rgpPlayerItems[i];

			if (m_pActiveItem != NULL && FClassnameIs(m_pActiveItem->pev, "item_thighpack"))
			{
				m_pActiveItem->Drop();
				m_rgpPlayerItems[i] = NULL;
			}
		}
	}

	UTIL_LogPrintf("\"%s<%i><%s><%s>\" joined team \"%s\" (auto)\n",
		STRING(pev->netname), GETPLAYERUSERID(edict()), GETPLAYERAUTHID(edict()), GetTeam(oldTeam), GetTeam(m_iTeam));

	if (IsBot())
	{
		// TODO: Implement and check me.
		// if( m_profile && ( m_iTeam == CT && m_profile->IsValidForTeam( 1 ) == 0 ) || ( m_iTeam == TERRORIST && m_profile->IsValidForTeam( 0 ) == 0 ) )
		// {
		//   SERVER_COMMAND( UTIL_VarArgs("kick \"%s\"\n", STRING( pev->netname ) ) );
		// }
	}
}

void CBasePlayer::SpawnClientSideCorpse(void)  // Last check: 2013, November 18.
{
	char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer(edict());
	char *pModel = g_engfuncs.pfnInfoKeyValue(infobuffer, "model");

	MESSAGE_BEGIN(MSG_ALL, gmsgSendCorpse);
		WRITE_STRING(pModel);
		WRITE_LONG(pev->origin.x * 128);
		WRITE_LONG(pev->origin.y * 128);
		WRITE_LONG(pev->origin.z * 128);
		WRITE_COORD(pev->angles.x);
		WRITE_COORD(pev->angles.y);
		WRITE_COORD(pev->angles.z);
		WRITE_LONG((pev->animtime - gpGlobals->time) * 100);
		WRITE_BYTE(pev->sequence);
		WRITE_BYTE(pev->body);
		WRITE_BYTE(m_iTeam);
		WRITE_BYTE(entindex());
	MESSAGE_END();

	m_canSwitchObserverModes = true;

// 	if (TheTutor)
// 	{
// 		TheTutor->OnEvent(EVENT_CLIENT_CORPSE_SPAWNED, this, NULL);
// 	}
}

// CS
void CBasePlayer::SyncRoundTimer(void)
{
	int timeRemaining = 0;

	if (g_pGameRules->IsMultiplayer())
	{
		timeRemaining = (int)g_pGameRules->TimeRemaining();

		if (timeRemaining < 0)
			timeRemaining = 0;
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgRoundTime, NULL, edict());
	WRITE_SHORT(timeRemaining);
	MESSAGE_END();

	if (g_pGameRules->IsMultiplayer())
	{
		// TODO: Implement me
		if (g_pGameRules->IsFreezePeriod() /*&& TheTutor*/ && !pev->iuser1 && IsAlive())
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgBlinkAcct, NULL, edict());
			WRITE_BYTE(30);
			MESSAGE_END();
		}
	}
	else if (g_pGameRules->IsCareer())
	{
		//TODO: Check me, it's incorrect/incomplete.
		/*BOOL active = FALSE;
		int flag = 0;

		if( timeRemaining != 0 )
		{
		timeRemaining = TheCareerTasks->GetFinishedTaskTime() - ( gpGlobals->time - g_pGameRules->m_fRoundCount );
		}
		else if( g_pGameRules->IsFreezePeriod() )
		{
		timeRemaining = -1;
		}
		else if( TheCareerTasks->GetFinishedTaskRound() )
		{
		timeRemaining = -TheCareerTasks->GetFinishedTaskRound();
		}

		if( TheCareerTasks->GetFinishedTaskRound() || TheCareerTasks->GetRoundElapsedTime() >= TheCareerTasks->GetFinishedTaskTime() )
		{
		flag = 3;
		}

		if( !g_pGameRules->IsFreezePeriod() && !TheCareerTasks->GetFinishedTaskRound() )
		{
		active = TheCareerTasks->GetFinishedTaskRound() == 0;
		}

		if( !TheCareerTasks->GetFinishedTaskRound() || TheCareerTasks->m_roundEndMessage == g_pGameRules->m_iTotalRoundsPlayed )
		{
		MESSAGE_BEGIN( MSG_ONE, gmsgTaskTime, NULL, edict() );
		WRITE_SHORT( timeRemaining );   // Time
		WRITE_BYTE(  );                 // Active
		WRITE_BYTE( flag );             // Flag
		MESSAGE_END();
		}*/
	}
}

// CS
BOOL CBasePlayer::SwitchWeapon(CBasePlayerItem *pWeapon)
{
	if (!pWeapon->CanDeploy())
	{
		return FALSE;
	}

	ResetAutoaim();

	if (m_pActiveItem)
	{
		m_pActiveItem->Holster();
	}

	m_pLastItem   = m_pActiveItem;
	m_pActiveItem = pWeapon;

	pWeapon->Deploy();

	if (pWeapon->m_pPlayer)
	{
		pWeapon->m_pPlayer->ResetMaxSpeed();
	}

	if (m_bOwnsShield)
	{
		m_iHideHUD &= ~HIDEHUD_CROSSHAIR2;
	}

	return TRUE;
}

// CS
void CBasePlayer::UpdateLocation(bool bForceUpdate)
{
	if (!bForceUpdate && m_flLastUpdateTime >= gpGlobals->time + 2.0)
	{
		return;
	}

	char *location = NULL;

	if (pev->deadflag == DEAD_NO && UTIL_IsGame("czero"))
	{
		// TODO: Implement and finish me.
		// TheNavAreaGrid->GetPlace( pev->origin );
		// ...
	}

	if (*location && (!m_lastLocation[0] || strcmp(location, &m_lastLocation[1])))
	{
		m_flLastUpdateTime = gpGlobals->time;
		_snprintf(m_lastLocation, sizeof(m_lastLocation), "#%s", location);

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer *pOther = (CBasePlayer*)UTIL_PlayerByIndex(i);

			if (pOther->m_iTeam == m_iTeam || pOther->m_iTeam == SPECTATOR)
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgLocation, NULL, pOther->edict());
				WRITE_BYTE(entindex());
				WRITE_STRING(m_lastLocation);
				MESSAGE_END();
			}
			else if (bForceUpdate)
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgLocation, NULL, pOther->edict());
				WRITE_BYTE(entindex());
				WRITE_STRING("");
				MESSAGE_END();
			}
		}
	}
}

// CS
void CBasePlayer::UpdateShieldCrosshair(bool draw)
{
	if (draw)
		m_iHideHUD &= ~HIDEHUD_CROSSHAIR2;
	else
		m_iHideHUD |= HIDEHUD_CROSSHAIR2;
}


void CBasePlayer::BuildRebuyStruct(void) // Last check: 2013, September 17.
{
	if (m_bIsInRebuy)
	{
		return;
	}

	CBasePlayerWeapon *pPrimary   = (CBasePlayerWeapon *)m_rgpPlayerItems[PRIMARY_WEAPON_SLOT];
	CBasePlayerWeapon *pSecondary = (CBasePlayerWeapon *)m_rgpPlayerItems[PISTOL_SLOT];

	if (pPrimary != NULL)
	{
		m_rebuyStruct.m_primaryWeapon = pPrimary->m_iId;
		m_rebuyStruct.m_primaryAmmo   = m_rgAmmo[pPrimary->m_iPrimaryAmmoType];
	}
	else
	{
		m_rebuyStruct.m_primaryAmmo   = 0;
		m_rebuyStruct.m_primaryWeapon = HasShield() ? WEAPON_SHIELDGUN : 0;
	}

	if (pSecondary != NULL)
	{
		m_rebuyStruct.m_secondaryWeapon = pSecondary->m_iId;
		m_rebuyStruct.m_secondaryAmmo   = m_rgAmmo[pSecondary->m_iPrimaryAmmoType];
	}
	else
	{
		m_rebuyStruct.m_secondaryWeapon = 0;
		m_rebuyStruct.m_secondaryAmmo   = 0;
	}

	int heGrenade    = GetAmmoIndex("HEGrenade");
	int flashbang    = GetAmmoIndex("Flashbang");
	int smokeGrenade = GetAmmoIndex("SmokeGrenade");

	m_rebuyStruct.m_heGrenade    = heGrenade != -1 ? m_rgAmmo[heGrenade] : 0;
	m_rebuyStruct.m_flashbang    = flashbang != -1 ? m_rgAmmo[flashbang] : 0;
	m_rebuyStruct.m_smokeGrenade = smokeGrenade != -1 ? m_rgAmmo[smokeGrenade] : 0;

	m_rebuyStruct.m_defuser     = m_bHasDefuser;
	m_rebuyStruct.m_nightVision = m_bHasNightVision;
	m_rebuyStruct.m_armor       = m_iKevlar;
}



/*
 *
 */
Vector VecVelocityForDamage(float flDamage)
{
	Vector vec(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));

	if (flDamage > -50)
		vec = vec * 0.7;
	else if (flDamage > -200)
		vec = vec * 2;
	else
		vec = vec * 10;

	return vec;
}

#if 0 /*
static void ThrowGib(entvars_t *pev, char *szGibModel, float flDamage)
{
edict_t *pentNew = CREATE_ENTITY();
entvars_t *pevNew = VARS(pentNew);

pevNew->origin = pev->origin;
SET_MODEL(ENT(pevNew), szGibModel);
UTIL_SetSize(pevNew, g_vecZero, g_vecZero);

pevNew->velocity        = VecVelocityForDamage(flDamage);
pevNew->movetype        = MOVETYPE_BOUNCE;
pevNew->solid           = SOLID_NOT;
pevNew->avelocity.x     = RANDOM_FLOAT(0,600);
pevNew->avelocity.y     = RANDOM_FLOAT(0,600);
pevNew->avelocity.z     = RANDOM_FLOAT(0,600);
CHANGE_METHOD(ENT(pevNew), em_think, SUB_Remove);
pevNew->ltime       = gpGlobals->time;
pevNew->nextthink   = gpGlobals->time + RANDOM_FLOAT(10,20);
pevNew->frame       = 0;
pevNew->flags       = 0;
}

static void ThrowHead(entvars_t *pev, char *szGibModel, floatflDamage)
{
SET_MODEL(ENT(pev), szGibModel);
pev->frame          = 0;
pev->nextthink      = -1;
pev->movetype       = MOVETYPE_BOUNCE;
pev->takedamage     = DAMAGE_NO;
pev->solid          = SOLID_NOT;
pev->view_ofs       = Vector(0,0,8);
UTIL_SetSize(pev, Vector(-16,-16,0), Vector(16,16,56));
pev->velocity       = VecVelocityForDamage(flDamage);
pev->avelocity      = RANDOM_FLOAT(-1,1) * Vector(0,600,0);
pev->origin.z -= 24;
ClearBits(pev->flags, FL_ONGROUND);
}

*/
#endif

int TrainSpeed(int iSpeed, int iMax)
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax = (float)iMax;
	fSpeed = iSpeed;

	fSpeed = fSpeed / fMax;

	if (iSpeed < 0)
		iRet = TRAIN_BACK;
	else if (iSpeed == 0)
		iRet = TRAIN_NEUTRAL;
	else if (fSpeed < 0.33)
		iRet = TRAIN_SLOW;
	else if (fSpeed < 0.66)
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

void CBasePlayer::DeathSound(void) // Last check: 2013, November 17.
{
	switch (RANDOM_LONG(1, 4))
	{
		case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/die1.wav", VOL_NORM, ATTN_NORM); break;
		case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/die2.wav", VOL_NORM, ATTN_NORM); break;
		case 3: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/die3.wav", VOL_NORM, ATTN_NORM); break;
		case 4: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/death6.wav", VOL_NORM, ATTN_NORM); break;
	}
}

// override takehealth
// bitsDamageType indicates type of damage healed.

int CBasePlayer::TakeHealth(float flHealth, int bitsDamageType)
{
	return CBaseMonster::TakeHealth(flHealth, bitsDamageType);
}

Vector CBasePlayer::GetGunPosition(void) // Last check: 2013, November 17.
{
	//  UTIL_MakeVectors(pev->v_angle);
	//  m_HackedGunPos = pev->view_ofs;
	Vector origin;

	origin = pev->origin + pev->view_ofs;

	return origin;
}

//=========================================================
// TraceAttack
//=========================================================
void CBasePlayer::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (pev->takedamage)
	{
		m_LastHitGroup = ptr->iHitgroup;

		switch (ptr->iHitgroup)
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			flDamage *= gSkillData.plrHead;
			break;
		case HITGROUP_CHEST:
			flDamage *= gSkillData.plrChest;
			break;
		case HITGROUP_STOMACH:
			flDamage *= gSkillData.plrStomach;
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			flDamage *= gSkillData.plrArm;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			flDamage *= gSkillData.plrLeg;
			break;
		default:
			break;
		}

		SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage);// a little surface blood.
		TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
		AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);
	}
}

/*
	Take some damage.
	NOTE: each call to TakeDamage with bitsDamageType set to a time-based damage
	type will cause the damage time countdown to be reset.  Thus the ongoing effects of poison, radiation
	etc are implemented with subsequent calls to TakeDamage using DMG_GENERIC.
	*/

#define ARMOR_RATIO  0.2    // Armor Takes 80% of the damage
#define ARMOR_BONUS  0.5    // Each Point of Armor is work 1/x points of health

int CBasePlayer::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	// have suit diagnose the problem - ie: report damage type
	int bitsDamage = bitsDamageType;
	int ffound = TRUE;
	int fmajor;
	int fcritical;
	int fTookDamage;
	int ftrivial;
	float flRatio;
	float flBonus;
	float flHealthPrev = pev->health;

	flBonus = ARMOR_BONUS;
	flRatio = ARMOR_RATIO;

	if ((bitsDamageType & DMG_BLAST) && g_pGameRules->IsMultiplayer())
	{
		// blasts damage armor more.
		flBonus *= 2;
	}

	// Already dead
	if (!IsAlive())
		return 0;
	// go take the damage first

	CBaseEntity *pAttacker = CBaseEntity::Instance(pevAttacker);

	if (!g_pGameRules->FPlayerCanTakeDamage(this, pAttacker))
	{
		// Refuse the damage
		return 0;
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = flDamage;

	// Armor.
	if (pev->armorvalue && !(bitsDamageType & (DMG_FALL | DMG_DROWN)))// armor doesn't protect against fall or drown damage!
	{
		float flNew = flDamage * flRatio;

		float flArmor;

		flArmor = (flDamage - flNew) * flBonus;

		// Does this use more armor than we have?
		if (flArmor > pev->armorvalue)
		{
			flArmor = pev->armorvalue;
			flArmor *= (1 / flBonus);
			flNew = flDamage - flArmor;
			pev->armorvalue = 0;
		}
		else
			pev->armorvalue -= flArmor;

		flDamage = flNew;
	}

	// this cast to INT is critical!!! If a player ends up with 0.5 health, the engine will get that
	// as an int (zero) and think the player is dead! (this will incite a clientside screentilt, etc)
	fTookDamage = CBaseMonster::TakeDamage(pevInflictor, pevAttacker, (int)flDamage, bitsDamageType);

	// reset damage time countdown for each type of time based damage player just sustained

	{
		for (int i = 0; i < CDMG_TIMEBASED; i++)
			if (bitsDamageType & (DMG_PARALYZE << i))
				m_rgbTimeBasedDamage[i] = 0;
	}

	// tell director about it
	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);   // command length in bytes
	WRITE_BYTE(DRC_CMD_EVENT);   // take damage event
	WRITE_SHORT(ENTINDEX(this->edict())); // index number of primary entity
	WRITE_SHORT(ENTINDEX(ENT(pevInflictor))); // index number of secondary entity
	WRITE_LONG(5);   // eventflags (priority and flags)
	MESSAGE_END();

	// how bad is it, doc?

	ftrivial = (pev->health > 75 || m_lastDamageAmount < 5);
	fmajor = (m_lastDamageAmount > 25);
	fcritical = (pev->health < 30);

	// handle all bits set in this damage message,
	// let the suit give player the diagnosis

	// UNDONE: add sounds for types of damage sustained (ie: burn, shock, slash )

	// UNDONE: still need to record damage and heal messages for the following types

	// DMG_BURN
	// DMG_FREEZE
	// DMG_BLAST
	// DMG_SHOCK

	m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;  // make sure the damage bits get resent

	while (fTookDamage && (!ftrivial || (bitsDamage & DMG_TIMEBASED)) && ffound && bitsDamage)
	{
		ffound = FALSE;

		if (bitsDamage & DMG_CLUB)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC);  // minor fracture
			bitsDamage &= ~DMG_CLUB;
			ffound = TRUE;
		}
		if (bitsDamage & (DMG_FALL | DMG_CRUSH))
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG5", FALSE, SUIT_NEXT_IN_30SEC);  // major fracture
			else
				SetSuitUpdate("!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC);  // minor fracture

			bitsDamage &= ~(DMG_FALL | DMG_CRUSH);
			ffound = TRUE;
		}

		if (bitsDamage & DMG_BULLET)
		{
			if (m_lastDamageAmount > 5)
				SetSuitUpdate("!HEV_DMG6", FALSE, SUIT_NEXT_IN_30SEC);  // blood loss detected
			//else
			//  SetSuitUpdate("!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC);  // minor laceration

			bitsDamage &= ~DMG_BULLET;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_SLASH)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG1", FALSE, SUIT_NEXT_IN_30SEC);  // major laceration
			else
				SetSuitUpdate("!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC);  // minor laceration

			bitsDamage &= ~DMG_SLASH;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_SONIC)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG2", FALSE, SUIT_NEXT_IN_1MIN);   // internal bleeding
			bitsDamage &= ~DMG_SONIC;
			ffound = TRUE;
		}

		if (bitsDamage & (DMG_POISON | DMG_PARALYZE))
		{
			SetSuitUpdate("!HEV_DMG3", FALSE, SUIT_NEXT_IN_1MIN);   // blood toxins detected
			bitsDamage &= ~(DMG_POISON | DMG_PARALYZE);
			ffound = TRUE;
		}

		if (bitsDamage & DMG_ACID)
		{
			SetSuitUpdate("!HEV_DET1", FALSE, SUIT_NEXT_IN_1MIN);   // hazardous chemicals detected
			bitsDamage &= ~DMG_ACID;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_NERVEGAS)
		{
			SetSuitUpdate("!HEV_DET0", FALSE, SUIT_NEXT_IN_1MIN);   // biohazard detected
			bitsDamage &= ~DMG_NERVEGAS;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_RADIATION)
		{
			SetSuitUpdate("!HEV_DET2", FALSE, SUIT_NEXT_IN_1MIN);   // radiation detected
			bitsDamage &= ~DMG_RADIATION;
			ffound = TRUE;
		}
		if (bitsDamage & DMG_SHOCK)
		{
			bitsDamage &= ~DMG_SHOCK;
			ffound = TRUE;
		}
	}

	pev->punchangle.x = -2;

	if (fTookDamage && !ftrivial && fmajor && flHealthPrev >= 75)
	{
		// first time we take major damage...
		// turn automedic on if not on
		SetSuitUpdate("!HEV_MED1", FALSE, SUIT_NEXT_IN_30MIN);  // automedic on

		// give morphine shot if not given recently
		SetSuitUpdate("!HEV_HEAL7", FALSE, SUIT_NEXT_IN_30MIN); // morphine shot
	}

	if (fTookDamage && !ftrivial && fcritical && flHealthPrev < 75)
	{
		// already took major damage, now it's critical...
		if (pev->health < 6)
			SetSuitUpdate("!HEV_HLTH3", FALSE, SUIT_NEXT_IN_10MIN); // near death
		else if (pev->health < 20)
			SetSuitUpdate("!HEV_HLTH2", FALSE, SUIT_NEXT_IN_10MIN); // health critical

		// give critical health warnings
		if (!RANDOM_LONG(0, 3) && flHealthPrev < 50)
			SetSuitUpdate("!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN); //seek medical attention
	}

	// if we're taking time based damage, warn about its continuing effects
	if (fTookDamage && (bitsDamageType & DMG_TIMEBASED) && flHealthPrev < 75)
	{
		if (flHealthPrev < 50)
		{
			if (!RANDOM_LONG(0, 3))
				SetSuitUpdate("!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN); //seek medical attention
		}
		else
			SetSuitUpdate("!HEV_HLTH1", FALSE, SUIT_NEXT_IN_10MIN); // health dropping
	}

	return fTookDamage;
}

//=========================================================
// PackDeadPlayerItems - call this when a player dies to
// pack up the appropriate weapons and ammo items, and to
// destroy anything that shouldn't be packed.
//
// This is pretty brute force :(
//=========================================================
void CBasePlayer::PackDeadPlayerItems(void)   // Last check: 2013, November 18.
{
	int iWeaponRules = g_pGameRules->DeadPlayerWeapons(this);
	int iAmmoRules   = g_pGameRules->DeadPlayerAmmo(this);

	if (iWeaponRules != GR_PLR_DROP_GUN_NO)
	{
		bool bHasShield = false;

		if (HasShield())
		{
			if (!m_pActiveItem || m_pActiveItem->CanHolster())
			{
				DropShield(true);
				bHasShield = true;
			}
		}

		int iBestWeight = 0;
		CBasePlayerItem *pItem = NULL;

		for (size_t i = 0; i < MAX_ITEM_TYPES; ++i)
		{
			CBasePlayerItem *pPlayerItem = m_rgpPlayerItems[i];

			while (pPlayerItem)
			{
				ItemInfo II;

				if (pPlayerItem->iItemSlot() < KNIFE_SLOT && !bHasShield)
				{
					if (pPlayerItem->GetItemInfo(&II))
					{
						if (II.iWeight > iBestWeight)
						{
							iBestWeight = II.iWeight;
							pItem = pPlayerItem;
						}
					}
				}
				else if (pPlayerItem->iItemSlot() == GRENADE_SLOT && UTIL_IsGame("czero"))
				{
					packPlayerItem(this, pItem, true);
				}

				pPlayerItem = pPlayerItem->m_pNext;
			}
		}

		packPlayerItem(this, pItem, iAmmoRules != GR_PLR_DROP_AMMO_NO);
	}

	RemoveAllItems(TRUE);
}

void CBasePlayer::RemoveAllItems(BOOL removeSuit)
{
	if (m_pActiveItem)
	{
		ResetAutoaim();
		m_pActiveItem->Holster();
		m_pActiveItem = NULL;
	}

	m_pLastItem = NULL;

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	int i;
	CBasePlayerItem *pPendingItem;
	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		m_pActiveItem = m_rgpPlayerItems[i];
		while (m_pActiveItem)
		{
			pPendingItem = m_pActiveItem->m_pNext;
			m_pActiveItem->Drop();
			m_pActiveItem = pPendingItem;
		}
		m_rgpPlayerItems[i] = NULL;
	}
	m_pActiveItem = NULL;

	pev->viewmodel      = 0;
	pev->weaponmodel    = 0;

	if (removeSuit)
		pev->weapons = 0;
	else
		pev->weapons &= ~WEAPON_ALLWEAPONS;

	for (i = 0; i < MAX_AMMO_SLOTS; i++)
		m_rgAmmo[i] = 0;

	UpdateClientData();
	// send Selected Weapon Message to our client
	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pev);
	WRITE_BYTE(0);
	WRITE_BYTE(0);
	WRITE_BYTE(0);
	MESSAGE_END();
}

/*
 * GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
 *
 * ENTITY_METHOD(PlayerDie)
 */
entvars_t *g_pevLastInflictor;  // Set in combat.cpp.  Used to pass the damage inflictor for death messages.
// Better solution:  Add as parameter to all Killed() functions.

void CBasePlayer::Killed(entvars_t *pevAttacker, int iGib)
{
	m_canSwitchObserverModes = false;

	if (m_LastHitGroup == HITGROUP_HEAD)
	{
		m_bHeadshotKilled = true;
	}

	CBaseEntity *pAttacker = CBaseEntity::Instance(pevAttacker);

	// TODO: Implement me.
	// TheBots->OnEvent(EVENT_PLAYER_DIED, this, pAttacker);

	if (g_pGameRules->IsCareer())
	{
		if (!IsBot())
		{
			TheCareerTasks->HandleEvent(EVENT_DIE, NULL, this);
			TheCareerTasks->HandleDeath(m_iTeam, NULL);
		}

		if (!m_bKilledByBomb)
		{
			// ...
		}
	}

	if (!m_bKilledByBomb)
	{
		g_pGameRules->PlayerKilled(this, pevAttacker, g_pevLastInflictor);
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgNVGToggle, NULL, pev);
		WRITE_BYTE(0);
	MESSAGE_END();

	m_bNightVisionOn = false;

	for (int i = 1; i < gpGlobals->maxClients; ++i)
	{
		CBasePlayer *pOther = (CBasePlayer *)UTIL_PlayerByIndex(i);

		if (pOther && pOther->IsObservingPlayer(this))
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgNVGToggle, NULL, pOther->pev);
				WRITE_BYTE(0);
			MESSAGE_END();

			pOther->m_bNightVisionOn = false;
		}
	}

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));
	
	if (pSound)
	{
		pSound->Reset();
	}
	
	if (pev->modelindex && (m_flFlinchTime < gpGlobals->time || pev->health <= 0))
	{
		SetAnimation(PLAYER_DIE);
	}

	if (m_pActiveItem && m_pActiveItem->m_pPlayer)
	{
		switch (m_pActiveItem->m_iId)
		{
			case WEAPON_HEGRENADE:
			{
				if ((pev->button & IN_ATTACK) && m_rgAmmo[((CBasePlayerWeapon *)m_pActiveItem)->m_iPrimaryAmmoType])
				{
					CGrenade::ShootTimed2(pev, GetGunPosition(), pev->angles, 1.5, m_iTeam, ((CHEGrenade *)m_pActiveItem)->m_usCreateExplosion);
				}
				break;
			}
			case WEAPON_FLASHBANG:
			{
				if (pev->button & IN_ATTACK && m_rgAmmo[((CBasePlayerWeapon *)m_pActiveItem)->m_iPrimaryAmmoType])
				{
					CGrenade::ShootTimed(pev, GetGunPosition(), pev->angles, 1.5);
				}
				break;
			}
			case WEAPON_SMOKEGRENADE:
			{
				if ((pev->button & IN_ATTACK) && m_rgAmmo[((CBasePlayerWeapon *)m_pActiveItem)->m_iPrimaryAmmoType])
				{
					CGrenade::ShootSmokeGrenade(pev, GetGunPosition(), pev->angles, 1.5, ((CSmokeGrenade *)m_pActiveItem)->m_usCreateSmoke);
				}
				break;
			}
		}
	}

	pev->modelindex = m_modelIndexPlayer;
	pev->deadflag   = DEAD_DYING;
	pev->movetype   = MOVETYPE_TOSS;
	pev->takedamage = DAMAGE_NO;

	pev->gamestate = 1;
	m_bShieldDrawn = false;

	pev->flags &= ~FL_ONGROUND;

	if (!fadetoblack.value)
	{
		pev->iuser1 = OBS_CHASE_FREE;
		pev->iuser2 = ENTINDEX(edict());
		pev->iuser3 = ENTINDEX(ENT(pevAttacker));

		m_hObserverTarget = UTIL_PlayerByIndex(pev->iuser3);

		MESSAGE_BEGIN(MSG_ONE, gmsgADStop, NULL, pev);
		MESSAGE_END();
	}
	else
	{
		UTIL_ScreenFade(this, Vector(0, 0, 0), 3, 3, 255, FFADE_OUT | FFADE_STAYOUT);
	}

	SetScoreboardAttributes();

	if (m_iThrowDirection)
	{
		switch (m_iThrowDirection)
		{
			case THROW_FORWARD:
			{
				UTIL_MakeVectors(pev->angles);

				pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT(100, 200);
				pev->velocity.z = RANDOM_FLOAT(50, 100);

				break;
			}
			case THROW_BACKWARD:
			{
				UTIL_MakeVectors(pev->angles);

				pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT(-100, -200);
				pev->velocity.z = RANDOM_FLOAT(50, 100);

				break;
			}
			case THROW_HITVEL:
			{
				if (FClassnameIs(pevAttacker, "player"))
				{
					UTIL_MakeVectors(pevAttacker->angles);

					pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT(200, 300);
					pev->velocity.z = RANDOM_FLOAT(200, 300);
				}

				break;
			}
			case THROW_BOMB:
			{
				pev->velocity = m_vBlastVector * (1 / m_vBlastVector.Length()) * (2300 - m_vBlastVector.Length()) * 0.25;
				pev->velocity.z = (2300 - m_vBlastVector.Length()) / 2.75;

				break;
			}
			case THROW_GRENADE:
			{
				pev->velocity = m_vBlastVector * (1 / m_vBlastVector.Length()) * (500 - m_vBlastVector.Length());
				pev->velocity.z = (350 - m_vBlastVector.Length()) * 1.5;

				break;
			}
			case THROW_HITVEL_MINUS_AIRVEL:
			{
				if (FClassnameIs(pevAttacker, "player"))
				{
					UTIL_MakeVectors(pevAttacker->angles);
					pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT(200, 300);
				}

				break;
			}
		}

		m_iThrowDirection = THROW_NONE;
	}

	SetSuitUpdate(NULL, FALSE, 0);

	m_iClientHealth = 0;

	MESSAGE_BEGIN(MSG_ONE, gmsgHealth, NULL, pev);
		WRITE_BYTE(m_iClientHealth);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pev);
		WRITE_BYTE(0);
		WRITE_BYTE(0xFF);
		WRITE_BYTE(0xFF);
	MESSAGE_END();

	SendFOV(0);

	g_pGameRules->CheckWinConditions();
	m_bNotKilled = false;

	if (m_bHasC4)
	{
		DropPlayerItem("weapon_c4");
		SetProgressBarTime(0);
	}
	else if (m_bHasDefuser)
	{
		m_bHasDefuser = false;
		pev->body = 0;

		GiveNamedItem("item_thighpack");

		MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
			WRITE_BYTE(STATUSICON_HIDE);
			WRITE_STRING("defuser");
		MESSAGE_END();

		SendItemStatus(this);
		SetProgressBarTime(0);
	}

	if (m_bIsDefusing)
	{
		SetProgressBarTime(0);
	}

	m_bIsDefusing = false;

	MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
		WRITE_BYTE(STATUSICON_HIDE);
		WRITE_STRING("buyzone");
	MESSAGE_END();

	if (m_iMenu >= Menu_Buy)
	{
		if (m_iMenu <= Menu_BuyItem)
		{
			CLIENT_COMMAND(ENT(pev), "slot10\n");
		}
		else if (m_iMenu == Menu_ClientBuy)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgBuyClose, NULL, pev);
			MESSAGE_END();
		}
	}

	SetThink(&CBasePlayer::PlayerDeathThink);
	pev->nextthink = gpGlobals->time + 0.1;
	pev->solid = SOLID_NOT;

	if (m_bPunishedForTK)
	{
		m_bPunishedForTK = false;
		HintMessage("#Hint_cannot_play_because_tk");
	}

	if ((pev->health < -9000 && iGib != GIB_NEVER) || iGib == GIB_ALWAYS)
	{
		pev->solid = SOLID_NOT;
		GibMonster();
		pev->effects |= EF_NODRAW;

		g_pGameRules->CheckWinConditions();

		return;
	}

	DeathSound();

	pev->angles.x = 0;
	pev->angles.z = 0;

	if (!(m_flDisplayHistory & Hint_Spec_Duck))
	{
		HintMessage("#Spec_Duck", TRUE, TRUE);
		m_flDisplayHistory |= Hint_Spec_Duck;
	}
}

// Set the activity based on an event or current state
void CBasePlayer::SetAnimation(PLAYER_ANIM playerAnim)
{
	int animDesired;
	float speed;
	char szAnim[64];

	speed = pev->velocity.Length2D();

	if (pev->flags & FL_FROZEN)
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	switch (playerAnim)
	{
	case PLAYER_JUMP:
		m_IdealActivity = ACT_HOP;
		break;

	case PLAYER_SUPERJUMP:
		m_IdealActivity = ACT_LEAP;
		break;

	case PLAYER_DIE:
		m_IdealActivity = ACT_DIESIMPLE;
		m_IdealActivity = GetDeathActivity();
		break;

	case PLAYER_ATTACK1:
		switch (m_Activity)
		{
		case ACT_HOVER:
		case ACT_SWIM:
		case ACT_HOP:
		case ACT_LEAP:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_RANGE_ATTACK1;
			break;
		}
		break;
	case PLAYER_IDLE:
	case PLAYER_WALK:
		if (!FBitSet(pev->flags, FL_ONGROUND) && (m_Activity == ACT_HOP || m_Activity == ACT_LEAP)) // Still jumping
		{
			m_IdealActivity = m_Activity;
		}
		else if (pev->waterlevel > 1)
		{
			if (speed == 0)
				m_IdealActivity = ACT_HOVER;
			else
				m_IdealActivity = ACT_SWIM;
		}
		else
		{
			m_IdealActivity = ACT_WALK;
		}
		break;
	}

	switch (m_IdealActivity)
	{
	case ACT_HOVER:
	case ACT_LEAP:
	case ACT_SWIM:
	case ACT_HOP:
	case ACT_DIESIMPLE:
	default:
		if (m_Activity == m_IdealActivity)
			return;
		m_Activity = m_IdealActivity;

		animDesired = LookupActivity(m_Activity);
		// Already using the desired animation?
		if (pev->sequence == animDesired)
			return;

		pev->gaitsequence = 0;
		pev->sequence       = animDesired;
		pev->frame          = 0;
		ResetSequenceInfo();
		return;

	case ACT_RANGE_ATTACK1:
		if (FBitSet(pev->flags, FL_DUCKING))    // crouching
			strcpy(szAnim, "crouch_shoot_");
		else
			strcpy(szAnim, "ref_shoot_");
		strcat(szAnim, m_szAnimExtention);
		animDesired = LookupSequence(szAnim);
		if (animDesired == -1)
			animDesired = 0;

		if (pev->sequence != animDesired || !m_fSequenceLoops)
		{
			pev->frame = 0;
		}

		if (!m_fSequenceLoops)
		{
			pev->effects |= EF_NOINTERP;
		}

		m_Activity = m_IdealActivity;

		pev->sequence       = animDesired;
		ResetSequenceInfo();
		break;

	case ACT_WALK:
		if (m_Activity != ACT_RANGE_ATTACK1 || m_fSequenceFinished)
		{
			if (FBitSet(pev->flags, FL_DUCKING))    // crouching
				strcpy(szAnim, "crouch_aim_");
			else
				strcpy(szAnim, "ref_aim_");
			strcat(szAnim, m_szAnimExtention);
			animDesired = LookupSequence(szAnim);
			if (animDesired == -1)
				animDesired = 0;
			m_Activity = ACT_WALK;
		}
		else
		{
			animDesired = pev->sequence;
		}
	}

	if (FBitSet(pev->flags, FL_DUCKING))
	{
		if (speed == 0)
		{
			pev->gaitsequence   = LookupActivity(ACT_CROUCHIDLE);
			// pev->gaitsequence    = LookupActivity( ACT_CROUCH );
		}
		else
		{
			pev->gaitsequence   = LookupActivity(ACT_CROUCH);
		}
	}
	else if (speed > 220)
	{
		pev->gaitsequence   = LookupActivity(ACT_RUN);
	}
	else if (speed > 0)
	{
		pev->gaitsequence   = LookupActivity(ACT_WALK);
	}
	else
	{
		// pev->gaitsequence    = LookupActivity( ACT_WALK );
		pev->gaitsequence   = LookupSequence("deep_idle");
	}

	// Already using the desired animation?
	if (pev->sequence == animDesired)
		return;

	//ALERT( at_console, "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	pev->sequence       = animDesired;
	pev->frame          = 0;
	ResetSequenceInfo();
}

/*
===========
TabulateAmmo
This function is used to find and store
all the ammo we have into the ammo vars.
============
*/
void CBasePlayer::TabulateAmmo()
{
	/*ammo_9mm = AmmoInventory( GetAmmoIndex( "9mm" ) );
	ammo_357 = AmmoInventory( GetAmmoIndex( "357" ) );
	ammo_argrens = AmmoInventory( GetAmmoIndex( "ARgrenades" ) );
	ammo_bolts = AmmoInventory( GetAmmoIndex( "bolts" ) );
	ammo_buckshot = AmmoInventory( GetAmmoIndex( "buckshot" ) );
	ammo_rockets = AmmoInventory( GetAmmoIndex( "rockets" ) );
	ammo_uranium = AmmoInventory( GetAmmoIndex( "uranium" ) );
	ammo_hornets = AmmoInventory( GetAmmoIndex( "Hornets" ) );*/
}

/*
===========
WaterMove
============
*/
#define AIRTIME 12      // lung full of air lasts this many seconds

void CBasePlayer::WaterMove()
{
	int air;

	if (pev->movetype == MOVETYPE_NOCLIP)
		return;

	if (pev->health < 0)
		return;

	// waterlevel 0 - not in water
	// waterlevel 1 - feet in water
	// waterlevel 2 - waist in water
	// waterlevel 3 - head in water

	if (pev->waterlevel != 3)
	{
		// not underwater

		// play 'up for air' sound
		if (pev->air_finished < gpGlobals->time)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade1.wav", 1, ATTN_NORM);
		else if (pev->air_finished < gpGlobals->time + 9)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade2.wav", 1, ATTN_NORM);

		pev->air_finished = gpGlobals->time + AIRTIME;
		pev->dmg = 2;

		// if we took drowning damage, give it back slowly
		if (m_idrowndmg > m_idrownrestored)
		{
			// set drowning damage bit.  hack - dmg_drownrecover actually
			// makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.

			// NOTE: this actually causes the count to continue restarting
			// until all drowning damage is healed.

			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}
	}
	else
	{   // fully under water
		// stop restoring damage while underwater
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if (pev->air_finished < gpGlobals->time)        // drown!
		{
			if (pev->pain_finished < gpGlobals->time)
			{
				// take drowning damage
				pev->dmg += 1;
				if (pev->dmg > 5)
					pev->dmg = 5;
				TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), pev->dmg, DMG_DROWN);
				pev->pain_finished = gpGlobals->time + 1;

				// track drowning damage, give it back when
				// player finally takes a breath

				m_idrowndmg += pev->dmg;
			}
		}
		else
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
	}

	if (!pev->waterlevel)
	{
		if (FBitSet(pev->flags, FL_INWATER))
		{
			ClearBits(pev->flags, FL_INWATER);
		}
		return;
	}

	// make bubbles

	air = (int)(pev->air_finished - gpGlobals->time);
	if (!RANDOM_LONG(0, 0x1f) && RANDOM_LONG(0, AIRTIME - 1) >= air)
	{
		switch (RANDOM_LONG(0, 3))
		{
		case 0: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim1.wav", 0.8, ATTN_NORM); break;
		case 1: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim2.wav", 0.8, ATTN_NORM); break;
		case 2: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim3.wav", 0.8, ATTN_NORM); break;
		case 3: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim4.wav", 0.8, ATTN_NORM); break;
		}
	}

	if (pev->watertype == CONTENT_LAVA)     // do damage
	{
		if (pev->dmgtime < gpGlobals->time)
			TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 10 * pev->waterlevel, DMG_BURN);
	}
	else if (pev->watertype == CONTENT_SLIME)       // do damage
	{
		pev->dmgtime = gpGlobals->time + 1;
		TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 4 * pev->waterlevel, DMG_ACID);
	}

	if (!FBitSet(pev->flags, FL_INWATER))
	{
		SetBits(pev->flags, FL_INWATER);
		pev->dmgtime = 0;
	}
}

// TRUE if the player is attached to a ladder
BOOL CBasePlayer::IsOnLadder(void)  // Last check: 2013, November 17.
{
	return pev->movetype == MOVETYPE_FLY;
}

void CBasePlayer::PlayerDeathThink(void)  // Last check: 2013, November 18.
{
	if (m_iJoiningState != JOINED)
	{
		return;
	}

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		float flForward = pev->velocity.Length() - 20;

		pev->velocity = flForward ? g_vecZero : flForward * pev->velocity.Normalize();
	}

	if (HasWeapons())
	{
		// we drop the guns here because weapons that have an area effect and can kill their user
		// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
		// player class sometimes is freed. It's safer to manipulate the weapons once we know
		// we aren't calling into any of their code anymore through the player pointer.
		PackDeadPlayerItems();
	}

	if (pev->modelindex && !m_fSequenceFinished && pev->deadflag == DEAD_DYING)
	{
		StudioFrameAdvance();
		return;
	}

	// once we're done animating our death and we're on the ground, we want to set movetype to None so our dead body won't do collisions and stuff anymore
	// this prevents a bug where the dead body would go to a player's head if he walked over it while the dead player was clicking their button to respawn
	if (pev->movetype != MOVETYPE_NONE && FBitSet(pev->flags, FL_ONGROUND))
	{
		pev->movetype = MOVETYPE_NONE;
	}

	if (pev->deadflag == DEAD_DYING)
	{
		m_fDeadTime = gpGlobals->time;
		pev->deadflag = DEAD_DEAD;
	}

	StopAnimation();
	pev->effects |= EF_NOINTERP;

	BOOL fAnyButtonDown = (pev->button & ~IN_SCORE);

	if (g_pGameRules->IsMultiplayer() && gpGlobals->time > m_fDeadTime + 3 && !(m_afPhysicsFlags & PFLAG_OBSERVER))
	{
		SpawnClientSideCorpse();
		StartDeathCam();
	}

	if (pev->deadflag == DEAD_DEAD && m_iTeam != UNASSIGNED && m_iTeam != SPECTATOR)
	{
		if (fAnyButtonDown)
		{
			return;
		}

		if (g_pGameRules->FPlayerCanRespawn(this))
		{
			pev->deadflag = DEAD_RESPAWNABLE;

			if (g_pGameRules->IsMultiplayer())
			{
				g_pGameRules->CheckWinConditions();
			}
		}

		pev->nextthink = gpGlobals->time + 0.1;
	}
	else if (pev->deadflag == DEAD_RESPAWNABLE)
	{
		respawn(pev, FALSE);

		pev->button    = 0;
		pev->nextthink = -1;
	}
}

//=========================================================
// StartDeathCam - find an intermission spot and send the
// player off into observer mode
//=========================================================
void CBasePlayer::StartDeathCam(void)
{
	edict_t *pSpot, *pNewSpot;
	int iRand;

	if (pev->view_ofs == g_vecZero)
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}

	pSpot = FIND_ENTITY_BY_CLASSNAME(NULL, "info_intermission");

	if (!FNullEnt(pSpot))
	{
		// at least one intermission spot in the world.
		iRand = RANDOM_LONG(0, 3);

		while (iRand > 0)
		{
			pNewSpot = FIND_ENTITY_BY_CLASSNAME(pSpot, "info_intermission");

			if (pNewSpot)
			{
				pSpot = pNewSpot;
			}

			iRand--;
		}

		CopyToBodyQue(pev);
		StartObserver(pSpot->v.origin, pSpot->v.v_angle);
	}
	else
	{
		// no intermission spot. Push them up in the air, looking down at their corpse
		TraceResult tr;
		CopyToBodyQue(pev);
		UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, 128), ignore_monsters, edict(), &tr);
		StartObserver(tr.vecEndPos, UTIL_VecToAngles(tr.vecEndPos - pev->origin));
		return;
	}
}

void CBasePlayer::StartObserver(Vector vecPosition, Vector vecViewAngle)
{
	// clear any clientside entities attached to this player
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_KILLPLAYERATTACHMENTS);
	WRITE_BYTE((BYTE)entindex());
	MESSAGE_END();

	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveItem)
		m_pActiveItem->Holster();

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, FALSE, 0);

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pev);
	WRITE_BYTE(0);
	WRITE_BYTE(0XFF);
	WRITE_BYTE(0xFF);
	MESSAGE_END();

	// reset FOV
	m_iFOV = m_iClientFOV = 0;
	pev->fov = m_iFOV;
	MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, pev);
	WRITE_BYTE(0);
	MESSAGE_END();

	// Setup flags
	m_iHideHUD = (HIDEHUD_HEALTH | HIDEHUD_WEAPONS);
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->effects = EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	ClearBits(m_afPhysicsFlags, PFLAG_DUCKING);
	ClearBits(pev->flags, FL_DUCKING);
	pev->deadflag = DEAD_RESPAWNABLE;
	pev->health = 1;

	// Clear out the status bar
	m_fInitHUD = TRUE;

	pev->team =  0;
	MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_STRING("");
	MESSAGE_END();

	// Remove all the player's stuff
	RemoveAllItems(FALSE);

	// Move them to the new position
	UTIL_SetOrigin(pev, vecPosition);

	// Find a player to watch
	m_flNextObserverInput = 0;
	Observer_SetMode(m_iObserverLastMode);
}

//
// PlayerUse - handles USE keypress
//
#define PLAYER_SEARCH_RADIUS    (float)64

void CBasePlayer::PlayerUse(void)
{
	//if ( IsObserver() )
	//	return;

	// Was use pressed or released?
	if (!((pev->button | m_afButtonPressed | m_afButtonReleased) & IN_USE))
		return;

	// Hit Use on a train?
	if (m_afButtonPressed & IN_USE)
	{
		if (m_pTank != NULL)
		{
			// Stop controlling the tank
			// TODO: Send HUD Update
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
			return;
		}
		else
		{
			if (m_afPhysicsFlags & PFLAG_ONTRAIN)
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				return;
			}
			else
			{   // Start controlling the train!
				CBaseEntity *pTrain = CBaseEntity::Instance(pev->groundentity);

				if (pTrain && !(pev->button & IN_JUMP) && FBitSet(pev->flags, FL_ONGROUND) && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) && pTrain->OnControls(pev))
				{
					m_afPhysicsFlags |= PFLAG_ONTRAIN;
					m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
					m_iTrain |= TRAIN_NEW;
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM);
					return;
				}
			}
		}
	}

	CBaseEntity *pObject = NULL;
	CBaseEntity *pClosest = NULL;
	Vector      vecLOS;
	float flMaxDot = VIEW_FIELD_NARROW;
	float flDot;

	UTIL_MakeVectors(pev->v_angle);// so we know which way we are facing

	while ((pObject = UTIL_FindEntityInSphere(pObject, pev->origin, PLAYER_SEARCH_RADIUS)) != NULL)
	{
		if (pObject->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE))
		{
			// !!!PERFORMANCE- should this check be done on a per case basis AFTER we've determined that
			// this object is actually usable? This dot is being done for every object within PLAYER_SEARCH_RADIUS
			// when player hits the use key. How many objects can be in that area, anyway? (sjb)
			vecLOS = (VecBModelOrigin(pObject->pev) - (pev->origin + pev->view_ofs));

			// This essentially moves the origin of the target to the corner nearest the player to test to see
			// if it's "hull" is in the view cone
			vecLOS = UTIL_ClampVectorToBox(vecLOS, pObject->pev->size * 0.5);

			flDot = DotProduct(vecLOS, gpGlobals->v_forward);
			if (flDot > flMaxDot)
			{// only if the item is in front of the user
				pClosest = pObject;
				flMaxDot = flDot;
				//              ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
			}
			//          ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
		}
	}
	pObject = pClosest;

	// Found an object
	if (pObject)
	{
		//!!!UNDONE: traceline here to prevent USEing buttons through walls
		int caps = pObject->ObjectCaps();

		if (m_afButtonPressed & IN_USE)
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_NORM);

		if (((pev->button & IN_USE) && (caps & FCAP_CONTINUOUS_USE)) ||
			((m_afButtonPressed & IN_USE) && (caps & (FCAP_IMPULSE_USE | FCAP_ONOFF_USE))))
		{
			if (caps & FCAP_CONTINUOUS_USE)
				m_afPhysicsFlags |= PFLAG_USING;

			pObject->Use(this, this, USE_SET, 1);
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ((m_afButtonReleased & IN_USE) && (pObject->ObjectCaps() & FCAP_ONOFF_USE))   // BUGBUG This is an "off" use
		{
			pObject->Use(this, this, USE_SET, 0);
		}
	}
	else
	{
		if (m_afButtonPressed & IN_USE)
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_NORM);
	}
}

void CBasePlayer::Jump(void)  // Last check: 2013, November 17.
{
	if (pev->flags & FL_WATERJUMP || pev->waterlevel >= 2)
	{
		return;
	}

	if (!(m_afButtonPressed & IN_JUMP))
	{
		return;
	}

	if (!(pev->flags & FL_ONGROUND) || !pev->groundentity)
	{
		return;
	}

	UTIL_MakeVectors(pev->angles);
	
	if (pev->modelindex && (m_flFlinchTime < gpGlobals->time || pev->health < 0))
	{
		SetAnimation(PLAYER_JUMP);
	}

	if ((pev->button & IN_DUCK) || (m_afPhysicsFlags & PFLAG_DUCKING))
	{
		if (m_fLongJump && (pev->button & IN_DUCK) && (gpGlobals->time - m_flDuckTime < 1) && pev->velocity.Length() > 50)
		{
			if (pev->modelindex && (m_flFlinchTime < gpGlobals->time || pev->health < 0))
			{
				SetAnimation(PLAYER_SUPERJUMP);
			}
		}
	}

	entvars_t *pevGround = VARS(pev->groundentity);

	if (pevGround)
	{
		if (pevGround->flags & FL_CONVEYOR)
		{
			pev->velocity = pev->velocity + pev->basevelocity;
		}

		if (FClassnameIs(pevGround, "func_tracktrain") || FClassnameIs(pevGround, "func_train") || FClassnameIs(pevGround, "func_vehicle"))
		{
			pev->velocity = pevGround->velocity + pev->velocity;
		}
	}
}

// This is a glorious hack to find free space when you've crouched into some solid space
// Our crouching collisions do not work correctly for some reason and this is easier
// than fixing the problem :(
void FixPlayerCrouchStuck(edict_t *pPlayer)
{
	TraceResult trace;

	// Move up as many as 18 pixels if the player is stuck.
	for (int i = 0; i < 18; i++)
	{
		UTIL_TraceHull(pPlayer->v.origin, pPlayer->v.origin, dont_ignore_monsters, head_hull, pPlayer, &trace);
		if (trace.fStartSolid)
			pPlayer->v.origin.z++;
		else
			break;
	}
}

void CBasePlayer::Duck(void)  // Last check: 2013, November 17.
{
	if (pev->button & IN_DUCK && pev->modelindex && (m_flFlinchTime < gpGlobals->time || pev->health <= 0))
	{
		SetAnimation(PLAYER_WALK);
	}
}

//
// ID's player as such.
//
int CBasePlayer::Classify(void) // Last check: 2013, November 17.
{
	return CLASS_PLAYER;
}

//Player ID
void CBasePlayer::InitStatusBar() // Last check: 2013, November 17.
{
	m_flStatusBarDisappearDelay = 0;
	m_SbarString0[0] = '\0';
}

void CBasePlayer::UpdateStatusBar()
{
	int newSBarState[SBAR_END];
	char sbuf0[SBAR_STRING_SIZE];
	char sbuf1[SBAR_STRING_SIZE];

	memset(newSBarState, 0, sizeof(newSBarState));
	strcpy(sbuf0, m_SbarString0);
	//strcpy( sbuf1, m_SbarString1 );

	// Find an ID Target
	TraceResult tr;
	UTIL_MakeVectors(pev->v_angle + pev->punchangle);
	Vector vecSrc = EyePosition();
	Vector vecEnd = vecSrc + (gpGlobals->v_forward * MAX_ID_RANGE);
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr);

	if (tr.flFraction != 1.0)
	{
		if (!FNullEnt(tr.pHit))
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if (pEntity->Classify() == CLASS_PLAYER)
			{
				newSBarState[SBAR_ID_TARGETNAME] = ENTINDEX(pEntity->edict());
				strcpy(sbuf1, "1 %p1\n2 Health: %i2%%\n3 Armor: %i3%%");

				// allies and medics get to see the targets health
				if (g_pGameRules->PlayerRelationship(this, pEntity) == GR_TEAMMATE)
				{
					newSBarState[SBAR_ID_TARGETHEALTH] = 100 * (pEntity->pev->health / pEntity->pev->max_health);
					newSBarState[SBAR_ID_TARGETARMOR] = pEntity->pev->armorvalue; //No need to get it % based since 100 it's the max.
				}

				m_flStatusBarDisappearDelay = gpGlobals->time + 1.0;
			}
		}
		else if (m_flStatusBarDisappearDelay > gpGlobals->time)
		{
			// hold the values for a short amount of time after viewing the object
			newSBarState[SBAR_ID_TARGETNAME] = m_izSBarState[SBAR_ID_TARGETNAME];
			newSBarState[SBAR_ID_TARGETHEALTH] = m_izSBarState[SBAR_ID_TARGETHEALTH];
			newSBarState[SBAR_ID_TARGETARMOR] = m_izSBarState[SBAR_ID_TARGETARMOR];
		}
	}

	BOOL bForceResend = FALSE;

	if (strcmp(sbuf0, m_SbarString0))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusText, NULL, pev);
		WRITE_BYTE(0);
		WRITE_STRING(sbuf0);
		MESSAGE_END();

		strcpy(m_SbarString0, sbuf0);

		// make sure everything's resent
		bForceResend = TRUE;
	}

	/*if ( strcmp( sbuf1, m_SbarString1 ) )
	{
	MESSAGE_BEGIN( MSG_ONE, gmsgStatusText, NULL, pev );
	WRITE_BYTE( 1 );
	WRITE_STRING( sbuf1 );
	MESSAGE_END();

	strcpy( m_SbarString1, sbuf1 );

	// make sure everything's resent
	bForceResend = TRUE;
	}*/

	// Check values and send if they don't match
	for (int i = 1; i < SBAR_END; i++)
	{
		if (newSBarState[i] != m_izSBarState[i] || bForceResend)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusValue, NULL, pev);
			WRITE_BYTE(i);
			WRITE_SHORT(newSBarState[i]);
			MESSAGE_END();

			m_izSBarState[i] = newSBarState[i];
		}
	}
}

#define CLIMB_SHAKE_FREQUENCY   22  // how many frames in between screen shakes when climbing
#define MAX_CLIMB_SPEED         200 // fastest vertical climbing speed possible
#define CLIMB_SPEED_DEC         15  // climbing deceleration rate
#define CLIMB_PUNCH_X           -7  // how far to 'punch' client X axis when climbing
#define CLIMB_PUNCH_Z           7   // how far to 'punch' client Z axis when climbing

void CBasePlayer::PreThink(void)
{
	int buttonsChanged = (m_afButtonLast ^ pev->button);    // These buttons have changed this frame

	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	m_afButtonPressed =  buttonsChanged & pev->button;      // The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & (~pev->button);   // The ones not down are "released"

	g_pGameRules->PlayerThink(this);

	if (g_fGameOver)
		return;         // intermission or finale

	UTIL_MakeVectors(pev->v_angle);             // is this still used?

	ItemPreFrame();
	WaterMove();

	if (g_pGameRules && g_pGameRules->FAllowFlashlight())
		m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_iHideHUD |= HIDEHUD_FLASHLIGHT;

	// JOHN: checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();

	CheckTimeBasedDamage();

	CheckSuitUpdate();

	// Observer Button Handling
	//if ( IsObserver() )
	//{
	Observer_HandleButtons();
	Observer_CheckTarget();
	Observer_CheckProperties();
	pev->impulse = 0;
	return;
	//}

	if (pev->deadflag >= DEAD_DYING)
	{
		PlayerDeathThink();
		return;
	}

	// So the correct flags get sent to client asap.
	//
	if (m_afPhysicsFlags & PFLAG_ONTRAIN)
		pev->flags |= FL_ONTRAIN;
	else
		pev->flags &= ~FL_ONTRAIN;

	// Train speed control
	if (m_afPhysicsFlags & PFLAG_ONTRAIN)
	{
		CBaseEntity *pTrain = CBaseEntity::Instance(pev->groundentity);
		float vel;

		if (!pTrain)
		{
			TraceResult trainTrace;
			// Maybe this is on the other side of a level transition
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -38), ignore_monsters, ENT(pev), &trainTrace);

			// HACKHACK - Just look for the func_tracktrain classname
			if (trainTrace.flFraction != 1.0 && trainTrace.pHit)
				pTrain = CBaseEntity::Instance(trainTrace.pHit);

			if (!pTrain || !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) || !pTrain->OnControls(pev))
			{
				//ALERT( at_error, "In train mode with no train!\n" );
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				return;
			}
		}
		else if (!FBitSet(pev->flags, FL_ONGROUND) || FBitSet(pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL) || (pev->button & (IN_MOVELEFT | IN_MOVERIGHT)))
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = TRAIN_NEW | TRAIN_OFF;
			return;
		}

		pev->velocity = g_vecZero;
		vel = 0;
		if (m_afButtonPressed & IN_FORWARD)
		{
			vel = 1;
			pTrain->Use(this, this, USE_SET, (float)vel);
		}
		else if (m_afButtonPressed & IN_BACK)
		{
			vel = -1;
			pTrain->Use(this, this, USE_SET, (float)vel);
		}

		if (vel)
		{
			m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
			m_iTrain |= TRAIN_ACTIVE | TRAIN_NEW;
		}
	}
	else if (m_iTrain & TRAIN_ACTIVE)
		m_iTrain = TRAIN_NEW; // turn off train

	if (pev->button & IN_JUMP)
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}

	// If trying to duck, already ducked, or in the process of ducking
	if ((pev->button & IN_DUCK) || FBitSet(pev->flags, FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING))
		Duck();

	if (!FBitSet(pev->flags, FL_ONGROUND))
	{
		m_flFallVelocity = -pev->velocity.z;
	}

	// StudioFrameAdvance( );//!!!HACKHACK!!! Can't be hit by traceline when not animating?

	// Clear out ladder pointer
	m_hEnemy = NULL;

	if (m_afPhysicsFlags & PFLAG_ONBARNACLE)
	{
		pev->velocity = g_vecZero;
	}
}
/* Time based Damage works as follows:
	1) There are several types of timebased damage:

	#define DMG_PARALYZE        (1 << 14)   // slows affected creature down
	#define DMG_NERVEGAS        (1 << 15)   // nerve toxins, very bad
	#define DMG_POISON          (1 << 16)   // blood poisioning
	#define DMG_RADIATION       (1 << 17)   // radiation exposure
	#define DMG_DROWNRECOVER    (1 << 18)   // drown recovery
	#define DMG_ACID            (1 << 19)   // toxic chemicals or acid burns
	#define DMG_SLOWBURN        (1 << 20)   // in an oven
	#define DMG_SLOWFREEZE      (1 << 21)   // in a subzero freezer

	2) A new hit inflicting tbd restarts the tbd counter - each monster has an 8bit counter,
	per damage type. The counter is decremented every second, so the maximum time
	an effect will last is 255/60 = 4.25 minutes.  Of course, staying within the radius
	of a damaging effect like fire, nervegas, radiation will continually reset the counter to max.

	3) Every second that a tbd counter is running, the player takes damage.  The damage
	is determined by the type of tdb.
	Paralyze        - 1/2 movement rate, 30 second duration.
	Nervegas        - 5 points per second, 16 second duration = 80 points max dose.
	Poison          - 2 points per second, 25 second duration = 50 points max dose.
	Radiation       - 1 point per second, 50 second duration = 50 points max dose.
	Drown           - 5 points per second, 2 second duration.
	Acid/Chemical   - 5 points per second, 10 second duration = 50 points max.
	Burn            - 10 points per second, 2 second duration.
	Freeze          - 3 points per second, 10 second duration = 30 points max.

	4) Certain actions or countermeasures counteract the damaging effects of tbds:

	Armor/Heater/Cooler - Chemical(acid),burn, freeze all do damage to armor power, then to body
	- recharged by suit recharger
	Air In Lungs        - drowning damage is done to air in lungs first, then to body
	- recharged by poking head out of water
	- 10 seconds if swiming fast
	Air In SCUBA        - drowning damage is done to air in tanks first, then to body
	- 2 minutes in tanks. Need new tank once empty.
	Radiation Syringe   - Each syringe full provides protection vs one radiation dosage
	Antitoxin Syringe   - Each syringe full provides protection vs one poisoning (nervegas or poison).
	Health kit          - Immediate stop to acid/chemical, fire or freeze damage.
	Radiation Shower    - Immediate stop to radiation damage, acid/chemical or fire damage.

	*/

// If player is taking time based damage, continue doing damage to player -
// this simulates the effect of being poisoned, gassed, dosed with radiation etc -
// anything that continues to do damage even after the initial contact stops.
// Update all time based damage counters, and shut off any that are done.

// The m_bitsDamageType bit MUST be set if any damage is to be taken.
// This routine will detect the initial on value of the m_bitsDamageType
// and init the appropriate counter.  Only processes damage every second.

//#define PARALYZE_DURATION 30      // number of 2 second intervals to take damage
//#define PARALYZE_DAMAGE       0.0     // damage to take each 2 second interval

//#define NERVEGAS_DURATION 16
//#define NERVEGAS_DAMAGE       5.0

//#define POISON_DURATION       25
//#define POISON_DAMAGE     2.0

//#define RADIATION_DURATION    50
//#define RADIATION_DAMAGE  1.0

//#define ACID_DURATION     10
//#define ACID_DAMAGE           5.0

//#define SLOWBURN_DURATION 2
//#define SLOWBURN_DAMAGE       1.0

//#define SLOWFREEZE_DURATION   1.0
//#define SLOWFREEZE_DAMAGE 3.0

/* */

void CBasePlayer::CheckTimeBasedDamage(void) // Last check: 2013, November 17.
{
	BYTE bDuration = 0;

	if (!(m_bitsDamageType & DMG_TIMEBASED))
	{
		return;
	}

	// only check for time based damage approx. every 2 seconds
	if (abs(gpGlobals->time - m_tbdPrev) < 2.0)
	{
		return;
	}

	m_tbdPrev = gpGlobals->time;

	for (size_t i = 0; i < CDMG_TIMEBASED; ++i)
	{
		// make sure bit is set for damage type
		if (m_bitsDamageType & (DMG_PARALYZE << i))
		{
			switch (i)
			{
				case itbd_Paralyze:
				{
					// UNDONE - flag movement as half-speed
					bDuration = PARALYZE_DURATION;
					break;
					}
				case itbd_NerveGas:
				{
					// TakeDamage(pev, pev, NERVEGAS_DAMAGE, DMG_GENERIC);
					bDuration = NERVEGAS_DURATION;
					break;
				}
				case itbd_Poison:
				{
					TakeDamage(pev, pev, POISON_DAMAGE, DMG_GENERIC);
					bDuration = POISON_DURATION;
					break;
				}
				case itbd_Radiation:
				{
					// TakeDamage(pev, pev, RADIATION_DAMAGE, DMG_GENERIC);
					bDuration = RADIATION_DURATION;
					break;
				}
				case itbd_DrownRecover:
				{
					// NOTE: this hack is actually used to RESTORE health
					// after the player has been drowning and finally takes a breath
					if (m_idrowndmg > m_idrownrestored)
					{
						int idif = min(m_idrowndmg - m_idrownrestored, 10);

						TakeHealth(idif, DMG_GENERIC);
						m_idrownrestored += idif;
					}
					bDuration = 4;  // get up to 5*10 = 50 points back
					break;
				}
				case itbd_Acid:
				{
					// TakeDamage(pev, pev, ACID_DAMAGE, DMG_GENERIC);
					bDuration = ACID_DURATION;
					break;
				}
				case itbd_SlowBurn:
				{
					// TakeDamage(pev, pev, SLOWBURN_DAMAGE, DMG_GENERIC);
					bDuration = SLOWBURN_DURATION;
					break;
				}
				case itbd_SlowFreeze:
				{
					// TakeDamage(pev, pev, SLOWFREEZE_DAMAGE, DMG_GENERIC);
					bDuration = SLOWFREEZE_DURATION;
					break;
				}
				default:
				{
					bDuration = 0;
				}
			}

			if (m_rgbTimeBasedDamage[i])
			{
				// use up an antitoxin on poison or nervegas after a few seconds of damage
				if (((i == itbd_NerveGas) && (m_rgbTimeBasedDamage[i] < NERVEGAS_DURATION)) ||
					((i == itbd_Poison)   && (m_rgbTimeBasedDamage[i] < POISON_DURATION)))
				{
					if (m_rgItems[ITEM_ANTIDOTE])
					{
						m_rgbTimeBasedDamage[i] = 0;
						m_rgItems[ITEM_ANTIDOTE]--;

						SetSuitUpdate("!HEV_HEAL4", FALSE, SUIT_REPEAT_OK);
					}
				}

				// decrement damage duration, detect when done.
				if (!m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					// if we're done, clear damage bits
					m_bitsDamageType &= ~(DMG_PARALYZE << i);
				}
			}
			else
			{
				// first time taking this damage type - init damage duration
				m_rgbTimeBasedDamage[i] = bDuration;
			}
		}
	}
}

/*
THE POWER SUIT

The Suit provides 3 main functions: Protection, Notification and Augmentation.
Some functions are automatic, some require power.
The player gets the suit shortly after getting off the train in C1A0 and it stays
with him for the entire game.

Protection

Heat/Cold
When the player enters a hot/cold area, the heating/cooling indicator on the suit
will come on and the battery will drain while the player stays in the area.
After the battery is dead, the player starts to take damage.
This feature is built into the suit and is automatically engaged.
Radiation Syringe
This will cause the player to be immune from the effects of radiation for N seconds. Single use item.
Anti-Toxin Syringe
This will cure the player from being poisoned. Single use item.
Health
Small (1st aid kits, food, etc.)
Large (boxes on walls)
Armor
The armor works using energy to create a protective field that deflects a
percentage of damage projectile and explosive attacks. After the armor has been deployed,
it will attempt to recharge itself to full capacity with the energy reserves from the battery.
It takes the armor N seconds to fully charge.

Notification (via the HUD)

x   Health
x   Ammo
x   Automatic Health Care
Notifies the player when automatic healing has been engaged.
x   Geiger counter
Classic Geiger counter sound and status bar at top of HUD
alerts player to dangerous levels of radiation. This is not visible when radiation levels are normal.
x   Poison
Armor
Displays the current level of armor.

Augmentation

Reanimation (w/adrenaline)
Causes the player to come back to life after he has been dead for 3 seconds.
Will not work if player was gibbed. Single use.
Long Jump
Used by hitting the ??? key(s). Caused the player to further than normal.
SCUBA
Used automatically after picked up and after player enters the water.
Works for N seconds. Single use.

Things powered by the battery

Armor
Uses N watts for every M units of damage.
Heat/Cool
Uses N watts for every second in hot/cold area.
Long Jump
Uses N watts for every jump.
Alien Cloak
Uses N watts for each use. Each use lasts M seconds.
Alien Shield
Augments armor. Reduces Armor drain by one half

*/

// if in range of radiation source, ping geiger counter

#define GEIGERDELAY 0.25

void CBasePlayer::UpdateGeigerCounter(void)
{
	BYTE range;

	// delay per update ie: don't flood net with these msgs
	if (gpGlobals->time < m_flgeigerDelay)
		return;

	m_flgeigerDelay = gpGlobals->time + GEIGERDELAY;

	// send range to radition source to client

	range = (BYTE)(m_flgeigerRange / 4);

	if (range != m_igeigerRangePrev)
	{
		m_igeigerRangePrev = range;

		MESSAGE_BEGIN(MSG_ONE, gmsgGeigerRange, NULL, pev);
		WRITE_BYTE(range);
		MESSAGE_END();
	}

	// reset counter and semaphore
	if (!RANDOM_LONG(0, 3))
		m_flgeigerRange = 1000;
}

/*
================
CheckSuitUpdate

Play suit update if it's time
================
*/

#define SUITUPDATETIME  3.5
#define SUITFIRSTUPDATETIME 0.1

void CBasePlayer::CheckSuitUpdate(void) // Last check: 2013, November 17.
{
	int i;
	int isentence = 0;
	int isearch = m_iSuitPlayNext;

	// Ignore suit updates if no suit
	if (!(pev->weapons & (1 << WEAPON_SUIT)))
	{
		return;
	}

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();

	if (g_pGameRules->IsMultiplayer())
	{
		// don't bother updating HEV voice in multiplayer.
		return;
	}

	if (gpGlobals->time >= m_flSuitUpdate && m_flSuitUpdate > 0)
	{
		// play a sentence off of the end of the queue
		for (i = 0; i < CSUITPLAYLIST; i++)
		{
			if ((isentence = m_rgSuitPlayList[isearch]))
			{
				break;
			}

			if (++isearch == CSUITPLAYLIST)
			{
				isearch = 0;
			}
		}

		if (isentence)
		{
			m_rgSuitPlayList[isearch] = 0;

			if (isentence > 0)
			{
				// play sentence number
				char sentence[CBSENTENCENAME_MAX + 1];

				strcpy(sentence, "!");
				strcat(sentence, gszallsentencenames[isentence]);

				EMIT_SOUND_SUIT(ENT(pev), sentence);
			}
			else
			{
				// play sentence group
				EMIT_GROUPID_SUIT(ENT(pev), -isentence);
			}
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
		}
		else
		{
			// queue is empty, don't check
			m_flSuitUpdate = 0;
		}
	}
}

// add sentence to suit playlist queue. if fgroup is true, then
// name is a sentence group (HEV_AA), otherwise name is a specific
// sentence name ie: !HEV_AA0.  If iNoRepeat is specified in
// seconds, then we won't repeat playback of this word or sentence
// for at least that number of seconds.

void CBasePlayer::SetSuitUpdate(char *name, int fgroup, int iNoRepeatTime) // Last check: 2013, November 17.
{
	int i;
	int isentence;
	int iempty = -1;

	// Ignore suit updates if no suit
	if (!(pev->weapons & (1 << WEAPON_SUIT)))
		return;

	if (g_pGameRules->IsMultiplayer())
	{
		// due to static channel design, etc. We don't play HEV sounds in multiplayer right now.
		return;
	}

	// if name == NULL, then clear out the queue

	if (!name)
	{
		for (i = 0; i < CSUITPLAYLIST; i++)
			m_rgSuitPlayList[i] = 0;
		return;
	}
	// get sentence or group number
	if (!fgroup)
	{
		isentence = SENTENCEG_Lookup(name, NULL);
		if (isentence < 0)
			return;
	}
	else
		// mark group number as negative
		isentence = -SENTENCEG_GetIndex(name);

	// check norepeat list - this list lets us cancel
	// the playback of words or sentences that have already
	// been played within a certain time.

	for (i = 0; i < CSUITNOREPEAT; i++)
	{
		if (isentence == m_rgiSuitNoRepeat[i])
		{
			// this sentence or group is already in
			// the norepeat list

			if (m_rgflSuitNoRepeatTime[i] < gpGlobals->time)
			{
				// norepeat time has expired, clear it out
				m_rgiSuitNoRepeat[i] = 0;
				m_rgflSuitNoRepeatTime[i] = 0.0;
				iempty = i;
				break;
			}
			else
			{
				// don't play, still marked as norepeat
				return;
			}
		}
		// keep track of empty slot
		if (!m_rgiSuitNoRepeat[i])
			iempty = i;
	}

	// sentence is not in norepeat list, save if norepeat time was given

	if (iNoRepeatTime)
	{
		if (iempty < 0)
			iempty = RANDOM_LONG(0, CSUITNOREPEAT - 1); // pick random slot to take over
		m_rgiSuitNoRepeat[iempty] = isentence;
		m_rgflSuitNoRepeatTime[iempty] = iNoRepeatTime + gpGlobals->time;
	}

	// find empty spot in queue, or overwrite last spot

	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;
	if (m_iSuitPlayNext == CSUITPLAYLIST)
		m_iSuitPlayNext = 0;

	if (m_flSuitUpdate <= gpGlobals->time)
	{
		if (m_flSuitUpdate == 0)
			// play queue is empty, don't delay too long before playback
			m_flSuitUpdate = gpGlobals->time + SUITFIRSTUPDATETIME;
		else
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
	}
}

//=========================================================
// UpdatePlayerSound - updates the position of the player's
// reserved sound slot in the sound list.
//=========================================================
void CBasePlayer::UpdatePlayerSound(void)
{
	int iBodyVolume;
	int iVolume;
	CSound *pSound;

	pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));

	if (!pSound)
	{
		ALERT(at_console, "Client lost reserved sound!\n");
		return;
	}

	pSound->m_iType = bits_SOUND_NONE;

	// now calculate the best target volume for the sound. If the player's weapon
	// is louder than his body/movement, use the weapon volume, else, use the body volume.

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		iBodyVolume = pev->velocity.Length();

		// clamp the noise that can be made by the body, in case a push trigger,
		// weapon recoil, or anything shoves the player abnormally fast.
		if (iBodyVolume > 512)
		{
			iBodyVolume = 512;
		}
	}
	else
	{
		iBodyVolume = 0;
	}

	if (pev->button & IN_JUMP)
	{
		iBodyVolume += 100;
	}

	// convert player move speed and actions into sound audible by monsters.
	if (m_iWeaponVolume > iBodyVolume)
	{
		m_iTargetVolume = m_iWeaponVolume;

		// OR in the bits for COMBAT sound if the weapon is being louder than the player.
		pSound->m_iType |= bits_SOUND_COMBAT;
	}
	else
	{
		m_iTargetVolume = iBodyVolume;
	}

	// decay weapon volume over time so bits_SOUND_COMBAT stays set for a while
	m_iWeaponVolume -= 250 * gpGlobals->frametime;
	if (m_iWeaponVolume < 0)
	{
		iVolume = 0;
	}

	// if target volume is greater than the player sound's current volume, we paste the new volume in
	// immediately. If target is less than the current volume, current volume is not set immediately to the
	// lower volume, rather works itself towards target volume over time. This gives monsters a much better chance
	// to hear a sound, especially if they don't listen every frame.
	iVolume = pSound->m_iVolume;

	if (m_iTargetVolume > iVolume)
	{
		iVolume = m_iTargetVolume;
	}
	else if (iVolume > m_iTargetVolume)
	{
		iVolume -= 250 * gpGlobals->frametime;

		if (iVolume < m_iTargetVolume)
		{
			iVolume = 0;
		}
	}

	if (m_fNoPlayerSound)
	{
		// debugging flag, lets players move around and shoot without monsters hearing.
		iVolume = 0;
	}

	if (gpGlobals->time > m_flStopExtraSoundTime)
	{
		// since the extra sound that a weapon emits only lasts for one client frame, we keep that sound around for a server frame or two
		// after actual emission to make sure it gets heard.
		m_iExtraSoundTypes = 0;
	}

	if (pSound)
	{
		pSound->m_vecOrigin = pev->origin;
		pSound->m_iType |= (bits_SOUND_PLAYER | m_iExtraSoundTypes);
		pSound->m_iVolume = iVolume;
	}

	// keep track of virtual muzzle flash
	m_iWeaponFlash -= 256 * gpGlobals->frametime;
	if (m_iWeaponFlash < 0)
		m_iWeaponFlash = 0;

	//UTIL_MakeVectors ( pev->angles );
	//gpGlobals->v_forward.z = 0;

	// Below are a couple of useful little bits that make it easier to determine just how much noise the
	// player is making.
	// UTIL_ParticleEffect ( pev->origin + gpGlobals->v_forward * iVolume, g_vecZero, 255, 25 );
	//ALERT ( at_console, "%d/%d\n", iVolume, m_iTargetVolume );
}

void CBasePlayer::PostThink()
{
	if (g_fGameOver)
		goto pt_end;         // intermission or finale

	if (!IsAlive())
		goto pt_end;

	// Handle Tank controlling
	if (m_pTank != NULL)
	{ // if they've moved too far from the gun,  or selected a weapon, unuse the gun
		if (m_pTank->OnControls(pev) && !pev->weaponmodel)
		{
			m_pTank->Use(this, this, USE_SET, 2); // try fire the gun
		}
		else
		{  // they've moved off the platform
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
		}
	}

	// do weapon stuff
	ItemPostFrame();

	// check to see if player landed hard enough to make a sound
	// falling farther than half of the maximum safe distance, but not as far a max safe distance will
	// play a bootscrape sound, and no damage will be inflicted. Fallling a distance shorter than half
	// of maximum safe distance will make no sound. Falling farther than max safe distance will play a
	// fallpain sound, and damage will be inflicted based on how far the player fell

	if ((FBitSet(pev->flags, FL_ONGROUND)) && (pev->health > 0) && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD)
	{
		// ALERT ( at_console, "%f\n", m_flFallVelocity );

		if (pev->watertype == CONTENT_WATER)
		{
			// Did he hit the world or a non-moving entity?
			// BUG - this happens all the time in water, especially when
			// BUG - water has current force
			// if ( !pev->groundentity || VARS(pev->groundentity)->velocity.z == 0 )
			// EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM);
		}
		else if (m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
		{// after this point, we start doing damage
			float flFallDamage = g_pGameRules->FlPlayerFallDamage(this);

			if (flFallDamage > pev->health)
			{//splat
				// note: play on item channel because we play footstep landing on body channel
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/bodysplat.wav", 1, ATTN_NORM);
			}

			if (flFallDamage > 0)
			{
				TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), flFallDamage, DMG_FALL);
				pev->punchangle.x = 0;
			}
		}

		if (IsAlive())
		{
			SetAnimation(PLAYER_WALK);
		}
	}

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		if (m_flFallVelocity > 64 && !g_pGameRules->IsMultiplayer())
		{
			CSoundEnt::InsertSound(bits_SOUND_PLAYER, pev->origin, m_flFallVelocity, 0.2);
			// ALERT( at_console, "fall %f\n", m_flFallVelocity );
		}
		m_flFallVelocity = 0;
	}

	// select the proper animation for the player character
	if (IsAlive())
	{
		if (!pev->velocity.x && !pev->velocity.y)
			SetAnimation(PLAYER_IDLE);
		else if ((pev->velocity.x || pev->velocity.y) && (FBitSet(pev->flags, FL_ONGROUND)))
			SetAnimation(PLAYER_WALK);
		else if (pev->waterlevel > 1)
			SetAnimation(PLAYER_WALK);
	}

	StudioFrameAdvance();
	CheckPowerups(pev);

	UpdatePlayerSound();

pt_end:
#if defined( CLIENT_WEAPONS )
	// Decay timers on weapons
	// go through all of the weapons and make a list of the ones to pack
	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			CBasePlayerItem *pPlayerItem = m_rgpPlayerItems[i];

			while (pPlayerItem)
			{
				CBasePlayerWeapon *gun;

				gun = (CBasePlayerWeapon *)pPlayerItem->GetWeaponPtr();

				if (gun && gun->UseDecrement())
				{
					gun->m_flNextPrimaryAttack      = max(gun->m_flNextPrimaryAttack - gpGlobals->frametime, -1.0);
					gun->m_flNextSecondaryAttack    = max(gun->m_flNextSecondaryAttack - gpGlobals->frametime, -0.001);

					if (gun->m_flTimeWeaponIdle != 1000)
					{
						gun->m_flTimeWeaponIdle     = max(gun->m_flTimeWeaponIdle - gpGlobals->frametime, -0.001);
					}

					if (gun->pev->fuser1 != 1000)
					{
						gun->pev->fuser1    = max(gun->pev->fuser1 - gpGlobals->frametime, -0.001);
					}

					// Only decrement if not flagged as NO_DECREMENT
					//                  if ( gun->m_flPumpTime != 1000 )
					//  {
					//      gun->m_flPumpTime   = max( gun->m_flPumpTime - gpGlobals->frametime, -0.001 );
					//  }
				}

				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}

	m_flNextAttack -= gpGlobals->frametime;
	if (m_flNextAttack < -0.001)
		m_flNextAttack = -0.001;

	/*if ( m_flNextAmmoBurn != 1000 )
	{
	m_flNextAmmoBurn -= gpGlobals->frametime;

	if ( m_flNextAmmoBurn < -0.001 )
	m_flNextAmmoBurn = -0.001;
	}

	if ( m_flAmmoStartCharge != 1000 )
	{
	m_flAmmoStartCharge -= gpGlobals->frametime;

	if ( m_flAmmoStartCharge < -0.001 )
	m_flAmmoStartCharge = -0.001;
	}*/
#endif

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;
}

// checks if the spot is clear of players
BOOL IsSpawnPointValid(CBaseEntity *pPlayer, CBaseEntity *pSpot)
{
	CBaseEntity *ent = NULL;

	if (!pSpot->IsTriggered(pPlayer))
	{
		return FALSE;
	}

	while ((ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 128)) != NULL)
	{
		// if ent is a client, don't spawn on 'em
		if (ent->IsPlayer() && ent != pPlayer)
			return FALSE;
	}

	return TRUE;
}

DLL_GLOBAL CBaseEntity  *g_pLastSpawn;
inline int FNullEnt(CBaseEntity *ent) { return (ent == NULL) || FNullEnt(ent->edict()); }

/*
============
EntSelectSpawnPoint

Returns the entity to spawn at

USES AND SETS GLOBAL g_pLastSpawn
============
*/
edict_t *EntSelectSpawnPoint(CBaseEntity *pPlayer)
{
	CBaseEntity *pSpot;
	edict_t     *player;

	player = pPlayer->edict();

	// choose a info_player_deathmatch point
	if (g_pGameRules->IsCoOp())
	{
		pSpot = UTIL_FindEntityByClassname(g_pLastSpawn, "info_player_coop");
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
		pSpot = UTIL_FindEntityByClassname(g_pLastSpawn, "info_player_start");
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
	}
	else if (g_pGameRules->IsDeathmatch())
	{
		pSpot = g_pLastSpawn;
		// Randomize the start spot
		for (int i = RANDOM_LONG(1, 5); i > 0; i--)
			pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");
		if (FNullEnt(pSpot))  // skip over the null point
			pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");

		CBaseEntity *pFirstSpot = pSpot;

		do
		{
			if (pSpot)
			{
				// check if pSpot is valid
				if (IsSpawnPointValid(pPlayer, pSpot))
				{
					if (pSpot->pev->origin == Vector(0, 0, 0))
					{
						pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");
						continue;
					}

					// if so, go to pSpot
					goto ReturnSpot;
				}
			}
			// increment pSpot
			pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");
		} while (pSpot != pFirstSpot); // loop if we're not back to the start

		// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
		if (!FNullEnt(pSpot))
		{
			CBaseEntity *ent = NULL;
			while ((ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 128)) != NULL)
			{
				// if ent is a client, kill em (unless they are ourselves)
				if (ent->IsPlayer() && !(ent->edict() == player))
					ent->TakeDamage(VARS(INDEXENT(0)), VARS(INDEXENT(0)), 300, DMG_GENERIC);
			}
			goto ReturnSpot;
		}
	}

	// If startspot is set, (re)spawn there.
	if (FStringNull(gpGlobals->startspot) || !strlen(STRING(gpGlobals->startspot)))
	{
		pSpot = UTIL_FindEntityByClassname(NULL, "info_player_start");
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
	}
	else
	{
		pSpot = UTIL_FindEntityByTargetname(NULL, STRING(gpGlobals->startspot));
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
	}

ReturnSpot:
	if (FNullEnt(pSpot))
	{
		ALERT(at_error, "PutClientInServer: no info_player_start on level");
		return INDEXENT(0);
	}

	g_pLastSpawn = pSpot;
	return pSpot->edict();
}

void CBasePlayer::Spawn(void)
{
	pev->classname      = MAKE_STRING("player");
	pev->health         = 100;
	pev->armorvalue     = 0;
	pev->takedamage     = DAMAGE_AIM;
	pev->solid          = SOLID_SLIDEBOX;
	pev->movetype       = MOVETYPE_WALK;
	pev->max_health     = pev->health;
	pev->flags         &= FL_PROXY; // keep proxy flag sey by engine
	pev->flags         |= FL_CLIENT;
	pev->air_finished   = gpGlobals->time + 12;
	pev->dmg            = 2;                // initial water damage
	pev->effects        = 0;
	pev->deadflag       = DEAD_NO;
	pev->dmg_take       = 0;
	pev->dmg_save       = 0;
	pev->friction       = 1.0;
	pev->gravity        = 1.0;
	m_bitsHUDDamage     = -1;
	m_bitsDamageType    = 0;
	m_afPhysicsFlags    = 0;
	m_fLongJump         = FALSE;// no longjump module.

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "0");
	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");

	pev->fov = m_iFOV               = 0;// init field of view.
	m_iClientFOV        = -1; // make sure fov reset is sent

	m_flNextDecalTime   = 0;// let this player decal as soon as he spawns.

	m_flgeigerDelay = gpGlobals->time + 2.0;    // wait a few seconds until user-defined message registrations
	// are recieved by all clients

	m_flTimeStepSound   = 0;
	m_iStepLeft = 0;
	m_flFieldOfView     = 0.5;// some monsters use this to determine whether or not the player is looking at them.

	m_bloodColor    = BLOOD_COLOR_RED;
	m_flNextAttack  = UTIL_WeaponTimeBase();
	StartSneaking();

	m_iFlashBattery = 99;
	m_flFlashLightTime = 1; // force first message

	// dont let uninitialized value here hurt the player
	m_flFallVelocity = 0;

	g_pGameRules->SetDefaultPlayerTeam(this);
	g_pGameRules->GetPlayerSpawnSpot(this);

	SET_MODEL(ENT(pev), "models/player.mdl");
	g_ulModelIndexPlayer = pev->modelindex;
	pev->sequence       = LookupActivity(ACT_IDLE);

	if (FBitSet(pev->flags, FL_DUCKING))
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);

	pev->view_ofs = VEC_VIEW;
	Precache();
	m_HackedGunPos      = Vector(0, 32, 0);

	if (m_iPlayerSound == SOUNDLIST_EMPTY)
	{
		ALERT(at_console, "Couldn't alloc player sound slot!\n");
	}

	m_fNoPlayerSound = FALSE;// normal sound behavior.

	m_pLastItem = NULL;
	m_fInitHUD = TRUE;
	m_iClientHideHUD = -1;  // force this to be recalculated
	m_fWeapon = FALSE;
	m_pClientActiveItem = NULL;
	m_iClientBattery = -1;

	// reset all ammo values to 0
	for (int i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		m_rgAmmo[i] = 0;
		m_rgAmmoLast[i] = 0;  // client ammo values also have to be reset  (the death hud clear messages does on the client side)
	}

	m_lastx = m_lasty = 0;

	//m_flNextChatTime = gpGlobals->time;

	g_pGameRules->PlayerSpawn(this);
}

void CBasePlayer::Precache(void)
{
	// in the event that the player JUST spawned, and the level node graph
	// was loaded, fix all of the node graph pointers before the game starts.

	// !!!BUGBUG - now that we have multiplayer, this needs to be moved!
	if (WorldGraph.m_fGraphPresent && !WorldGraph.m_fGraphPointersSet)
	{
		if (!WorldGraph.FSetGraphPointers())
		{
			ALERT(at_console, "**Graph pointers were not set!\n");
		}
		else
		{
			ALERT(at_console, "**Graph Pointers Set!\n");
		}
	}

	// SOUNDS / MODELS ARE PRECACHED in ClientPrecache() (game specific)
	// because they need to precache before any clients have connected

	// init geiger counter vars during spawn and each time
	// we cross a level transition

	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;

	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;

	m_iClientBattery = -1;

	m_iTrain = TRAIN_NEW;

	// Make sure any necessary user messages have been registered
	LinkUserMessages();

	m_iUpdateTime = 5;  // won't update for 1/2 a second

	if (gInitHUD)
		m_fInitHUD = TRUE;
}

int CBasePlayer::Save(CSave &save)
{
	if (!CBaseMonster::Save(save))
		return 0;

	return save.WriteFields("PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData));
}

//
// Marks everything as new so the player will resend this to the hud.
//
void CBasePlayer::RenewItems(void)
{
}

int CBasePlayer::Restore(CRestore &restore)
{
	if (!CBaseMonster::Restore(restore))
		return 0;

	int status = restore.ReadFields("PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData));

	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;
	// landmark isn't present.
	if (!pSaveData->fUseLandmark)
	{
		ALERT(at_console, "No Landmark:%s\n", pSaveData->szLandmarkName);

		// default to normal spawn
		edict_t* pentSpawnSpot = EntSelectSpawnPoint(this);
		pev->origin = VARS(pentSpawnSpot)->origin + Vector(0, 0, 1);
		pev->angles = VARS(pentSpawnSpot)->angles;
	}
	pev->v_angle.z = 0; // Clear out roll
	pev->angles = pev->v_angle;

	pev->fixangle = TRUE;           // turn this way immediately

	// Copied from spawn() for now
	m_bloodColor    = BLOOD_COLOR_RED;

	g_ulModelIndexPlayer = pev->modelindex;

	if (FBitSet(pev->flags, FL_DUCKING))
	{
		// Use the crouch HACK
		//FixPlayerCrouchStuck( edict() );
		// Don't need to do this with new player prediction code.
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	}
	else
	{
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	}

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");

	if (m_fLongJump)
	{
		g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "1");
	}
	else
	{
		g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "0");
	}

	RenewItems();

#if defined( CLIENT_WEAPONS )
	// HACK:    This variable is saved/restored in CBaseMonster as a time variable, but we're using it
	//          as just a counter.  Ideally, this needs its own variable that's saved as a plain float.
	//          Barring that, we clear it out here instead of using the incorrect restored time value.
	m_flNextAttack = UTIL_WeaponTimeBase();
#endif

	return status;
}

void CBasePlayer::SelectNextItem(int iItem)
{
	CBasePlayerItem *pItem;

	pItem = m_rgpPlayerItems[iItem];

	if (!pItem)
		return;

	if (pItem == m_pActiveItem)
	{
		// select the next one in the chain
		pItem = m_pActiveItem->m_pNext;
		if (!pItem)
		{
			return;
		}

		CBasePlayerItem *pLast;
		pLast = pItem;
		while (pLast->m_pNext)
			pLast = pLast->m_pNext;

		// relink chain
		pLast->m_pNext = m_pActiveItem;
		m_pActiveItem->m_pNext = NULL;
		m_rgpPlayerItems[iItem] = pItem;
	}

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
	{
		m_pActiveItem->Holster();
	}

	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy();
		m_pActiveItem->UpdateItemInfo();
	}
}

void CBasePlayer::SelectItem(const char *pstr)
{
	if (!pstr)
		return;

	CBasePlayerItem *pItem = NULL;

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			pItem = m_rgpPlayerItems[i];

			while (pItem)
			{
				if (FClassnameIs(pItem->pev, pstr))
					break;
				pItem = pItem->m_pNext;
			}
		}

		if (pItem)
			break;
	}

	if (!pItem)
		return;

	if (pItem == m_pActiveItem)
		return;

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
		m_pActiveItem->Holster();

	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy();
		m_pActiveItem->UpdateItemInfo();
	}
}

void CBasePlayer::SelectLastItem(void)
{
	if (!m_pLastItem)
	{
		return;
	}

	if (m_pActiveItem && !m_pActiveItem->CanHolster())
	{
		return;
	}

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
		m_pActiveItem->Holster();

	CBasePlayerItem *pTemp = m_pActiveItem;
	m_pActiveItem = m_pLastItem;
	m_pLastItem = pTemp;
	m_pActiveItem->Deploy();
	m_pActiveItem->UpdateItemInfo();
}

//==============================================
// HasWeapons - do I have any weapons at all?
//==============================================
BOOL CBasePlayer::HasWeapons(void)  // Last check: 2013, November 17.
{
	for (size_t i = 0; i < MAX_ITEM_TYPES; ++i)
	{
		if (m_rgpPlayerItems[i])
		{
			return TRUE;
		}
	}

	return FALSE;
}

void CBasePlayer::SelectPrevItem(int iItem)
{
}

const char *CBasePlayer::TeamID(void)
{
	if (pev == NULL)      // Not fully connected yet
		return "";

	// return their team name
	return m_szTeamName;
}

//==============================================
// !!!UNDONE:ultra temporary SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CBaseEntity
{
public:
	void    Spawn(entvars_t *pevOwner);
	void    Think(void);

	virtual int ObjectCaps(void) { return FCAP_DONT_SAVE; }
};

void CSprayCan::Spawn(entvars_t *pevOwner)
{
	pev->origin = pevOwner->origin + Vector(0, 0, 32);
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);
	pev->frame = 0;

	pev->nextthink = gpGlobals->time + 0.1;
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/sprayer.wav", 1, ATTN_NORM);
}

void CSprayCan::Think(void)
{
	TraceResult tr;
	int playernum;
	int nFrames;
	CBasePlayer *pPlayer;

	pPlayer = (CBasePlayer *)GET_PRIVATE(pev->owner);

	if (pPlayer)
		nFrames = pPlayer->GetCustomDecalFrames();
	else
		nFrames = -1;

	playernum = ENTINDEX(pev->owner);

	// ALERT(at_console, "Spray by player %i, %i of %i\n", playernum, (int)(pev->frame + 1), nFrames);

	UTIL_MakeVectors(pev->angles);
	UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr);

	// No customization present.
	if (nFrames == -1)
	{
		UTIL_DecalTrace(&tr, DECAL_LAMBDA6);
		UTIL_Remove(this);
	}
	else
	{
		UTIL_PlayerDecalTrace(&tr, playernum, pev->frame, TRUE);
		// Just painted last custom frame.
		if (pev->frame++ >= (nFrames - 1))
			UTIL_Remove(this);
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

class   CBloodSplat : public CBaseEntity
{
public:
	void    Spawn(entvars_t *pevOwner);
	void    Spray(void);
};

void CBloodSplat::Spawn(entvars_t *pevOwner)
{
	pev->origin = pevOwner->origin + Vector(0, 0, 32);
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);

	SetThink(&CBloodSplat::Spray);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBloodSplat::Spray(void)
{
	TraceResult tr;

	if (g_Language != LANGUAGE_GERMAN)
	{
		UTIL_MakeVectors(pev->angles);
		UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr);

		UTIL_BloodDecalTrace(&tr, BLOOD_COLOR_RED);
	}
	SetThink(&CBloodSplat::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}

//==============================================

void CBasePlayer::GiveNamedItem(const char *pszName) // Last check: 2013, November 17.
{
	string_t istr = MAKE_STRING(pszName);

	edict_t *pent = CREATE_NAMED_ENTITY(istr);
	
	if (FNullEnt(pent))
	{
		ALERT(at_console, "NULL Ent in GiveNamedItem!\n");
		return;
	}

	VARS(pent)->origin = pev->origin;
	pent->v.spawnflags |= SF_NORESPAWN;

	DispatchSpawn(pent);
	DispatchTouch(pent, ENT(pev));
}

CBaseEntity *FindEntityForward(CBaseEntity *pMe)
{
	TraceResult tr;

	UTIL_MakeVectors(pMe->pev->v_angle);
	UTIL_TraceLine(pMe->pev->origin + pMe->pev->view_ofs, pMe->pev->origin + pMe->pev->view_ofs + gpGlobals->v_forward * 8192, dont_ignore_monsters, pMe->edict(), &tr);
	if (tr.flFraction != 1.0 && !FNullEnt(tr.pHit))
	{
		CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
		return pHit;
	}
	return NULL;
}

BOOL CBasePlayer::FlashlightIsOn(void)  // Last check: 2013, November 17.
{
	return FBitSet(pev->effects, EF_DIMLIGHT);
}

void CBasePlayer::FlashlightTurnOn(void) // Last check: 2013, November 17.
{
	if (!g_pGameRules->FAllowFlashlight())
	{
		return;
	}

	if ((pev->weapons & (1 << WEAPON_SUIT)))
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_ON, 1.0, ATTN_NORM, 0, PITCH_NORM);

		SetBits(pev->effects, EF_DIMLIGHT);

		MESSAGE_BEGIN(MSG_ONE, gmsgFlashlight, NULL, edict());
			WRITE_BYTE(1);
			WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();

		m_flFlashLightTime = gpGlobals->time + FLASH_DRAIN_TIME;
	}
}

void CBasePlayer::FlashlightTurnOff(void) // Last check: 2013, November 17.
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_OFF, 1.0, ATTN_NORM, 0, PITCH_NORM);

	ClearBits(pev->effects, EF_DIMLIGHT);

	MESSAGE_BEGIN(MSG_ONE, gmsgFlashlight, NULL, edict());
		WRITE_BYTE(0);
		WRITE_BYTE(m_iFlashBattery);
	MESSAGE_END();

	m_flFlashLightTime = gpGlobals->time + FLASH_CHARGE_TIME;
}

/*
===============
ForceClientDllUpdate

When recording a demo, we need to have the server tell us the entire client state
so that the client side .dll can behave correctly.
Reset stuff so that the state is transmitted.
===============
*/
void CBasePlayer::ForceClientDllUpdate(void)  // Last check: 2013, November 17.
{
	m_iClientHealth  = -1;
	m_iClientBattery = -1;
	
	m_fWeapon  = FALSE;      // Force weapon send
	m_fInitHUD = TRUE;       // Force HUD gmsgResetHUD message
	m_iTrain  |= TRAIN_NEW;  // Force new train message.

	// Now force all the necessary messages to be sent.
	UpdateClientData();
	HandleSignals();
}

/*
============
ImpulseCommands
============
*/
extern float g_flWeaponCheat;

void CBasePlayer::ImpulseCommands()  // Last check: 2013, November 17
{
	PlayerUse();

	TraceResult tr;
	int iImpulse = pev->impulse;

	switch (iImpulse)
	{
		case 99:
		{
			int iOn;

			if (!gmsgLogo)
			{
				iOn = 1;
				gmsgLogo = REG_USER_MSG("Logo", 1);
			}
			else
			{
				iOn = 0;
			}

			ASSERT(gmsgLogo > 0);
		
			MESSAGE_BEGIN(MSG_ONE, gmsgLogo, NULL, pev);
				WRITE_BYTE(iOn);
			MESSAGE_END();

			if (!iOn)
			{
				gmsgLogo = 0;
			}
			break;
		}
		case 100:
		{
			if (FlashlightIsOn())
			{
				FlashlightTurnOff();
			}
			else
			{
				FlashlightTurnOn();
			}
			break;
		}
		case 201:// paint decal
		{
			if (gpGlobals->time < m_flNextDecalTime)
			{
				break;
			}

			UTIL_MakeVectors(pev->v_angle);
			UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, edict(), &tr);

			if (tr.flFraction != 1.0)
			{
				m_flNextDecalTime = gpGlobals->time + decalfrequency.value;
				CSprayCan *pCan = GetClassPtr((CSprayCan *)NULL);
				pCan->Spawn(pev);
			}

			break;
		}
		default:
		{
			CheatImpulseCommands(iImpulse);
			break;
		}
	}

	pev->impulse = 0;
}


void CBasePlayer::CheatImpulseCommands(int iImpulse) // Last check: 2013, November 17.
{
	if (!g_flWeaponCheat)
	{
		return;
	}

	CBaseEntity *pEntity;
	TraceResult tr;

	switch (iImpulse)
	{
		case 204:
		{
			UTIL_BloodDrips(pev->origin, Vector(0, 0, 1), BLOOD_COLOR_RED, 2000);
		}
		case 76:
		{
			if (!giPrecacheGrunt)
			{
				giPrecacheGrunt = 1;
				ALERT(at_console, "You must now restart to use Grunt-o-matic.\n");
			}
			else
			{
				UTIL_MakeVectors(Vector(0, pev->v_angle.y, 0));
				Create("monster_human_grunt", pev->origin + gpGlobals->v_forward * 128, pev->angles);
			}
			break;
		}

		case 101:
		{
			gEvilImpulse101 = TRUE;
			AddAccount(16000, true);
			ALERT(at_console, "Crediting %s with $16000\n", STRING(pev->netname));
			break;
		}
		case 102:
		{
			// Gibbage!!!
			CGib::SpawnRandomGibs(pev, 1, 1);
			break;
		}
		case 103:
		{
			// What the hell are you doing?
			pEntity = FindEntityForward(this);

			if (pEntity)
			{
				CBaseMonster *pMonster = pEntity->MyMonsterPointer();

				if (pMonster)
				{
					pMonster->ReportAIState();
				}
			}
			break;
		}
		case 104:
		{
			// Dump all of the global state varaibles (and global entity names)
			gGlobalState.DumpGlobals();
			break;
		}
		case 105:
		{
			// player makes no sound for monsters to hear.
			if (m_fNoPlayerSound)
			{
				ALERT(at_console, "Player is audible\n");
				m_fNoPlayerSound = FALSE;
			}
			else
			{
				ALERT(at_console, "Player is silent\n");
				m_fNoPlayerSound = TRUE;
			}
			break;
		}
		case 106:
		{
			// Give me the classname and targetname of this entity.
			pEntity = FindEntityForward(this);

			if (pEntity)
			{
				ALERT(at_console, "Classname: %s", STRING(pEntity->pev->classname));

				if (!FStringNull(pEntity->pev->targetname))
				{
					ALERT(at_console, " - Targetname: %s\n", STRING(pEntity->pev->targetname));
				}
				else
				{
					ALERT(at_console, " - TargetName: No Targetname\n");
				}

				ALERT(at_console, "Model: %s\n", STRING(pEntity->pev->model));

				if (pEntity->pev->globalname)
				{
					ALERT(at_console, "Globalname: %s\n", STRING(pEntity->pev->globalname));
				}
			}

			break;
		}
		case 107:
		{
			TraceResult tr;
			edict_t *pWorld = g_engfuncs.pfnPEntityOfEntIndex(0);

			Vector start = pev->origin + pev->view_ofs;
			Vector end = start + gpGlobals->v_forward * 1024;

			UTIL_TraceLine(start, end, ignore_monsters, edict(), &tr);

			if (tr.pHit)
			{
				pWorld = tr.pHit;
			}

			const char *pTextureName = TRACE_TEXTURE(pWorld, start, end);

			if (pTextureName)
			{
				ALERT(at_console, "Texture: %s\n", pTextureName);
			}

			break;
		}
		case 195:
		{
			// show shortest paths for entire level to nearest node
			Create("node_viewer_fly", pev->origin, pev->angles);
			break;
		}
		case 196:
		{
			// show shortest paths for entire level to nearest node
			Create("node_viewer_large", pev->origin, pev->angles);
			break;
		}
		case 197:
		{
			// show shortest paths for entire level to nearest node
			Create("node_viewer_human", pev->origin, pev->angles);
			break;
		}
		case 199:
		{
			// show nearest node and all connections
			ALERT(at_console, "%d\n", WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
			WorldGraph.ShowNodeConnections(WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
			break;
		}
		case 202:// Random blood splatter
		{
			UTIL_MakeVectors(pev->v_angle);
			UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), &tr);

			if (tr.flFraction != 1.0)
			{
				// line hit something, so paint a decal
				CBloodSplat *pBlood = GetClassPtr((CBloodSplat *)NULL);
				pBlood->Spawn(pev);
			}
			break;
		}
		case 203:
		{
			// remove creature.
			pEntity = FindEntityForward(this);

			if (pEntity)
			{
				if (pEntity->pev->takedamage)
				{
					pEntity->SetThink(&CBaseEntity::SUB_Remove);
				}
			}
			break;
		}
	}
}

int CBasePlayer::RemovePlayerItem(CBasePlayerItem *pItem)
{
	if (m_pActiveItem == pItem)
	{
		ResetAutoaim();
		pItem->Holster();
		pItem->pev->nextthink = 0;// crowbar may be trying to swing again, etc.
		pItem->SetThink(NULL);
		m_pActiveItem = NULL;
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}
	else if (m_pLastItem == pItem)
		m_pLastItem = NULL;

	CBasePlayerItem *pPrev = m_rgpPlayerItems[pItem->iItemSlot()];

	if (pPrev == pItem)
	{
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem->m_pNext;
		return TRUE;
	}
	else
	{
		while (pPrev && pPrev->m_pNext != pItem)
		{
			pPrev = pPrev->m_pNext;
		}
		if (pPrev)
		{
			pPrev->m_pNext = pItem->m_pNext;
			return TRUE;
		}
	}
	return FALSE;
}

//
// Returns the unique ID for the ammo, or -1 if error
//
int CBasePlayer::GiveAmmo(int iCount, char *szName, int iMax) // Last check: 2013, November 17.
{
	if (pev->flags & FL_SPECTATOR || !szName || !g_pGameRules->CanHaveAmmo(this, szName, iMax))
	{
		return -1;
	}

	int i = GetAmmoIndex(szName);

	if (i < 0 || i >= MAX_AMMO_SLOTS)
	{
		return -1;
	}

	int iAdd = min(iCount, iMax - m_rgAmmo[i]);

	if (iAdd < 1)
	{
		return i;
	}

	m_rgAmmo[i] += iAdd;

	if (gmsgAmmoPickup)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgAmmoPickup, NULL, pev);
			WRITE_BYTE(GetAmmoIndex(szName));     // ammo ID
			WRITE_BYTE(iAdd);					  // amount
		MESSAGE_END();
	}

	TabulateAmmo();

	return i;
}

void CBasePlayer::GiveDefaultItems(void) // Last check: 2013, November 17.
{
	RemoveAllItems(FALSE);

	m_bHasPrimary = false;

	switch (m_iTeam)
	{
		case CT:
		{
			GiveNamedItem("weapon_knife");
			GiveNamedItem("weapon_usp");

			GiveAmmo(m_bIsVIP ? 12 : 24, "45acp", _45ACP_MAX_CARRY);

			break;
		}
		case TERRORIST:
		{
			GiveNamedItem("weapon_knife");
			GiveNamedItem("weapon_glock18");

			GiveAmmo(40, "9mm", _9MM_MAX_CARRY);

			break;
		}
	}
}

/*
============
ItemPreFrame

Called every frame by the player PreThink
============
*/
void CBasePlayer::ItemPreFrame()  // Last check: 2013, November 17.
{
#if defined( CLIENT_WEAPONS )
	if (m_flNextAttack > 0)
#else
	if (gpGlobals->time < m_flNextAttack)
#endif
	{
		return;
	}

	if (!m_pActiveItem)
	{
		m_pActiveItem->ItemPreFrame();
	}
}

/*
============
ItemPostFrame

Called every frame by the player PostThink
============
*/
void CBasePlayer::ItemPostFrame()   // Last check: 2013, November 17.
{
	if (m_pTank != NULL)
	{
		return;
	}

	if (m_pActiveItem && HasShield() && IsReloading() && (pev->button & IN_ATTACK2))
	{
		m_flNextAttack = UTIL_WeaponTimeBase();
	}

#if defined( CLIENT_WEAPONS )
	if (m_flNextAttack > 0)
#else
	if (gpGlobals->time < m_flNextAttack)
#endif
	{
		return;
	}

	ImpulseCommands();

	if (!m_pActiveItem)
	{
		m_pActiveItem->ItemPostFrame();
	}
}

int CBasePlayer::GetAmmoIndex(const char *psz)  // Last check: 2013, November 17.
{
	if (!psz)
	{
		return -1;
	}

	for (size_t i = 1; i < MAX_AMMO_SLOTS; ++i)
	{
		if (!CBasePlayerItem::AmmoInfoArray[i].pszName)
		{
			continue;
		}

		if (stricmp(psz, CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
		{
			return i;
		}
	}

	return -1;
}

// Called from UpdateClientData
// makes sure the client has all the necessary ammo info,  if values have changed
void CBasePlayer::SendAmmoUpdate(void)
{
	for (int i=0; i < MAX_AMMO_SLOTS; i++)
	{
		if (m_rgAmmo[i] != m_rgAmmoLast[i])
		{
			m_rgAmmoLast[i] = m_rgAmmo[i];

			ASSERT(m_rgAmmo[i] >= 0);
			ASSERT(m_rgAmmo[i] < 255);

			// send "Ammo" update message
			MESSAGE_BEGIN(MSG_ONE, gmsgAmmoX, NULL, pev);
			WRITE_BYTE(i);
			WRITE_BYTE(max(min(m_rgAmmo[i], 254), 0));  // clamp the value to one byte
			MESSAGE_END();
		}
	}
}

/*
=========================================================
UpdateClientData

resends any changed player HUD info to the client.
Called every frame by PlayerPreThink
Also called at start of demo recording and playback by
ForceClientDllUpdate to ensure the demo gets messages
reflecting all of the HUD state info.
=========================================================
*/
void CBasePlayer::UpdateClientData(void)
{
	if (m_fInitHUD)
	{
		m_fInitHUD = FALSE;
		gInitHUD = FALSE;

		MESSAGE_BEGIN(MSG_ONE, gmsgResetHUD, NULL, pev);
		WRITE_BYTE(0);
		MESSAGE_END();

		if (!m_fGameHUDInitialized)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgInitHUD, NULL, pev);
			MESSAGE_END();

			g_pGameRules->InitHUD(this);
			m_fGameHUDInitialized = TRUE;

			m_iObserverLastMode = OBS_ROAMING;

			if (g_pGameRules->IsMultiplayer())
			{
				FireTargets("game_playerjoin", this, this, USE_TOGGLE, 0);
			}
		}

		FireTargets("game_playerspawn", this, this, USE_TOGGLE, 0);

		InitStatusBar();
	}

	if (m_iHideHUD != m_iClientHideHUD)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgHideWeapon, NULL, pev);
		WRITE_BYTE(m_iHideHUD);
		MESSAGE_END();

		m_iClientHideHUD = m_iHideHUD;
	}

	if (m_iFOV != m_iClientFOV)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, pev);
		WRITE_BYTE(m_iFOV);
		MESSAGE_END();

		// cache FOV change at end of function, so weapon updates can see that FOV has changed
	}

	// HACKHACK -- send the message to display the game title
	if (gDisplayTitle)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgShowGameTitle, NULL, pev);
		WRITE_BYTE(0);
		MESSAGE_END();
		gDisplayTitle = 0;
	}

	if (pev->health != m_iClientHealth)
	{
#define clamp( val, min, max ) ( ((val) > (max)) ? (max) : ( ((val) < (min)) ? (min) : (val) ) )
		int iHealth = clamp(pev->health, 0, 255);  // make sure that no negative health values are sent
		if (pev->health > 0.0f && pev->health <= 1.0f)
			iHealth = 1;

		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgHealth, NULL, pev);
		WRITE_BYTE(iHealth);
		MESSAGE_END();

		m_iClientHealth = pev->health;
	}

	if (pev->armorvalue != m_iClientBattery)
	{
		m_iClientBattery = pev->armorvalue;

		ASSERT(gmsgBattery > 0);
		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgBattery, NULL, pev);
		WRITE_SHORT((int)pev->armorvalue);
		MESSAGE_END();
	}

	if (pev->dmg_take || pev->dmg_save || m_bitsHUDDamage != m_bitsDamageType)
	{
		// Comes from inside me if not set
		Vector damageOrigin = pev->origin;
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		edict_t *other = pev->dmg_inflictor;
		if (other)
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(other);
			if (pEntity)
				damageOrigin = pEntity->Center();
		}

		// only send down damage type that have hud art
		int visibleDamageBits = m_bitsDamageType & DMG_SHOWNHUD;

		MESSAGE_BEGIN(MSG_ONE, gmsgDamage, NULL, pev);
		WRITE_BYTE(pev->dmg_save);
		WRITE_BYTE(pev->dmg_take);
		WRITE_LONG(visibleDamageBits);
		WRITE_COORD(damageOrigin.x);
		WRITE_COORD(damageOrigin.y);
		WRITE_COORD(damageOrigin.z);
		MESSAGE_END();

		pev->dmg_take = 0;
		pev->dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;

		// Clear off non-time-based damage indicators
		m_bitsDamageType &= DMG_TIMEBASED;
	}

	// Update Flashlight
	if ((m_flFlashLightTime) && (m_flFlashLightTime <= gpGlobals->time))
	{
		if (FlashlightIsOn())
		{
			if (m_iFlashBattery)
			{
				m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;
				m_iFlashBattery--;

				if (!m_iFlashBattery)
					FlashlightTurnOff();
			}
		}
		else
		{
			if (m_iFlashBattery < 100)
			{
				m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;
				m_iFlashBattery++;
			}
			else
				m_flFlashLightTime = 0;
		}

		MESSAGE_BEGIN(MSG_ONE, gmsgFlashBattery, NULL, pev);
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();
	}

	if (m_iTrain & TRAIN_NEW)
	{
		ASSERT(gmsgTrain > 0);
		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgTrain, NULL, pev);
		WRITE_BYTE(m_iTrain & 0xF);
		MESSAGE_END();

		m_iTrain &= ~TRAIN_NEW;
	}

	//
	// New Weapon?
	//
	/*if (!m_fKnownItem)
	{
	m_fKnownItem = TRUE;

	// WeaponInit Message
	// byte  = # of weapons
	//
	// for each weapon:
	// byte     name str length (not including null)
	// bytes... name
	// byte     Ammo Type
	// byte     Ammo2 Type
	// byte     bucket
	// byte     bucket pos
	// byte     flags
	// ????     Icons

	// Send ALL the weapon info now
	int i;

	for (i = 0; i < MAX_WEAPONS; i++)
	{
	ItemInfo& II = CBasePlayerItem::ItemInfoArray[i];

	if ( !II.iId )
	continue;

	const char *pszName;
	if (!II.pszName)
	pszName = "Empty";
	else
	pszName = II.pszName;

	MESSAGE_BEGIN( MSG_ONE, gmsgWeaponList, NULL, pev );
	WRITE_STRING(pszName);          // string   weapon name
	WRITE_BYTE(GetAmmoIndex(II.pszAmmo1));  // byte     Ammo Type
	WRITE_BYTE(II.iMaxAmmo1);               // byte     Max Ammo 1
	WRITE_BYTE(GetAmmoIndex(II.pszAmmo2));  // byte     Ammo2 Type
	WRITE_BYTE(II.iMaxAmmo2);               // byte     Max Ammo 2
	WRITE_BYTE(II.iSlot);                   // byte     bucket
	WRITE_BYTE(II.iPosition);               // byte     bucket pos
	WRITE_BYTE(II.iId);                     // byte     id (bit index into pev->weapons)
	WRITE_BYTE(II.iFlags);                  // byte     Flags
	MESSAGE_END();
	}
	}*/

	SendAmmoUpdate();

	// Update all the items
	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])  // each item updates it's successors
			m_rgpPlayerItems[i]->UpdateClientData(this);
	}

	// Cache and client weapon change
	m_pClientActiveItem = m_pActiveItem;
	m_iClientFOV = m_iFOV;

	// Update Status Bar
	if (m_flNextSBarUpdateTime < gpGlobals->time)
	{
		UpdateStatusBar();
		m_flNextSBarUpdateTime = gpGlobals->time + 0.2;
	}
}

//=========================================================
// FBecomeProne - Overridden for the player to set the proper
// physics flags when a barnacle grabs player.
//=========================================================
BOOL CBasePlayer::FBecomeProne(void)  // Last check: 2013, November 17.
{
	m_afPhysicsFlags |= PFLAG_ONBARNACLE;
	return TRUE;
}

//=========================================================
// BarnacleVictimBitten - bad name for a function that is called
// by Barnacle victims when the barnacle pulls their head
// into its mouth. For the player, just die.
//=========================================================
void CBasePlayer::BarnacleVictimBitten(entvars_t *pevBarnacle) // Last check : 2014, November 11.
{
	TakeDamage(pevBarnacle, pevBarnacle, pev->health + pev->armorvalue, DMG_SLASH | DMG_ALWAYSGIB);
}

//=========================================================
// BarnacleVictimReleased - overridden for player who has
// physics flags concerns.
//=========================================================
void CBasePlayer::BarnacleVictimReleased(void) // Last check : 2014, November 11.
{
	m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
}

//=========================================================
// Illumination
// return player light level plus virtual muzzle flash
//=========================================================
int CBasePlayer::Illumination(void)  // Last check : 2014, November 11.
{
	int iIllum = CBaseEntity::Illumination();

	iIllum += m_iWeaponFlash;

	if (iIllum > 255)
	{
		return 255;
	}

	return iIllum;
}

void CBasePlayer::EnableControl(BOOL fControl) // Last check : 2014, November 117.
{
	if (!fControl)
	{
		pev->flags |= FL_FROZEN;
	}
	else
	{
		pev->flags &= ~FL_FROZEN;
	}
}

#define DOT_1DEGREE   0.9998476951564
#define DOT_2DEGREE   0.9993908270191
#define DOT_3DEGREE   0.9986295347546
#define DOT_4DEGREE   0.9975640502598
#define DOT_5DEGREE   0.9961946980917
#define DOT_6DEGREE   0.9945218953683
#define DOT_7DEGREE   0.9925461516413
#define DOT_8DEGREE   0.9902680687416
#define DOT_9DEGREE   0.9876883405951
#define DOT_10DEGREE  0.9848077530122
#define DOT_15DEGREE  0.9659258262891
#define DOT_20DEGREE  0.9396926207859
#define DOT_25DEGREE  0.9063077870367

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector CBasePlayer::GetAutoaimVector(float flDelta) // Last check : 2013, June 14.
{
	if (g_iSkillLevel == SKILL_HARD)
	{
		UTIL_MakeVectors(pev->v_angle + pev->punchangle);
		return gpGlobals->v_forward;
	}

	Vector vecSrc = GetGunPosition();
	float flDist = 8192;

	m_vecAutoAim = g_vecZero;

	BOOL m_fOldTargeting = m_fOnTarget;
	Vector angles = AutoaimDeflection(vecSrc, flDist, flDelta);

	if (!g_pGameRules->AllowAutoTargetCrosshair())
	{
		m_fOnTarget = FALSE;
	}
	else if (m_fOldTargeting != m_fOnTarget)
	{
		m_pActiveItem->UpdateItemInfo();
	}

	if (angles.x > 180)   angles.x -= 360;
	if (angles.x < -180)   angles.x += 360;
	if (angles.y > 180)   angles.y -= 360;
	if (angles.y < -180)   angles.y += 360;

	if (angles.x > 25)   angles.x = 25;
	if (angles.x < -25)   angles.x = -25;
	if (angles.y > 12)   angles.y = 12;
	if (angles.y < -12)   angles.y = -12;

	if (g_iSkillLevel == SKILL_EASY)
		m_vecAutoAim = m_vecAutoAim * 0.67 + angles * 0.33;
	else
		m_vecAutoAim = angles * 0.9;

	if (g_psv_aim && g_psv_aim->value > 0)
	{
		if (m_vecAutoAim.x != m_lastx || m_vecAutoAim.y != m_lasty)
		{
			SET_CROSSHAIRANGLE(edict(), -m_vecAutoAim.x, m_vecAutoAim.y);

			m_lastx = m_vecAutoAim.x;
			m_lasty = m_vecAutoAim.y;
		}
	}

	UTIL_MakeVectors(pev->v_angle + pev->punchangle + m_vecAutoAim);

	return gpGlobals->v_forward;
}

Vector CBasePlayer::AutoaimDeflection(Vector &vecSrc, float flDist, float flDelta) // Last check : 2013, June 14.
{
	m_fOnTarget = FALSE;

	return g_vecZero;
}

void CBasePlayer::ResetAutoaim(void) // Last check : 2013, June 14.
{
	if (m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0)
	{
		m_vecAutoAim = Vector(0, 0, 0);
		SET_CROSSHAIRANGLE(edict(), 0, 0);
	}
	m_fOnTarget = FALSE;
}

/*
=============
SetCustomDecalFrames

UNDONE:  Determine real frame limit, 8 is a placeholder.
Note:  -1 means no custom frames present.
=============
*/
void CBasePlayer::SetCustomDecalFrames(int nFrames)
{
	if (nFrames > 0 &&
		nFrames < 8)
		m_nCustomSprayFrames = nFrames;
	else
		m_nCustomSprayFrames = -1;
}

/*
=============
GetCustomDecalFrames

Returns the # of custom frames this player's custom clan logo contains.
=============
*/
int CBasePlayer::GetCustomDecalFrames(void)  // Last check: 2013, November 17.
{
	return m_nCustomSprayFrames;
}

//=========================================================
// DropPlayerItem - drop the named item, or if no name,
// the active item.
//=========================================================
void CBasePlayer::DropPlayerItem(char *pszItemName) // Last check: 2013, November 17.
{
	if (!*pszItemName)
	{
		pszItemName = NULL;
	}

	if (m_bIsVIP)
	{
		ClientPrint(pev, HUD_PRINTCENTER, "#Weapon_Cannot_Be_Dropped");
		return;
	}

	if (!pszItemName && HasShield() && (!m_pActiveItem || m_pActiveItem->CanHolster()))
	{
		DropShield(true);
	}

	for (size_t i = 0; i < MAX_ITEM_TYPES; ++i)
	{
		CBasePlayerItem *pWeapon = m_rgpPlayerItems[i];

		while (pWeapon)
		{
			if (pszItemName)
			{
				if (!strcmp(pszItemName, STRING(pWeapon->pev->classname)))
				{
					break;
				}
			}
			else if (pWeapon == m_pActiveItem)
			{
				break;
			}

			pWeapon = pWeapon->m_pNext;
		}

		// if we land here with a valid pWeapon pointer, that's because we found the
		// item we want to drop and hit a BREAK;  pWeapon is the item.
		if (pWeapon)
		{
			if (!pWeapon->CanDrop())
			{
				ClientPrint(pev, HUD_PRINTCENTER, "#Weapon_Cannot_Be_Dropped");
				continue;
			}

			g_pGameRules->GetNextBestWeapon(this, pWeapon);
			UTIL_MakeVectors(pev->angles);

			pev->weapons &= ~(1 << pWeapon->m_iId);

			if (pWeapon->iItemSlot() == PRIMARY_WEAPON_SLOT)
			{
				m_bHasPrimary = false;
			}

			if (FClassnameIs(pWeapon->pev, "weapon_c4"))
			{
				m_bHasC4 = false;
				pev->body = 0;

				SetBombIcon(FALSE);
				pWeapon->m_pPlayer->SetProgressBarTime(0);

				if (!g_pGameRules->m_fTeamCount)
				{
					UTIL_LogPrintf("\"%s<%i><%s><TERRORIST>\" triggered \"Dropped_The_Bomb\"\n", STRING(pev->netname), GETPLAYERUSERID(edict()), GETPLAYERAUTHID(edict()));
					g_pGameRules->m_bBombDropped = TRUE;

					CBaseEntity *pEntity = NULL;

					while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
					{
						if (FNullEnt(pEntity->edict()))
						{
							break;
						}

						if (!pEntity->IsPlayer() || pEntity->IsDormant())
						{
							continue;
						}

						CBasePlayer *pOther = GetClassPtr((CBasePlayer *)pEntity->pev);

						if (pOther->pev->deadflag == DEAD_NO && pOther->m_iTeam == TERRORIST)
						{
							ClientPrint(pOther->pev, HUD_PRINTCENTER, "#Game_bomb_drop", STRING(pev->netname));

							MESSAGE_BEGIN(MSG_ONE, gmsgBombDrop, NULL, pOther->edict());
								WRITE_COORD(pev->origin.x);
								WRITE_COORD(pev->origin.y);
								WRITE_COORD(pev->origin.z);
								WRITE_BYTE(0);
							MESSAGE_END();
						}
					}
				}
			}

			CWeaponBox *pWeaponBox = (CWeaponBox *)CBaseEntity::Create("weaponbox", pev->origin + gpGlobals->v_forward * 10, pev->angles, edict());
			
			pWeaponBox->pev->angles.x = 0;
			pWeaponBox->pev->angles.z = 0;

			pWeaponBox->SetThink(&CWeaponBox::Kill);
			pWeaponBox->pev->nextthink = gpGlobals->time + 300;

			pWeaponBox->PackWeapon(pWeapon);
			pWeaponBox->pev->velocity = gpGlobals->v_forward * 400;

			if (FClassnameIs(pWeapon->pev, "weapon_c4"))
			{
				pWeaponBox->m_bIsBomb = true;
				pWeaponBox->SetThink(&CWeaponBox::BombThink);
				pWeaponBox->pev->nextthink = gpGlobals->time + 1;

				// TODO: Implements me.
				// TheBots->SetLooseBomb(pWeaponBox);
				// TheBots->OnEvent(EVENT_BOMB_DROPPED, NULL, NULL);
			}

			if (pWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE)
			{
				int iAmmoIndex = GetAmmoIndex(pWeapon->pszAmmo1());

				if (iAmmoIndex != -1)
				{
					pWeaponBox->PackAmmo(MAKE_STRING(pWeapon->pszAmmo1()), m_rgAmmo[iAmmoIndex] > 0);
					m_rgAmmo[iAmmoIndex] = 0;
				}
			}

			const char *modelname = GetCSModelName(pWeapon->m_iId);

			if (modelname)
			{
				SET_MODEL(pWeaponBox->edict(), modelname);
			}
		}
	}
}

void CBasePlayer::DropShield(bool bDeploy) // Last check: 2013, November 17.
{
	if (!HasShield())
	{
		return;
	}

	if (m_pActiveItem && !m_pActiveItem->CanHolster())
	{
		return;
	}

	if (m_pActiveItem)
	{
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;

		if (pWeapon->m_iId == WEAPON_HEGRENADE || pWeapon->m_iId == WEAPON_FLASHBANG || pWeapon->m_iId == WEAPON_SMOKEGRENADE)
		{
			if (m_rgAmmo[pWeapon->m_iPrimaryAmmoType] <= 0)
			{
				g_pGameRules->GetNextBestWeapon(this, pWeapon);
			}
		}
	}

	if (m_pActiveItem)
	{
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;

		if (pWeapon->m_flStartThrow != 0)
		{
			m_pActiveItem->Holster();
		}
	}

	if (IsReloading())
	{
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;

		pWeapon->m_fInReload = FALSE;
		m_flNextAttack = UTIL_WeaponTimeBase();
	}

	if (IsProtectedByShield() && m_pActiveItem)
	{
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;
		pWeapon->SecondaryAttack();
	}

	m_bShieldDrawn = false;

	RemoveShield();

	if (m_pActiveItem && bDeploy)
	{
		m_pActiveItem->Deploy();
	}

	UTIL_MakeVectors(pev->angles);

	CWShield *pShield = (CWShield *)CBaseEntity::Create("weapon_shield", pev->origin + gpGlobals->v_forward * 10, pev->angles, edict());
	
	pShield->pev->angles.x = 0;
	pShield->pev->angles.y = 0;
	pShield->pev->velocity = gpGlobals->v_forward * 400;

	pShield->SetThink(&CBaseEntity::SUB_Remove);
	pShield->pev->nextthink = gpGlobals->time + 300;

	pShield->SetCantBePickedUpByUser(this, 2.0);
}

bool CBasePlayer::IsProtectedByShield(void) // Last check: 2013, November 17.
{
	return HasShield() && m_bShieldDrawn;
}

bool CBasePlayer::IsReloading(void) // Last check: 2013, November 17.
{
	return m_pActiveItem && ((CBasePlayerWeapon *)m_pActiveItem)->m_fInReload;
}

void CBasePlayer::GiveShield(bool bRetire) // Last check: 2013, November 17.
{
	m_bOwnsShield = true;
	m_bHasPrimary = true;

	if (m_pActiveItem)
	{
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;

		if (bRetire)
		{
			if (m_rgAmmo[pWeapon->m_iPrimaryAmmoType] > 0)
			{
				pWeapon->Holster();
			}

			if (!pWeapon->Deploy())
			{
				pWeapon->RetireWeapon();
			}
		}
	}

	pev->gamestate = 0;
}

void CBasePlayer::RemoveShield(void) // Last check: 2013, November 17.
{
	if (HasShield())
	{
		m_bOwnsShield  = false;
		m_bHasPrimary  = false;
		m_bShieldDrawn = false;

		pev->gamestate = 1;

		UpdateShieldCrosshair(false);
	}
}

//=========================================================
// HasPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasPlayerItem(CBasePlayerItem *pCheckItem) // Last check: 2013, November 17.
{
	CBasePlayerItem *pItem = m_rgpPlayerItems[pCheckItem->iItemSlot()];

	while (pItem)
	{
		if (FClassnameIs(pItem->pev, STRING(pCheckItem->pev->classname)))
		{
			return TRUE;
		}

		pItem = pItem->m_pNext;
	}

	return FALSE;
}

//=========================================================
// HasNamedPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasNamedPlayerItem(const char *pszItemName) // Last check: 2013, November 17.
{
	CBasePlayerItem *pItem;

	for (size_t i = 0; i < MAX_ITEM_TYPES; ++i)
	{
		pItem = m_rgpPlayerItems[i];

		while (pItem)
		{
			if (!strcmp(pszItemName, STRING(pItem->pev->classname)))
			{
				return TRUE;
			}

			pItem = pItem->m_pNext;
		}
	}

	return FALSE;
}

//=========================================================
// Dead HEV suit prop
//=========================================================
class CDeadHEV : public CBaseMonster
{
public:
	void Spawn(void);
	int Classify(void) { return  CLASS_HUMAN_MILITARY; }

	void KeyValue(KeyValueData *pkvd);

	int m_iPose;// which sequence to display    -- temporary, don't need to save
	static char *m_szPoses[4];
};

char *CDeadHEV::m_szPoses[] ={ "deadback", "deadsitting", "deadstomach", "deadtable" };

void CDeadHEV::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_hevsuit_dead, CDeadHEV);

//=========================================================
// ********** DeadHEV SPAWN **********
//=========================================================
void CDeadHEV::Spawn(void)
{
	PRECACHE_MODEL("models/player.mdl");
	SET_MODEL(ENT(pev), "models/player.mdl");

	pev->effects        = 0;
	pev->yaw_speed      = 8;
	pev->sequence       = 0;
	pev->body           = 1;
	m_bloodColor        = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);

	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead hevsuit with bad pose\n");
		pev->sequence = 0;
		pev->effects = EF_BRIGHTFIELD;
	}

	// Corpses have less health
	pev->health         = 8;

	MonsterInitDead();
}

class CStripWeapons : public CPointEntity
{
public:
	void    Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

private:
};

LINK_ENTITY_TO_CLASS(player_weaponstrip, CStripWeapons);

void CStripWeapons::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBasePlayer *pPlayer = NULL;

	if (pActivator && pActivator->IsPlayer())
	{
		pPlayer = (CBasePlayer *)pActivator;
	}
	else if (!g_pGameRules->IsDeathmatch())
	{
		pPlayer = (CBasePlayer *)CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(1));
	}

	if (pPlayer)
		pPlayer->RemoveAllItems(FALSE);
}

class CRevertSaved : public CPointEntity
{
public:
	void    Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void    EXPORT MessageThink(void);
	void    EXPORT LoadThink(void);
	void    KeyValue(KeyValueData *pkvd);

	virtual int     Save(CSave &save);
	virtual int     Restore(CRestore &restore);
	static  TYPEDESCRIPTION m_SaveData[];

	inline  float   Duration(void) { return pev->dmg_take; }
	inline  float   HoldTime(void) { return pev->dmg_save; }
	inline  float   MessageTime(void) { return m_messageTime; }
	inline  float   LoadTime(void) { return m_loadTime; }

	inline  void    SetDuration(float duration) { pev->dmg_take = duration; }
	inline  void    SetHoldTime(float hold) { pev->dmg_save = hold; }
	inline  void    SetMessageTime(float time) { m_messageTime = time; }
	inline  void    SetLoadTime(float time) { m_loadTime = time; }

private:
	float   m_messageTime;
	float   m_loadTime;
};

LINK_ENTITY_TO_CLASS(player_loadsaved, CRevertSaved);

TYPEDESCRIPTION CRevertSaved::m_SaveData[] =
{
	DEFINE_FIELD(CRevertSaved, m_messageTime, FIELD_FLOAT),   // These are not actual times, but durations, so save as floats
	DEFINE_FIELD(CRevertSaved, m_loadTime, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CRevertSaved, CPointEntity);

void CRevertSaved::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		SetHoldTime(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "messagetime"))
	{
		SetMessageTime(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "loadtime"))
	{
		SetLoadTime(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CRevertSaved::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	UTIL_ScreenFadeAll(pev->rendercolor, Duration(), HoldTime(), pev->renderamt, FFADE_OUT);
	pev->nextthink = gpGlobals->time + MessageTime();
	SetThink(&CRevertSaved::MessageThink);
}

void CRevertSaved::MessageThink(void)
{
	UTIL_ShowMessageAll(STRING(pev->message));
	float nextThink = LoadTime() - MessageTime();
	if (nextThink > 0)
	{
		pev->nextthink = gpGlobals->time + nextThink;
		SetThink(&CRevertSaved::LoadThink);
	}
	else
		LoadThink();
}

void CRevertSaved::LoadThink(void)
{
	if (!gpGlobals->deathmatch)
	{
		SERVER_COMMAND("reload\n");
	}
}

//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission :public CPointEntity
{
	void Spawn(void);
	void Think(void);
};

void CInfoIntermission::Spawn(void)
{
	UTIL_SetOrigin(pev, pev->origin);
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 2;// let targets spawn!
}

void CInfoIntermission::Think(void)
{
	edict_t *pTarget;

	// find my target
	pTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));

	if (!FNullEnt(pTarget))
	{
		pev->v_angle = UTIL_VecToAngles((pTarget->v.origin - pev->origin).Normalize());
		pev->v_angle.x = -pev->v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS(info_intermission, CInfoIntermission);