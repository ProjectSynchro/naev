/*
 * See Licensing and Copyright notice in naev.h
 */

/**
 * @file nlua_pilotoutfit.c
 *
 * @brief Handles the Lua pilot outfit (equipped) bindings.
 */

/** @cond */
#include <lauxlib.h>

#include "naev.h"
/** @endcond */

#include "nlua_pilotoutfit.h"

#include "log.h"
#include "nlua_outfit.h"
#include "nluadef.h"
#include "rng.h"
#include "slots.h"


int pilotoutfit_modified = 0;


/* Pilot outfit metatable methods. */
static int poL_outfit( lua_State *L );
static int poL_state( lua_State *L );
static int poL_progress( lua_State *L );
static int poL_set( lua_State *L );
static int poL_clear( lua_State *L );
static const luaL_Reg poL_methods[] = {
   { "outfit", poL_outfit },
   { "state", poL_state },
   { "progress", poL_progress },
   { "set", poL_set },
   { "clear", poL_clear },
   {0,0}
}; /**< Pilot outfit metatable methods. */



/**
 * @brief Loads the pilot outfit library.
 *
 *    @param env Environment to load pilot outfit library into.
 *    @return 0 on success.
 */
int nlua_loadPilotOutfit( nlua_env env )
{
   nlua_register(env, PILOTOUTFIT_METATABLE, poL_methods, 1);
   return 0;
}


/**
 * @brief Lua bindings to interact with pilot outfits.
 *
 * @luamod pilotoutfit
 */
/**
 * @brief Gets outfit at index.
 *
 *    @param L Lua state to get outfit from.
 *    @param ind Index position to find the outfit.
 *    @return Outfit found at the index in the state.
 */
PilotOutfitSlot* lua_topilotoutfit( lua_State *L, int ind )
{
   return *((PilotOutfitSlot**) lua_touserdata(L,ind));
}
/**
 * @brief Gets outfit at index or raises error if there is no outfit at index.
 *
 *    @param L Lua state to get outfit from.
 *    @param ind Index position to find outfit.
 *    @return Outfit found at the index in the state.
 */
PilotOutfitSlot* luaL_checkpilotoutfit( lua_State *L, int ind )
{
   if (lua_ispilotoutfit(L,ind))
      return lua_topilotoutfit(L,ind);
   luaL_typerror(L, ind, PILOTOUTFIT_METATABLE);
   return NULL;
}
/**
 * @brief Makes sure the outfit is valid or raises a Lua error.
 *
 *    @param L State currently running.
 *    @param ind Index of the outfit to validate.
 *    @return The outfit (doesn't return if fails - raises Lua error ).
 */
PilotOutfitSlot* luaL_validpilotoutfit( lua_State *L, int ind )
{
   PilotOutfitSlot *o;

   if (lua_ispilotoutfit(L, ind))
      o  = luaL_checkpilotoutfit(L,ind);
   else {
      luaL_typerror(L, ind, PILOTOUTFIT_METATABLE);
      return NULL;
   }

   if (o == NULL)
      NLUA_ERROR(L, _("Pilot Outfit is invalid."));

   return o;
}
/**
 * @brief Pushes a pilot outfit on the stack.
 *
 *    @param L Lua state to push outfit into.
 *    @param po Pilot outfit to push.
 *    @return Newly pushed pilotoutfit.
 */
PilotOutfitSlot** lua_pushpilotoutfit( lua_State *L, PilotOutfitSlot *po )
{
   PilotOutfitSlot **lpo;
   lpo = (PilotOutfitSlot**) lua_newuserdata(L, sizeof(PilotOutfitSlot*));
   *lpo = po;
   luaL_getmetatable(L, PILOTOUTFIT_METATABLE);
   lua_setmetatable(L, -2);
   return lpo;
}
/**
 * @brief Checks to see if ind is a pilot outfit.
 *
 *    @param L Lua state to check.
 *    @param ind Index position to check.
 *    @return 1 if ind is a pilot outfit.
 */
int lua_ispilotoutfit( lua_State *L, int ind )
{
   int ret;

   if (lua_getmetatable(L,ind)==0)
      return 0;
   lua_getfield(L, LUA_REGISTRYINDEX, PILOTOUTFIT_METATABLE);

   ret = 0;
   if (lua_rawequal(L, -1, -2))  /* does it have the correct mt? */
      ret = 1;

   lua_pop(L, 2);  /* remove both metatables */
   return ret;
}


/**
 * @brief Gets the outfit of the PilotOutfit.
 *
 *    @luatparam PilotOutfit po Pilot outfit to get the outfit of.
 *    @luatparam Outfit Outfit corresponding to the Pilot outfit.
 * @luafunc outfit
 */
static int poL_outfit( lua_State *L )
{
   PilotOutfitSlot *po  = luaL_validpilotoutfit(L,1);
   lua_pushoutfit(L, po->outfit );
   return 1;
}


/**
 * @brief Sets the state of the PilotOutfit.
 *
 *    @luatparam PilotOutfit po Pilot outfit to set the state of.
 *    @luatparam string state State to set the pilot outfit to. Can be either "off", "warmup", "on", or "cooldown".
 * @luafunc state
 */
static int poL_state( lua_State *L )
{
   PilotOutfitSlot *po  = luaL_validpilotoutfit(L,1);
   const char *state = luaL_optstring(L,2,NULL);
   PilotOutfitState pos = po->state;

   if (state==NULL || strcmp(state,"off")==0)
      po->state = PILOT_OUTFIT_OFF;
   else if (strcmp(state,"warmup")==0)
      po->state = PILOT_OUTFIT_WARMUP;
   else if (strcmp(state,"on")==0)
      po->state = PILOT_OUTFIT_ON;
   else if (strcmp(state,"cooldown")==0)
      po->state = PILOT_OUTFIT_COOLDOWN;
   else
      NLUA_ERROR( L, _("Unknown PilotOutfit state '%s'!"), state );

   /* Mark as modified if state changed. */
   if (pos != po->state)
      pilotoutfit_modified = 1;

   return 0;
}


/**
 * @brief Sets the state progress of the PilotOutfit.
 *
 *    @luatparam PilotOutfit po Pilot outfit to set the state of.
 *    @luatparam number progress Progress of the current state with 1 being started and 0 being done.
 * @luafunc progress
 */
static int poL_progress( lua_State *L )
{
   PilotOutfitSlot *po  = luaL_validpilotoutfit(L,1);
   po->progress = CLAMP( 0., 1., luaL_checknumber(L,2) );
   return 0;
}


/**
 * @brief Sets a temporary ship stat modifier of the pilot outfit.
 *
 *    @luatparam PilotOutfit po Pilot outfit to set the modifier of.
 *    @luatparam string mod Modifier name to set to. Same as in the XML definition.
 *    @luatparam number val Value to set the modifier to. In the case of booleans, 0 indicates false, while 1 indicates true.
 * @luafunc set
 */
static int poL_set( lua_State *L )
{
   PilotOutfitSlot *po  = luaL_validpilotoutfit(L,1);
   const char *name = luaL_checkstring(L,2);
   double value = luaL_checknumber(L,3);
   ss_statsSet( &po->lua_stats, name, value, 1 );
   pilotoutfit_modified = 1;
   return 0;
}


/**
 * @brief Clears all the temporary ship stat modifiers of the pilot outfit.
 *
 *    @luatparam PilotOutfit po Pilot outfit to clear temporary modifiers from.
 * @luafunc clear
 */
static int poL_clear( lua_State *L )
{
   PilotOutfitSlot *po  = luaL_validpilotoutfit(L,1);
   ss_statsInit( &po->lua_stats );
   pilotoutfit_modified = 1;
   return 0;
}

