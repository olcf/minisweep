/*---------------------------------------------------------------------------*/
/*!
 * \file   sweeper_acc.h
 * \author Robert Searles, Wayne Joubert
 * \date   Wed Apr 11 9:12:00 EST 2018
 * \brief  Definitions for performing a sweep, OpenACC/KBA version.
 * \note   Copyright (C) 2018 Oak Ridge National Laboratory, UT-Battelle, LLC.
 */
/*---------------------------------------------------------------------------*/

#ifndef _sweeper_acc_h_
#define _sweeper_acc_h_

#include "env.h"
#include "definitions.h"
#include "dimensions.h"
#include "arguments.h"
#include "quantities.h"

#include "faces_kba.h"
#include "stepscheduler_kba.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef SWEEPER_KBA_ACC

/*===========================================================================*/
/*---Struct with pointers etc. used to perform sweep---*/

typedef struct
{
  //P* __restrict__  facexy;
  //P* __restrict__  facexz;
  //P* __restrict__  faceyz;
  P* __restrict__  vslocal_host_;

  int              nblock_z;
  int              nblock_octant;
  int              noctant_per_block;

  Dimensions       dims;
  Dimensions       dims_b;

  StepScheduler    stepscheduler;

  Faces            faces;
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
/*---Perform a sweep---*/

void Sweeper_sweep(
  Sweeper*               sweeper,
  Pointer*               vo,
  Pointer*               vi,
  const Quantities*      quan,
  Env*                   env );
#endif

/*===========================================================================*/

#endif /*---SWEEPER_KBA_ACC---*/

/*===========================================================================*/
/*---Perform a sweep on a block---*/

void Sweeper_sweep_block(
  Sweeper*               sweeper,
  Pointer*               vo,
  Pointer*               vi,
  int*                   is_block_init,
  Pointer*               facexy,
  Pointer*               facexz,
  Pointer*               faceyz,
  const Pointer*         a_from_m,
  const Pointer*         m_from_a,
  int                    step,
  const Quantities*      quan,
  Env*                   env );

/*===========================================================================*/

#ifdef __cplusplus
} /*---extern "C"---*/
#endif

#endif /*---_sweeper_acc_h_---*/

/*---------------------------------------------------------------------------*/
