/*
 * See Licensing and Copyright notice in naev.h
 */


#ifndef PILOT_EW_H
#  define PILOT_EW_H


#include "pilot.h"



/*
 * Sensors and range.
 */
void pilot_updateSensorRange (void);
double pilot_sensorRange( void );
int pilot_inRange( const Pilot *p, double x, double y );
int pilot_inRangePilot( const Pilot *p, const Pilot *target, double *dist2);
int pilot_inRangePlanet( const Pilot *p, int target );
int pilot_inRangeAsteroid( const Pilot *p, int ast, int fie );
int pilot_inRangeJump( const Pilot *p, int target );

/*
 * Weapon tracking.
 */
double pilot_ewWeaponTrack( const Pilot *p, const Pilot *t, double trackmin, double trackmax );

/*
 * Electronic warfare updating.
 */
int pilot_ewScanCheck( const Pilot *p );
double pilot_ewScanTime( const Pilot *p );
void pilot_ewScanStart( Pilot *p );
void pilot_ewUpdateStatic( Pilot *p );
void pilot_ewUpdateDynamic( Pilot *p, double dt );

/*
 * Stealth.
 */
void pilot_ewUpdateStealth( Pilot *p, double dt );
int pilot_stealth( Pilot *p );
void pilot_destealth( Pilot *p );


#endif /* PILOT_EW_H */
