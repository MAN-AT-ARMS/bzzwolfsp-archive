/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "g_local.h"

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent ) {
	char entry[1024];
	char string[1400];
	int stringlength;
	int i, j;
	gclient_t   *cl;
        gentity_t *tmpent;
	int numSorted;
	int scoreFlags;

        int counter = 0;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;
	scoreFlags = 0;


	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	//if ( numSorted > 32 ) {
//		numSorted = 32;
//	}
/*
        for ( i = 0; i < g_maxclients.integer; i++ ) 
                player = &g_entities[i];
                if ( !player->inuse ) 
*/
	for ( i = 0 ; i < numSorted ; i++ ) {
		int ping;
                int respawnsLeft;
//fretn

		cl = &level.clients[level.sortedClients[i]];
                
                // fretn - coop, don't send the score of the AI
                //if ( (g_entities[level.sortedClients[i]].r.svFlags & SVF_CASTAI) )
                tmpent = &g_entities[level.sortedClients[i]];
                if ( (tmpent->r.svFlags & SVF_CASTAI) )
                {
                        continue;
                }

                // fretn
                respawnsLeft = cl->ps.persistant[PERS_RESPAWNS_LEFT];
                if ( respawnsLeft == 0 && ( ( cl->ps.pm_flags & PMF_LIMBO ) || ( level.intermissiontime && g_entities[level.sortedClients[i]].health <= 0 ) ) ) {
                        respawnsLeft = -2;
                }

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

                // TODO: fretn: add respawnsLeft to the end of the message
#ifndef MONEY
		Com_sprintf( entry, sizeof( entry ),
					 " %i %i %i %i %i %i", level.sortedClients[i],
					 g_entities[level.sortedClients[i]].client->ps.persistant[PERS_SCORE], ping, ( level.time - cl->pers.enterTime ) / 60000,
					 scoreFlags, g_entities[level.sortedClients[i]].s.powerups );
#else
		Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i %i %i", level.sortedClients[i], g_entities[level.sortedClients[i]].client->ps.persistant[PERS_SCORE], ping, ( level.time - cl->pers.enterTime ) / 60000, g_entities[level.sortedClients[i]].client->ps.persistant[PERS_KILLED], cl->sess.damage_given, cl->sess.damage_received );
#endif
		j = strlen( entry );
		if ( stringlength + j > 1024 ) {
			break;
		}
		strcpy( string + stringlength, entry );
		stringlength += j;

                counter ++;
	}

        //G_Printf("VERZONDEN: %s", va( "scores %i %i %i%s\n", counter, level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], string ) );
	trap_SendServerCommand( ent - g_entities, va( "scores %i %i %i%s", counter,
												  level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE],
												  string ) );
}

/*
==================
Cmd_PlayerStart_f

Starts the coop game
==================
*/
void Cmd_PlayerStart_f( gentity_t *ent ) {
        //gentity_t *player;

        if ( g_gametype.integer > GT_COOP )
                return;

        //player = AICast_FindEntityForName( "player" );
        //if ( player ) {
                trap_Cvar_Set( "g_playerstart", "1" ); 
                //AICast_ScriptEvent( AICast_GetCastState( player->s.number ), "playerstart", "" );

                // now let it think
                //AICast_CastScriptThink();
        //}            
}

/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
	DeathmatchScoreboardMessage( ent );
}


/*
==================
CheatsOk
==================
*/
qboolean    CheatsOk( gentity_t *ent ) {
#ifdef _DEBUG
	return qtrue;
#endif
	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent - g_entities, "print \"Cheats are not enabled on this server.\n\"" );
		return qfalse;
	}
	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent - g_entities, "print \"You must be alive to use this command.\n\"" );
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char    *ConcatArgs( int start ) {
	int i, c, tlen;
	static char line[MAX_STRING_CHARS];
	int len;
	char arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString( char *in, char *out ) {
	while ( *in ) {
		if ( *in == 27 ) {
			in += 2;        // skip color code
			continue;
		}
		if ( *in < 32 ) {
			in++;
			continue;
		}
		*out++ = tolower( *in++ );
	}

	*out = 0;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, char *s ) {
	gclient_t   *cl;
	int idnum;
	char s2[MAX_STRING_CHARS];
	char n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if ( s[0] >= '0' && s[0] <= '9' ) {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			trap_SendServerCommand( to - g_entities, va( "print \"Bad client slot: %i\n\"", idnum ) );
			return -1;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap_SendServerCommand( to - g_entities, va( "print \"Client %i is not active\n\"", idnum ) );
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString( s, s2 );
	for ( idnum = 0,cl = level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) {
			return idnum;
		}
	}

	trap_SendServerCommand( to - g_entities, va( "print \"User %s is not on the server\n\"", s ) );
	return -1;
}



//----(SA)	added
/*
==============
G_setfog
==============
*/
void G_setfog( char *fogstring ) {
	trap_SetConfigstring( CS_FOGVARS, fogstring );
}

/*
==============
Cmd_Fogswitch_f
==============
*/
void Cmd_Fogswitch_f( void ) {
	G_setfog( ConcatArgs( 1 ) );
}

//----(SA)	end

#ifdef MONEY
struct product_s {
        const char *name;
        int     weapon;
        int     price;
        int     ammoprice;
};

typedef struct product_s product_t;

product_t products[] = 
{
        {"Luger",               WP_LUGER, 50, 1},
        {"Mauser Rifle",        WP_MAUSER, 150, 2},
        {"Thompson",            WP_THOMPSON, 100, 1},
        {"Sten",                WP_STEN, 160, 1},
        {"Dual Colts",          WP_AKIMBO, 100, 2},
        {"Colt",                WP_COLT, 50, 1},
        {"Snooper Rifle",       WP_SNOOPERSCOPE, 200, 2},
        {"MP40",                WP_MP40, 100, 1},
        {"FG42 Paratroop Rifle", WP_FG42, 150, 2},
        {"sp5 pistol",          WP_SILENCER, 50, 2},
        {"Panzerfaust",         WP_PANZERFAUST, 300, 100},
        {"Grenade",             WP_GRENADE_LAUNCHER, 50, 50},
        {"Pineapple",           WP_GRENADE_PINEAPPLE, 50, 50},
        {"Dynamite Weapon",     WP_DYNAMITE, 300, 300},
        {"Venom",               WP_VENOM, 300, 5},
        {"Flamethrower",        WP_FLAMETHROWER, 300, 1},
        {"Sniper Scope",        WP_SNIPERRIFLE, 200, 2},
        {"FG42 Scope",          WP_FG42SCOPE, 200, 2},
        {NULL}
};

int numProducts = sizeof( products ) / sizeof( products[0] ) - 1;

int G_GetAmmoPrice(int weapon) {
        int i;

        for (i=0; i<numProducts; i++) {
                if (products[i].weapon == weapon)
                        return products[i].ammoprice;
        }

        return 0;
}

int G_GetWeaponPrice(int weapon) {
        int i;

        for (i=0; i<numProducts; i++) {
                if (products[i].weapon == weapon)
                        return products[i].price;
        }

        return 0;
}

void Cmd_Buy_f( gentity_t *ent ) {
	char        *name, *amt;
	gitem_t     *it;
        gentity_t       *it_ent;
        int amount;
        int i;
        int money = 0;
        trace_t trace;

        // TODO: players should be able to go below zero 
        // but if they are below zero, at map end, they loose, no matter what

        if (g_gametype.integer != GT_COOP_BATTLE)
                return;

        if (trap_Argc() == 1) {
                trap_SendServerCommand( ent-g_entities, "print \"Usage: \nbuy health <value>\nbuy ammo <value>\nbuy <productname> (see list below)\n\"");
                for (i=0; i<numProducts; i++) {
                        if (products[i].name) 
                                trap_SendServerCommand( ent-g_entities, va("print \"%-20s ^3%3d ^3%3d\n\"", products[i].name, products[i].price, products[i].ammoprice));
                }
                return;
        }

        money = ent->client->ps.persistant[PERS_SCORE];

        /*if ( money <= 0 ) {
                trap_SendServerCommand( ent-g_entities, "print \"Not enough money\n\"");
                return;
        }*/

	// check for an amount (like "give health 30")
	amt = ConcatArgs( 2 );
	amount = atoi( amt );

	name = ConcatArgs( 1 );

	if ( !name || !strlen( name ) ) {
		return;
        }

	if ( Q_stricmpn( name, "stamina", 7 ) == 0 ) {
                        // TODO: check if at max stamina
                        it = BG_FindItem( "stamina" );
                        if ( !it ) {
                                trap_SendServerCommand( ent-g_entities, "print \"Cannot buy an item that does not exist\n\"");
                                return;
                        }    

                        /*if ( money < 20) {
                                trap_SendServerCommand( ent-g_entities, "print \"Not enough money\n\"");
                                return;
                        }*/

                        it_ent = G_Spawn();
                        VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
                        it_ent->classname = it->classname;
                        G_SpawnItem( it_ent, it );
                        FinishSpawningItem( it_ent );
                        memset( &trace, 0, sizeof( trace ) ); 
                        it_ent->active = qtrue;
                        Touch_Item( it_ent, ent, &trace );
                        it_ent->active = qfalse;
                        if ( it_ent->inuse ) {
                                G_FreeEntity( it_ent );
                        } 

                        ent->client->ps.persistant[PERS_SCORE] -= 20;;
                        return;
        }

	if ( Q_stricmpn( name, "health", 6 ) == 0 ) {
		if ( amount ) {
                        /*if (money < amount) {
                                trap_SendServerCommand( ent-g_entities, "print \"Not enough money\n\"");
                                return;
                        }*/

			ent->health += amount;
                        ent->client->ps.persistant[PERS_SCORE] -= amount;
		} else {
                        int cost = ent->client->ps.stats[STAT_MAX_HEALTH] - ent->health;
                        if (money < cost) {
                                ent->health += money;
                                ent->client->ps.persistant[PERS_SCORE] = 0;
                                trap_SendServerCommand( ent-g_entities, "print \"Filled up your health with all your available money\n\"");
                                return;
                        }

			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
                        ent->client->ps.persistant[PERS_SCORE] -= cost;
		}

                return;
	}

	if ( Q_stricmpn( name, "ammo", 4 ) == 0 ) {
                int cost;
                int inclip, maxclip;
                // TODO: play sound
                for (i=0; i<numProducts; i++) {
                        if ( ent->client->ps.weapon == products[i].weapon ) {
                                if ( amount ) {
                                        cost = amount * products[i].ammoprice;
                                        /*if (money < cost) {
                                                trap_SendServerCommand( ent-g_entities, "print \"Not enough money\n\"");
                                                return;
                                        }*/
                                        Add_Ammo( ent, ent->client->ps.weapon, amount, qtrue );
                                } else { // todo if no amount is given, just fill the current clip

                                        inclip  = ent->client->ps.ammoclip[BG_FindClipForWeapon( products[i].weapon )];
                                        maxclip = ammoTable[products[i].weapon].maxclip;

                                        amount = maxclip - inclip;    // max amount that can be moved into the clip
                                        cost = products[i].ammoprice * amount;

                                        /*if (money < cost) {
                                                trap_SendServerCommand( ent-g_entities, "print \"Not enough money\n\"");
                                                return;
                                        }*/

                                        Add_Ammo( ent, ent->client->ps.weapon, amount, qtrue );
                                }
                                ent->client->ps.persistant[PERS_SCORE] -= cost;
                        }
		}

                return;
	}


        for (i=0; i<numProducts; i++) {
                if (products[i].name && Q_stricmp(products[i].name, name) == 0) {
                        it = BG_FindItem( name );
                        if ( !it ) {
                                trap_SendServerCommand( ent-g_entities, "print \"Cannot buy an item that does not exist\n\"");
                                return;
                        }    

                        /*if ( money < products[i].price) {
                                trap_SendServerCommand( ent-g_entities, "print \"Not enough money\n\"");
                                return;
                        }*/

                        if (COM_BitCheck( ent->client->ps.weapons, products[i].weapon )) {
                                trap_SendServerCommand( ent-g_entities, "print \"You already have this item\n\"");
                                return;
                        }

                        it_ent = G_Spawn();
                        VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
                        it_ent->classname = it->classname;
                        G_SpawnItem( it_ent, it );
                        FinishSpawningItem( it_ent );
                        memset( &trace, 0, sizeof( trace ) ); 
                        it_ent->active = qtrue;
                        Touch_Item( it_ent, ent, &trace );
                        it_ent->active = qfalse;
                        if ( it_ent->inuse ) {
                                G_FreeEntity( it_ent );
                        } 

                        ent->client->ps.persistant[PERS_SCORE] -= products[i].price;
                        return;
                }

        }        


}
#endif

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f( gentity_t *ent ) {
	char        *name, *amt;
	gitem_t     *it;
	int i;
	qboolean give_all;
	gentity_t       *it_ent;
	trace_t trace;
	int amount;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	//----(SA)	check for an amount (like "give health 30")
	amt = ConcatArgs( 2 );
	amount = atoi( amt );
	//----(SA)	end

	name = ConcatArgs( 1 );

	if ( !name || !strlen( name ) ) {
		return;
	}

	if ( Q_stricmp( name, "all" ) == 0 ) {
		give_all = qtrue;
	} else {
		give_all = qfalse;
	}


	if ( give_all || Q_stricmpn( name, "health", 6 ) == 0 ) {
		//----(SA)	modified
		if ( amount ) {
			ent->health += amount;
		} else {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || Q_stricmp( name, "weapons" ) == 0 ) {
		//ent->client->ps.weapons[0] = (1 << (WP_MONSTER_ATTACK1)) - 1 - (1<<WP_NONE);	//----(SA)	gives the cross now as well

		//(SA) we really don't want to give anything beyond WP_DYNAMITE
		for ( i = 0; i <= WP_DYNAMITE; i++ )
			COM_BitSet( ent->client->ps.weapons, i );

//		for (i=0; i<WP_NUM_WEAPONS; i++) {
//			switch (i) {
//			case WP_MONSTER_ATTACK1:
//			case WP_MONSTER_ATTACK2:
//			case WP_MONSTER_ATTACK3:
//			case WP_NONE:
//				break;
//			default:
//				COM_BitSet( ent->client->ps.weapons, i );
//			}
//		}
		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || Q_stricmp( name, "holdable" ) == 0 ) {
		ent->client->ps.stats[STAT_HOLDABLE_ITEM] = ( 1 << ( HI_BOOK3 - 1 ) ) - 1 - ( 1 << HI_NONE );
		for ( i = 1 ; i <= HI_BOOK3 ; i++ ) {
			ent->client->ps.holdable[i] = 10;
		}

		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || Q_stricmpn( name, "ammo", 4 ) == 0 ) {
		if ( amount ) {
			if ( ent->client->ps.weapon ) {
				Add_Ammo( ent, ent->client->ps.weapon, amount, qtrue );
			}
		} else {
			for ( i = 1 ; i < WP_MONSTER_ATTACK1 ; i++ )
				Add_Ammo( ent, i, 999, qtrue );
		}

		if ( !give_all ) {
			return;
		}
	}

	//	"give allammo <n>" allows you to give a specific amount of ammo to /all/ weapons while
	//	allowing "give ammo <n>" to only give to the selected weap.
	if ( Q_stricmpn( name, "allammo", 7 ) == 0 && amount ) {
		for ( i = 1 ; i < WP_MONSTER_ATTACK1 ; i++ )
			Add_Ammo( ent, i, amount, qtrue );

		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || Q_stricmpn( name, "armor", 5 ) == 0 ) {
                if ( amount ) {
                        ent->client->ps.stats[STAT_ARMOR] += amount;
                } else {
                        ent->client->ps.stats[STAT_ARMOR] = 100;
                }
		if ( !give_all ) {
			return;
		}
	}

	//---- (SA) Wolf keys
	if ( give_all || Q_stricmp( name, "keys" ) == 0 ) {
		ent->client->ps.stats[STAT_KEYS] = ( 1 << KEY_NUM_KEYS ) - 2;
		if ( !give_all ) {
			return;
		}
	}
	//---- (SA) end

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem( name );
		if ( !it ) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		it_ent->classname = it->classname;
		G_SpawnItem( it_ent, it );
		FinishSpawningItem( it_ent );
		memset( &trace, 0, sizeof( trace ) );
		it_ent->active = qtrue;
		Touch_Item( it_ent, ent, &trace );
		it_ent->active = qfalse;
		if ( it_ent->inuse ) {
			G_FreeEntity( it_ent );
		}
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f( gentity_t *ent ) {
	char    *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if ( !( ent->flags & FL_GODMODE ) ) {
		msg = "godmode OFF\n";
	} else {
		msg = "godmode ON\n";
	}

	trap_SendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}

/*
==================
Cmd_Nofatigue_f

Sets client to nofatigue

argv(0) nofatigue
==================
*/

void Cmd_Nofatigue_f( gentity_t *ent ) {
	char    *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOFATIGUE;
	if ( !( ent->flags & FL_NOFATIGUE ) ) {
		msg = "nofatigue OFF\n";
	} else {
		msg = "nofatigue ON\n";
	}

	trap_SendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	char    *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if ( !( ent->flags & FL_NOTARGET ) ) {
		msg = "notarget OFF\n";
	} else {
		msg = "notarget ON\n";
	}

	trap_SendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}


void Cmd_SetCoopSpawn_f( gentity_t *ent ) {
        gentity_t *groundEnt = &g_entities[ent->client->ps.groundEntityNum];
        
        if ( g_gametype.integer == GT_SINGLE_PLAYER )
                return;

        if ( ent->client->ps.pm_flags & PMF_LIMBO )
                return;

        if ( ent->health <= 0 )
                return;

        // don't save spawns when not on the ground
        if ( ent->s.groundEntityNum == ENTITYNUM_NONE)
                return;

        // the same counts for a mover
        if ( groundEnt->s.eType == ET_MOVER ) {
                return;
        }

        // if automatic spawnpoints are enabled, don't allow manual saving
        if (g_spawnpoints.integer == 0) {
                trap_SendServerCommand( ent - g_entities, "cp \"Can't save spawnpoint, \nautosave is activated on the server\n\"" );
                return;
        }

        // if spawnpoint triggers are enabled, don't allow manual saving
        if (g_spawnpoints.integer == 2) {
                trap_SendServerCommand( ent - g_entities, "cp \"Can't save spawnpoint.\n\"" );
                return;
        }

        if ( ent->client->ps.persistant[PERS_SPAWNPOINTS_LEFT] < 0 && g_maxspawnpoints.integer != 0 ) {
                trap_SendServerCommand( ent - g_entities, "cp \"Can't save spawnpoint, \nyour personal limit is reached.\n\"");
                return;
        }

        if (ent->client->ps.lastcoopSpawnSaveTime <= level.time)
        {

                VectorCopy(ent->client->ps.origin, ent->client->coopSpawnPointOrigin);
                VectorCopy(ent->client->ps.viewangles, ent->client->coopSpawnPointAngles);
                ent->client->hasCoopSpawn = qtrue;

                if ( g_maxspawnpoints.integer )
                        ent->client->ps.persistant[PERS_SPAWNPOINTS_LEFT]--;

                trap_SendServerCommand( ent - g_entities, "cp \"Saved current position as \nnext spawnpoint.\n\"" );
                ent->client->ps.lastcoopSpawnSaveTime = level.time + 5000;
        }
        else
        {
                int seconds = (ent->client->ps.lastcoopSpawnSaveTime - level.time) / 1000;
                trap_SendServerCommand( ent - g_entities, va( "cp \"You must wait %d seconds before saving \na new spawnpoint\n\"", seconds ) );
        }
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent ) {
	char    *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	if ( ent->client->noclip ) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

	trap_SendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent ) {
	if ( !CheatsOk( ent ) ) {
		return;
	}

	// doesn't work in single player
	if ( g_gametype.integer != 0 ) {
		trap_SendServerCommand( ent - g_entities,
								"print \"Must be in g_gametype 0 for levelshot\n\"" );
		return;
	}

	BeginIntermission();
	trap_SendServerCommand( ent - g_entities, "clientLevelShot" );
}


/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}

//	if(reloading)	// waiting to start map, or exiting to next map
	if ( g_reloading.integer ) {
		return;
	}

#ifdef MONEY
        if (g_gametype.integer == GT_COOP_BATTLE)
                ent->client->ps.persistant[PERS_SCORE] -= ent->health;
#endif

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
	player_die( ent, ent, ent, 100000, MOD_SUICIDE );
}


/*
=================
SetTeam
=================
*/
void SetTeam( gentity_t *ent, char *s ) {
	int team, oldTeam;
	gclient_t           *client;
	int clientNum;
	spectatorState_t specState;
	int specClient;

	// L0 - No joining if team is locked
	if (g_gamelocked.integer) {		
		trap_SendServerCommand( ent-g_entities, va( "cp \"You can't join because game is locked^1!\n\"2") );
	return;
	}
	// end

	//
	// see what change is requested
	//
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;

	specState = SPECTATOR_NOT;
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" )  ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if ( !Q_stricmp( s, "follow1" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if ( !Q_stricmp( s, "follow2" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	} else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}

	if ( g_maxGameClients.integer > 0 &&
				level.numNonSpectatorClients >= g_maxGameClients.integer ) {
		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	oldTeam = client->sess.sessionTeam;
	if ( team == oldTeam && team != TEAM_SPECTATOR ) {
		return;
	}

	//
	// execute the team change
	//

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if ( oldTeam != TEAM_SPECTATOR ) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die( ent, ent, ent, 100000, MOD_SUICIDE );

	}
	// they go to the end of the line for tournements
	if ( team == TEAM_SPECTATOR ) {
		client->sess.spectatorTime = level.time;
                if (ent->client->ps.persistant[PERS_SCORE] >= 0)
                        ent->client->ps.persistant[PERS_SCORE] = -1000;
	} else {
                // fretn: if you have a very low score, and you go to spectator and back to the game you get one point
                // this can be abused to reset your score in case its bad, need to fix this
#ifdef MONEY
                if (g_gametype.integer != GT_COOP_BATTLE) {
                        ent->client->ps.persistant[PERS_SCORE] = 1;
                } else {
                        ent->client->ps.persistant[PERS_SCORE] = 0;
                }
#else
                ent->client->ps.persistant[PERS_SCORE] = 1;
#endif
        }

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	if ( team == TEAM_RED ) {
		trap_SendServerCommand( -1, va( "cp \"%s" S_COLOR_WHITE " joined the red team.\n\"",
										client->pers.netname ) );
	} else if ( team == TEAM_BLUE ) {
		trap_SendServerCommand( -1, va( "cp \"%s" S_COLOR_WHITE " joined the blue team.\n\"",
										client->pers.netname ) );
	} else if ( team == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		trap_SendServerCommand( -1, va( "cp \"%s" S_COLOR_WHITE " joined the spectators.\n\"",
										client->pers.netname ) );
	} else if ( team == TEAM_FREE ) {
		trap_SendServerCommand( -1, va( "cp \"%s" S_COLOR_WHITE " joined the battle.\n\"",
										client->pers.netname ) );
	}

	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );

	ClientBegin( clientNum );
}

// DHM - Nerve
/*
=================
SetWolfData
=================
*/
void SetWolfData( gentity_t *ent, char *ptype, char *weap, char *pistol, char *grenade, char *skinnum ) {   // DHM - Nerve
	gclient_t   *client;

	client = ent->client;

	client->sess.playerType = atoi( ptype );
	client->sess.playerWeapon = atoi( weap );
	client->sess.playerPistol = atoi( pistol );
	client->sess.playerItem = atoi( grenade );
	client->sess.playerSkin = atoi( skinnum );
}
// dhm - end

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing( gentity_t *ent ) {
	ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
	int oldTeam;
	char s[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		oldTeam = ent->client->sess.sessionTeam;
		switch ( oldTeam ) {
		case TEAM_BLUE:
			trap_SendServerCommand( ent - g_entities, "print \"Blue team\n\"" );
			break;
		case TEAM_RED:
			trap_SendServerCommand( ent - g_entities, "print \"Red team\n\"" );
			break;
		case TEAM_FREE:
			trap_SendServerCommand( ent - g_entities, "print \"Free team\n\"" );
			break;
		case TEAM_SPECTATOR:
			trap_SendServerCommand( ent - g_entities, "print \"Spectator team\n\"" );
			break;
		}
		return;
	}

	trap_Argv( 1, s, sizeof( s ) );

	SetTeam( ent, s );
}


/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
	int i;
	char arg[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		return;
	}

	// can't follow self
	if ( &level.clients[ i ] == ent->client ) {
		return;
	}

	// can't follow another spectator
	if ( level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}

	// first set them to spectator
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam( ent, "spectator" );
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int clientnum;
	int original;

	// first set them to spectator
	if ( ( ent->client->sess.spectatorState == SPECTATOR_NOT ) && ( !( ent->client->ps.pm_flags & PMF_LIMBO ) ) ) { // JPW NERVE for limbo state
		SetTeam( ent, "spectator" );
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;
	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients ) {
			clientnum = 0;
		}
		if ( clientnum < 0 ) {
			clientnum = level.maxclients - 1;
		}

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator
		if ( level.clients[ clientnum ].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		// L0 - Only follow players when spectacting.
		// Dunno if it's a general idea to be able to follow bots as well 
		// but if it is, then following needs to get fixed as spectator crashes
		// when following certain bots...
		if (g_entities[clientnum].r.svFlags & SVF_BOT) {
			continue;
		} // end

// JPW NERVE -- couple extra checks for limbo mode
		if ( ent->client->ps.pm_flags & PMF_LIMBO ) {
			if ( level.clients[clientnum].ps.pm_flags & PMF_LIMBO ) {
				continue;
			}
			if ( level.clients[clientnum].sess.sessionTeam != ent->client->sess.sessionTeam ) {
				continue;
			}
		}
// jpw

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}


/*
==================
G_Say
==================
*/
#define MAX_SAY_TEXT    150

#define SAY_ALL     0
#define SAY_TEAM    1
#define SAY_TELL    2
#define SAY_LIMBO   3           // NERVE - SMF

void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message ) { // removed static so it would link
	if ( !other ) {
		return;
	}
	if ( !other->inuse ) {
		return;
	}
	if ( !other->client ) {
		return;
	}
	if ( ( mode == SAY_TEAM || mode == SAY_LIMBO )  && !OnSameTeam( ent, other ) ) {
		return;
	}

	// NERVE - SMF
	if ( mode == SAY_LIMBO ) {
		trap_SendServerCommand( other - g_entities, va( "%s \"%s%c%c%s\"",
														"lchat", name, Q_COLOR_ESCAPE, color, message ) );
	}
	// -NERVE - SMF
	else {
		trap_SendServerCommand( other - g_entities, va( "%s \"%s%c%c%s\"",
														mode == SAY_TEAM ? "tchat" : "chat",
														name, Q_COLOR_ESCAPE, color, message ) );
	}
}

void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	int j;
	gentity_t   *other;
	int color;
	char name[64];
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT];
	char location[64];
	// L0 - Admin stuff
	char *tag;	
	char arg[MAX_SAY_TEXT]; // ! & ? 
	char cmd1[128];
	char cmd2[128];
	char cmd3[128];	
	
	// Admin commands
	Q_strncpyz ( text, chatText, sizeof( text ) );
	if ( !ent->client->sess.admin == ADM_NONE ) {
		// Command
		if ( text[0] == '!' ){
			ParseAdmStr(text, cmd1, arg);	
			ParseAdmStr(arg, cmd2, cmd3);
			Q_strncpyz ( ent->client->pers.cmd1, cmd1, sizeof( ent->client->pers.cmd1 ) );
			Q_strncpyz ( ent->client->pers.cmd2, cmd2, sizeof( ent->client->pers.cmd2 ) );
			Q_strncpyz ( ent->client->pers.cmd3, cmd3, sizeof( ent->client->pers.cmd3 ) );
			cmds_admin("!", ent);
			return;
		// Help
		} else if ( text[0] == '?' ){
			ParseAdmStr(text, cmd1, arg);	
			ParseAdmStr(arg, cmd2, cmd3);
			Q_strncpyz ( ent->client->pers.cmd1, cmd1, sizeof( ent->client->pers.cmd1 ) );
			Q_strncpyz ( ent->client->pers.cmd2, cmd2, sizeof( ent->client->pers.cmd2 ) );
			Q_strncpyz ( ent->client->pers.cmd3, cmd3, sizeof( ent->client->pers.cmd3 ) );
			cmds_admin("?", ent);
			return;
		}
	}   // end

	// L0 - Ignored players..
	if (ent->client->sess.ignored) {
		trap_SendServerCommand(ent-g_entities, "cp \"You are ^1Ignored^7! Chat cancelled..\"2");
	return;
	} // End

	// L0 - Deal with Admin tags..
	if (!ent->client->sess.incognito) {
		if (ent->client->sess.admin == ADM_MEM) 
			tag = va("^7(%s^7)", a1_tag.string);
		else if (ent->client->sess.admin == ADM_MED)
			tag = va("^7(%s^7)", a2_tag.string);
		else if (ent->client->sess.admin == ADM_FULL)
			tag = va("^7(%s^7)", a3_tag.string);
		else
			tag = "";
	// If Admin is hidden or not an admin at all..no tag.
	} else {
		tag = ""; 
	} // End

	if ( g_gametype.integer <= GT_SINGLE_PLAYER && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	switch ( mode ) {
	default:
	case SAY_ALL:
			G_LogPrintf( "say: %s%s: %s\n", ent->client->pers.netname, tag, chatText ); // L0 - Added admin tag for log print..
			Com_sprintf( name, sizeof( name ), "%s%s%c%c: ", ent->client->pers.netname, tag, Q_COLOR_ESCAPE, COLOR_WHITE ); // L0 - Added admin tag..
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		G_LogPrintf( "sayteam: %s: %s\n", ent->client->pers.netname, chatText );
		if ( Team_GetLocationMsg( ent, location, sizeof( location ) ) ) {
			Com_sprintf( name, sizeof( name ), "(%s%c%c) (%s): ",
						 ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location );
		} else {
			Com_sprintf( name, sizeof( name ), "(%s%c%c): ",
						 ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		}
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		Com_sprintf( name, sizeof( name ), "[%s%c%c]: ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_MAGENTA;
		break;
		// NERVE - SMF
	case SAY_LIMBO:
		G_LogPrintf( "sayteam: %s: %s\n", ent->client->pers.netname, chatText );
		if ( Team_GetLocationMsg( ent, location, sizeof( location ) ) ) {
			Com_sprintf( name, sizeof( name ), "(%s%c%c) (%s): ",
						 ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location );
		} else {
			Com_sprintf( name, sizeof( name ), "(%s%c%c): ",
						 ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		}
		color = COLOR_CYAN;
		break;
		// -NERVE - SMF
	}

	Q_strncpyz( text, chatText, sizeof( text ) );

	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "%s%s\n", name, text );
	}

	// send it to all the apropriate clients
	for ( j = 0; j < level.maxclients; j++ ) {
		other = &g_entities[j];
		G_SayTo( ent, other, mode, color, name, text );
	}
}


/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( gentity_t *ent, int mode, qboolean arg0 ) {
	char        *p;

	if ( trap_Argc() < 2 && !arg0 ) {
		return;
	}

	if ( arg0 ) {
		p = ConcatArgs( 0 );
	} else
	{
		p = ConcatArgs( 1 );
	}

	G_Say( ent, NULL, mode, p );
}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f( gentity_t *ent ) {
	int targetNum;
	gentity_t   *target;
	char        *p;
	char arg[MAX_TOKEN_CHARS];

	// L0 - Ignored players..
	if (ent->client->sess.ignored) {
		trap_SendServerCommand(ent-g_entities, "cp \"You are ^1Ignored^7! Tell cancelled..\"2");
	return;
	} // End

	if ( trap_Argc() < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	p = ConcatArgs( 2 );

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p );
	G_Say( ent, target, SAY_TELL, p );
	G_Say( ent, ent, SAY_TELL, p );
}

// NERVE - SMF
static void G_VoiceTo( gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly ) {
        int color;
        char *cmd;

        if ( !other ) {
                return;
        }    
        if ( !other->inuse ) {
                return;
        }    
        if ( !other->client ) {
                return;
        }    
        /*if ( mode == SAY_TEAM && !OnSameTeam( ent, other ) ) {
                return;
        }*/
        // no chatting to players in sp
        if ( ( g_gametype.integer == GT_SINGLE_PLAYER ) ) {
                return;
        }    

        if ( mode == SAY_TEAM ) {
                color = COLOR_CYAN;
                cmd = "vtchat";
        } else if ( mode == SAY_TELL )     {    
                color = COLOR_MAGENTA;
                cmd = "vtell";
        } else {
                color = COLOR_GREEN;
                cmd = "vchat";
        }    

        trap_SendServerCommand( other - g_entities, va( "%s %d %d %d %s %i %i %i", cmd, voiceonly, ent->s.number, color, id,
                                                                                                        (int)ent->s.pos.trBase[0], (int)ent->s.pos.trBase[1], (int)ent->s.pos.trBase[2] ) ); 
}
void G_Voice( gentity_t *ent, gentity_t *target, int mode, const char *id, qboolean voiceonly ) {
        int j;
        gentity_t   *other;

		// L0 - Ignored players..
		if (ent->client->sess.ignored) {
			trap_SendServerCommand(ent-g_entities, "cp \"You are ^1Ignored^7! Voice cancelled..\"2");
		return;
		} // End

        /*if ( g_gametype.integer < GT_TEAM && mode == SAY_TEAM ) {
                mode = SAY_ALL;
        }
        */

        // DHM - Nerve :: Don't allow excessive spamming of voice chats
        ent->voiceChatSquelch -= ( level.time - ent->voiceChatPreviousTime );
        ent->voiceChatPreviousTime = level.time;

        if ( ent->voiceChatSquelch < 0 ) {
                ent->voiceChatSquelch = 0; 
        }    

        if ( ent->voiceChatSquelch >= 30000 ) {
                trap_SendServerCommand( ent - g_entities, "print \"^1Spam Protection^7: VoiceChat ignored\n\"" );
                return;
        }    

        /*   
        if ( g_voiceChatsAllowed.integer ) {
                ent->voiceChatSquelch += ( 34000 / g_voiceChatsAllowed.integer );
        } else {
                return;
        }*/
        ent->voiceChatSquelch += ( 34000 / 4 ); 
        // dhm

        if ( target ) {
                G_VoiceTo( ent, target, mode, id, voiceonly );
                return;
        }    

        // echo the text to the console
        if ( g_dedicated.integer ) {
                G_Printf( "voice: %s %s\n", ent->client->pers.netname, id );
        }    

        // send it to all the apropriate clients
        for ( j = 0; j < level.maxclients; j++ ) {
                other = &g_entities[j];
                G_VoiceTo( ent, other, mode, id, voiceonly );
        }    
}

/*
==================
Cmd_Voice_f
==================
*/
static void Cmd_Voice_f( gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly ) {
        char        *p;  

        if ( trap_Argc() < 2 && !arg0 ) {
                return;
        }    

        if ( arg0 ) {
                p = ConcatArgs( 0 ); 
        } else 
        {    
                p = ConcatArgs( 1 ); 
        }    

        G_Voice( ent, NULL, mode, p, voiceonly );
}


static char *gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

void Cmd_GameCommand_f( gentity_t *ent ) {
	int player;
	int order;
	char str[MAX_TOKEN_CHARS];

	trap_Argv( 1, str, sizeof( str ) );
	player = atoi( str );
	trap_Argv( 2, str, sizeof( str ) );
	order = atoi( str );

	if ( player < 0 || player >= MAX_CLIENTS ) {
		return;
	}
	if ( order < 0 || order > sizeof( gc_orders ) / sizeof( char * ) ) {
		return;
	}
	G_Say( ent, &g_entities[player], SAY_TELL, gc_orders[order] );
	G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	trap_SendServerCommand( ent - g_entities, va( "print \"%s\n\"", vtos( ent->s.origin ) ) );
}

static const char *gameNames[] = {
        "Battle",
        "Speedrun",
        "Cooperative",
        "Single Player"
};

/*
==================
L0 - Wish it would be like in php and i wouldn't need to bother with this..
==================
*/
int is_numeric(const char *p) {
     if (*p) {
          char c;
          while ((c=*p++)) {
                if (!isdigit(c)) return 0;
          }
          return 1;
      }
return 0;
}

/*
==================
Cmd_CallVote_f
==================
*/
void Cmd_CallVote_f( gentity_t *ent ) {
	int i;
	char arg1[MAX_STRING_TOKENS];
	char arg2[MAX_STRING_TOKENS];
    char cleanName[64];
	char *check; // L0 - callvote exploit fix

	// L0 - Ignored players..
	if (ent->client->sess.ignored) {
		trap_SendServerCommand(ent-g_entities, "cp \"You are ^1Ignored^7! ^7Vote refused..\"2");
	return;
	} // End

    if ( g_gametype.integer == GT_SINGLE_PLAYER) {
		trap_SendServerCommand( ent - g_entities, "print \"Voting not allowed in single player.\n\"" );
                return;
        }

	if ( !g_allowVote.integer ) {
		trap_SendServerCommand( ent - g_entities, "print \"Voting not allowed here.\n\"" );
		return;
	}

	if ( level.voteTime ) {
		trap_SendServerCommand( ent - g_entities, "print \"A vote is already in progress.\n\"" );
		return;
	}

	// L0 - To cope with spam voting I removed hardcoded MAX_VOTE_COUNT as well as fixed to "more" so count is right.
	if ( ent->client->pers.voteCount > g_votesPerUser.integer ) { 
		trap_SendServerCommand( ent - g_entities, "print \"You have called the maximum number of votes.\n\"" );
		return;
	} // End

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	trap_Argv( 2, arg2, sizeof( arg2 ) );

        if ( strchr( arg1, ';' ) || strchr( arg2, ';' ) ) {
                trap_SendServerCommand( ent - g_entities, "print \"Invalid vote string.\n\"" );
                return;
        } 
	
	// L0 - callvote exploit fix - what escapes above is dealed with here
	for( check = arg2; *check; ++check) {
		switch(*check) {
			case '\n':
			case '\r':
			case '"':
			case ';':
				trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
				return;
			break;
		}
	} // end

	if ( !Q_stricmp( arg1, "map_restart" ) ) {
	} else if ( !Q_stricmp( arg1, "map" ) ) {
	} else if ( !Q_stricmp( arg1, "g_gametype" ) ) {
	} else if ( !Q_stricmp( arg1, "kick" ) ) {
	} else if ( !Q_stricmp( arg1, "nextmap" ) ) {
	// L0 - ( not wrapping this in _ADMIN flag as there's no need to )
	} else if ( !Q_stricmp( arg1, "clientkick" ) ) {
	} else if ( !Q_stricmp( arg1, "?" ) ) {
	// Once admins get a part of the project this will be dumped in
#ifdef _ADMINS
	} else if ( !Q_stricmp( arg1, "ignore" ) ) {
	} else if ( !Q_stricmp( arg1, "unignore" ) ) {
#endif
	// L0 : Enhancements - view.php?id=54
	} else if ( !Q_stricmp( arg1, "g_gameskill" ) ) {
	} else if ( !Q_stricmp( arg1, "g_reinforce" ) ) {
    } else if ( !Q_stricmp( arg1, "g_freeze" ) ) {
	// End
	} else {
#ifdef _ADMINS
		trap_SendServerCommand( ent - g_entities, "print \"Invalid vote string.\nFollowing are valid: map_restart, map, g_gametype, kick, clientkick, ignore, unignore, ? <question>, nextmap, g_gameskill, g_reinforce, g_freeze\"" ); // L0 - Enhanced 
#else
		trap_SendServerCommand( ent - g_entities, "print \"Invalid vote string.\nFollowing are valid: map_restart, map, g_gametype, kick, clientkick, ? <question>, nextmap, g_gameskill, g_reinforce, g_freeze\"" ); // L0 - Enhanced 
#endif
		return;
	}

	if ( !Q_stricmp( arg1, "nextmap" ) ) {
		char s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof( s ) );
		if ( !*s ) {
			trap_SendServerCommand( ent - g_entities, "print \"nextmap not set.\n\"" );
			return;
		}
		Com_sprintf( level.voteString, sizeof( level.voteString ), "coopmap %s", s );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	} else if ( !Q_stricmp( arg1, "map") ) {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "coopmap %s", arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
// L0 - Callvotes enhancements 
	// Poll option
	} else if ( !Q_stricmp( arg1, "?" ) ) {
		char	*s;
		s = ConcatArgs(2); 		

		if (*s) {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "\"Poll:\"", s);  
			trap_SendServerCommand( -1, va("chat \"^2Poll: ^7%s\n\"", s )); // Temporarily set like this as vote draw needs to be fixed to look nicer so on TODO list..
		} 		
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s%s", level.voteString, s ); 
#ifdef _ADMINS
	// Ignore
	} else if ( !Q_stricmp( arg1, "ignore" ) ) {
		int i, num = MAX_CLIENTS;
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (level.clients[i].pers.connected != CON_CONNECTED && g_entities[i].r.svFlags & SVF_BOT) // Not a bot!
				continue;

			Q_strncpyz(cleanName, level.clients[i].pers.netname, sizeof(cleanName));
			Q_CleanStr(cleanName);
			if (!Q_stricmp(cleanName, arg2))
				num = i;
		}
		Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "ignore %s", level.clients[num].pers.netname);
		if (level.clients[num].sess.ignored) {
			trap_SendServerCommand(ent-g_entities, va("print \"^3Notice: ^7%s ^7is already Ignored^3!\n\"", level.clients[num].pers.netname));
		return;
		} else if (num != MAX_CLIENTS){
			Com_sprintf(level.voteString, sizeof(level.voteString), "ignore \"%d\"", num);
		}else{
			trap_SendServerCommand(ent-g_entities, "print \"Client not on server.\n\"");
			return;
		}
	// Unignore
	} else if ( !Q_stricmp( arg1, "unignore" ) ) {
		int i, num = MAX_CLIENTS;
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (level.clients[i].pers.connected != CON_CONNECTED && g_entities[i].r.svFlags & SVF_BOT) // Not a bot!
				continue;

			Q_strncpyz(cleanName, level.clients[i].pers.netname, sizeof(cleanName));
			Q_CleanStr(cleanName);
			if (!Q_stricmp(cleanName, arg2))
				num = i;
		}
		Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "unignore %s", level.clients[num].pers.netname);
		if (!level.clients[num].sess.ignored) {
			trap_SendServerCommand(ent-g_entities, va("print \"^3Notice: ^7%s ^7is already unignored^3!\n\"", level.clients[num].pers.netname));
		return;
		} else if (num != MAX_CLIENTS){
			Com_sprintf(level.voteString, sizeof(level.voteString), "unignore \"%d\"", num);
		}else{
			trap_SendServerCommand(ent-g_entities, "print \"Client not on server.\n\"");
			return;
		}
#endif
	// g_gameSkill
	} else if ( !Q_stricmp( arg1, "g_gameskill" ) ) {	
		i = atoi(arg2);
        if ( !is_numeric(arg2) || i < 0 || i > 3) {
			trap_SendServerCommand( ent - g_entities, "print \"Invalid argument for g_gameskill.\n\"" );
        return;
        }               
		//Com_sprintf( level.voteString, sizeof( level.voteString ), "g_gameskill %s; map_restart 5", arg2);  // Use map restart.          
		Com_sprintf( level.voteString, sizeof( level.voteString ), "g_gameskill %s; map_restart 5", arg2);  // Switch in game.          
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	// g_reinforce
	} else if ( !Q_stricmp( arg1, "g_reinforce" ) ) {	
		i = atoi(arg2);
        if ( !is_numeric(arg2) || i < 0 || i > 2) {
			trap_SendServerCommand( ent - g_entities, "print \"Invalid argument for g_reinforce.\n\"" );
        return;
        }               
		Com_sprintf( level.voteString, sizeof( level.voteString ), "g_reinforce %s; map_restart 5", arg2);  // Use map restart.          
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	// g_freeze
	} else if ( !Q_stricmp( arg1, "g_freeze" ) ) {	
		i = atoi(arg2);
        if ( !is_numeric(arg2) || i < 0 || i > 1) {
			trap_SendServerCommand( ent - g_entities, "print \"Invalid argument for g_freeze.\n\"" );
        return;
        }               
		Com_sprintf( level.voteString, sizeof( level.voteString ), "g_freeze %s", arg2);  // Kick starts instantly..
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
// L0 - Callvotes enhancements ends here
    } else if ( !Q_stricmp( arg1, "g_gametype" ) ) {
		i = atoi(arg2);
        if ( i >= GT_SINGLE_PLAYER || i < 0 ) {
			trap_SendServerCommand( ent - g_entities, "print \"Invalid gametype.\n\"" );
        return;
        }
        Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %d; map_restart 5", arg1, i ); 
        Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s", arg1, gameNames[i] );
    } else if ( !Q_stricmp( arg1,"kick" ) ) {
        int i,kicknum = MAX_CLIENTS;
			for ( i = 0; i < MAX_CLIENTS; i++ ) {
				if ( level.clients[i].pers.connected != CON_CONNECTED ) {
					continue;
                }
				// strip the color crap out
				Q_strncpyz( cleanName, level.clients[i].pers.netname, sizeof( cleanName ) );
                Q_CleanStr( cleanName );
                if ( !Q_stricmp( cleanName, arg2 ) ) {
					kicknum = i;
                }
            }
        //Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "kick %s", level.clients[kicknum].pers.netname );
        if ( kicknum != MAX_CLIENTS ) { // found a client # to kick, so override votestring with better one
			Com_sprintf( level.voteString, sizeof( level.voteString ),"clientkick \"%d\"",kicknum );
        } else { // if it can't do a name match, don't allow kick (to prevent votekick text spam wars)
			trap_SendServerCommand( ent - g_entities, "print \"Client not on server.\n\"" );
        return;
        }
	} else {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );
	}

	trap_SendServerCommand( -1, va( "print \"%s ^7called a vote.\n\"", ent->client->pers.netname ) );

	// start the voting, the caller autoamtically votes yes
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;
	// L0 - Ehm, and where's the user vote count? Doh..
	ent->client->pers.voteCount++;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		level.clients[i].ps.eFlags &= ~EF_VOTED;
	}
	ent->client->ps.eFlags |= EF_VOTED;

	trap_SetConfigstring( CS_VOTE_TIME, va( "%i", level.voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING, level.voteString );
	trap_SetConfigstring( CS_VOTE_YES, va( "%i", level.voteYes ) );
	trap_SetConfigstring( CS_VOTE_NO, va( "%i", level.voteNo ) );
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent ) {
	char msg[64];

	if ( !level.voteTime ) {
		trap_SendServerCommand( ent - g_entities, "print \"No vote in progress.\n\"" );
		return;
	}
	if ( ent->client->ps.eFlags & EF_VOTED ) {
		trap_SendServerCommand( ent - g_entities, "print \"Vote already cast.\n\"" );
		return;
	}

	trap_SendServerCommand( ent - g_entities, "print \"Vote cast.\n\"" );

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.voteYes++;
		trap_SetConfigstring( CS_VOTE_YES, va( "%i", level.voteYes ) );
	} else {
		level.voteNo++;
		trap_SetConfigstring( CS_VOTE_NO, va( "%i", level.voteNo ) );
	}

	// a majority will be determined in G_CheckVote, which will also account
	// for players entering or leaving
}


qboolean G_canPickupMelee( gentity_t *ent ) {

	if ( !( ent->client ) ) {
		return qfalse;  // hmm, shouldn't be too likely...

	}
	if ( !( ent->s.weapon ) ) {  // no weap, go ahead
		return qtrue;
	}

	if ( ent->client->ps.weaponstate == WEAPON_RELOADING ) {
		return qfalse;
	}

//	if( WEAPS_ONE_HANDED & (1<<(ent->s.weapon)) )
	if ( WEAPS_ONE_HANDED & ( 1 << ( ent->client->pers.cmd.weapon ) ) ) {
		return qtrue;
	}

	return qfalse;
}




/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t origin, angles;
	char buffer[MAX_TOKEN_CHARS];
	int i;

	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent - g_entities, "print \"Cheats are not enabled on this server.\n\"" );
		return;
	}
	if ( trap_Argc() != 5 ) {
		trap_SendServerCommand( ent - g_entities, "print \"usage: setviewpos x y z yaw\n\"" );
		return;
	}

	VectorClear( angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	trap_Argv( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	TeleportPlayer( ent, origin, angles );
}

/*
=================
Cmd_Teleport_f
=================
 - if pointing at a player, teleport current player to other players spawnpoint
 - if not pointing at a player, teleport to a random spawnpoint
 - can only be activated once every 30 seconds
 - will be usefull for cases where players are stuck behind closed doors, elevators
*/
void Cmd_Teleport_f( gentity_t *ent ) {
        vec3_t start, end, forward, right, up;
        trace_t trace;
        gentity_t *newent = NULL;
        qboolean foundPlayer = qtrue;
        gentity_t *clients[MAX_COOP_CLIENTS];

        if ( g_gametype.integer == GT_COOP_SPEEDRUN || g_gametype.integer == GT_SINGLE_PLAYER )
                return;

        // FIXME: in speedrun, we need to teleport to somewhere else ..

        // not for dead people !
        if ( ent->client->ps.eFlags & EF_DEAD )
                return;

        if ( ent->client->ps.pm_flags & PMF_LIMBO )
                return;

        if ( !(ent->client->ps.lastTeleportTime <= level.time) ) {
                int seconds = (ent->client->ps.lastTeleportTime - level.time) / 1000;
                trap_SendServerCommand( ent - g_entities, va( "cp \"You must wait %d seconds before \nteleporting again\n\"", seconds ) );
                return;
        }

        // first check if we are pointing at a player:

        AngleVectors( ent->client->ps.viewangles, forward, right, up );

        VectorCopy( ent->r.currentOrigin, start );

        start[2] += 24;
        VectorMA( start, 17, forward, start );

        VectorCopy( start, end );
        VectorMA( end, 8192, forward, end );

        trap_Trace( &trace, start, NULL, NULL, end, ent->s.number, MASK_SOLID | MASK_MISSILESHOT | CONTENTS_BODY );

        newent = &g_entities[ trace.entityNum ];

        if ( trace.fraction >= 1.0 )
                foundPlayer = qfalse;

        if ( !newent ) 
                foundPlayer = qfalse;

        if ( !newent->client )
                foundPlayer = qfalse;

        if ( newent->r.svFlags & SVF_CASTAI )
                foundPlayer = qfalse;

        // todo: teleporting should 'cost' something
        if ( foundPlayer ) { // teleport to that player his personal spawnpoint
                if ( newent->client->hasCoopSpawn )
                        TeleportPlayer( ent, newent->client->coopSpawnPointOrigin, newent->client->coopSpawnPointAngles );
                else
                        newent = NULL;
        } else { // teleport to a random player his personal spawnpoint
                int count = 0, selection = 0, i = 0;

                for (i=0; i<level.numConnectedClients; i++) {
                        newent = &g_entities[level.sortedClients[i]];
                        if ( !(newent->r.svFlags & SVF_CASTAI) && count <= MAX_COOP_CLIENTS && 
                                newent != ent && newent->client && newent->client->hasCoopSpawn)
                                clients[count++] = newent;
                }

                if (count) {
                        selection = rand() % count;
                        newent = clients[ selection ];
                        if ( newent->client->hasCoopSpawn )
                                TeleportPlayer( ent, newent->client->coopSpawnPointOrigin, newent->client->coopSpawnPointAngles );
                } else {
                        newent = NULL;
                }
        }

        if (ent->client->ps.lastTeleportTime <= level.time)
        {
                if ( newent ) {
                        trap_SendServerCommand( ent - g_entities, va( "cp \"You just teleported to \nthe spawnpoint of %s.\"", newent->client->pers.netname ) );
                        ent->client->ps.lastTeleportTime = level.time + g_teleporttime.integer;
                }
        }
}

/*
=================
Cmd_StartCamera_f
=================
*/
void Cmd_StartCamera_f( gentity_t *ent ) {
	g_camEnt->r.svFlags |= SVF_PORTAL;
	g_camEnt->r.svFlags &= ~SVF_NOCLIENT;
	ent->client->cameraPortal = g_camEnt;
	ent->client->ps.eFlags |= EF_VIEWING_CAMERA;
	ent->s.eFlags |= EF_VIEWING_CAMERA;

#ifdef INGAME_CUTSCENES
        if (g_skipcutscenes.integer && g_gametype.integer != GT_SINGLE_PLAYER ) {
#else
        // fretn: the cutscenes are broken, byebye
    //    if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
#endif
                AICast_ScriptEvent( AICast_GetCastState( ent->s.number ), "trigger", "cameraInterrupt" );
     //   }

// (SA) trying this in client to avoid 1 frame of player drawing
//	ent->client->ps.eFlags |= EF_NODRAW;
//	ent->s.eFlags |= EF_NODRAW;
}

/*
=================
Cmd_StopCamera_f
=================
*/
void Cmd_StopCamera_f( gentity_t *ent ) {
	gentity_t *sp;

	if ( ent->client->cameraPortal ) {
		// send a script event
		G_Script_ScriptEvent( ent->client->cameraPortal, "stopcam", "" );
		// go back into noclient mode
		ent->client->cameraPortal->r.svFlags |= SVF_NOCLIENT;
		ent->client->cameraPortal = NULL;
		ent->s.eFlags &= ~EF_VIEWING_CAMERA;
		ent->client->ps.eFlags &= ~EF_VIEWING_CAMERA;

// (SA) trying this in client to avoid 1 frame of player drawing
//		ent->s.eFlags &= ~EF_NODRAW;
//		ent->client->ps.eFlags &= ~EF_NODRAW;

		// RF, if we are near the spawn point, save the "current" game, for reloading after death
		sp = NULL;
		// gcc: suggests () around assignment used as truth value
		while ( ( sp = G_Find( sp, FOFS( classname ), "info_player_deathmatch" ) ) ) { // info_player_start becomes info_player_deathmatch in it's spawn functon
			if ( Distance( ent->s.pos.trBase, sp->s.origin ) < 256 && trap_InPVS( ent->s.pos.trBase, sp->s.origin ) ) {
				G_SaveGame( NULL );
				break;
			}
		}
	}
}

/*
=================
Cmd_SetCameraOrigin_f
=================
*/
void Cmd_SetCameraOrigin_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	int i;

	if ( trap_Argc() != 4 ) {
		return;
	}

	VectorClear( ent->client->cameraOrigin );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		ent->client->cameraOrigin[i] = atof( buffer );
	}
}


/*
==============
Cmd_InterruptCamera_f
==============
*/
void Cmd_InterruptCamera_f( gentity_t *ent ) {
	AICast_ScriptEvent( AICast_GetCastState( ent->s.number ), "trigger", "cameraInterrupt" );
}

/*
==============
G_ThrowChair
==============
*/
qboolean G_ThrowChair( gentity_t *ent, vec3_t dir, qboolean force ) {
	trace_t trace;
	vec3_t mins, maxs;
//	vec3_t		forward;
	vec3_t start, end;
	qboolean isthrown = qtrue;
	gentity_t   *traceEnt;

	if ( !ent->active || !ent->melee ) {
		return qfalse;
	}

	VectorCopy( ent->r.mins, mins );
	VectorCopy( ent->r.maxs, maxs );

//	AngleVectors (ent->r.currentAngles, forward, NULL, NULL);
	VectorCopy( ent->r.currentOrigin, start );

	start[2] += 24;
	VectorMA( start, 17, dir, start );
//	start[2] += 24;

	VectorCopy( start, end );
	VectorMA( end, 32, dir, end );

	trap_Trace( &trace, start, mins, maxs, end, ent->s.number, MASK_SOLID | MASK_MISSILESHOT );

	traceEnt = &g_entities[ trace.entityNum ];

	if ( trace.startsolid ) {
		isthrown = qfalse;
	}

	if ( trace.fraction != 1 ) {
		isthrown = qfalse;
	}

	if ( isthrown || force ) {
		// successful drop
		traceEnt->active = qfalse;

		ent->melee = NULL;
		ent->active = qfalse;
		ent->client->ps.eFlags &= ~EF_MELEE_ACTIVE;
//		ent->s.eFlags &= ~EF_MELEE_ACTIVE;
	}

	if ( !isthrown && force ) {    // was not successfully thrown, but you /need/ to drop it.  break it.
		G_Damage( traceEnt, ent, ent, NULL, NULL, 99999, 0, MOD_CRUSH );    // Die!
	}

	return ( isthrown || force );
}


// Rafael
/*
==================
Cmd_Activate_f
==================
*/
void Cmd_Activate_f( gentity_t *ent ) {
	trace_t tr;
	vec3_t end;
	gentity_t   *traceEnt;
	vec3_t forward, right, up, offset;
	static int oldactivatetime = 0;
	int activatetime = level.time;
	qboolean walking = qfalse;

	if ( ent->client->pers.cmd.buttons & BUTTON_WALKING ) {
		walking = qtrue;
	}

	AngleVectors( ent->client->ps.viewangles, forward, right, up );

	CalcMuzzlePointForActivate( ent, forward, right, up, offset );

	VectorMA( offset, 96, forward, end );

	// cs: removed corpse since they can block doors.
	trap_Trace( &tr, offset, NULL, NULL, end, ent->s.number, ( CONTENTS_SOLID | CONTENTS_BODY /*| CONTENTS_CORPSE*/ | CONTENTS_TRIGGER ) );

	//----(SA)	removed erroneous code

	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// G_Printf( "%s activate %s\n", ent->classname, traceEnt->classname);

	// Ridah, check for using a friendly AI
	if ( traceEnt->aiCharacter ) {
		AICast_Activate( ent->s.number, traceEnt->s.number );
		return;
	}

	if ( traceEnt->classname ) {
		traceEnt->flags &= ~FL_SOFTACTIVATE;    // FL_SOFTACTIVATE will be set if the user is holding his 'walk' key down when activating things

		if ( ( ( Q_stricmp( traceEnt->classname, "func_door" ) == 0 ) || ( Q_stricmp( traceEnt->classname, "func_door_rotating" ) == 0 ) ) ) {
//----(SA)	modified
			if ( walking ) {
				traceEnt->flags |= FL_SOFTACTIVATE;     // no noise
			}
			G_TryDoor( traceEnt, ent, ent );      // (door,other,activator)
//----(SA)	end
		} else if ( ( Q_stricmp( traceEnt->classname, "func_button" ) == 0 )
					&& ( traceEnt->s.apos.trType == TR_STATIONARY && traceEnt->s.pos.trType == TR_STATIONARY )
					&& traceEnt->active == qfalse ) {
			G_TryDoor( traceEnt, ent, ent );      // (door,other,activator)
//			Use_BinaryMover (traceEnt, ent, ent);
//			traceEnt->active = qtrue;
		} else if ( !Q_stricmp( traceEnt->classname, "func_invisible_user" ) )     {
			if ( walking ) {
				traceEnt->flags |= FL_SOFTACTIVATE;     // no noise
			}
			traceEnt->use( traceEnt, ent, ent );
		} else if ( !Q_stricmp( traceEnt->classname, "props_footlocker" ) )     {
			traceEnt->use( traceEnt, ent, ent );
		} else if ( !Q_stricmp( traceEnt->classname, "script_mover" ) )     {
			G_Script_ScriptEvent( traceEnt, "activate", ent->aiName );
		} else if ( traceEnt->s.eType == ET_ALARMBOX )     {
			trace_t trace;

			if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
				return;
			}

			memset( &trace, 0, sizeof( trace ) );

			if ( traceEnt->use ) {
				traceEnt->use( traceEnt, ent, 0 );
			}
		} else if ( traceEnt->s.eType == ET_ITEM )     {
			trace_t trace;

			if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
				return;
			}

			memset( &trace, 0, sizeof( trace ) );

			if ( traceEnt->touch ) {
				if ( ent->client->pers.autoActivate == PICKUP_ACTIVATE ) {
					ent->client->pers.autoActivate = PICKUP_FORCE;      //----(SA) force the pickup of a normally autoactivate only item
				}
				traceEnt->active = qtrue;
				traceEnt->touch( traceEnt, ent, &trace );
			}

		} else if ( ( Q_stricmp( traceEnt->classname, "misc_mg42" ) == 0 ) /*&& activatetime > oldactivatetime + 1000*/ && traceEnt->active == qfalse )         {
			if ( !ent->active && traceEnt->takedamage ) {  // not a dead gun
				// RF, dont allow activating MG42 if crouching
				if ( !( ent->client->ps.pm_flags & PMF_DUCKED ) && !infront( traceEnt, ent ) ) {
					gclient_t   *cl;
					cl = &level.clients[ ent->s.clientNum ];

					// no mounting while using a scoped weap
					switch ( cl->ps.weapon ) {
					case WP_SNIPERRIFLE:
					case WP_SNOOPERSCOPE:
					case WP_FG42SCOPE:
						return;

					default:
						break;
					}

					if ( !( cl->ps.grenadeTimeLeft ) ) { // make sure the client isn't holding a hot potato
						traceEnt->active = qtrue;
						ent->active = qtrue;
						traceEnt->r.ownerNum = ent->s.number;
						VectorCopy( traceEnt->s.angles, traceEnt->TargetAngles );

						if ( !( ent->r.svFlags & SVF_CASTAI ) ) {
							G_UseTargets( traceEnt, ent );   //----(SA)	added for Mike so mounting an MG42 can be a trigger event (let me know if there's any issues with this)

						}
						return; // avoid dropping down to below, where we get thrown straight off again (AI)
					}
				}
			}
		} else if ( ( Q_stricmp( traceEnt->classname, "misc_flak" ) == 0 ) /*&& activatetime > oldactivatetime + 1000*/ && traceEnt->active == qfalse )         {
			if ( !infront( traceEnt, ent ) ) {     // make sure the client isn't holding a hot potato
				gclient_t   *cl;
				cl = &level.clients[ ent->s.clientNum ];
				if ( !( cl->ps.grenadeTimeLeft ) ) {
					traceEnt->active = qtrue;
					ent->active = qtrue;
					traceEnt->r.ownerNum = ent->s.number;
					// Rafael fix for wierd mg42 movement
					VectorCopy( traceEnt->s.angles, traceEnt->TargetAngles );
				}
			}
		}
		// chairs
		else if ( traceEnt->isProp && traceEnt->takedamage && traceEnt->s.pos.trType == TR_STATIONARY && !traceEnt->nopickup ) {
			if ( !ent->active ) {
				if ( traceEnt->active ) {
					// ?
					traceEnt->active = qfalse;
				} else

				// pickup item
				{
					// only allow if using a 'one-handed' weapon
					if ( G_canPickupMelee( ent ) ) {
						traceEnt->active = qtrue;
						traceEnt->r.ownerNum = ent->s.number;
						ent->active = qtrue;
						ent->melee = traceEnt;
						ent->client->ps.eFlags |= EF_MELEE_ACTIVE;
//						ent->s.eFlags |= EF_MELEE_ACTIVE;
					}
				}
			}
		}

	}

	if ( ent->active ) {

		if ( ent->client->ps.persistant[PERS_HWEAPON_USE] ) {
			// we wish to dismount mg42
			ent->active = 2;

		} else if ( ent->melee ) {
			// throw chair
			if ( ( tr.fraction == 1 ) || ( !( traceEnt->r.contents & CONTENTS_SOLID ) ) ) {
				G_ThrowChair( ent, forward, qfalse );
			}

		} else {
			ent->active = qfalse;
		}
	}


	if ( activatetime > oldactivatetime + 1000 ) {
		oldactivatetime = activatetime;
	}
}

// Rafael WolfKick
//===================
//	Cmd_WolfKick
//===================

#define WOLFKICKDISTANCE    96
int Cmd_WolfKick_f( gentity_t *ent ) {
	trace_t tr;
	vec3_t end;
	gentity_t   *traceEnt;
	vec3_t forward, right, up, offset;
	gentity_t   *tent;
	static int oldkicktime = 0;
	int kicktime = level.time;
	qboolean solidKick = qfalse;    // don't play "hit" sound on a trigger unless it's an func_invisible_user

	int damage = 15;

	if ( ent->client->ps.leanf ) {
		return 0;   // no kick when leaning

	}
	if ( oldkicktime > kicktime ) {
		return ( 0 );
	} else {
		oldkicktime = kicktime + 1000;
	}

	// play the anim
	BG_AnimScriptEvent( &ent->client->ps, ANIM_ET_KICK, qfalse, qtrue );

	ent->client->ps.persistant[PERS_WOLFKICK] = 1;

	AngleVectors( ent->client->ps.viewangles, forward, right, up );

	CalcMuzzlePointForActivate( ent, forward, right, up, offset );

	// note to self: we need to determine the usable distance for wolf
	VectorMA( offset, WOLFKICKDISTANCE, forward, end );

	trap_Trace( &tr, offset, NULL, NULL, end, ent->s.number, ( CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER ) );

	if ( tr.surfaceFlags & SURF_NOIMPACT || tr.fraction == 1.0 ) {
		tent = G_TempEntity( tr.endpos, EV_WOLFKICK_MISS );
		tent->s.eventParm = ent->s.number;
		return ( 1 );
	}

	traceEnt = &g_entities[ tr.entityNum ];

	if ( !ent->melee ) { // because we dont want you to open a door with a prop
		if ( ( Q_stricmp( traceEnt->classname, "func_door_rotating" ) == 0 )
			 && ( traceEnt->s.apos.trType == TR_STATIONARY && traceEnt->s.pos.trType == TR_STATIONARY )
			 && traceEnt->active == qfalse ) {
//			if(traceEnt->key < 0) {	// door force locked
			if ( traceEnt->key >= KEY_LOCKED_TARGET ) {    // door force locked

				//----(SA)	play kick "hit" sound
				tent = G_TempEntity( tr.endpos, EV_WOLFKICK_HIT_WALL );
				tent->s.otherEntityNum = ent->s.number;	\
				//----(SA)	end

				AICast_AudibleEvent( ent->s.clientNum, tr.endpos, HEAR_RANGE_DOOR_KICKLOCKED ); // "someone kicked a locked door near me!"

				G_AddEvent( traceEnt, EV_GENERAL_SOUND, traceEnt->soundPos3 );

				return 1;   //----(SA)	changed.  shows boot for locked doors
			}

//			if(traceEnt->key > 0) {	// door requires key
			if ( traceEnt->key > KEY_NONE && traceEnt->key < KEY_NUM_KEYS ) {
				gitem_t *item = BG_FindItemForKey( traceEnt->key, 0 );
				if ( !( ent->client->ps.stats[STAT_KEYS] & ( 1 << item->giTag ) ) ) {
					//----(SA)	play kick "hit" sound
					tent = G_TempEntity( tr.endpos, EV_WOLFKICK_HIT_WALL );
					tent->s.otherEntityNum = ent->s.number;	\
					//----(SA)	end

					AICast_AudibleEvent( ent->s.clientNum, tr.endpos, HEAR_RANGE_DOOR_KICKLOCKED ); // "someone kicked a locked door near me!"

					// player does not have key
					G_AddEvent( traceEnt, EV_GENERAL_SOUND, traceEnt->soundPos3 );

					return 1;   //----(SA)	changed.  shows boot animation for locked doors
				}
			}

			if ( traceEnt->teammaster && traceEnt->team && traceEnt != traceEnt->teammaster ) {
				traceEnt->teammaster->active = qtrue;
				traceEnt->teammaster->flags |= FL_KICKACTIVATE;
				Use_BinaryMover( traceEnt->teammaster, ent, ent );
				G_UseTargets( traceEnt->teammaster, ent );
			} else
			{
				traceEnt->active = qtrue;
				traceEnt->flags |= FL_KICKACTIVATE;
				Use_BinaryMover( traceEnt, ent, ent );
				G_UseTargets( traceEnt, ent );
			}
		} else if ( ( Q_stricmp( traceEnt->classname, "func_button" ) == 0 )
					&& ( traceEnt->s.apos.trType == TR_STATIONARY && traceEnt->s.pos.trType == TR_STATIONARY )
					&& traceEnt->active == qfalse ) {
			Use_BinaryMover( traceEnt, ent, ent );
			traceEnt->active = qtrue;

		} else if ( !Q_stricmp( traceEnt->classname, "func_invisible_user" ) )     {
			traceEnt->flags |= FL_KICKACTIVATE;     // so cell doors know they were kicked
													// It doesn't hurt to pass this along since only ent use() funcs who care about it will check.
													// However, it may become handy to put a "KICKABLE" or "NOTKICKABLE" flag on the invisible_user
			traceEnt->use( traceEnt, ent, ent );
			traceEnt->flags &= ~FL_KICKACTIVATE;    // reset

			solidKick = qtrue;  //----(SA)
		} else if ( !Q_stricmp( traceEnt->classname, "props_flippy_table" ) && traceEnt->use )       {
			traceEnt->use( traceEnt, ent, ent );
		} else if ( !Q_stricmp( traceEnt->classname, "misc_mg42" ) )     {
			solidKick = qtrue;  //----(SA)	play kick hit sound
		}
	}

	// snap the endpos to integers, but nudged towards the line
	SnapVectorTowards( tr.endpos, offset );

	// send bullet impact
	if ( traceEnt->takedamage && traceEnt->client ) {
		tent = G_TempEntity( tr.endpos, EV_WOLFKICK_HIT_FLESH );
		tent->s.eventParm = traceEnt->s.number;
		if ( LogAccuracyHit( traceEnt, ent ) ) {
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}
	} else {
		// Ridah, bullet impact should reflect off surface
		vec3_t reflect;
		float dot;

		if ( traceEnt->r.contents >= 0 && ( traceEnt->r.contents & CONTENTS_TRIGGER ) && !solidKick ) {
			tent = G_TempEntity( tr.endpos, EV_WOLFKICK_MISS ); // (SA) don't play the "hit" sound if you kick most triggers
		} else {
			tent = G_TempEntity( tr.endpos, EV_WOLFKICK_HIT_WALL );
		}


		dot = DotProduct( forward, tr.plane.normal );
		VectorMA( forward, -2 * dot, tr.plane.normal, reflect );
		VectorNormalize( reflect );

		tent->s.eventParm = DirToByte( reflect );
		// done.

		// (SA) should break...
		if ( ent->melee ) {
			ent->active = qfalse;
			ent->melee->health = 0;
			ent->client->ps.eFlags &= ~EF_MELEE_ACTIVE; // whoops, missed this one
		}
	}

	tent->s.otherEntityNum = ent->s.number;

	// try to swing chair
	if ( traceEnt->takedamage ) {

		if ( ent->melee ) {
			ent->active = qfalse;
			ent->melee->health = 0;
			ent->client->ps.eFlags &= ~EF_MELEE_ACTIVE;

		}

		G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_KICKED );   //----(SA)	modified
	}

	return ( 1 );
}
// done

/*
============================
Cmd_ClientMonsterSlickAngle
============================
*/
/*
void Cmd_ClientMonsterSlickAngle (gentity_t *clent) {

	char s[MAX_STRING_CHARS];
	int	entnum;
	int angle;
	gentity_t *ent;
	vec3_t	dir, kvel;
	vec3_t	forward;

	if (trap_Argc() != 3) {
		G_Printf( "ClientDamage command issued with incorrect number of args\n" );
	}

	trap_Argv( 1, s, sizeof( s ) );
	entnum = atoi(s);
	ent = &g_entities[entnum];

	trap_Argv( 2, s, sizeof( s ) );
	angle = atoi(s);

	// sanity check (also protect from cheaters)
	if (g_gametype.integer != GT_SINGLE_PLAYER && entnum != clent->s.number) {
		trap_DropClient( clent->s.number, "Dropped due to illegal ClientMonsterSlick command\n" );
		return;
	}

	VectorClear (dir);
	dir[YAW] = angle;
	AngleVectors (dir, forward, NULL, NULL);

	VectorScale (forward, 32, kvel);
	VectorAdd (ent->client->ps.velocity, kvel, ent->client->ps.velocity);
}
*/

// NERVE - SMF
/*
============
ClientDamage
============
*/
void ClientDamage( gentity_t *clent, int entnum, int enemynum, int id ) {
	gentity_t *enemy, *ent;
	vec3_t vec;
#ifdef MONEY
        // no no no no no no no no no no no 
        // aka: I dont want this in a competitive gametype
        // yes this breaks the tesla and the spirits and others in battle gametype
        if (g_gametype.integer == GT_COOP_BATTLE)
                return;
#endif

	ent = &g_entities[entnum];

	enemy = &g_entities[enemynum];

	switch ( id ) {
	case CLDMG_DEBRIS:
                G_Damage( ent, enemy, enemy, vec3_origin, vec3_origin, 3 + rand() % 3, DAMAGE_NO_KNOCKBACK, MOD_EXPLOSIVE );
		break;
	case CLDMG_SPIRIT:
                if ( enemy->aiCharacter == AICHAR_ZOMBIE ) {
                        G_Damage( ent, enemy, enemy, vec3_origin, vec3_origin, 6, DAMAGE_NO_KNOCKBACK, MOD_ZOMBIESPIRIT );
                } else {
                        G_Damage( ent, enemy, enemy, vec3_origin, vec3_origin, 8 + rand() % 4, DAMAGE_NO_KNOCKBACK, MOD_ZOMBIESPIRIT );
		}
		break;
	case CLDMG_BOSS1LIGHTNING:
		if ( ent->takedamage ) {
			VectorSubtract( ent->r.currentOrigin, enemy->r.currentOrigin, vec );
			VectorNormalize( vec );
			G_Damage( ent, enemy, enemy, vec, ent->r.currentOrigin, 6 + rand() % 3, 0, MOD_LIGHTNING );
		}
		break;
	case CLDMG_TESLA:
		if (    ( ent->aiCharacter == AICHAR_PROTOSOLDIER ) ||
				( ent->aiCharacter == AICHAR_SUPERSOLDIER ) ||
				( ent->aiCharacter == AICHAR_LOPER ) ||
				( ent->aiCharacter >= AICHAR_STIMSOLDIER1 && ent->aiCharacter <= AICHAR_STIMSOLDIER3 ) ) {
			break;
		}

		if ( ent->takedamage /*&& !AICast_NoFlameDamage(ent->s.number)*/ ) {
			VectorSubtract( ent->r.currentOrigin, enemy->r.currentOrigin, vec );
			VectorNormalize( vec );
			if ( !( enemy->r.svFlags & SVF_CASTAI ) ) {
				G_Damage( ent, enemy, enemy, vec, ent->r.currentOrigin, 8, 0, MOD_LIGHTNING );
			} else {
				G_Damage( ent, enemy, enemy, vec, ent->r.currentOrigin, 4, 0, MOD_LIGHTNING );
			}
		}
		break;
	}
}
// -NERVE - SMF

/*
============
Cmd_ClientDamage_f
============
*/
void Cmd_ClientDamage_f( gentity_t *clent ) {
	char s[MAX_STRING_CHARS];
	int entnum, id, enemynum;

	if ( trap_Argc() != 4 ) {
		G_Printf( "ClientDamage command issued with incorrect number of args\n" );
	}

	trap_Argv( 1, s, sizeof( s ) );
	entnum = atoi( s );

	trap_Argv( 2, s, sizeof( s ) );
	enemynum = atoi( s );

	trap_Argv( 3, s, sizeof( s ) );
	id = atoi( s );

	ClientDamage( clent, entnum, enemynum, id );
}

/*
==============
Cmd_EntityCount_f
==============
*/
#define AITEAM_NAZI     0
#define AITEAM_ALLIES   1
#define AITEAM_MONSTER  2
void Cmd_EntityCount_f( gentity_t *ent ) {
	if ( !g_cheats.integer ) {
		return;
	}

	G_Printf( "entity count = %i\n", level.num_entities );

	{
		int kills[2];
		int nazis[2];
		int monsters[2];
		int i;
		gentity_t *ent;

		// count kills
		kills[0] = kills[1] = 0;
		nazis[0] = nazis[1] = 0;
		monsters[0] = monsters[1] = 0;
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			ent = &g_entities[i];

			if ( !ent->inuse ) {
				continue;
			}

			if ( !( ent->r.svFlags & SVF_CASTAI ) ) {
				continue;
			}

			if ( ent->aiTeam == AITEAM_ALLIES ) {
				continue;
			}

			kills[1]++;

			if ( ent->health <= 0 ) {
				kills[0]++;
			}

			if ( ent->aiTeam == AITEAM_NAZI ) {
				nazis[1]++;
				if ( ent->health <= 0 ) {
					nazis[0]++;
				}
			} else {
				monsters[1]++;
				if ( ent->health <= 0 ) {
					monsters[0]++;
				}
			}
		}
		G_Printf( "kills %i/%i nazis %i/%i monsters %i/%i \n",kills[0], kills[1], nazis[0], nazis[1], monsters[0], monsters[1] );

	}
}

void Cmd_DropAmmo_f ( gentity_t *ent )
{
        gclient_t *client;
        gentity_t *ent2;
        gitem_t *item;
        vec3_t angles, velocity, offset, mins, maxs, org;
        int     ammo_in_clip;
        trace_t tr;
        weapon_t weapon;
        weapon_t ammo_for_weapon;
        
        if (!ent->client)
                return;

        client = ent->client;

        // no ammo dropping while reloading
        if (client->ps.weaponstate == WEAPON_RELOADING)
                return;

        weapon = (weapon_t)client->ps.weapon;

        if (weapon == WP_KNIFE)
                return;

		ammo_for_weapon = BG_FindAmmoForWeapon( weapon );
		ammo_in_clip = client->ps.ammo[ ammo_for_weapon ];

		if (ammo_in_clip < 1) {
			trap_SendServerCommand( ent - g_entities, "print \"You do not have any extra ammo to drop.\n\"" );
			return;
		}

		if (ammo_in_clip > ammoTable[weapon].maxclip) {
			ammo_in_clip = ammoTable[weapon].maxclip;
		}

        item = BG_FindItemForAmmo( ammo_for_weapon );
        VectorCopy( client->ps.viewangles, angles );

        // clamp pitch
        if ( angles[PITCH] < -30 ) {
                angles[PITCH] = -30;
        } else if ( angles[PITCH] > 30 ) {
                angles[PITCH] = 30;
        }

        AngleVectors( angles, velocity, NULL, NULL );
        VectorScale( velocity, 64, offset );
        offset[2] += client->ps.viewheight / 2;
        VectorScale( velocity, 75, velocity );
        velocity[2] += 50 + random() * 35;

        VectorAdd( client->ps.origin,offset,org );

        VectorSet( mins, -ITEM_RADIUS, -ITEM_RADIUS, 0 );
        VectorSet( maxs, ITEM_RADIUS, ITEM_RADIUS, 2 * ITEM_RADIUS );

        trap_Trace( &tr, client->ps.origin, mins, maxs, org, ent->s.number, MASK_SOLID );
        VectorCopy( tr.endpos, org );

        ent2 = LaunchItem( item, org, velocity );

        ent2->count = ammo_in_clip;
        ent2->item->quantity = ammo_in_clip;
        
        // remove ammo from clip
        client->ps.ammo[ ammo_for_weapon ] -= ammo_in_clip;

}
// NERVE - SMF
/*
============
Cmd_SetSpawnPoint_f
============
*/
void Cmd_SetSpawnPoint_f( gentity_t *clent ) {
	char arg[MAX_TOKEN_CHARS];
	int spawnIndex;

	if ( trap_Argc() != 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	spawnIndex = atoi( arg );
}
// -NERVE - SMF

/*
============
Cmd_DrawSpawns_f
============
*/
void Cmd_DrawSpawns_f( gentity_t *clent ) {
	gentity_t *ent;
	gentity_t *tempEnt;
	int i;
	vec3_t playerMins = {-18, -18, -24};
	vec3_t playerMaxs = {18, 18, 48};
	
	ent = g_entities + MAX_CLIENTS;
	for ( i = MAX_CLIENTS; i < MAX_GENTITIES; i++, ent++ ) {
		if ( !Q_stricmp(ent->classname, "coop_spawnpoint") ) {
			tempEnt = G_TempEntity(ent->r.currentOrigin, EV_DBG_AABB);

			// in the spirit of this sdk, abuse some existing fields ...
			VectorCopy(ent->r.currentOrigin, tempEnt->s.origin);
			VectorCopy(playerMins, tempEnt->s.angles); // mins
			VectorCopy(playerMaxs, tempEnt->s.angles2); // maxs
			tempEnt->s.time = 999999; // duration
			tempEnt->s.time2 = COOP_SPAWNPOINT; // class name mapping
			tempEnt->s.solid = 6; // sides
                        tempEnt->r.svFlags |= SVF_BROADCAST;
		}
	}
}

/*
============
Cmd_DrawTriggers_f
============
*/
void Cmd_DrawTriggers_f( gentity_t *clent ) {
	gentity_t *ent;
	gentity_t *tempEnt;
	int i;
	
	ent = g_entities + MAX_CLIENTS;
	for ( i = MAX_CLIENTS; i < MAX_GENTITIES; i++, ent++ ) {
	    if ( !Q_stricmp(ent->classname, "ai_trigger") ) {
			tempEnt = G_TempEntity(ent->r.currentOrigin, EV_DBG_AABB);

			// in the spirit of this sdk, abuse some existing fields ...
			VectorCopy(ent->r.currentOrigin, tempEnt->s.origin);
			VectorCopy(ent->r.absmin, tempEnt->s.angles); // mins
			VectorCopy(ent->r.absmax, tempEnt->s.angles2); // maxs
			tempEnt->s.time = 999999; // duration
			tempEnt->s.time2 = AI_TRIGGER; // class name mapping
			tempEnt->s.solid = 6; // sides
                        tempEnt->r.svFlags |= SVF_BROADCAST;
		}
	}
}

#ifdef _DEBUG
void Cmd_AlertEntity_f() {
	char targetName[MAX_STRING_TOKENS];
	gentity_t   *alertent;

	trap_Argv( 1, targetName, sizeof( targetName ) );

	if ( !targetName[0] ) {
		trap_Print( "Usage: alert <target>\n" );
		return;
	}

	// find this targetname
	alertent = G_Find( NULL, FOFS( targetname ), targetName );
	if ( !alertent ) {
		trap_Print( va("alert cannot find targetname \"%s\"\n", targetName) );
		return;
	}

	if ( alertent->client ) {
		// call this entity's AlertEntity function
		if ( !alertent->AIScript_AlertEntity ) {
			trap_Print( va("alert \"%s\" (classname = %s) doesn't have an \"AIScript_AlertEntity\" function\n", targetName, alertent->classname) );
		}
		alertent->AIScript_AlertEntity( alertent );
	} else {
		if ( !alertent->use ) {
			trap_Print( va("alert \"%s\" (classname = %s) doesn't have a \"use\" function\n", targetName, alertent->classname) );
		}
		alertent->use( alertent, NULL, NULL );
	}
}
#endif


/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum ) {
	gentity_t *ent;
	char cmd[MAX_TOKEN_CHARS];

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;     // not fully in game yet
	}


	trap_Argv( 0, cmd, sizeof( cmd ) );

	// Ridah, AI Cast debugging
	if ( Q_stricmp( cmd, "aicast" ) == 0 ) {
		extern void AICast_DBG_Cmd_f( int clientNum );
		//
		AICast_DBG_Cmd_f( clientNum );
		return;
	}
	// done.

	// RF, client damage commands
	if ( Q_stricmp( cmd, "cld" ) == 0 ) {
		Cmd_ClientDamage_f( ent );
		return;
	}
	// done.

        if ( Q_stricmp( cmd, "vsay" ) == 0 ) {
                Cmd_Voice_f( ent, SAY_ALL, qfalse, qfalse );
                return;
        }
        if ( Q_stricmp( cmd, "vsay_team" ) == 0 ) {
                Cmd_Voice_f( ent, SAY_TEAM, qfalse, qfalse );
                return;
        }
	if ( Q_stricmp( cmd, "say" ) == 0 ) {
		Cmd_Say_f( ent, SAY_ALL, qfalse );
		return;
	}
	if ( Q_stricmp( cmd, "say_team" ) == 0 ) {
		Cmd_Say_f( ent, SAY_TEAM, qfalse );
		return;
	}
	// NERVE - SMF
	if ( Q_stricmp( cmd, "say_limbo" ) == 0 ) {
		Cmd_Say_f( ent, SAY_LIMBO, qfalse );
		return;
	}
	// -NERVE - SMF
	if ( Q_stricmp( cmd, "tell" ) == 0 ) {
		Cmd_Tell_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "score" ) == 0 ) {
		Cmd_Score_f( ent );
		return;
	}
        if ( Q_stricmp( cmd, "playerstart" ) == 0 ) {
                Cmd_PlayerStart_f( ent );
                return;
        }

//----(SA)	added
	if ( Q_stricmp( cmd, "fogswitch" ) == 0 ) {
		Cmd_Fogswitch_f();
		return;
	}
//----(SA)	end

	// L0 - Hook our commands above intermission
	if ( Q_stricmp( cmd, "login" ) == 0 ) {
		cmd_do_login( ent, qfalse );
	return;
	} else if ( Q_stricmp( cmd, "@login" ) == 0 ) {
		cmd_do_login( ent, qtrue );
	return;
	} else if ( Q_stricmp( cmd, "logout" ) == 0 ) {
		cmd_do_logout( ent );
	return;
	} else if ( Q_stricmp( cmd, "incognito" ) == 0 ) {
		cmd_incognito( ent );
	return;
	} else if ( Q_stricmp( cmd, "getstatus" ) == 0 ) {
		cmd_getstatus( ent );
	return;
	}
	// End

	// ignore all other commands when at intermission
	if ( level.intermissiontime ) {
		Cmd_Say_f( ent, qfalse, qtrue );
		return;
	}

	if ( Q_stricmp( cmd, "give" ) == 0 ) {
		Cmd_Give_f( ent );
#ifdef MONEY
	} else if ( Q_stricmp( cmd, "buy" ) == 0 ) {
		Cmd_Buy_f( ent );
#endif
	} else if ( Q_stricmp( cmd, "god" ) == 0 )  {
		Cmd_God_f( ent );
	} else if ( Q_stricmp( cmd, "nofatigue" ) == 0 )  {
		Cmd_Nofatigue_f( ent );
	} else if ( Q_stricmp( cmd, "notarget" ) == 0 )  {
		Cmd_Notarget_f( ent );
	} else if ( Q_stricmp( cmd, "noclip" ) == 0 )  {
		Cmd_Noclip_f( ent );
	} else if ( Q_stricmp( cmd, "spawnpoint" ) == 0 )  {
		Cmd_SetCoopSpawn_f( ent );
	} else if ( Q_stricmp( cmd, "kill" ) == 0 )  {
		Cmd_Kill_f( ent );
	} else if ( Q_stricmp( cmd, "levelshot" ) == 0 )  {
		Cmd_LevelShot_f( ent );
	} else if ( Q_stricmp( cmd, "follow" ) == 0 )  {
		Cmd_Follow_f( ent );
	} else if ( Q_stricmp( cmd, "follownext" ) == 0 )  {
		Cmd_FollowCycle_f( ent, 1 );
	} else if ( Q_stricmp( cmd, "followprev" ) == 0 )  {
		Cmd_FollowCycle_f( ent, -1 );
	} else if ( Q_stricmp( cmd, "team" ) == 0 )  {
		Cmd_Team_f( ent );
	} else if ( Q_stricmp( cmd, "where" ) == 0 )  {
		Cmd_Where_f( ent );
	}
	else if (Q_stricmp (cmd, "callvote") == 0)	//----(SA)	id requests these gone in sp
		Cmd_CallVote_f (ent);
	else if (Q_stricmp (cmd, "vote") == 0)		//----(SA)	id requests these gone in sp
		Cmd_Vote_f (ent);
	else if ( Q_stricmp( cmd, "gc" ) == 0 ) {
		Cmd_GameCommand_f( ent );
	} else if ( Q_stricmp( cmd, "startCamera" ) == 0 )  {
		Cmd_StartCamera_f( ent );
	} else if ( Q_stricmp( cmd, "stopCamera" ) == 0 )  {
		Cmd_StopCamera_f( ent );
	} else if ( Q_stricmp( cmd, "setCameraOrigin" ) == 0 )  {
		Cmd_SetCameraOrigin_f( ent );
	} else if ( Q_stricmp( cmd, "cameraInterrupt" ) == 0 )  {
		Cmd_InterruptCamera_f( ent );
	} else if ( Q_stricmp( cmd, "setviewpos" ) == 0 )  {
		Cmd_SetViewpos_f( ent );
	} else if ( Q_stricmp( cmd, "entitycount" ) == 0 )  {
		Cmd_EntityCount_f( ent );
	} else if ( Q_stricmp( cmd, "setspawnpt" ) == 0 )  {
		Cmd_SetSpawnPoint_f( ent );
	} else if ( Q_stricmp( cmd, "dropammo" ) == 0 ) {
                Cmd_DropAmmo_f( ent );
	} else if ( Q_stricmp( cmd, "drawspawns" ) == 0 ) {
                Cmd_DrawSpawns_f( ent );
	} else if ( Q_stricmp( cmd, "drawtriggers" ) == 0 ) {
                Cmd_DrawTriggers_f( ent );
        } else if ( Q_stricmp( cmd, "teleport" ) == 0 ) {
                Cmd_Teleport_f( ent );
#ifdef _DEBUG
        } else if ( Q_stricmp( cmd, "alert" ) == 0 ) {
                Cmd_AlertEntity_f();
#endif
	} else {
		trap_SendServerCommand( clientNum, va( "print \"unknown cmd %s\n\"", cmd ) );
	}
}
