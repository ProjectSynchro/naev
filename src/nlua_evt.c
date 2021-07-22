/*
 * See Licensing and Copyright notice in naev.h
 */

/**
 * @file nlua_evt.c
 *
 * @brief Handles the event Lua bindings.
 */


/** @cond */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "naev.h"
/** @endcond */

#include "nlua_evt.h"

#include "event.h"
#include "land.h"
#include "log.h"
#include "mission.h"
#include "ndata.h"
#include "nlua.h"
#include "nlua_hook.h"
#include "nlua_system.h"
#include "nluadef.h"
#include "npc.h"
#include "nstring.h"
#include "player.h"


/**
 * @brief Event system Lua bindings.
 *
 * An example would be:
 * @code
 * evt.finish() -- finishes the event
 * @endcode
 *
 * @luamod evt
 */


/*
 * libraries
 */
/* evt */
static int evtL_npcAdd( lua_State *L );
static int evtL_npcRm( lua_State *L );
static int evtL_finish( lua_State *L );
static int evtL_save( lua_State *L );
static int evtL_claim( lua_State *L );
static const luaL_Reg evtL_methods[] = {
   { "npcAdd", evtL_npcAdd },
   { "npcRm", evtL_npcRm },
   { "save", evtL_save },
   { "finish", evtL_finish },
   { "claim", evtL_claim },
   {0,0}
}; /**< Mission Lua methods. */


/*
 * individual library loading
 */
/**
 * @brief Loads the event Lua library.
 *    @param env Lua environment.
 */
int nlua_loadEvt( nlua_env env )
{
   nlua_register(env, "evt", evtL_methods, 0);
   return 0;
}


/**
 * @brief Sets up the Lua environment to run a function.
 */
void event_setupLua( Event_t *ev, const char *func )
{
   Event_t **evptr;

   /* Set up event pointer. */
   evptr = lua_newuserdata( naevL, sizeof(Event_t*) );
   *evptr = ev;
   nlua_setenv( ev->env, "__evt" );

   /* Get function. */
   nlua_getenv(ev->env, func );
}


/**
 * @brief Runs the Lua for an event.
 */
int event_runLua( Event_t *ev, const char *func )
{
   int ret;
   event_setupLua( ev, func );
   ret = event_runLuaFunc( ev, func, 0 );
   return ret;
}


/**
 * @brief Gets the current running event from user data.
 *
 * This should ONLY be called below an nlua_pcall, so __NLUA_CURENV is set
 */
Event_t *event_getFromLua( lua_State *L )
{
   Event_t *ev, **evptr;
   nlua_getenv(__NLUA_CURENV, "__evt");
   evptr = lua_touserdata( L, -1 );
   ev = evptr ? *evptr : NULL;
   lua_pop( L, 1 );
   return ev;
}


/**
 * @brief Runs a Lua func with nargs.
 *
 *    @return -1 on error, 1 on misn.finish() call, 2 if event got deleted
 *            and 0 normally.
 */
int event_runLuaFunc( Event_t *ev, const char *func, int nargs )
{
   int ret;
   const char* err;
   int evt_delete;

   ret = nlua_pcall(ev->env, nargs, 0);
   if (ret != 0) { /* error has occurred */
      err = (lua_isstring(naevL,-1)) ? lua_tostring(naevL,-1) : NULL;
      if ((err==NULL) || (strcmp(err,NLUA_DONE)!=0)) {
         WARN(_("Event '%s' -> '%s': %s"),
               event_getData(ev->id), func, (err) ? err : _("unknown error"));
         ret = -1;
      }
      else
         ret = 1;
      lua_pop(naevL, 1);
   }

   /* Time to remove the event. */
   nlua_getenv(ev->env, "__evt_delete");
   evt_delete = lua_toboolean(naevL,-1);
   lua_pop(naevL,1);
   if (evt_delete) {
      ret = 2;
      event_remove( ev->id );
   }

   return ret;
}


/**
 * @brief Adds an NPC.
 *
 * @usage npc_id = evt.npcAdd( "my_func", "Mr. Test", "none.webp", "A test." ) -- Creates an NPC.
 *
 *    @luatparam string func Name of the function to run when approaching, gets passed the npc_id when called.
 *    @luatparam string name Name of the NPC
 *    @luatparam string portrait Portrait file name to use for the NPC (from GFX_PATH/portraits/).
 *    @luatparam string desc Description associated to the NPC.
 *    @luatparam[opt=5] number priority Optional priority argument (highest is 0, lowest is 10).
 *    @luatparam[opt=nil] string background Background file name to use (from GFX_PATH/portraits/).
 *    @luatreturn number The ID of the NPC to pass to npcRm.
 * @luafunc npcAdd
 */
static int evtL_npcAdd( lua_State *L )
{
   unsigned int id;
   int priority;
   const char *func, *name, *gfx, *desc, *bg;
   char portrait[PATH_MAX], background[PATH_MAX];
   Event_t *cur_event;

   /* Handle parameters. */
   func = luaL_checkstring(L, 1);
   name = luaL_checkstring(L, 2);
   gfx  = luaL_checkstring(L, 3);
   desc = luaL_checkstring(L, 4);

   /* Optional parameters. */
   priority = luaL_optinteger(L,5,5);
   bg   = luaL_optstring(L,6,NULL);

   /* Set path. */
   ndata_getPathDefault( portrait, sizeof(portrait), GFX_PATH"portraits/", gfx );
   if (bg!=NULL)
      ndata_getPathDefault( background, sizeof(background), GFX_PATH"portraits/", bg );

   cur_event = event_getFromLua(L);

   /* Add npc. */
   id = npc_add_event( cur_event->id, func, name, priority, portrait, desc, (bg==NULL) ? bg : background );

   bar_regen();

   /* Return ID. */
   if (id > 0) {
      lua_pushnumber( L, id );
      return 1;
   }
   return 0;
}


/**
 * @brief Removes an NPC.
 *
 * @usage evt.npcRm( npc_id )
 *
 *    @luatparam number id ID of the NPC to remove.
 * @luafunc npcRm
 */
static int evtL_npcRm( lua_State *L )
{
   unsigned int id;
   int ret;
   Event_t *cur_event;

   id = luaL_checklong(L, 1);

   cur_event = event_getFromLua(L);
   ret = npc_rm_event( id, cur_event->id );

   bar_regen();

   if (ret != 0)
      NLUA_ERROR(L, _("Invalid NPC ID!"));
   return 0;
}


/**
 * @brief Finishes the event.
 *
 *    @luatparam[opt=false] boolean properly If true and the event is unique it marks the event
 *                     as completed. If false it deletes the event but
 *                     doesn't mark it as completed.
 * @luafunc finish
 */
static int evtL_finish( lua_State *L )
{
   int b;
   Event_t *cur_event;

   cur_event = event_getFromLua(L);

   b = lua_toboolean(L,1);
   lua_pushboolean( L, 1 );
   nlua_setenv(cur_event->env, "__evt_delete");

   if (b && event_isUnique(cur_event->id))
      player_eventFinished( cur_event->data );

   lua_pushstring(L, NLUA_DONE);
   lua_error(L); /* shouldn't return */

   return 0;
}


/**
 * @brief Saves an event.
 *
 * @usage evt.save() -- Saves an event, which is by default disabled.
 *
 *    @luatparam[opt=true] boolean enable If true sets the event to save, otherwise tells the event to not save.
 * @luafunc save
 */
static int evtL_save( lua_State *L )
{
   int b;
   Event_t *cur_event;
   if (lua_gettop(L)==0)
      b = 1;
   else
      b = lua_toboolean(L,1);
   cur_event = event_getFromLua(L);
   cur_event->save = b;
   return 0;
}


/**
 * @brief Tries to claim systems or strings.
 *
 * Claiming systems and strings is a way to avoid mission collisions preemptively.
 *
 * Note it does not actually perform the claim if it fails to claim. It also
 *  does not work more than once.
 *
 * @usage if not evt.claim( { system.get("Gamma Polaris") } ) then evt.finish( false ) end
 * @usage if not evt.claim( system.get("Gamma Polaris") ) then evt.finish( false ) end
 * @usage if not evt.claim( 'some_string' ) then evt.finish( false ) end
 * @usage if not evt.claim( { system.get("Gamma Polaris"), 'some_string' } ) then evt.finish( false ) end
 *
 *    @luatparam System|String|{System,String...} params Table of systems/strings to claim or a single system/string.
 *    @luatreturn boolean true if was able to claim, false otherwise.
 * @luafunc claim
 */
static int evtL_claim( lua_State *L )
{
   Claim_t *claim;
   Event_t *cur_event;

   /* Get current event. */
   cur_event = event_getFromLua(L);

   /* Check to see if already claimed. */
   if (cur_event->claims != NULL) {
      NLUA_ERROR(L, _("Event trying to claim but already has."));
      return 0;
   }

   /* Create the claim. */
   claim = claim_create();

   /* Handle parameters. */
   if (lua_istable(L,1)) {
      /* Iterate over table. */
      lua_pushnil(L);
      while (lua_next(L, 1) != 0) {
         if (lua_issystem(L,-1))
            claim_addSys( claim, lua_tosystem( L, -1 ) );
         else if (lua_isstring(L,-1))
            claim_addStr( claim, lua_tostring( L, -1 ) );
         lua_pop(L,1);
      }
   }
   else if (lua_issystem(L, 1))
      claim_addSys( claim, lua_tosystem( L, 1 ) );
   else if (lua_isstring(L, 1))
      claim_addStr( claim, lua_tostring( L, 1 ) );
   else
      NLUA_INVALID_PARAMETER(L);

   /* Test claim. */
   if (claim_test( claim )) {
      claim_destroy( claim );
      lua_pushboolean(L,0);
      return 1;
   }

   /* Set the claim. */
   cur_event->claims = claim;
   claim_activate( claim );
   lua_pushboolean(L,1);
   return 1;
}

