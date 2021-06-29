/*
 * See Licensing and Copyright notice in naev.h
 */

/**
 * @file colour.c
 *
 * @brief Predefined colours for use with Naev.
 */


/** @cond */
#include <math.h>

#include "naev.h"
/** @endcond */

#include "colour.h"

#include "log.h"
#include "nmath.h"
#include "nstring.h"


/*
 * default colours
 */
/* grey */
const glColour cWhite      = { .r=1.00, .g=1.00, .b=1.00, .a=1. }; /**< White */
const glColour cGrey90     = { .r=0.90, .g=0.90, .b=0.90, .a=1. }; /**< Grey 90% */
const glColour cGrey80     = { .r=0.80, .g=0.80, .b=0.80, .a=1. }; /**< Grey 80% */
const glColour cGrey70     = { .r=0.70, .g=0.70, .b=0.70, .a=1. }; /**< Grey 70% */
const glColour cGrey60     = { .r=0.60, .g=0.60, .b=0.60, .a=1. }; /**< Grey 60% */
const glColour cGrey50     = { .r=0.50, .g=0.50, .b=0.50, .a=1. }; /**< Grey 50% */
const glColour cGrey45     = { .r=0.45, .g=0.45, .b=0.45, .a=1. }; /**< Grey 45% */
const glColour cGrey40     = { .r=0.40, .g=0.40, .b=0.40, .a=1. }; /**< Grey 40% */
const glColour cGrey35     = { .r=0.35, .g=0.35, .b=0.35, .a=1. }; /**< Grey 35% */
const glColour cGrey30     = { .r=0.30, .g=0.30, .b=0.30, .a=1. }; /**< Grey 30% */
const glColour cGrey25     = { .r=0.25, .g=0.25, .b=0.25, .a=1. }; /**< Grey 25% */
const glColour cGrey20     = { .r=0.20, .g=0.20, .b=0.20, .a=1. }; /**< Grey 20% */
const glColour cGrey15     = { .r=0.15, .g=0.15, .b=0.15, .a=1. }; /**< Grey 15% */
const glColour cGrey10     = { .r=0.10, .g=0.10, .b=0.10, .a=1. }; /**< Grey 10% */
const glColour cGrey5      = { .r=0.05, .g=0.05, .b=0.05, .a=1. }; /**< Grey 5% */
const glColour cBlack      = { .r=0.00, .g=0.00, .b=0.00, .a=1. }; /**< Black */

/* Greens. */
const glColour cDarkGreen      = { .r=0.10, .g=0.50, .b=0.10, .a=1. }; /**< Dark Green */
const glColour cGreen      = { .r=0.20, .g=0.80, .b=0.20, .a=1. }; /**< Green */
const glColour cPrimeGreen = { .r=0.00, .g=1.00, .b=0.00, .a=1. }; /**< Primary Green */
/* Reds. */
const glColour cDarkRed    = { .r=0.60, .g=0.10, .b=0.10, .a=1. }; /**< Dark Red */
const glColour cRed        = { .r=0.80, .g=0.20, .b=0.20, .a=1. }; /**< Red */
const glColour cPrimeRed   = { .r=1.00, .g=0.00, .b=0.00, .a=1. }; /**< Primary Red */
const glColour cBrightRed  = { .r=1.00, .g=0.60, .b=0.60, .a=1. }; /**< Bright Red */
/* Oranges. */
const glColour cOrange     = { .r=0.90, .g=0.70, .b=0.10, .a=1. }; /**< Orange */
/* Yellows. */
const glColour cGold       = { .r=1.00, .g=0.84, .b=0.00, .a=1. }; /**< Gold */
const glColour cYellow     = { .r=0.80, .g=0.80, .b=0.00, .a=1. }; /**< Yellow */
/* Blues. */
const glColour cMidnightBlue = { .r=0.10, .g=0.10, .b=0.4, .a=1. }; /**< Midnight Blue. */
const glColour cDarkBlue   = { .r=0.10, .g=0.10, .b=0.60, .a=1. }; /**< Dark Blue */
const glColour cBlue       = { .r=0.20, .g=0.20, .b=0.80, .a=1. }; /**< Blue */
const glColour cLightBlue  = { .r=0.40, .g=0.40, .b=1.00, .a=1. }; /**< Light Blue */
const glColour cPrimeBlue  = { .r=0.00, .g=0.00, .b=1.00, .a=1. }; /**< Primary Blue */
const glColour cCyan       = { .r=0.00, .g=1.00, .b=1.00, .a=1. }; /**< Cyan. */
/* Purples. */
const glColour cPurple     = { .r=0.90, .g=0.10, .b=0.90, .a=1. }; /**< Purple */
const glColour cDarkPurple = { .r=0.68, .g=0.18, .b=0.64, .a=1. }; /**< Dark Purple */
/* Browns. */
const glColour cBrown      = { .r=0.59, .g=0.28, .b=0.00, .a=1. }; /**< Brown */
/* Misc. */
const glColour cSilver     = { .r=0.75, .g=0.75, .b=0.75, .a=1. }; /**< Silver */
const glColour cAqua       = { .r=0.00, .g=0.75, .b=1.00, .a=1. }; /**< Aqua */


/*
 * game specific
 */
const glColour cBlackHilight  =  { .r = 0.0, .g = 0.0, .b = 0.0, .a = 0.4 }; /**< Hilight colour over black background. */
/* toolkit */
const glColour cHilight       =  { .r = 0.1, .g = 0.9, .b = 0.1, .a = 0.6 }; /**< Hilight colour */
/* outfit slot colours.
 * Taken from https://cran.r-project.org/web/packages/khroma/vignettes/tol.html#muted
 */
const glColour cOutfitHeavy = { 0.8, 0.4, 0.46, 1.0 }; /**< Heavy outfit colour (reddish). */
const glColour cOutfitMedium = { 0.2, 0.73, 0.93, 1.0 }; /**< Medium outfit colour (blueish). */
//const glColour cOutfitMedium = { 0.55, 0.8, 0.93, 1.0 }; /**< Medium outfit colour (blueish). Technically color safe but doesn't work with our colorblind filter. */
const glColour cOutfitLight = { 0.86, 0.8, 0.46, 1.0 }; /**< Light outfit colour (yellowish). */
/* objects */
const glColour cInert         =  { .r=221./255., .g=221./255., .b=221./255., .a=1. }; /**< Inert object colour */
const glColour cNeutral       =  { .r=221./255., .g=204./255., .b=119./255., .a=1. }; /**< Neutral object colour */
const glColour cFriend        =  {  .r=68./255., .g=170./255., .b=153./255., .a=1. }; /**< Friend object colour */
const glColour cHostile       =  { .r=170./255.,  .g=68./255., .b=153./255., .a=1. }; /**< Hostile object colour */
const glColour cRestricted    =  { .r=153./255., .g=153./255.,  .b=51./255., .a=1. }; /**< Restricted object colour. */
/* mission markers */
const glColour cMarkerNew     =  { .r=154./255., .g=112./255., .b=158./255., .a=1. }; /**< New mission marker colour. */
const glColour cMarkerComputer = { .r=208./255., .g=231./255., .b=202./255., .a=1. }; /**< Computer mission marker colour. */
const glColour cMarkerLow     =  { .r=234./255., .g=240./255., .b=181./255., .a=1. }; /**< Low priority mission marker colour. */
const glColour cMarkerHigh    =  { .r=252./255., .g=247./255., .b=213./255., .a=1. }; /**< High priority mission marker colour. */
const glColour cMarkerPlot    =  { .r=255./255., .g=255./255., .b=255./255., .a=1. }; /**< Plot mission marker colour. */
/* radar */
const glColour cRadar_player  =  { .r = 0.9, .g = 0.1, .b = 0.9, .a = 1.  }; /**< Player colour on radar. */
const glColour cRadar_tPilot  =  { .r = 1.0, .g = 1.0, .b = 1.0, .a = 1.  }; /**< Targeted object colour on radar. */
const glColour cRadar_tPlanet =  { .r = 1.0, .g = 1.0, .b = 1.0, .a = 1.  }; /**< Targeted planet colour. */
const glColour cRadar_weap    =  { .r = 0.8, .g = 0.2, .b = 0.2, .a = 1.  }; /**< Weapon colour on radar. */
const glColour cRadar_hilight =  { .r = 0.6, .g = 1.0, .b = 1.0, .a = 1.  }; /**< Radar hilighted object. */
/* health */
const glColour cShield        =  { .r = 0.2, .g = 0.2, .b = 0.8, .a = 1.  }; /**< Shield bar colour. */
const glColour cArmour        =  { .r = 0.5, .g = 0.5, .b = 0.5, .a = 1.  }; /**< Armour bar colour. */
const glColour cEnergy        =  { .r = 0.2, .g = 0.8, .b = 0.2, .a = 1.  }; /**< Energy bar colour. */
const glColour cFuel          =  { .r = 0.9, .g = 0.1, .b = 0.4, .a = 1.  }; /**< Fuel bar colour. */

/* Deiz's Super Font Palette */

const glColour cFontRed       =  { .r = 1.0, .g = 0.4, .b = 0.4, .a = 1.  }; /**< Red font colour. */
const glColour cFontGreen     =  { .r = 0.6, .g = 1.0, .b = 0.4, .a = 1.  }; /**< Green font colour. */
const glColour cFontBlue      =  { .r = 0.4, .g = 0.6, .b = 1.0, .a = 1.  }; /**< Blue font colour. */
const glColour cFontYellow    =  { .r = 1.0, .g = 1.0, .b = 0.5, .a = 1.  }; /**< Yellow font colour. */
const glColour cFontWhite     =  { .r = 0.95, .g = 0.95, .b = 0.95, .a = 1.  }; /**< White font colour. */
const glColour cFontGrey      =  { .r = 0.7, .g = 0.7, .b = 0.7, .a = 1.  }; /**< Grey font colour. */
const glColour cFontPurple    =  { .r = 1.0, .g = 0.3, .b = 1.0, .a = 1.  }; /**< Purple font colour. */
const glColour cFontOrange    =  { .r = 1.0, .g = 0.7, .b = 0.3, .a = 1.  }; /**< Orange font colour. */


/**
 * @brief Changes colour space from HSV to RGB.
 *
 * All values go from 0 to 1, except H which is 0-360.
 *
 *    @param[out] c Colour to be converted to from hsv.
 *    @param h Hue to convert.
 *    @param s Saturation to convert.
 *    @param v Value to convert.
 */
void col_hsv2rgb( glColour *c, float h, float s, float v )
{
   float var_h, var_i, var_1, var_2, var_3;

   if (v > 1)
      v = 1;

   if (s == 0) {
      c->r = v;
      c->g = v;
      c->b = v;
   }
   else {
      var_h = h * 6 / 360.;
      var_i = floor(var_h);
      var_1 = v * (1 - s);
      var_2 = v * (1 - s * (var_h - var_i));
      var_3 = v * (1 - s * (1 - (var_h - var_i)));

      if      (var_i == 0) { c->r = v     ; c->g = var_3 ; c->b = var_1; }
      else if (var_i == 1) { c->r = var_2 ; c->g = v     ; c->b = var_1; }
      else if (var_i == 2) { c->r = var_1 ; c->g = v     ; c->b = var_3; }
      else if (var_i == 3) { c->r = var_1 ; c->g = var_2 ; c->b = v;     }
      else if (var_i == 4) { c->r = var_3 ; c->g = var_1 ; c->b = v;     }
      else                 { c->r = v     ; c->g = var_1 ; c->b = var_2; }
   }
}


/**
 * @brief Changes colour space from RGB to HSV.
 *
 * All values go from 0 to 1, except H which is 0-360.
 *
 * Taken from (GIFT) GNU Image Finding Tool.
 *
 *    @param[out] H Stores Hue.
 *    @param[out] S Stores Saturation.
 *    @param[out] V Stores Value.
 *    @param R Red to convert.
 *    @param G Green to convert.
 *    @param B Blue to convert.
 */
void col_rgb2hsv( float *H, float *S, float *V, float R, float G, float B )
{
   float H1, S1, V1;
#ifdef HSV_TRAVIS
   float R1, G1, B1;
#endif /* HSV_TRAVIS */
   float max, min, diff;

   max = max3( R, G, B );
   min = min3( R, G, B );
   diff = max - min;

   if (max == 0)
      H1 = S1 = V1 = 0;
   else {
      V1 = max;
      S1 = diff/max;
      if (S1 == 0)
         /* H1 is undefined, but give it a value anyway */
         H1 = 0;
      else {
#ifdef HSV_TRAVIS
         R1 = (max - R)/diff;
         G1 = (max - G)/diff;
         B1 = (max - B)/diff;

         if ((R == max) && (G == min))
            H1 = 5 + B1;
         else {
            if ((R == max) && (G != min))
               H1 = 1 - G1;
            else {
               if ((G == max) && (B == min))
                  H1 = 1 + R1;
               else {
                  if ((G == max) && (B != min))
                     H1 = 3 - B1;
                  else {
                     if (R == max)
                        H1 = 3 + G1;
                     else
                        H1 = 5 - R1;
                  }
               }
            }
         }

         H1 *= 60; /* convert to range [0, 360] degrees */
#else /* HSV_TRAVIS */
         H1 = 0.; /* Shuts up Clang. */
         /* assume Foley & VanDam HSV */
         if (R == max)
            H1 = (G - B)/diff;
         if (G == max)
            H1 = 2 + (B - R)/diff;
         if (B == max)
            H1 = 4 + (R - G)/diff;

         H1 *= 60; /* convert to range [0, 360] degrees */
         if (H1 < 0)
            H1 += 360;
#endif /* HSV_TRAVIS */
      }
   }
   *H = H1;
   *S = S1;
   *V = V1;
}


/**
 * @brief Blends two colours.
 *
 *    @param[out] blend Stores blended output colour.
 *    @param fg Foreground colour.
 *    @param bg Background colour.
 *    @param alpha Alpha value to use (0 to 1).
 */
void col_blend( glColour *blend, const glColour *fg, const glColour *bg, float alpha )
{
   blend->r = (1. - alpha) * bg->r + alpha * fg->r;
   blend->g = (1. - alpha) * bg->g + alpha * fg->g;
   blend->b = (1. - alpha) * bg->b + alpha * fg->b;
   blend->a = (1. - alpha) * bg->a + alpha * fg->a;
}


#define CHECK_COLOUR(colour) \
      if (strcasecmp(name, #colour) == 0) return &c##colour /**< Checks the colour. */
/**
 * @brief Returns a colour from its name
 *
 *    @param name Colour's name
 *    @return the colour
 */
const glColour* col_fromName( const char* name )
{
   if (name[0] == 'a' || name[0] == 'A') {
      CHECK_COLOUR(Aqua);
   }

   if (name[0] == 'b' || name[0] == 'B') {
      CHECK_COLOUR(Blue);
      CHECK_COLOUR(Black);
      CHECK_COLOUR(Brown);
   }

   if (name[0] == 'c' || name[0] == 'C') {
      CHECK_COLOUR(Cyan);
   }

   if (name[0] == 'd' || name[0] == 'D') {
      CHECK_COLOUR(DarkRed);
      CHECK_COLOUR(DarkBlue);
      CHECK_COLOUR(DarkPurple);
   }

   if (name[0] == 'g' || name[0] == 'G') {
      CHECK_COLOUR(Gold);
      CHECK_COLOUR(Green);
      CHECK_COLOUR(Grey90);
      CHECK_COLOUR(Grey80);
      CHECK_COLOUR(Grey70);
      CHECK_COLOUR(Grey60);
      CHECK_COLOUR(Grey50);
      CHECK_COLOUR(Grey40);
      CHECK_COLOUR(Grey30);
      CHECK_COLOUR(Grey20);
      CHECK_COLOUR(Grey10);
   }

   if (name[0] == 'l' || name[0] == 'L') {
      CHECK_COLOUR(LightBlue);
   }

   if (name[0] == 'o' || name[0] == 'O') {
      CHECK_COLOUR(Orange);
   }

   if (name[0] == 'p' || name[0] == 'P') {
      CHECK_COLOUR(Purple);
   }

   if (name[0] == 'r' || name[0] == 'R') {
      CHECK_COLOUR(Red);
   }

   if (name[0] == 's' || name[0] == 'S') {
      CHECK_COLOUR(Silver);
   }

   if (name[0] == 'w' || name[0] == 'W') {
      CHECK_COLOUR(White);
   }

   if (name[0] == 'y' || name[0] == 'Y') {
      CHECK_COLOUR(Yellow);
   }

   if (name[0] == 'm' || name[0] == 'M') {
      CHECK_COLOUR(MidnightBlue);
   }

   WARN(_("Unknown colour %s"), name);
   return NULL;
}
#undef CHECK_COLOUR

