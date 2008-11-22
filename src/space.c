/*
 * See Licensing and Copyright notice in naev.h
 */

/**
 * @file space.c
 *
 * @brief Handles all the space stuff, namely systems and planets.
 */

#include "space.h"

#include <malloc.h>
#include <math.h>

#include "xml.h"

#include "naev.h"
#include "opengl.h"
#include "log.h"
#include "rng.h"
#include "pack.h"
#include "player.h"
#include "pause.h"
#include "weapon.h"
#include "toolkit.h"
#include "spfx.h"
#include "ntime.h"
#include "nebulae.h"


#define XML_PLANET_ID         "Planets" /**< Planet xml document tag. */
#define XML_PLANET_TAG        "planet" /**< Individual planet xml tag. */

#define XML_SYSTEM_ID         "Systems" /**< Systems xml document tag. */
#define XML_SYSTEM_TAG        "ssys" /**< Individual systems xml tag. */

#define PLANET_DATA           "dat/planet.xml" /**< XML file containing planets. */
#define SYSTEM_DATA           "dat/ssys.xml" /**< XML file containing systems. */

#define PLANET_GFX_SPACE      "gfx/planet/space/" /**< Location of planet space graphics. */
#define PLANET_GFX_EXTERIOR   "gfx/planet/exterior/" /**< Location of planet exterior graphics (when landed). */

#define PLANET_GFX_EXTERIOR_W 400 /**< Planet exterior graphic width. */
#define PLANET_GFX_EXTERIOR_H 400 /**< Planet exterior graphic height. */

#define CHUNK_SIZE            32 /**< Size to allocate by. */
#define CHUNK_SIZE_SMALL       8 /**< Smaller size to allocate chunks by. */

/* used to overcome warnings due to 0 values */
#define FLAG_XSET             (1<<0) /**< Set the X position value. */
#define FLAG_YSET             (1<<1) /**< Set the Y position value. */
#define FLAG_ASTEROIDSSET     (1<<2) /**< Set the asteroid value. */
#define FLAG_INTERFERENCESET  (1<<3) /**< Set the interference value. */
#define FLAG_SERVICESSET      (1<<4) /**< Set the service value. */
#define FLAG_TECHSET          (1<<5) /**< Set the tech value. */
#define FLAG_FACTIONSET       (1<<6) /**< Set the faction value. */


/*
 * planet <-> system name stack
 */
static char** planetname_stack = NULL; /**< Planet name stack corresponding to system. */
static char** systemname_stack = NULL; /**< System name stack corresponding to planet. */
static int spacename_nstack = 0; /**< Size of planet<->system stack. */
static int spacename_mstack = 0; /**< Size of memory in planet<->system stack. */


/* 
 * Star system stack.
 */
StarSystem *systems_stack = NULL; /**< Star system stack. */
int systems_nstack = 0; /**< Number of star systems. */
static int systems_mstack = 0; /**< Number of memory allocated for star system stack. */

/*
 * Planet stack.
 */
static Planet *planet_stack = NULL; /**< Planet stack. */
static int planet_nstack = 0; /**< Planet stack size. */
static int planet_mstack = 0; /**< Memory size of planet stack. */

/*
 * Misc.
 */
StarSystem *cur_system = NULL; /**< Current star system. */


/*
 * fleet spawn rate
 */
int space_spawn = 1; /**< Spawn enabled by default. */
static double spawn_timer = 0; /**< Timer that controls spawn rate. */


/*
 * star stack and friends
 */
#define STAR_BUF  100   /**< Area to leave around screen for stars, more = less repitition */
/**
 * @struct Star
 *
 * @brief Represents a background star. */
typedef struct Star_ {
   double x; /**< X position of the star. */
   double y; /**< Y position of the star. */
   double brightness; /**< Brightness of the star, also affects movement speed. */
} Star;
static Star *stars = NULL; /**< star array */
static int nstars = 0; /**< total stars */
static int mstars = 0; /**< memory stars are taking */


/*
 * External stuff.
 */
extern int planet_target; /* player.c */


/* 
 * Internal Prototypes.
 */
/* planet load */
static int planet_parse( Planet* planet, const xmlNodePtr parent );
/* system load */
static int systems_load (void);
static StarSystem* system_parse( StarSystem *system, const xmlNodePtr parent );
static void system_parseJumps( const xmlNodePtr parent );
/* misc */
static void system_setFaction( StarSystem *sys );
static void space_renderStars( const double dt );
static void space_addFleet( Fleet* fleet, int init );
static PlanetClass planetclass_get( const char a );
/*
 * External prototypes.
 */
extern void player_message ( const char *fmt, ... );
/*
 * Externed prototypes.
 */
void planets_minimap( const double res, const double w,
      const double h, const RadarShape shape );
int space_sysSave( xmlTextWriterPtr writer );
int space_sysLoad( xmlNodePtr parent );



#define PIXEL(x,y)      \
   if ((shape==RADAR_RECT && ABS(x)<w/2. && ABS(y)<h/2.) || \
         (shape==RADAR_CIRCLE && (((x)*(x)+(y)*(y))<rc)))   \
   glVertex2i((x),(y))
/**
 * @brief Draws the planets in the minimap.  Used by player.c.
 *
 * Matrix mode is already displaced to center of the minimap.
 *
 *    @param res Current minimap resolution.
 *    @param w Current minimap width.
 *    @param h Current minimap height.
 *    @param shape Current minimap shape.
 */
void planets_minimap( const double res, const double w,
      const double h, const RadarShape shape )
{
   int i;
   int cx, cy, x, y, r, rc;
   double p;
   double a, tx,ty;
   Planet *planet;
   glColour *col;

   if (shape==RADAR_CIRCLE)
      rc = (int)(w*w);

   for (i=0; i<cur_system->nplanets; i++) {
      planet = cur_system->planets[i];

      /* Get the colour. */
      col = faction_getColour(planet->faction);
      if (i == planet_target)
         col = &cRadar_tPlanet;
      else if ((col != &cHostile) && !planet_hasService(planet,PLANET_SERVICE_BASIC))
         col = &cInert; /* Override non-hostile planets without service. */
      COLOUR(*col);

      /* Some parameters. */
      r = (int)(cur_system->planets[i]->gfx_space->sw / res);
      cx = (int)((cur_system->planets[i]->pos.x - player->solid->pos.x) / res);
      cy = (int)((cur_system->planets[i]->pos.y - player->solid->pos.y) / res);

      /* Check if in range. */
      if (shape == RADAR_RECT) {
         /* Out of range. */
         if ((ABS(cx) - r > w/2.) || (ABS(cy) - r  > h/2.))
            continue;
      }
      else if (shape == RADAR_CIRCLE) {
         x = cx-r;
         y = cy-r;
         /* Out of range. */
         if (x*x + y*y > rc) {
            if (planet_target == i) {
               /* Draw a line like for pilots. */
               a = ANGLE(cx,cy);
               tx = w*cos(a);
               ty = w*sin(a);

               COLOUR(cRadar_tPlanet);
               glBegin(GL_LINES);
                  glVertex2d(      tx,      ty );
                  glVertex2d( 0.85*tx, 0.85*ty );
               glEnd(); /* GL_LINES */
            }
            continue;
         }
      }

      x = 0;
      y = r;
      p = (5. - (double)(r*4)) / 4.;

      glBegin(GL_POINTS);

      PIXEL( cx,   cy+y );
      PIXEL( cx,   cy-y );
      PIXEL( cx+y, cy   );
      PIXEL( cx-y, cy   );

      while (x<y) {
         x++;
         if (p < 0) p += 2*(double)(x)+1;
         else p += 2*(double)(x-(--y))+1;

         if (x==0) {
            PIXEL( cx,   cy+y );
            PIXEL( cx,   cy-y );
            PIXEL( cx+y, cy   );
            PIXEL( cx-y, cy   );
         }
         else 
            if (x==y) {
               PIXEL( cx+x, cy+y );
               PIXEL( cx-x, cy+y );
               PIXEL( cx+x, cy-y );
               PIXEL( cx-x, cy-y );
            }
            else 
               if (x<y) {
               PIXEL( cx+x, cy+y );
               PIXEL( cx-x, cy+y );
               PIXEL( cx+x, cy-y );
               PIXEL( cx-x, cy-y );
               PIXEL( cx+y, cy+x );
               PIXEL( cx-y, cy+x );
               PIXEL( cx+y, cy-x );
               PIXEL( cx-y, cy-x );
            }
         }
      glEnd(); /* GL_POINTS */
   }
}
#undef PIXEL


/**
 * @fn static PlanetClass planetclass_get( const char a )
 *
 * @brief Basically returns a PlanetClass integer from a char
 *
 *    @param a Char to get class from.
 *    @return Identifier matching the char.
 */
static PlanetClass planetclass_get( const char a )
{
   switch (a) {
      /* planets use letters */
      case 'A': return PLANET_CLASS_A;
      case 'B': return PLANET_CLASS_B;
      case 'C': return PLANET_CLASS_C;
      case 'D': return PLANET_CLASS_D;
      case 'E': return PLANET_CLASS_E;
      case 'F': return PLANET_CLASS_F;
      case 'G': return PLANET_CLASS_G;
      case 'H': return PLANET_CLASS_H;
      case 'I': return PLANET_CLASS_I;
      case 'J': return PLANET_CLASS_J;
      case 'K': return PLANET_CLASS_K;
      case 'L': return PLANET_CLASS_L;
      case 'M': return PLANET_CLASS_M;
      case 'N': return PLANET_CLASS_N;
      case 'O': return PLANET_CLASS_O;
      case 'P': return PLANET_CLASS_P;
      case 'Q': return PLANET_CLASS_Q;
      case 'R': return PLANET_CLASS_R;
      case 'S': return PLANET_CLASS_S;
      case 'T': return PLANET_CLASS_T;
      case 'X': return PLANET_CLASS_X;
      case 'Y': return PLANET_CLASS_Y;
      case 'Z': return PLANET_CLASS_Z;
      /* stations use numbers - not as many types */
      case '0': return STATION_CLASS_A;
      case '1': return STATION_CLASS_B;
      case '2': return STATION_CLASS_C;
      case '3': return STATION_CLASS_D;

      default:
         WARN("Invalid planet class.");
         return PLANET_CLASS_NULL;
   };
}
/**
 * @fn char planet_getClass( Planet *p )
 *
 * @brief Gets the char representing the planet class from the planet.
 *
 *    @param p Planet to get the class char from.
 *    @return The planet's class char.
 */
char planet_getClass( Planet *p )
{
   switch (p->class) {
      case PLANET_CLASS_A: return 'A';
      case PLANET_CLASS_B: return 'B';
      case PLANET_CLASS_C: return 'C';
      case PLANET_CLASS_D: return 'D';
      case PLANET_CLASS_E: return 'E';
      case PLANET_CLASS_F: return 'F';
      case PLANET_CLASS_G: return 'G';
      case PLANET_CLASS_H: return 'H';
      case PLANET_CLASS_I: return 'I';
      case PLANET_CLASS_J: return 'J';
      case PLANET_CLASS_K: return 'K';
      case PLANET_CLASS_L: return 'L';
      case PLANET_CLASS_M: return 'M';
      case PLANET_CLASS_N: return 'N';
      case PLANET_CLASS_O: return 'O';
      case PLANET_CLASS_P: return 'P';
      case PLANET_CLASS_Q: return 'Q';
      case PLANET_CLASS_R: return 'R';
      case PLANET_CLASS_S: return 'S';
      case PLANET_CLASS_T: return 'T';
      case PLANET_CLASS_X: return 'X';
      case PLANET_CLASS_Y: return 'Y';
      case PLANET_CLASS_Z: return 'Z';
      /* Stations */
      case STATION_CLASS_A: return '0';
      case STATION_CLASS_B: return '1';
      case STATION_CLASS_C: return '2';
      case STATION_CLASS_D: return '3';

      default: 
         WARN("Invalid planet class.");
         return 0;
   };
}


/**
 * @Fn int space_canHyperspace( Pilot* p)
 *
 * @brief Checks to make sure if pilot is far enough away to hyperspace.
 *
 *    @param p Pilot to check if he can hyperspace.
 *    @return 1 if he can hyperspace, 0 else.
 */
int space_canHyperspace( Pilot* p)
{
   int i;
   double d;
   if (p->fuel < HYPERSPACE_FUEL) return 0;

   for (i=0; i < cur_system->nplanets; i++) {
      d = vect_dist(&p->solid->pos, &cur_system->planets[i]->pos);
      if (d < HYPERSPACE_EXIT_MIN)
         return 0;
   }
   return 1;
}


/**
 * @fn int space_hyperspace( Pilot* p )
 *
 * @brief Tries to get the pilot into hyperspace.
 *
 *    @param p Pilot to try to start hyperspacing.
 *    @return 0 on success.
 */
int space_hyperspace( Pilot* p )
{
   if (p->fuel < HYPERSPACE_FUEL) return -3;
   if (!space_canHyperspace(p)) return -1;

   /* pilot is now going to get automatically ready for hyperspace */
   pilot_setFlag(p, PILOT_HYP_PREP);

   return 0;
}


/**
 * @fn char** space_getFactionPlanet( int *nplanets, int *factions, int nfactions )
 *
 * @brief Gets the name of all the planets that belong to factions.
 *
 *    @param[out] nplanets Number of planets found.
 *    @param factions Factions to check against.
 *    @param nfactions Number of factions in factions.
 *    @return An array of faction names.  Individual names are not allocated.
 */
char** space_getFactionPlanet( int *nplanets, int *factions, int nfactions )
{
   int i,j,k;
   Planet* planet;
   char **tmp;
   int ntmp;
   int mtmp;

   ntmp = 0;
   mtmp = CHUNK_SIZE;
   tmp = malloc(sizeof(char*) * mtmp);

   for (i=0; i<systems_nstack; i++)
      for (j=0; j<systems_stack[i].nplanets; j++) {
         planet = systems_stack[i].planets[j];
         for (k=0; k<nfactions; k++)
            if (planet->faction == factions[k]) {
               ntmp++;
               if (ntmp > mtmp) { /* need more space */
                  mtmp += CHUNK_SIZE;
                  tmp = realloc(tmp, sizeof(char*) * mtmp);
               }
               tmp[ntmp-1] = planet->name;
               break; /* no need to check all factions */
            }
      }

   (*nplanets) = ntmp;
   return tmp;
}


/**
 * @fn char* space_getRndPlanet (void)
 *
 * @brief Gets the name of a random planet.
 *
 *    @return The name of a random planet.
 */
char* space_getRndPlanet (void)
{
   int i,j;
   char **tmp;
   int ntmp;
   int mtmp;
   char *res;

   ntmp = 0;
   mtmp = CHUNK_SIZE;
   tmp = malloc(sizeof(char*) * mtmp);

   for (i=0; i<systems_nstack; i++)
      for (j=0; j<systems_stack[i].nplanets; j++) {
         ntmp++;
         if (ntmp > mtmp) { /* need more space */
            mtmp += CHUNK_SIZE;
            tmp = realloc(tmp, sizeof(char*) * mtmp);
         }
         tmp[ntmp-1] = systems_stack[i].planets[j]->name;
      }
   
   res = tmp[RNG(0,ntmp-1)];
   free(tmp);

   return res; 
}


/**
 * @fn int space_sysReachable( StarSystem *sys )
 *
 * @brief Sees if a system is reachable.
 *
 *    @return 1 if target system is reachable, 0 if it isn't.
 */
int space_sysReachable( StarSystem *sys )
{
   int i;

   if (sys_isKnown(sys)) return 1; /* it is known */

   /* check to see if it is adjacent to known */
   for (i=0; i<sys->njumps; i++)
      if (sys_isKnown(&systems_stack[ sys->jumps[i]]))
         return 1;

   return 0;
}


/**
 * @fn StarSystem* system_get( const char* sysname )
 *
 * @brief Get the system from it's name.
 *
 *    @param sysname Name to match.
 *    @return System matching sysname.
 */
StarSystem* system_get( const char* sysname )
{
   int i;

   for (i=0; i<systems_nstack; i++)
      if (strcmp(sysname, systems_stack[i].name)==0)
         return &systems_stack[i];

   DEBUG("System '%s' not found in stack", sysname);
   return NULL;
}


/**
 * @fn char* planet_getSystem( char* planetname )
 *
 * @brief Get the name of a system from a planetname.
 *
 *    @param planetname Planet name to match.
 *    @return Name of the system planet belongs to.
 */
char* planet_getSystem( char* planetname )
{
   int i;

   for (i=0; i<spacename_nstack; i++)
      if (strcmp(planetname_stack[i],planetname)==0)
         return systemname_stack[i];
   
   DEBUG("Planet '%s' not found in planetname stack", planetname);
   return NULL;
}


/**
 * @fn Planet* planet_get( char* planetname )
 *
 * @brief Gets a planet based on it's name.
 *
 *    @param planetname Name to match.
 *    @return Planet matching planetname.
 */
Planet* planet_get( char* planetname )
{
   int i;

   for (i=0; i<planet_nstack; i++)
      if (strcmp(planet_stack[i].name,planetname)==0)
         return &planet_stack[i];

   DEBUG("Planet '%s' not found in the universe", planetname);
   return NULL;
}


/**
 * @fn void space_update( const double dt )
 *
 * @brief Controls fleet spawning.
 *
 *    @param dt Current delta tick.
 */
void space_update( const double dt )
{
   int i, j, f;

   if (cur_system == NULL) return; /* can't update a null system */

   /* Spawning. */
   if (space_spawn) {
      spawn_timer -= dt;

      if (cur_system->nfleets == 0) /* stop checking if there are no fleets */
         spawn_timer = 300.;
      
      if (spawn_timer < 0.) { /* time to possibly spawn */

         /* spawn chance is based on overall % */
         f = RNG(0,100*cur_system->nfleets);
         j = 0;
         for (i=0; i < cur_system->nfleets; i++) {
            j += cur_system->fleets[i].chance;
            if (f < j) { /* add one fleet */
               space_addFleet( cur_system->fleets[i].fleet, 0 );
               break;
            }
         }

         spawn_timer = 60./(float)cur_system->nfleets;
      }
   }

   /* Volatile system. */
   if (cur_system->nebu_volatility > 0.) {
      /* Player takes damage. */
      if (player)
         pilot_hit( player, NULL, 0, DAMAGE_TYPE_RADIATION,
               pow2(cur_system->nebu_volatility) / 500. * dt );
   }
}


/**
 * @fn static void space_addFleet( Fleet* fleet, int init )
 *
 * @brief Creates a fleet.
 *
 *    @param fleet Fleet to add to the system.
 *    @param init Is being run during the space initialization.
 */
static void space_addFleet( Fleet* fleet, int init )
{
   FleetPilot *plt;
   Planet *planet;
   int i, c;
   double a, d;
   Vector2d vv,vp, vn;

   /* Needed to determine angle. */
   vectnull(&vn);

   /* c will determino how to create the fleet, only non-zero if it's run in init. */
   if (init == 1) {
      if (RNGF() < 0.5) /* 50% chance of starting out en route. */
         c = 2;
      else if (RNGF() < 0.5) /* 25% of starting out landed. */
         c = 1;
      else /* 25% chance starting out entering hyperspace. */
         c = 0;
   }
   else c = 0;

   /* simulate they came from hyperspace */
   if (c==0) {
      d = RNGF()*(HYPERSPACE_ENTER_MAX-HYPERSPACE_ENTER_MIN) + HYPERSPACE_ENTER_MIN;
      vect_pset( &vp, d, RNG(0,360)*M_PI/180.);
   }
   /* Starting out landed or heading towards landing.. */
   else if ((c==1) || (c==2)) {
      /* Get friendly planet to land on. */
      planet = NULL;
      for (i=0; i<cur_system->nplanets; i++)
         if (planet_hasService(cur_system->planets[i],PLANET_SERVICE_BASIC) &&
               !areEnemies(fleet->faction,cur_system->planets[i]->faction)) {
            planet = cur_system->planets[i];
            break;
         }

      /* No suitable planet found. */
      if (planet == NULL) {
         d = RNGF()*(HYPERSPACE_ENTER_MAX-HYPERSPACE_ENTER_MIN) + HYPERSPACE_ENTER_MIN;
         vect_pset( &vp, d, RNG(0,360)*M_PI/180.);
         c = 0;
      }
      else {
         /* Start out landed. */
         if (c==1)
            vectcpy( &vp, &planet->pos );
         /* Start out near landed. */
         else if (c==2) {
            d = RNGF()*(HYPERSPACE_ENTER_MAX-HYPERSPACE_ENTER_MIN) + HYPERSPACE_ENTER_MIN;
            vect_pset( &vp, d, RNG(0,360)*M_PI/180.);
         }
      }
   }

   for (i=0; i < fleet->npilots; i++)
      plt = &fleet->pilots[i];
      if (RNG(0,100) <= plt->chance) {
         /* other ships in the fleet should start split up */
         vect_cadd(&vp, RNG(75,150) * (RNG(0,1) ? 1 : -1),
               RNG(75,150) * (RNG(0,1) ? 1 : -1));
         a = vect_angle(&vp, &vn);

         /* Entering via hyperspace. */
         if (c==0)
            vect_pset( &vv, HYPERSPACE_VEL, a );
         /* Starting out landed. */
         else if (c==1)
            vectnull(&vv);
         /* Starting out almost landed. */
         else if (c==2)
            /* Put speed at half in case they start very near. */
            vect_pset( &vv, plt->ship->speed * 0.5, a );

         pilot_create( plt->ship,
               plt->name,
               fleet->faction,
               (plt->ai != NULL) ? plt->ai : fleet->ai, /* Pilot AI override */
               a,
               &vp,
               &vv,
               0 );
      }
}


/**
 * @fn void space_init ( const char* sysname )
 *
 * @brief Initializes the system.
 *
 *    @param sysname Name of the system to initialize.
 */
void space_init ( const char* sysname )
{
   char* nt;
   int i;

   /* cleanup some stuff */
   player_clear(); /* clears targets */
   pilots_clean(); /* destroy all the current pilots, except player */
   weapon_clear(); /* get rid of all the weapons */
   spfx_clear(); /* get rid of the explosions */
   space_spawn = 1; /* spawn is enabled by default. */

   /* Clear player escorts since they don't automatically follow. */
   if (player) {
      player->nescorts = 0;
      if (player->escorts) {
         free(player->escorts);
         player->escorts = NULL;
      }
   }

   if ((sysname==NULL) && (cur_system==NULL))
      ERR("Cannot reinit system if there is no system previously loaded");
   else if (sysname!=NULL) {
      for (i=0; i < systems_nstack; i++)
         if (strcmp(sysname, systems_stack[i].name)==0)
            break;

      if (i==systems_nstack) ERR("System %s not found in stack", sysname);
      cur_system = systems_stack+i;

      nt = ntime_pretty(0);
      player_message("Entering System %s on %s.", sysname, nt);
      free(nt);

      /* Handle background */
      if (cur_system->nebu_density > 0.) {
         /* Background is Nebulae */
         nebu_prep( cur_system->nebu_density, cur_system->nebu_volatility );
      }
      else {
         /* Backrgound is Stary */
         nstars = (cur_system->stars*SCREEN_W*SCREEN_H+STAR_BUF*STAR_BUF)/(800*640);
         if (mstars < nstars)
            stars = realloc(stars,sizeof(Star)*nstars); /* should realloc, not malloc */
         for (i=0; i < nstars; i++) {
            stars[i].brightness = (double)RNG( 50, 200 )/256.;
            stars[i].x = (double)RNG( -STAR_BUF, SCREEN_W + STAR_BUF );
            stars[i].y = (double)RNG( -STAR_BUF, SCREEN_H + STAR_BUF );
         }
      }
   }

   /* set up fleets -> pilots */
   for (i=0; i < cur_system->nfleets; i++)
      if (RNG(0,100) <= (cur_system->fleets[i].chance/2)) /* fleet check (50% chance) */
         space_addFleet( cur_system->fleets[i].fleet, 1 );
   
   /* start the spawn timer */
   spawn_timer = 120./(float)(cur_system->nfleets+1);

   /* we now know this system */
   sys_setFlag(cur_system,SYSTEM_KNOWN);
}


/**
 * @fn static int planets_load ( void )
 *
 * @brief Loads all the planets in the game.
 *
 *    @return 0 on success.
 */
static int planets_load ( void )
{
   uint32_t bufsize;
   char *buf;
   xmlNodePtr node;
   xmlDocPtr doc;

   buf = pack_readfile( DATA, PLANET_DATA, &bufsize );
   doc = xmlParseMemory( buf, bufsize );

   node = doc->xmlChildrenNode;
   if (strcmp((char*)node->name,XML_PLANET_ID)) {
      ERR("Malformed "PLANET_DATA" file: missing root element '"XML_PLANET_ID"'");
      return -1;
   }

   node = node->xmlChildrenNode; /* first system node */
   if (node == NULL) {
      ERR("Malformed "PLANET_DATA" file: does not contain elements");
      return -1;
   }

   /* Initialize stack if needed. */
   if (planet_stack == NULL) {
      planet_mstack = CHUNK_SIZE;
      planet_stack = malloc( sizeof(Planet) * planet_mstack );
      planet_nstack = 0;
   }

   do {
      if (xml_isNode(node,XML_PLANET_TAG)) {

         /* See if stack must grow. */
         planet_nstack++;
         if (planet_nstack > planet_mstack) {
            planet_mstack += CHUNK_SIZE;
            planet_stack = realloc( planet_stack, sizeof(Planet) * planet_mstack );
         }

         planet_parse( &planet_stack[planet_nstack-1], node );
      }
   } while (xml_nextNode(node));

   /*
    * free stuff
    */
   xmlFreeDoc(doc);
   free(buf);

   return 0;
}


/**
 * @fn static int planet_parse( Planet *planet, const xmlNodePtr parent )
 *
 * @brief Parses a planet from an xml node.
 *
 *    @param planet Planet to fill up.
 *    @param parent Node that contains planet data.
 *    @return 0 on success.
 */
static int planet_parse( Planet *planet, const xmlNodePtr parent )
{
   int i;
   char str[PATH_MAX];
   xmlNodePtr node, cur, ccur;
   int len;
   unsigned int flags;

   /* Clear up memory for sane defaults. */
   memset( planet, 0, sizeof(Planet) );
   planet->faction = -1;
   str[0] = '\0';
   flags = 0;

   /* Get the name. */
   xmlr_attr( parent, "name", planet->name );

   node = parent->xmlChildrenNode;
   do {
      if (xml_isNode(node,"GFX")) {
         cur = node->children;
         do {
            if (xml_isNode(cur,"space")) { /* load space gfx */
               snprintf( str, PATH_MAX, PLANET_GFX_SPACE"%s", xml_get(cur));
               planet->gfx_space = gl_newImage(str);
            }
            else if (xml_isNode(cur,"exterior")) { /* load land gfx */
               len = strlen(xml_raw(cur)) + sizeof(PLANET_GFX_EXTERIOR);
               planet->gfx_exterior = malloc( len );
               snprintf( planet->gfx_exterior, len, PLANET_GFX_EXTERIOR"%s", xml_get(cur));
            }
         } while (xml_nextNode(cur));
      }
      else if (xml_isNode(node,"pos")) {
         cur = node->children;
         do {
            if (xml_isNode(cur,"x")) {
               flags |= FLAG_XSET;
               planet->pos.x = xml_getFloat(cur);
            }
            else if (xml_isNode(cur,"y")) {
               flags |= FLAG_YSET;
               planet->pos.y = xml_getFloat(cur);
            }
         } while(xml_nextNode(cur));
      }
      else if (xml_isNode(node,"general")) {
         cur = node->children;
         do {
            if (xml_isNode(cur,"class"))
               planet->class =
                  planetclass_get(cur->children->content[0]);
            else if (xml_isNode(cur,"faction")) {
               flags |= FLAG_FACTIONSET;
               planet->faction = faction_get( xml_get(cur) );
            }
            else if (xml_isNode(cur, "description"))
               planet->description = xml_getStrd(cur);

            else if (xml_isNode(cur, "bar"))
               planet->bar_description = xml_getStrd(cur);

            else if (xml_isNode(cur, "services")) {
               flags |= FLAG_SERVICESSET;
               planet->services = xml_getInt(cur); /* flags gotten by data */
            }

            else if (xml_isNode(cur, "tech")) {
               ccur = cur->children;
               do {
                  if (xml_isNode(ccur,"main")) {
                     flags |= FLAG_TECHSET;
                     planet->tech[0] = xml_getInt(ccur);
                  }
                  else if (xml_isNode(ccur,"special")) {
                     for (i=1; i<PLANET_TECH_MAX; i++)
                        if (planet->tech[i]==0) {
                           planet->tech[i] = xml_getInt(ccur);
                           break;
                        }
                     if (i==PLANET_TECH_MAX) WARN("Planet '%s' has too many"
                           "'special tech' entries", planet->name);
                  }
               } while (xml_nextNode(ccur));
            }

            else if (xml_isNode(cur, "commodities")) {
               ccur = cur->children;
               do {
                  if (xml_isNode(ccur,"commodity")) {
                     planet->commodities = realloc(planet->commodities,
                           (planet->ncommodities+1) * sizeof(Commodity*));
                     planet->commodities[planet->ncommodities] =
                        commodity_get( xml_get(ccur) );
                     planet->ncommodities++;
                  }
               } while (xml_nextNode(ccur));
            }
         } while(xml_nextNode(cur));
      }
   } while (xml_nextNode(node));


/* 
 * verification
 */
#define MELEMENT(o,s)   if (o) WARN("Planet '%s' missing '"s"' element", planet->name)
   MELEMENT(planet->gfx_space==NULL,"GFX space");
   MELEMENT( planet_hasService(planet,PLANET_SERVICE_LAND) &&
         planet->gfx_exterior==NULL,"GFX exterior");
   MELEMENT((flags&FLAG_XSET)==0,"x");
   MELEMENT((flags&FLAG_YSET)==0,"y");
   MELEMENT(planet->class==PLANET_CLASS_NULL,"class");
   MELEMENT( planet_hasService(planet,PLANET_SERVICE_LAND) &&
         planet->description==NULL,"desription");
   MELEMENT( planet_hasService(planet,PLANET_SERVICE_BASIC) &&
         planet->bar_description==NULL,"bar");
   MELEMENT( planet_hasService(planet,PLANET_SERVICE_BASIC) &&
         (flags&FLAG_FACTIONSET)==0,"faction");
   MELEMENT((flags&FLAG_SERVICESSET)==0,"services");
   MELEMENT( (planet_hasService(planet,PLANET_SERVICE_OUTFITS) ||
            planet_hasService(planet,PLANET_SERVICE_SHIPYARD)) &&
         (flags&FLAG_TECHSET)==0, "tech" );
   MELEMENT( planet_hasService(planet,PLANET_SERVICE_COMMODITY) &&
         (planet->ncommodities==0),"commodity" );
#undef MELEMENT

   return 0;
}


/**
 * @fn int system_addPlanet( StarSystem *sys, char *planetname )
 *
 * @brief Adds a planet to a star system.
 *
 *    @param sys Star System to add planet to.
 *    @param planetname Name of the planet to add.
 *    @return 0 on success.
 */
int system_addPlanet( StarSystem *sys, char *planetname )
{
   Planet *planet;

   if (sys == NULL)
      return -1;

   /* Check if need to grow the star system planet stack. */
   sys->nplanets++;
   if (sys->planets == NULL) {
      sys->planets = malloc( sizeof(Planet*) * CHUNK_SIZE_SMALL );
   }
   else if (sys->nplanets > CHUNK_SIZE_SMALL) {
      sys->planets = realloc( sys->planets, sizeof(Planet*) * sys->nplanets );
   }
   planet = planet_get(planetname);
   sys->planets[sys->nplanets-1] = planet;

   /* add planet <-> star system to name stack */
   spacename_nstack++;
   if (spacename_nstack > spacename_mstack) {
      spacename_mstack += CHUNK_SIZE;
      planetname_stack = realloc(planetname_stack,
            sizeof(char*) * spacename_mstack);
      systemname_stack = realloc(systemname_stack,
            sizeof(char*) * spacename_mstack);
   }
   planetname_stack[spacename_nstack-1] = planet->name;
   systemname_stack[spacename_nstack-1] = sys->name;

   system_setFaction(sys);

   return 0;
}


/**
 * @fn int system_rmPlanet( StarSystem *sys, char *planetname )
 *
 * @brief Removes a planet from a star system.
 *
 *    @param sys Star System to remove planet from.
 *    @param planetname Name of the planet to remove.
 *    @return 0 on success.
 */
int system_rmPlanet( StarSystem *sys, char *planetname )
{
   int i, found;
   Planet *planet ;

   if (sys == NULL) {
      WARN("Unable to remove planet '%s' from NULL system.", planetname);
      return -1;
   }

   /* Try to find planet. */
   planet = planet_get( planetname );
   for (i=0; i<sys->nplanets; i++)
      if (sys->planets[i] == planet)
         break;

   /* Planet not found. */
   if (i>=sys->nplanets) {
      WARN("Planet '%s' not found in system '%s' for removal.", planetname, sys->name);
      return -1;
   }

   /* Remove planet from system. */
   sys->nplanets--;
   memmove( &sys->planets[i], &sys->planets[i+1], sizeof(Planet*) * (sys->nplanets-i) );

   /* Remove from the name stack thingy. */
   found = 0;
   for (i=0; i<spacename_nstack; i++)
      if (strcmp(planetname, planetname_stack[i])==0) {
         spacename_nstack--;
         memmove( &planetname_stack[i], &planetname_stack[i+1],
               sizeof(char*) * (spacename_nstack-i) );
         memmove( &systemname_stack[i], &systemname_stack[i+1],
               sizeof(char*) * (spacename_nstack-i) );
         found = 1;
         break;
      }
   if (found == 0)
      WARN("Unable to find planet '%s' and system '%s' in planet<->system stack.",
            planetname, sys->name );

   system_setFaction(sys);

   return 0;
}


/**
 * @fn int system_addFleet( StarSystem *sys, SystemFleet *fleet )
 *
 * @brief Adds a fleet to a star system.
 *
 *    @param sys Star System to add fleet to.
 *    @param fleet Fleet to add.
 *    @return 0 on success.
 */
int system_addFleet( StarSystem *sys, SystemFleet *fleet )
{
   if (sys == NULL)
      return -1;

   sys->fleets = realloc(sys->fleets, sizeof(SystemFleet)*(++sys->nfleets));
   memcpy(sys->fleets+(sys->nfleets-1), fleet, sizeof(SystemFleet));

   return 0;
}


/**
 * @fn int system_rmFleet( StarSystem *sys, SystemFleet *fleet )
 *
 * @brief Removes a fleet from a star system.
 *
 *    @param sys Star System to remove fleet from.
 *    @param fleet Fleet to remove.
 *    @return 0 on success.
 */
int system_rmFleet( StarSystem *sys, SystemFleet *fleet )
{
   int i;

   /* Find a matching fleet (will grab first since can be duplicates). */
   for (i=0; i<sys->nfleets; i++)
      if ((fleet->fleet == sys->fleets[i].fleet) &&
            (fleet->chance == sys->fleets[i].chance))
         break;

   /* Not found. */
   if (i >= sys->nfleets)
      return -1;
   
   sys->nfleets--;
   memmove(&sys->fleets[i], &sys->fleets[i+1], sizeof(SystemFleet) * (sys->nfleets - i));
   sys->fleets = realloc(sys->fleets, sizeof(SystemFleet) * sys->nfleets);

   return 0;
}


/**
 * @brief Creates a system from an XML node.
 *
 *    @param parent XML node to get system from.
 *    @return System matching parent data.
 */
static StarSystem* system_parse( StarSystem *sys, const xmlNodePtr parent )
{
   Planet* planet;
   SystemFleet fleet;
   char* ptrc;
   xmlNodePtr cur, node;
   uint32_t flags;
   int size;

   /* Clear memory for sane defaults. */
   memset( sys, 0, sizeof(StarSystem) );
   sys->faction = -1;
   planet = NULL;
   size = 0;
   
   sys->name = xml_nodeProp(parent,"name"); /* already mallocs */

   node  = parent->xmlChildrenNode;

   do { /* load all the data */
      if (xml_isNode(node,"pos")) {
         cur = node->children;
         do {
            if (xml_isNode(cur,"x")) {
               flags |= FLAG_XSET;
               sys->pos.x = xml_getFloat(cur);
            }
            else if (xml_isNode(cur,"y")) {
               flags |= FLAG_YSET;
               sys->pos.y = xml_getFloat(cur);
            }
         } while (xml_nextNode(cur));
      }
      else if (xml_isNode(node,"general")) {
         cur = node->children;
         do {
            if (xml_isNode(cur,"stars")) /* non-zero */
               sys->stars = xml_getInt(cur);
            else if (xml_isNode(cur,"asteroids")) {
               flags |= FLAG_ASTEROIDSSET;
               sys->asteroids = xml_getInt(cur);
            }
            else if (xml_isNode(cur,"interference")) {
               flags |= FLAG_INTERFERENCESET;
               sys->interference = xml_getFloat(cur)/100.;
            }
            else if (xml_isNode(cur,"nebulae")) {
               ptrc = xml_nodeProp(cur,"volatility");
               if (ptrc != NULL) { /* Has volatility  */
                  sys->nebu_volatility = atof(ptrc);
                  free(ptrc);
               }
               sys->nebu_density = xml_getFloat(cur);
            }
         } while (xml_nextNode(cur));
      }
      /* loads all the planets */
      else if (xml_isNode(node,"planets")) {
         cur = node->children;
         do {
            if (xml_isNode(cur,"planet"))
               system_addPlanet( sys, xml_get(cur) );
         } while (xml_nextNode(cur));
      }
      /* loads all the fleets */
      else if (xml_isNode(node,"fleets")) {
         cur = node->children;
         do {
            if (xml_isNode(cur,"fleet")) {
               memset(&fleet, 0, sizeof(SystemFleet));

               fleet.fleet = fleet_get(xml_get(cur));
               if (fleet.fleet==NULL)
                  WARN("Fleet '%s' for Star System '%s' not found",
                        xml_get(cur), sys->name);

               xmlr_attr(cur,"chance",ptrc); /* mallocs ptrc */
               if (ptrc==NULL) fleet.chance = 0; /* gives warning */
               else fleet.chance = atoi(ptrc);
               if (fleet.chance == 0)
                  WARN("Fleet '%s' for Star System '%s' has 0%% chance to appear",
                     fleet.fleet->name, sys->name);
               if (ptrc) free(ptrc); /* free the ptrc */

               system_addFleet( sys, &fleet );
            }
         } while (xml_nextNode(cur));
      }
   } while (xml_nextNode(node));

#define MELEMENT(o,s)      if ((o) == 0) WARN("Star System '%s' missing '"s"' element", sys->name)
   if (sys->name == NULL) WARN("Star System '%s' missing 'name' tag", sys->name);
   MELEMENT(flags&FLAG_XSET,"x");
   MELEMENT(flags&FLAG_YSET,"y");
   MELEMENT(sys->stars,"stars");
   MELEMENT(flags&FLAG_ASTEROIDSSET,"asteroids");
   MELEMENT(flags&FLAG_INTERFERENCESET,"inteference");
#undef MELEMENT

   /* post-processing */
   system_setFaction( sys );

   return 0;
}


/**
 * @fn static void system_setFaction( StarSystem *sys )
 *
 * @brief Sets the system faction based on the planets it has.
 *
 *    @param sys System to set the faction of.
 */
static void system_setFaction( StarSystem *sys )
{
   int i;
   sys->faction = -1;
   for (i=0; i<sys->nplanets; i++) /** @todo Handle multiple different factions. */
      if (sys->planets[i]->faction > 0) {
         sys->faction = sys->planets[i]->faction;
         break;
      }
}


/**
 * @fn static void system_parseJumps( const xmlNodePtr parent )
 *
 * @brief Loads the jumps into a system.
 *
 *    @param parent System parent node.
 */
static void system_parseJumps( const xmlNodePtr parent )
{
   int i;
   StarSystem *sys;
   char* name;
   xmlNodePtr cur, node;

   name = xml_nodeProp(parent,"name"); /* already mallocs */
   for (i=0; i<systems_nstack; i++)
      if (strcmp( systems_stack[i].name, name)==0) {
         sys = &systems_stack[i];
         break;
      }
   if (i==systems_nstack) WARN("System '%s' was not found in the stack for some reason",name);
   free(name); /* no more need for it */

   node  = parent->xmlChildrenNode;

   do { /* load all the data */
      if (xml_isNode(node,"jumps")) {
         cur = node->children;
         do {
            if (xml_isNode(cur,"jump")) {
               for (i=0; i<systems_nstack; i++)
                  if (strcmp( systems_stack[i].name, xml_raw(cur))==0) {
                     sys->njumps++;
                     sys->jumps = realloc(sys->jumps, sys->njumps*sizeof(int));
                     sys->jumps[sys->njumps-1] = i;
                     break;
                  }
               if (i==systems_nstack)
                  WARN("System '%s' not found for jump linking",xml_get(cur));
            }
         } while (xml_nextNode(cur));
      }
   } while (xml_nextNode(node));
}


/**
 * @fn int space_load (void)
 *
 * @brief Loads the entire universe into ram - pretty big feat eh?
 *
 *    @return 0 on success.
 */
int space_load (void)
{
   int ret;

   ret = planets_load();
   if (ret < 0)
      return ret;
   ret = systems_load();
   if (ret < 0)
      return ret;

   return 0;
}

/**
 * @fn int systems_load (void)
 *
 * @brief Loads the entire systems, needs to be called after planets_load.
 *
 * Uses a two system pass to first load the star systems_stack and then set
 *  jump routes.
 *
 *    @return 0 on success.
 */
static int systems_load (void)
{
   uint32_t bufsize;
   char *buf = pack_readfile( DATA, SYSTEM_DATA, &bufsize );

   xmlNodePtr node;
   xmlDocPtr doc = xmlParseMemory( buf, bufsize );

   node = doc->xmlChildrenNode;
   if (!xml_isNode(node,XML_SYSTEM_ID)) {
      ERR("Malformed "SYSTEM_DATA" file: missing root element '"XML_SYSTEM_ID"'");
      return -1;
   }

   node = node->xmlChildrenNode; /* first system node */
   if (node == NULL) {
      ERR("Malformed "SYSTEM_DATA" file: does not contain elements");
      return -1;
   }

   /* Allocate if needed. */
   if (systems_stack == NULL) {
      systems_mstack = CHUNK_SIZE;
      systems_stack = malloc( sizeof(StarSystem) * systems_mstack );
      systems_nstack = 0;
   }

   /*
    * first pass - loads all the star systems_stack
    */
   do {
      if (xml_isNode(node,XML_SYSTEM_TAG)) {
         /* Check if memory needs to grow. */
         systems_nstack++;
         if (systems_nstack > systems_mstack) {
            systems_mstack += CHUNK_SIZE;
            systems_stack = realloc(systems_stack, sizeof(StarSystem) * systems_mstack );
         }

         system_parse(&systems_stack[systems_nstack-1],node);
      }                                                                             
   } while (xml_nextNode(node));                                       

   /*
    * second pass - loads all the jump routes
    */
   node = doc->xmlChildrenNode->xmlChildrenNode;
   do {

      if (xml_isNode(node,XML_SYSTEM_TAG))
         system_parseJumps(node); /* will automatically load the jumps into the system */

   } while (xml_nextNode(node));


   /*
    * cleanup
    */
   xmlFreeDoc(doc);
   free(buf);

   DEBUG("Loaded %d Star System%s with %d Planet%s",
         systems_nstack, (systems_nstack==1) ? "" : "s",
         planet_nstack, (planet_nstack==1) ? "" : "s" );

   return 0;
}


/**
 * @fn void space_render( const double dt )
 *
 * @brief Renders the system.
 *
 *    @param dt Current delta tick.
 */
void space_render( const double dt )
{
   if (cur_system == NULL) return;

   if (cur_system->nebu_density > 0.)
      nebu_render(dt);
   else
      space_renderStars(dt);
}


/**
 * @fn void space_renderOverlay( const double dt )
 *
 * @brief Renders the system overlay.
 *
 *    @param dt Current delta tick.
 */
void space_renderOverlay( const double dt )
{
   if (cur_system == NULL) return;

   if (cur_system->nebu_density > 0.)
      nebu_renderOverlay(dt);
}


/**
 * @fn static void space_renderStars( const double dt )
 *
 * @brief Renders the starry background.
 *
 *    @param dt Current delta tick.
 */
static void space_renderStars( const double dt )
{
   int i;
   unsigned int t, timer;
   double x, y, m, b;

   /*
    * gprof claims it's the slowest thing in the game!
    */

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix(); /* translation matrix */
      glTranslated( -(double)SCREEN_W/2., -(double)SCREEN_H/2., 0);


   t = SDL_GetTicks();
   if (!player_isFlag(PLAYER_DESTROYED) && !player_isFlag(PLAYER_CREATING) &&
         pilot_isFlag(player,PILOT_HYPERSPACE) && /* hyperspace fancy effects */
         ((!paused && (player->ptimer-HYPERSPACE_STARS_BLUR < t)) ||
            (paused && (player->ptimer < HYPERSPACE_STARS_BLUR)))) {

      timer = player->ptimer - HYPERSPACE_STARS_BLUR;
      if (paused) timer += t;

      glShadeModel(GL_SMOOTH);

      /* Enable AA if possible. */
      if (gl_has(OPENGL_AA_LINE))
         glEnable(GL_LINE_SMOOTH);

      glBegin(GL_LINES);

      /* lines will be based on velocity */
      m = HYPERSPACE_STARS_LENGTH * (double)(t-timer) / (HYPERSPACE_STARS_BLUR);
      x = m*cos(VANGLE(player->solid->vel)+M_PI);
      y = m*sin(VANGLE(player->solid->vel)+M_PI);

      for (i=0; i < nstars; i++) {
         glColor4d( 1., 1., 1., stars[i].brightness );
         glVertex2d( stars[i].x, stars[i].y );
         glColor4d( 1., 1., 1., 0. );
         glVertex2d( stars[i].x + x*stars[i].brightness,
               stars[i].y + y*stars[i].brightness );
      }
      glEnd(); /* GL_LINES */

      if (gl_has(OPENGL_AA_LINE))
         glDisable(GL_LINE_SMOOTH);

      glShadeModel(GL_FLAT);
   }
   else { /* normal rendering */
      glBegin(GL_POINTS);

      if (!paused && !player_isFlag(PLAYER_DESTROYED) &&
            !player_isFlag(PLAYER_CREATING)) { /* update position */
         for (i=0; i < nstars; i++) {

            /* calculate new position */
            b = 13.-10.*stars[i].brightness;
            stars[i].x -= player->solid->vel.x/b*dt;
            stars[i].y -= player->solid->vel.y/b*dt;

            /* check boundries */
            if (stars[i].x > SCREEN_W + STAR_BUF) stars[i].x = -STAR_BUF;
            else if (stars[i].x < -STAR_BUF) stars[i].x = SCREEN_W + STAR_BUF;
            if (stars[i].y > SCREEN_H + STAR_BUF) stars[i].y = -STAR_BUF;
            else if (stars[i].y < -STAR_BUF) stars[i].y = SCREEN_H + STAR_BUF;

            /* render */
            if ((stars[i].x < SCREEN_W) && (stars[i].x > 0) &&
                  (stars[i].y < SCREEN_H) && (stars[i].y > 0)) {
               glColor4d( 1., 1., 1., stars[i].brightness );
               glVertex2d( stars[i].x, stars[i].y );
            }
         }
      }
      else { /* just render */
         for (i=0; i < nstars; i++) {
            if ((stars[i].x < SCREEN_W) && (stars[i].x > 0) &&
                  (stars[i].y < SCREEN_H) && (stars[i].y > 0)) {
               glColor4d( 1., 1., 1., stars[i].brightness );
               glVertex2d( stars[i].x, stars[i].y );
            }
         }
      }
      glEnd(); /* GL_POINTS */
   }

   glPopMatrix(); /* translation matrix */
}


/**
 * @fn void planets_render (void)
 *
 * @brief Renders the current systemsplanets.
 */
void planets_render (void)
{
   if (cur_system==NULL) return;

   int i;
   for (i=0; i < cur_system->nplanets; i++)
      gl_blitSprite( cur_system->planets[i]->gfx_space,
            cur_system->planets[i]->pos.x, cur_system->planets[i]->pos.y,
            0, 0, NULL );
}


/**
 * @fn void space_exit (void)
 *
 * @brief Cleans up the system.
 */
void space_exit (void)
{
   int i;

   /* Free the names. */
   if (planetname_stack) free(planetname_stack);
   if (systemname_stack) free(systemname_stack);
   spacename_nstack = 0;

   /* Free the planets. */
   for (i=0; i < planet_nstack; i++) {
      free(planet_stack[i].name);

      if (planet_stack[i].description)
         free(planet_stack[i].description);
      if (planet_stack[i].bar_description)
         free(planet_stack[i].bar_description);

      /* graphics */
      if (planet_stack[i].gfx_space)
         gl_freeTexture(planet_stack[i].gfx_space);
      if (planet_stack[i].gfx_exterior)
         free(planet_stack[i].gfx_exterior);

      /* commodities */
      free(planet_stack[i].commodities);
   }
   free(planet_stack);
   planet_stack = NULL;
   planet_nstack = 0;
   planet_mstack = 0;
   
   /* Free the systems. */
   for (i=0; i < systems_nstack; i++) {
      free(systems_stack[i].name);
      if (systems_stack[i].fleets)
         free(systems_stack[i].fleets);
      if (systems_stack[i].jumps)
         free(systems_stack[i].jumps);

      free(systems_stack[i].planets);
   }
   free(systems_stack);
   systems_stack = NULL;
   systems_nstack = 0;
   systems_mstack = 0;

   /* stars must be free too */
   if (stars) free(stars);
   stars = NULL;
   nstars = 0;
   mstars = 0;
}


/**
 * @fn void space_clearKnown (void)
 *
 * @brief Clears all system knowledge.
 */
void space_clearKnown (void)
{
   int i;
   for (i=0; i<systems_nstack; i++)
      sys_rmFlag(&systems_stack[i],SYSTEM_KNOWN);
}


/**
 * @brief Clears all system markers.
 */
void space_clearMarkers (void)
{
   int i;
   for (i=0; i<systems_nstack; i++)
      sys_rmFlag(&systems_stack[i],SYSTEM_MARKED);
}


/**
 * @brief Clears all the system computer markers.
 */
void space_clearComputerMarkers (void)
{
   int i;
   for (i=0; i<systems_nstack; i++)
      sys_rmFlag(&systems_stack[i],SYSTEM_CMARKED);
}


/**
 * @fn int space_sysSave( xmlTextWriterPtr writer )
 *
 * @brief Saves what is needed to be saved for space.
 *
 *    @param writer XML writer to use.
 *    @return 0 on success.
 */
int space_sysSave( xmlTextWriterPtr writer )
{
   int i;

   xmlw_startElem(writer,"space");

   for (i=0; i<systems_nstack; i++) {

      if (!sys_isKnown(&systems_stack[i])) continue; /* not known */

      xmlw_elem(writer,"known","%s",systems_stack[i].name);
   }

   xmlw_endElem(writer); /* "space" */

   return 0;
}


/**
 * @fn int space_sysLoad( xmlNodePtr parent )
 *
 * @brief Loads player's space properties from an XML node.
 *
 *    @param parent Parent node for space.
 *    @return 0 on success.
 */
int space_sysLoad( xmlNodePtr parent )
{
   xmlNodePtr node, cur;
   StarSystem *sys;

   space_clearKnown();

   node = parent->xmlChildrenNode;
   do {
      if (xml_isNode(node,"space")) {
         cur = node->xmlChildrenNode;

         do {
            if (xml_isNode(cur,"known")) {
               sys = system_get(xml_get(cur));
               if (sys != NULL) /* Must exist */
                  sys_setFlag(sys,SYSTEM_KNOWN);
            }
         } while (xml_nextNode(cur));
      }
   } while (xml_nextNode(node));

   return 0;
}


