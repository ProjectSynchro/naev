/*
 * See Licensing and Copyright notice in naev.h
 */

/**
 * @file nlua_outfit.c
 *
 * @brief Handles the Lua outfit bindings.
 */

/** @cond */
#include <lauxlib.h>

#include "naev.h"
/** @endcond */

#include "nlua_outfit.h"

#include "log.h"
#include "nlua_pilot.h"
#include "nlua_tex.h"
#include "nluadef.h"
#include "rng.h"
#include "slots.h"


/* Outfit metatable methods. */
static int outfitL_eq( lua_State *L );
static int outfitL_get( lua_State *L );
static int outfitL_name( lua_State *L );
static int outfitL_nameRaw( lua_State *L );
static int outfitL_type( lua_State *L );
static int outfitL_typeBroad( lua_State *L );
static int outfitL_cpu( lua_State *L );
static int outfitL_mass( lua_State *L );
static int outfitL_slot( lua_State *L );
static int outfitL_limit( lua_State *L );
static int outfitL_icon( lua_State *L );
static int outfitL_price( lua_State *L );
static int outfitL_description( lua_State *L );
static int outfitL_unique( lua_State *L );
static int outfitL_getShipStat( lua_State *L );
static int outfitL_weapStats( lua_State *L );
static const luaL_Reg outfitL_methods[] = {
   { "__tostring", outfitL_name },
   { "__eq", outfitL_eq },
   { "get", outfitL_get },
   { "name", outfitL_name },
   { "nameRaw", outfitL_nameRaw },
   { "type", outfitL_type },
   { "typeBroad", outfitL_typeBroad },
   { "cpu", outfitL_cpu },
   { "mass", outfitL_mass },
   { "slot", outfitL_slot },
   { "limit", outfitL_limit },
   { "icon", outfitL_icon },
   { "price", outfitL_price },
   { "description", outfitL_description },
   { "unique", outfitL_unique },
   { "shipstat", outfitL_getShipStat },
   { "weapstats", outfitL_weapStats },
   {0,0}
}; /**< Outfit metatable methods. */



/**
 * @brief Loads the outfit library.
 *
 *    @param env Environment to load outfit library into.
 *    @return 0 on success.
 */
int nlua_loadOutfit( nlua_env env )
{
   nlua_register(env, OUTFIT_METATABLE, outfitL_methods, 1);
   return 0;
}


/**
 * @brief Lua bindings to interact with outfits.
 *
 * This will allow you to create and manipulate outfits in-game.
 *
 * An example would be:
 * @code
 * o = outfit.get( "Heavy Laser" ) -- Gets the outfit by name
 * cpu_usage = o:cpu() -- Gets the cpu usage of the outfit
 * slot_name, slot_size = o:slot() -- Gets slot information about the outfit
 * @endcode
 *
 * @luamod outfit
 */
/**
 * @brief Gets outfit at index.
 *
 *    @param L Lua state to get outfit from.
 *    @param ind Index position to find the outfit.
 *    @return Outfit found at the index in the state.
 */
Outfit* lua_tooutfit( lua_State *L, int ind )
{
   return *((Outfit**) lua_touserdata(L,ind));
}
/**
 * @brief Gets outfit at index or raises error if there is no outfit at index.
 *
 *    @param L Lua state to get outfit from.
 *    @param ind Index position to find outfit.
 *    @return Outfit found at the index in the state.
 */
Outfit* luaL_checkoutfit( lua_State *L, int ind )
{
   if (lua_isoutfit(L,ind))
      return lua_tooutfit(L,ind);
   luaL_typerror(L, ind, OUTFIT_METATABLE);
   return NULL;
}
/**
 * @brief Makes sure the outfit is valid or raises a Lua error.
 *
 *    @param L State currently running.
 *    @param ind Index of the outfit to validate.
 *    @return The outfit (doesn't return if fails - raises Lua error ).
 */
Outfit* luaL_validoutfit( lua_State *L, int ind )
{
   Outfit *o;

   if (lua_isoutfit(L, ind))
      o  = luaL_checkoutfit(L,ind);
   else if (lua_isstring(L, ind))
      o = outfit_get( lua_tostring(L, ind) );
   else {
      luaL_typerror(L, ind, OUTFIT_METATABLE);
      return NULL;
   }

   if (o == NULL)
      NLUA_ERROR(L, _("Outfit is invalid."));

   return o;
}
/**
 * @brief Pushes a outfit on the stack.
 *
 *    @param L Lua state to push outfit into.
 *    @param outfit Outfit to push.
 *    @return Newly pushed outfit.
 */
Outfit** lua_pushoutfit( lua_State *L, Outfit *outfit )
{
   Outfit **o;
   o = (Outfit**) lua_newuserdata(L, sizeof(Outfit*));
   *o = outfit;
   luaL_getmetatable(L, OUTFIT_METATABLE);
   lua_setmetatable(L, -2);
   return o;
}
/**
 * @brief Checks to see if ind is a outfit.
 *
 *    @param L Lua state to check.
 *    @param ind Index position to check.
 *    @return 1 if ind is a outfit.
 */
int lua_isoutfit( lua_State *L, int ind )
{
   int ret;

   if (lua_getmetatable(L,ind)==0)
      return 0;
   lua_getfield(L, LUA_REGISTRYINDEX, OUTFIT_METATABLE);

   ret = 0;
   if (lua_rawequal(L, -1, -2))  /* does it have the correct mt? */
      ret = 1;

   lua_pop(L, 2);  /* remove both metatables */
   return ret;
}


/**
 * @brief Checks to see if two outfits are the same.
 *
 * @usage if o1 == o2 then -- Checks to see if outfit o1 and o2 are the same
 *
 *    @luatparam Outfit o1 First outfit to compare.
 *    @luatparam Outfit o2 Second outfit to compare.
 *    @luatreturn boolean true if both outfits are the same.
 * @luafunc __eq
 */
static int outfitL_eq( lua_State *L )
{
   Outfit *a, *b;

   a = luaL_checkoutfit(L,1);
   b = luaL_checkoutfit(L,2);
   if (a == b)
      lua_pushboolean(L,1);
   else
      lua_pushboolean(L,0);
   return 1;
}




/**
 * @brief Gets a outfit.
 *
 * @usage s = outfit.get( "Heavy Laser" ) -- Gets the heavy laser
 *
 *    @luatparam string s Raw (untranslated) name of the outfit to get.
 *    @luatreturn Outfit|nil The outfit matching name or nil if error.
 * @luafunc get
 */
static int outfitL_get( lua_State *L )
{
   const char *name;
   Outfit *lo;

   /* Handle parameters. */
   name = luaL_checkstring(L,1);

   /* Get outfit. */
   lo = outfit_get( name );
   if (lo == NULL) {
      NLUA_ERROR(L,_("Outfit '%s' not found!"), name);
      return 0;
   }

   /* Push. */
   lua_pushoutfit(L, lo);
   return 1;
}


/**
 * @brief Gets the translated name of the outfit.
 *
 * This translated name should be used for display purposes (e.g.
 * messages). It cannot be used as an identifier for the outfit; for
 * that, use outfit.nameRaw() instead.
 *
 * @usage outfitname = s:name() -- Equivalent to `_(s:nameRaw())`
 *
 *    @luatparam Outfit s Outfit to get the translated name of.
 *    @luatreturn string The translated name of the outfit.
 * @luafunc name
 */
static int outfitL_name( lua_State *L )
{
   Outfit *o;

   /* Get the outfit. */
   o  = luaL_validoutfit(L,1);

   /** Return the outfit name. */
   lua_pushstring(L, _(o->name));
   return 1;
}


/**
 * @brief Gets the raw (untranslated) name of the outfit.
 *
 * This untranslated name should be used for identification purposes
 * (e.g. can be passed to outfit.get()). It should not be used directly
 * for display purposes without manually translating it with _().
 *
 * @usage outfitrawname = s:nameRaw()
 *
 *    @luatparam Outfit s Outfit to get the raw name of.
 *    @luatreturn string The raw name of the outfit.
 * @luafunc nameRaw
 */
static int outfitL_nameRaw( lua_State *L )
{
   Outfit *o;

   /* Get the outfit. */
   o  = luaL_validoutfit(L,1);

   /** Return the outfit name. */
   lua_pushstring(L, o->name);
   return 1;
}


/**
 * @brief Gets the type of an outfit.
 *
 * @usage print( o:type() ) -- Prints the type of the outfit
 *
 *    @luatparam Outfit o Outfit to get information of.
 *    @luatreturn string The name of the outfit type (in English).
 * @luafunc type
 */
static int outfitL_type( lua_State *L )
{
   Outfit *o = luaL_validoutfit(L,1);
   lua_pushstring(L, outfit_getType(o));
   return 1;
}


/**
 * @brief Gets the broad type of an outfit.
 *
 * This name is more generic and vague than type().
 *
 * @usage print( o:typeBroad() ) -- Prints the broad type of the outfit
 *
 *    @luatparam Outfit o Outfit to get information of.
 *    @luatreturn string The name of the outfit broad type (in English).
 * @luafunc typeBroad
 */
static int outfitL_typeBroad( lua_State *L )
{
   Outfit *o = luaL_validoutfit(L,1);
   lua_pushstring(L, outfit_getTypeBroad(o));
   return 1;
}


/**
 * @brief Gets the cpu usage of an outfit.
 *
 * @usage print( o:cpu() ) -- Prints the cpu usage of an outfit
 *
 *    @luatparam Outfit o Outfit to get information of.
 *    @luatreturn string The amount of cpu the outfit uses.
 * @luafunc cpu
 */
static int outfitL_cpu( lua_State *L )
{
   Outfit *o = luaL_validoutfit(L,1);
   lua_pushnumber(L, outfit_cpu(o));
   return 1;
}


/**
 * @brief Gets the mass of an outfit.
 *
 * @usage print( o:mass() ) -- Prints the mass of an outfit
 *
 *    @luatparam Outfit o Outfit to get information of.
 *    @luatreturn string The amount of mass the outfit uses.
 * @luafunc mass
 */
static int outfitL_mass( lua_State *L )
{
   Outfit *o = luaL_validoutfit(L,1);
   lua_pushnumber(L, o->mass);
   return 1;
}


/**
 * @brief Gets the slot name, size and property of an outfit.
 *
 * @usage slot_name, slot_size, slot_prop = o:slot() -- Gets an outfit's slot info
 *
 *    @luatparam Outfit o Outfit to get information of.
 *    @luatreturn string Human readable name (in English).
 *    @luatreturn string Human readable size (in English).
 *    @luatreturn string Human readable property (in English).
 *    @luatreturn boolean Slot is required.
 *    @luatreturn boolean Slot is exclusive.
 * @luafunc slot
 */
static int outfitL_slot( lua_State *L )
{
   Outfit *o = luaL_validoutfit(L,1);
   lua_pushstring(L, outfit_slotName(o));
   lua_pushstring(L, outfit_slotSize(o));
   lua_pushstring(L, sp_display( o->slot.spid ));
   lua_pushboolean(L, sp_required( o->slot.spid ));
   lua_pushboolean(L, sp_exclusive( o->slot.spid ));
   return 5;
}


/**
 * @brief Gets the limit string of the outfit. Only one outfit can be equipped at the same time for each limit string.
 *
 *    @luatparam Outfit o Outfit to get information of.
 *    @luatreturn string|nil Limit string or nil if not applicable.
 * @luafunc limit
 */
static int outfitL_limit( lua_State *L )
{
   Outfit *o = luaL_validoutfit(L,1);
   if (o->limit)
      lua_pushstring(L,o->limit);
   else
      lua_pushnil(L);
   return 1;
}


/**
 * @brief Gets the store icon for an outfit.
 *
 * @usage ico = o:icon() -- Gets the shop icon for an outfit
 *
 *    @luatparam Outfit o Outfit to get information of.
 *    @luatreturn Tex The texture containing the icon of the outfit.
 * @luafunc icon
 */
static int outfitL_icon( lua_State *L )
{
   Outfit *o = luaL_validoutfit(L,1);
   lua_pushtex( L, gl_dupTexture( o->gfx_store ) );
   return 1;
}


/**
 * @brief Gets the price of an outfit.
 *
 * @usage price = o:price()
 *
 *    @luatparam String o Outfit to get the price of.
 *    @luatreturn number The price, in credits.
 * @luafunc price
 */
static int outfitL_price( lua_State *L )
{
   Outfit *o = luaL_validoutfit(L,1);
   lua_pushnumber(L, o->price);
   return 1;
}


/**
 * @brief Gets the description of an outfit.
 *
 * @usage description = o:description()
 *
 *    @luatparam String o Outfit to get the description of.
 *    @luatreturn string The description (without translating).
 * @luafunc description
 */
static int outfitL_description( lua_State *L )
{
   Outfit *o = luaL_validoutfit(L,1);
   lua_pushstring(L, o->description);
   return 1;
}


/**
 * @brief Gets whether or not an outfit is unique
 *
 * @usage isunique = o:unique()
 *
 *    @luatparam String o Outfit to get the uniqueness of.
 *    @luatreturn boolean The uniqueness of the outfit.
 * @luafunc unique
 */
static int outfitL_unique( lua_State *L )
{
   Outfit *o = luaL_validoutfit(L,1);
   lua_pushboolean(L, outfit_isProp(o, OUTFIT_PROP_UNIQUE));
   return 1;
}


/**
 * @brief Gets a shipstat from an Outfit by name, or a table containing all the ship stats if not specified.
 *
 *    @luatparam Outfit o Outfit to get ship stat of.
 *    @luatparam[opt=nil] string name Name of the ship stat to get.
 *    @luatparam[opt=false] boolean internal Whether or not to use the internal representation.
 *    @luareturn Value of the ship stat or a tale containing all the ship stats if name is not specified.
 * @luafunc shipstat
 */
static int outfitL_getShipStat( lua_State *L )
{
   ShipStats ss;
   Outfit *o = luaL_validoutfit(L,1);
   ss_statsInit( &ss );
   ss_statsModFromList( &ss, o->stats );
   const char *str = luaL_optstring(L,2,NULL);
   int internal      = lua_toboolean(L,3);
   ss_statsGetLua( L, &ss, str, internal );
   return 1;
}


/**
 * @brief Computes the DPS and EPS for weapons.
 *
 *    @luatparam Outfit o Outfit to compute for.
 *    @luatparam[opt=nil] Pilot p Pilot to use ship stats when computing.
 *    @luatreturn number DPS of the outfit.
 *    @luatreturn number EPS of the outfit.
 * @luafunc weapstats
 */
static int outfitL_weapStats( lua_State *L )
{
   double eps, dps, shots;
   double mod_energy, mod_damage, mod_shots;
   const Damage *dmg;
   Outfit *o = luaL_validoutfit( L, 1 );
   Pilot *p = (lua_ispilot(L,2)) ? luaL_validpilot(L,2) : NULL;

   /* Just return 0 for non-wapons. */
   if (o->slot.type != OUTFIT_SLOT_WEAPON) {
      lua_pushnumber( L, 0. );
      lua_pushnumber( L, 0. );
      return 2;
   }

   /* Special case beam weapons .*/
   if (outfit_isBeam(o)) {
      if (p) {
         /* Special case due to continuous fire. */
         if (o->type == OUTFIT_TYPE_BEAM) {
            mod_energy = p->stats.fwd_energy;
            mod_damage = p->stats.fwd_damage;
            mod_shots  = 1. / p->stats.fwd_firerate;
         }
         else {
            mod_energy = p->stats.tur_energy;
            mod_damage = p->stats.tur_damage;
            mod_shots  = 1. / p->stats.tur_firerate;
         }
      }
      else {
         mod_energy = 1.;
         mod_damage = 1.;
         mod_shots = 1.;
      }
      shots = outfit_duration(o);
      mod_shots = shots / (shots + mod_shots * outfit_delay(o));
      dps = mod_shots * mod_damage * outfit_damage(o)->damage;
      eps = mod_shots * mod_energy * outfit_energy(o);
      lua_pushnumber( L, dps );
      lua_pushnumber( L, eps );
      return 2;
   }

   if (p) {
      switch (o->type) {
         case OUTFIT_TYPE_BOLT:
            mod_energy = p->stats.fwd_energy;
            mod_damage = p->stats.fwd_damage;
            mod_shots  = 1. / p->stats.fwd_firerate;
            break;
         case OUTFIT_TYPE_TURRET_BOLT:
            mod_energy = p->stats.tur_energy;
            mod_damage = p->stats.tur_damage;
            mod_shots  = 1. / p->stats.tur_firerate;
            break;
         case OUTFIT_TYPE_LAUNCHER:
         case OUTFIT_TYPE_TURRET_LAUNCHER:
            mod_energy = 1.;
            mod_damage = p->stats.launch_damage;
            mod_shots  = 1. / p->stats.launch_rate;
            break;
         case OUTFIT_TYPE_BEAM:
         case OUTFIT_TYPE_TURRET_BEAM:
         default:
            return 0;
      }
   }
   else {
      mod_energy = 1.;
      mod_damage = 1.;
      mod_shots = 1.;
   }

   shots = 1. / (mod_shots * outfit_delay(o));
   /* Special case: Ammo-based weapons. */
   if (outfit_isLauncher(o))
      dmg = outfit_damage(o->u.lau.ammo);
   else
      dmg = outfit_damage(o);
   dps = shots * mod_damage * dmg->damage;
   eps = shots * mod_energy * MAX( outfit_energy(o), 0. );

   lua_pushnumber( L, dps );
   lua_pushnumber( L, eps );
   return 2;
}


