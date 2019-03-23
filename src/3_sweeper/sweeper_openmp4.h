/*---------------------------------------------------------------------------*/
/*!
 * \file   sweeper_acc.h
 * \author Robert Searles, Wayne Joubert
 * \date   Wed Apr 11 9:12:00 EST 2018
 * \brief  Definitions for performing a sweep, OpenACC/KBA version.
 * \note   Copyright (C) 2018 Oak Ridge National Laboratory, UT-Battelle, LLC.
 */
/*---------------------------------------------------------------------------*/

#ifndef _sweeper_openmp4_h_
#define _sweeper_openmp4_h_

#include "env.h"
#include "definitions.h"
#include "dimensions.h"
#include "arguments.h"
#include "quantities.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*===========================================================================*/
/*---Struct with pointers etc. used to perform sweep---*/

typedef struct
{
  P* __restrict__  facexy;
  P* __restrict__  facexz;
  P* __restrict__  faceyz;
  P* __restrict__  vslocal;

  Dimensions       dims;
} Sweeper;

/*===========================================================================*/
/*---Null object---*/

Sweeper Sweeper_null(void);

/*===========================================================================*/
/*---Pseudo-constructor for Sweeper struct---*/

void Sweeper_create( Sweeper*          sweeper,
                     Dimensions        dims,
                     const Quantities* quan,
                     Env*              env,
                     Arguments*        args );

/*===========================================================================*/
/*---Pseudo-destructor for Sweeper struct---*/

void Sweeper_destroy( Sweeper* sweeper,
                      Env*     env );

/*===========================================================================*/
/*---Number of octants in an octant block---*/

static int Sweeper_noctant_per_block( const Sweeper* sweeper )
{
  return 1;
}

#if 0
/*===========================================================================*/
/*---In-gricell computations---*/
#ifdef USE_OPENMP4
#pragma omp declare target
#endif
#ifdef USE_ACC
#pragma acc routine vector
#endif
void Sweeper_in_gridcell(  Dimensions dims,
			     int wavefront,
			     int octant,
			     int ix, int iy,
			     int dir_x, int dir_y, int dir_z,
			     P* __restrict__ facexy,
			     P* __restrict__ facexz,
			     P* __restrict__ faceyz,
			     P* v_a_from_m,
			     P* v_m_from_a,
			     P* vi_h,
			     P* vo_h,
			     P* vs_local
			   );
#ifdef USE_OPENMP4
#pragma omp end declare target
#endif
#endif

/*===========================================================================*/
/*---Perform a sweep---*/

void Sweeper_sweep(
  Sweeper*               sweeper,
  Pointer*               vo,
  Pointer*               vi,
  const Quantities*      quan,
  Env*                   env );

/*===========================================================================*/

#ifdef __cplusplus
} /*---extern "C"---*/
#endif

#endif /*---_sweeper_openmp4_h_---*/

/*---------------------------------------------------------------------------*/
