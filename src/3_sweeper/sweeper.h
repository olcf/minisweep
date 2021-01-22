/*---------------------------------------------------------------------------*/
/*!
 * \file   sweeper.h
 * \author Wayne Joubert
 * \date   Wed Jan 15 16:06:28 EST 2014
 * \brief  Definitions for performing a sweep.
 * \note   Copyright (C) 2014 Oak Ridge National Laboratory, UT-Battelle, LLC.
 */
/*---------------------------------------------------------------------------*/

#ifndef _sweep_h_
#define _sweep_h_

/* #ifndef SWEEPER_SIMPLE */
/* #ifndef SWEEPER_TILEOCTANTS */
/* #ifndef SWEEPER_KBA */
/* #define SWEEPER_ACC */
/* #endif */
/* #endif */
/* #endif */

#ifdef SPEC
#ifdef SPEC_OPENACC
  #define USE_ACC
#else
#ifdef SPEC_OPENMP_TARGET
  #define USE_OPENMP_TARGET
#endif
#endif
#endif

#ifdef USE_ACC
  #ifdef USE_KBA
    #define SWEEPER_KBA_ACC
  #else
    #define SWEEPER_ACC
  #endif
#elif defined(USE_OPENMP_TARGET)
  #ifdef USE_KBA
    //TODO - make this work
    #define SWEEPER_KBA_OPENMP_TARGET
  #else
    #define SWEEPER_OPENMP_TARGET
  #endif
#else
  #ifndef SWEEPER_SIMPLE
  #ifndef SWEEPER_TILEOCTANTS
    #define SWEEPER_KBA
  #endif
  #endif
#endif

#ifdef SWEEPER_SIMPLE
  #include "sweeper_simple.h"
#endif

#ifdef SWEEPER_TILEOCTANTS
  #include "sweeper_tileoctants.h"
#endif

#ifdef SWEEPER_KBA
  #include "sweeper_kba.h"
#endif

#ifdef SWEEPER_KBA_ACC
  #include "sweeper_kba.h"
  #include "sweeper_gpu.h"
#endif

#ifdef SWEEPER_KBA_OPENMP_TARGET
  #include "sweeper_kba.h"
  #include "sweeper_gpu.h"
#endif

#ifdef SWEEPER_ACC
  #include "sweeper_gpu.h"
#endif

#ifdef SWEEPER_OPENMP_TARGET
  #include "sweeper_gpu.h"
#endif

#endif /*---_sweep_h_---*/

/*---------------------------------------------------------------------------*/
