/*
 * See Licensing and Copyright notice in naev.h
 */

/**
 * @file nlua_hook.c
 *
 * @brief Lua hook module.
 */


/** @cond */
#include <lauxlib.h>
#include <lua.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "naev.h"
/** @endcond */

#include "nlua_hook.h"

#include "event.h"
#include "hook.h"
#include "log.h"
#include "mission.h"
#include "nlua_evt.h"
#include "nlua_misn.h"
#include "nlua_pilot.h"
#include "nlua_time.h"
#include "nluadef.h"
#include "nstring.h"


/* Hook methods. */
static int hookL_rm( lua_State *L );
static int hook_load( lua_State *L );
static int hook_land( lua_State *L );
static int hook_takeoff( lua_State *L );
static int hook_jumpout( lua_State *L );
static int hook_jumpin( lua_State *L );
static int hook_enter( lua_State *L );
static int hook_hail( lua_State *L );
static int hook_boarding( lua_State *L );
static int hook_board( lua_State *L );
static int hook_timer( lua_State *L );
static int hook_date( lua_State *L );
static int hook_commbuy( lua_State *L );
static int hook_commsell( lua_State *L );
static int hook_gather( lua_State *L );
static int hook_outfitbuy( lua_State *L );
static int hook_outfitsell( lua_State *L );
static int hook_shipbuy( lua_State *L );
static int hook_shipsell( lua_State *L );
static int hook_input( lua_State *L );
static int hook_mouse( lua_State *L );
static int hook_safe( lua_State *L );
static int hook_update( lua_State *L );
static int hook_renderbg( lua_State *L );
static int hook_renderfg( lua_State *L );
static int hook_standing( lua_State *L );
static int hook_discover( lua_State *L );
static int hook_pay( lua_State *L );
static int hook_custom( lua_State *L );
static int hook_trigger( lua_State *L );
static int hook_pilot( lua_State *L );
static const luaL_Reg hook_methods[] = {
   { "rm", hookL_rm },
   { "load", hook_load },
   { "land", hook_land },
   { "takeoff", hook_takeoff },
   { "jumpout", hook_jumpout },
   { "jumpin", hook_jumpin },
   { "enter", hook_enter },
   { "hail", hook_hail },
   { "boarding", hook_boarding },
   { "board", hook_board },
   { "timer", hook_timer },
   { "date", hook_date },
   { "comm_buy", hook_commbuy },
   { "gather", hook_gather },
   { "comm_sell", hook_commsell },
   { "outfit_buy", hook_outfitbuy },
   { "outfit_sell", hook_outfitsell },
   { "ship_buy", hook_shipbuy },
   { "ship_sell", hook_shipsell },
   { "input", hook_input },
   { "mouse", hook_mouse },
   { "safe", hook_safe },
   { "update", hook_update },
   { "renderbg", hook_renderbg },
   { "renderfg", hook_renderfg },
   { "standing", hook_standing },
   { "discover", hook_discover },
   { "pay", hook_pay },
   { "custom", hook_custom },
   { "trigger", hook_trigger },
   { "pilot", hook_pilot },
   {0,0}
}; /**< Hook Lua methods. */


/*
 * Prototypes.
 */
static int hookL_setarg( unsigned int hook, int ind );
static unsigned int hook_generic( lua_State *L, const char* stack, double ms, int pos, ntime_t date );


/**
 * @brief Loads the hook Lua library.
 *    @param env Lua environment.
 *    @return 0 on success.
 */
int nlua_loadHook( nlua_env env )
{
   nlua_register(env, "hook", hook_methods, 0);
   return 0;
}


/**
 * @brief Lua bindings to manipulate hooks.
 *
 * Hooks allow you to trigger functions to certain actions like when the player
 *  jumps or a pilot dies.
 *
 * They can have arguments passed to them which will then get passed to the
 *  called hook function.
 *
 * Example usage would be:
 * @code
 * function penter( arg )
 *    -- Function to run when player enters a system
 * end
 *
 * hookid = hook.enter( "penter", 5 )
 * @endcode
 *
 * @luamod hook
 */


/**
 * @brief Removes a hook previously created.
 *
 * @usage hook.rm( h ) -- Hook is removed
 *
 *    @luatparam number h Identifier of the hook to remove.
 * @luafunc rm
 */
static int hookL_rm( lua_State *L )
{
   long h;

   /* Remove the hook. */
   h = luaL_optlong( L, 1, -1 );
   /* ... Or do a no-op if caller passes nil. */
   if (h < 0)
      return 0;
   hook_rm( (unsigned int) h );

   /* Clean up hook data. */
   nlua_getenv(__NLUA_CURENV, "__hook_arg");
   if (!lua_isnil(L,-1)) {
      lua_pushnumber( L, h ); /* t, n */
      lua_pushnil( L );       /* t, n, nil */
      lua_settable( L, -3 );  /* t */
   }
   lua_pop( L, 1 );        /* */

   return 0;
}


/**
 * @brief Sets a Lua argument for a hook.
 *
 *    @param hook Hook to set argument for.
 *    @param ind Index of argument to set.
 *    @return 0 on success.
 */
static int hookL_setarg( unsigned int hook, int ind )
{
   nlua_env env = hook_env(hook);

   lua_pushvalue( naevL, ind );   /* v */
   /* If a table set __save, this won't work for tables of tables however. */
   if (lua_istable(naevL, -1)) {
      lua_pushboolean( naevL, 1 );/* v, b */
      lua_setfield( naevL, -2, "__save" ); /* v */
   }
   /* Create if necessary the actual hook argument table. */
   nlua_getenv(env, "__hook_arg"); /* v, t */
   if (lua_isnil(naevL,-1)) {     /* v, nil */
      lua_pop( naevL, 1 );        /* v */
      lua_newtable( naevL );      /* v, t */
      lua_pushvalue( naevL, -1 ); /* v, t, t */
      nlua_setenv(env, "__hook_arg"); /* v, t */
      lua_pushboolean( naevL, 1 ); /* v, t, s */
      lua_setfield( naevL, -2, "__save" ); /* v, t */
   }
   lua_pushnumber( naevL, hook ); /* v, t, k */
   lua_pushvalue( naevL, -3 );    /* v, t, k, v */
   lua_settable( naevL, -3 );     /* v, t */
   lua_pop( naevL, 2 );           /* */
   return 0;
}


/**
 * @brief Unsets a Lua argument.
 */
void hookL_unsetarg( unsigned int hook )
{
   nlua_env env = hook_env(hook);

   if (env == LUA_NOREF)
       return;

   nlua_getenv(env, "__hook_arg"); /* t */
   if (!lua_isnil(naevL,-1)) {
      lua_pushnumber( naevL, hook );      /* t, h */
      lua_pushnil( naevL );               /* t, h, n */
      lua_settable( naevL, -3 );          /* t */
   }
   lua_pop( naevL, 1 );
}


/**
 * @brief Gets a Lua argument for a hook.
 *
 *    @param hook Hook to get argument of.
 *    @return 0 on success.
 */
int hookL_getarg( unsigned int hook )
{
   nlua_env env = hook_env(hook);

   if (env == LUA_NOREF) {
       lua_pushnil(naevL);
       return 0;
   }

   nlua_getenv(env, "__hook_arg"); /* t */
   if (!lua_isnil(naevL,-1)) {    /* t */
      lua_pushnumber( naevL, hook ); /* t, k */
      lua_gettable( naevL, -2 );  /* t, v */
      lua_remove( naevL, -2 );    /* v */
   }
   return 0;
}


/**
 * @brief Creates a mission hook to a certain stack.
 *
 * Basically a generic approach to hooking.
 *
 *    @param L Lua state.
 *    @param stack Stack to put the hook in.
 *    @param ms Milliseconds to delay (pass stack as NULL to set as timer).
 *    @param pos Position in the stack of the function name.
 *    @param date Resolution of the timer. (If passed, create a date-based hook.)
 *    @return The hook ID or 0 on error.
 */
static unsigned int hook_generic( lua_State *L, const char* stack, double ms, int pos, ntime_t date )
{
   int i;
   const char *func;
   unsigned int h;
   Event_t *running_event;
   Mission *running_mission;

   /* Last parameter must be function to hook */
   func = luaL_checkstring(L,pos);

   /* Get stuff. */
   running_event = event_getFromLua(L);
   running_mission = misn_getFromLua(L);

   if (running_mission != NULL) {
      /* make sure mission is a player mission */
      for (i=0; i<MISSION_MAX; i++)
         if (player_missions[i]->id == running_mission->id)
            break;
      if (i>=MISSION_MAX) {
         WARN(_("Mission not in stack trying to hook, forgot to run misn.accept()?"));
         return 0;
      }

      if (stack != NULL)
         h = hook_addMisn( running_mission->id, func, stack );
      else if (date != 0)
         h = hook_addDateMisn( running_mission->id, func, date );
      else
         h = hook_addTimerMisn( running_mission->id, func, ms );
   }
   else if (running_event != NULL) {
      if (stack != NULL)
         h = hook_addEvent( running_event->id, func, stack );
      else if (date != 0)
         h = hook_addDateEvt( running_event->id, func, date );
      else
         h = hook_addTimerEvt( running_event->id, func, ms );
   }
   else {
      NLUA_ERROR(L,_("Attempting to set a hook outside of a mission or event."));
      return 0;
   }

   if (h == 0) {
      NLUA_ERROR(L,_("No hook target was set."));
      return 0;
   }

   /* Check parameter. */
   if (!lua_isnil(L,pos+1))
      hookL_setarg( h, pos+1 );

   return h;
}
/**
 * @brief Hooks the function to the player landing.
 *
 * Can also be used to hook the various subparts of the landing menu. Possible targets
 *  for where are:<br />
 *   - "land" - when landed (default with no parameter )<br />
 *   - "outfits" - when visited outfitter<br />
 *   - "shipyard" - when visited shipyard<br />
 *   - "bar" - when visited bar<br />
 *   - "mission" - when visited mission computer<br />
 *   - "commodity" - when visited commodity exchange<br />
 *   - "equipment" - when visiting equipment place<br />
 *
 * @usage hook.land( "my_function" ) -- Land calls my_function
 * @usage hook.land( "my_function", "equipment" ) -- Calls my_function at equipment screen
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luatparam[opt] string where Where to hook the function.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc land
 */
static int hook_land( lua_State *L )
{
   const char *where;
   unsigned int h;

   if (lua_gettop(L) < 2)
      h = hook_generic( L, "land", 0., 1, 0 );
   else {
      where = luaL_checkstring(L, 2);
      h = hook_generic( L, where, 0., 1, 0 );
   }

   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player loading the game (starts landed).
 *
 * @usage hook.load( "my_function" ) -- Load calls my_function
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc load
 */
static int hook_load( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "load", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player taking off.
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc takeoff
 */
static int hook_takeoff( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "takeoff", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player jumping (before changing systems).
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc jumpout
 */
static int hook_jumpout( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "jumpout", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player jumping (after changing systems).
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc jumpin
 */
static int hook_jumpin( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "jumpin", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player entering a system (triggers when taking
 *  off too).
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc enter
 */
static int hook_enter( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "enter", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player hailing any ship (not a planet).
 *
 * The hook receives a single parameter which is the ship being hailed.
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc hail
 */
static int hook_hail( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "hail", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player boarding any ship.
 *
 * The hook receives a single parameter which is the ship doing the boarding.
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc boarding
 */
static int hook_boarding( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "boarding", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player boarding any ship.
 *
 * The hook receives a single parameter which is the ship being boarded.
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc board
 */
static int hook_board( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "board", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks a timer.
 *
 * The hook receives only the optional argument.
 *
 *    @luatparam number ms Milliseconds to delay.
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc timer
 */
static int hook_timer( lua_State *L )
{
   unsigned int h;
   double ms;
   ms = luaL_checknumber( L, 1 );
   h  = hook_generic( L, NULL, ms/1000., 2, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks a date change with custom resolution.
 *
 * The hook receives only the optional argument.
 *
 * @usage hook.date( time.create( 0, 0, 1000 ), "some_func", nil ) -- Hooks with a 1000 second resolution
 *
 *    @luatparam Time resolution Resolution of the timer (should be a time structure).
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc date
 */
static int hook_date( lua_State *L )
{
   unsigned int h;
   ntime_t t;
   t  = luaL_validtime( L, 1 );
   h  = hook_generic( L, NULL, 0., 2, t );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player buying any sort of commodity.
 *
 * The hook receives the name of the commodity and the quantity being bought.
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc comm_buy
 */
static int hook_commbuy( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "comm_buy", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player selling any sort of commodity.
 *
 * The hook receives the name of the commodity and the quantity being bought.
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc comm_sell
 */
static int hook_commsell( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "comm_sell", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player gatehring any sort of commodity in space.
 *
 * The hook receives the name of the commodity and the quantity being gathered.
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc gather
 */
static int hook_gather( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "gather", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player buying any sort of outfit.
 *
 * The hook receives the name of the outfit and the quantity being bought.
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc outfit_buy
 */
static int hook_outfitbuy( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "outfit_buy", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player selling any sort of outfit.
 *
 * The hook receives the name of the outfit and the quantity being sold.
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc outfit_sell
 */
static int hook_outfitsell( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "outfit_sell", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player buying any sort of ship.
 *
 * The hook receives the name of the ship type bought.
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc ship_buy
 */
static int hook_shipbuy( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "ship_buy", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player selling any sort of ship.
 *
 * The hook receives the name of the ship type sold and the player-given name of the ship.
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc ship_sell
 */
static int hook_shipsell( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "ship_sell", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player pressing any input.
 *
 * It returns the name of the key being pressed like "accel" and whether or not it's a press.<br/>
 * <br/>
 * Functions should be in format:<br/>
 *   function f( inputname, inputpress, args )
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc input
 */
static int hook_input( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "input", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to the player clicking the mouse.
 *
 * The parameter passed to the function is the button pressed (1==left,2==middle,3==right).
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc mouse
 */
static int hook_mouse( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "mouse", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to any faction standing change.
 *
 * The parameters passed to the function are faction whose standing is being
 * changed and the amount changed:<br/>
 * function f( faction, change, args )
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc standing
 */
static int hook_standing( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "standing", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to when the player discovers an asset, jump point or the likes.
 *
 * The parameters passed to the function are the type which can be one of:<br/>
 * - "asset" <br/>
 * - "jump" <br/>
 * and the actual asset or jump point discovered with the following format: <br/>
 * function f( type, discovery )
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc discover
 */
static int hook_discover( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "discover", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hooks the function to when the player receives  or loses money through player.pay() (the Lua function only).
 *
 * The amount paid (or taken from the player) and reason (which is nil by default) is passed as a parameter:<br/>
 * function f( amount, reason, args )
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc pay
 */
static int hook_pay( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "pay", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hook run once at the end of the next frame regardless of anything that can happen.
 *
 * This hook is a good way to do possibly breaking stuff like for example player.teleport().
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc safe
 */
static int hook_safe( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "safe", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hook run at the end of each frame when the update routine is run (game is not paused, etc.).
 *
 * It is closely related to hook.safe(), but you have to manually remove it or it continues forever.
 *
 * The current delta-tick (time passed in game) and real delta-tick (independent of game status) are passed as parameters:<br/>
 * function f( dt, real_dt, args )
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc update
 */
static int hook_update( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "update", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hook that runs during rendering the background (just above the static background stuff). Meant to be only for rendering things.
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc renderbg
 */
static int hook_renderbg( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "renderbg", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hook that runs during rendering the foreground (just below the gui stuff). Meant to be only for rendering things.
 *
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc renderfg
 */
static int hook_renderfg( lua_State *L )
{
   unsigned int h;
   h = hook_generic( L, "renderfg", 0., 1, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Hook run once at the end of the next frame regardless when manually triggered.
 *
 *    @luatparam string hookname Name to give the hook. This should not overlap with standard names.
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @see safe
 * @luafunc custom
 */
static int hook_custom( lua_State *L )
{
   unsigned int h;
   const char *hookname = luaL_checkstring(L,1);
   h = hook_generic( L, hookname, 0., 2, 0 );
   lua_pushnumber( L, h );
   return 1;
}
/**
 * @brief Triggers manually a hook stack. This is run deferred (next frame). Meant mainly to be used with hook.custom, but can work with other hooks too (if you know what you are doing).
 *
 * You can pass multiple parameters that get directly passed to the hook. However, this is limited by HOOK_MAX_PARAM.
 *
 *    @luatparam string hookname Name of the hook to be run.
 * @see custom
 * @luafunc trigger
 */
static int hook_trigger( lua_State *L )
{
   int i, n;
   HookParam hp[HOOK_MAX_PARAM], *p;
   const char *hookname = luaL_checkstring(L,1);
   n = lua_gettop(L);

   /* Set up hooks. */
   for (i=0; i< MIN(n, HOOK_MAX_PARAM-1); i++) {
      p = &hp[i];
      switch (lua_type(L,i+1)) {
         case LUA_TNIL:
            p->type = HOOK_PARAM_NIL;
            break;
         case LUA_TNUMBER:
            p->type = HOOK_PARAM_NUMBER;
            p->u.num = lua_tonumber(L,i+1);
            break;
         case LUA_TBOOLEAN:
            p->type = HOOK_PARAM_BOOL;
            p->u.b = lua_toboolean(L,i+1);
            break;
         case LUA_TSTRING:
            p->type = HOOK_PARAM_STRING;
            p->u.str = lua_tostring(L,i+1);
            break;
         case LUA_TUSERDATA:
            if (lua_ispilot(L,i+1)) {
               p->type = HOOK_PARAM_PILOT;
               p->u.lp = lua_topilot(L,i+1);
            }
            else if (lua_isfaction(L,i+1)) {
               p->type = HOOK_PARAM_FACTION;
               p->u.lf = lua_tofaction(L,i+1);
            }
            else if (lua_isplanet(L,i+1)) {
               p->type = HOOK_PARAM_ASSET;
               p->u.la = *lua_toplanet(L,i+1);
            }
            else if (lua_isjump(L,i+1)) {
               p->type = HOOK_PARAM_JUMP;
               p->u.lj = *lua_tojump(L,i+1);
            }
            break;
         default:
            NLUA_ERROR(L, _("Unsupported Lua hook paramater type '%s'!"), lua_typename(L,i+1));
      }
   }
   hp[i].type = HOOK_PARAM_SENTINEL;

   /* Run the deferred hooks. */
   hooks_runParamDeferred( hookname, hp );
   return 0;
}
/**
 * @brief Hooks the function to a specific pilot.
 *
 * You can hook to different actions.  Currently hook system only supports:<br />
 * <ul>
 *    <li> "death" : triggered when pilot dies (before marked as dead). </li>
 *    <li> "exploded" : triggered when pilot has died and the final explosion has begun. </li>
 *    <li> "boarding" : triggered when a pilot boards another ship (start of boarding).</li>
 *    <li> "board" : triggered when a pilot is boarded (start of boarding).</li>
 *    <li> "disable" : triggered when pilot is disabled (with disable set).</li>
 *    <li> "undisable" : triggered when pilot recovers from being disabled.</li>
 *    <li> "jump" : triggered when pilot jumps to hyperspace (before he actually jumps out).</li>
 *    <li> "hail" : triggered when pilot is hailed.</li>
 *    <li> "land" : triggered when pilot is landing (right when starting land descent).</li>
 *    <li> "attacked" : triggered when the pilot is attacked. </li>
 *    <li> "idle" : triggered when the pilot becomes idle in manual control.</li>
 *    <li> "lockon" : triggered when the pilot locked on a missile on it's target.</li>
 *    <li> "stealth" : triggered when the pilot either enters or leaves stealth.</li>
 * </ul>
 * <br />
 * If you pass nil as pilot, it will set it as a global hook that will jump for all pilots.<br />
 * <br />
 * DO NOT DO UNSAFE THINGS IN PILOT HOOKS. THIS MEANS STUFF LIKE player.teleport(). IF YOU HAVE DOUBTS USE A "safe" HOOK.<br />
 * <br />
 * These hooks all pass the pilot triggering the hook as a parameter, so they should have the structure of:
 * <p>
 *    function my_hook( pilot, arg )<br />
 *    end
 * </p>
 * The combat hooks also pass the pilot acting on it, so for example the pilot
 *  that disabled, attacked or killed the selected pilot. They have the
 *  following format:
 * <p>
 *    function combat_hook( pilot, attacker, arg )<br />
 *    end
 * </p>
 * Please note that in the case of disable or death hook the attacker may be nil
 *  indicating that it was killed by other means like for example the shockwave
 *  of a dying ship or nebula volatility.<br />
 * <br />
 * The land and jump hooks also pass the asset or jump point the pilot is
 * landing at or jumped from, respectively:
 * <p>
 *    function land_hook( pilot, planet, arg )<br />
 *    end
 * </p>
 * <p style="margin-bottom: 0">
 *    function jump_hook( pilot, jump_point, arg )<br />
 *    end
 * </p>
 * The stealth hook passes whether or not the ship is stealthing or destealthing as a boolean:
 * <p>
 *    function stealth_hook( pilot, status, arg )<br />
 *    end
 * </p>
 *    @luatparam Pilot|nil pilot Pilot identifier to hook (or nil for all).
 *    @luatparam string type One of the supported hook types.
 *    @luatparam string funcname Name of function to run when hook is triggered.
 *    @luaparam arg Argument to pass to hook.
 *    @luatreturn number Hook identifier.
 * @luafunc pilot
 */
static int hook_pilot( lua_State *L )
{
   unsigned int h;
   LuaPilot p;
   int type;
   const char *hook_type;
   char buf[ PATH_MAX ];

   /* Parameters. */
   if (lua_ispilot(L,1))
      p           = luaL_checkpilot(L,1);
   else if (lua_isnil(L,1))
      p           = 0;
   else {
      NLUA_ERROR(L, _("Invalid parameter #1 for hook.pilot, expecting pilot or nil."));
      return 0;
   }
   hook_type   = luaL_checkstring(L,2);

   /* Check to see if hook_type is valid */
   if (strcmp(hook_type,"death")==0)         type = PILOT_HOOK_DEATH;
   else if (strcmp(hook_type,"exploded")==0) type = PILOT_HOOK_EXPLODED;
   else if (strcmp(hook_type,"boarding")==0) type = PILOT_HOOK_BOARDING;
   else if (strcmp(hook_type,"board")==0)    type = PILOT_HOOK_BOARD;
   else if (strcmp(hook_type,"disable")==0)  type = PILOT_HOOK_DISABLE;
   else if (strcmp(hook_type,"undisable")==0) type = PILOT_HOOK_UNDISABLE;
   else if (strcmp(hook_type,"jump")==0)     type = PILOT_HOOK_JUMP;
   else if (strcmp(hook_type,"hail")==0)     type = PILOT_HOOK_HAIL;
   else if (strcmp(hook_type,"land")==0)     type = PILOT_HOOK_LAND;
   else if (strcmp(hook_type,"attacked")==0) type = PILOT_HOOK_ATTACKED;
   else if (strcmp(hook_type,"idle")==0)     type = PILOT_HOOK_IDLE;
   else if (strcmp(hook_type,"lockon")==0)   type = PILOT_HOOK_LOCKON;
   else if (strcmp(hook_type,"stealth")==0)  type = PILOT_HOOK_STEALTH;
   else { /* hook_type not valid */
      NLUA_ERROR(L, _("Invalid pilot hook type: '%s'"), hook_type);
      return 0;
   }

   /* actually add the hook */
   snprintf( buf, sizeof(buf), "p_%s", hook_type );
   h = hook_generic( L, buf, 0., 3, 0 );
   if (p==0)
      pilots_addGlobalHook( type, h );
   else
      pilot_addHook( pilot_get(p), type, h );

   lua_pushnumber( L, h );
   return 1;
}
