/*
 * See Licensing and Copyright notice in naev.h
 */


#ifndef NPNG_H
#  define NPNG_H


#include "SDL.h"

#include <png.h>


typedef struct npng_s npng_t;


npng_t *npng_open( SDL_RWops *rw );
void npng_close( npng_t *npng );
int npng_info( npng_t *npng );
int npng_dim( npng_t *npng, png_uint_32 *w, png_uint_32 *h );
int npng_pitch( npng_t *npng );
int npng_readInto( npng_t *npng, png_bytep *row_pointers );
png_bytep npng_readImage( npng_t *npng, png_bytep **rows, int *channels, int *pitch );
SDL_Surface *npng_readSurface( npng_t *npng );


#endif /* NPNG_H */

