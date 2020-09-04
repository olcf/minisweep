/*---------------------------------------------------------------------------*/
/*!
 * \file   sweeper_acc_c.h
 * \author Robert Searles, Wayne Joubert
 * \date   Wed Apr 11 9:12:00 EST 2018
 * \brief  Definitions for performing a sweep, OpenACC/KBA version.
 * \note   Copyright (C) 2018 Oak Ridge National Laboratory, UT-Battelle, LLC.
 */
/*---------------------------------------------------------------------------*/

#ifndef _sweeper_acc_c_h_
#define _sweeper_acc_c_h_

#include "env.h"
#include "definitions.h"
#include "dimensions.h"
#include "quantities.h"
#include "array_accessors.h"
#include "array_operations.h"
#include "sweeper_acc.h"

//#ifdef USE_MPI
//#include "openacc.h"
//#endif

#ifdef __cplusplus
extern "C"
{
#endif

/*===========================================================================*/
/*---Null object---*/

Sweeper Sweeper_null()
{
  Sweeper result;
  memset( (void*)&result, 0, sizeof(Sweeper) );
  return result;
}

/*===========================================================================*/
/*---Pseudo-constructor for Sweeper struct---*/

void Sweeper_create( Sweeper*          sweeper,
                     Dimensions        dims,
                     const Quantities* quan,
                     Env*              env,
                     Arguments*        args )
{
  sweeper->nblock_z = 1; //FIX
  sweeper->noctant_per_block = NOCTANT;
  sweeper->nblock_octant     = NOCTANT / sweeper->noctant_per_block;

  const int dims_b_ncell_z = dims.ncell_z / sweeper->nblock_z;

  sweeper->dims = dims;
  sweeper->dims_b = sweeper->dims;
  sweeper->dims_b.ncell_z = dims_b_ncell_z;

  StepScheduler_create( &(sweeper->stepscheduler),
                              sweeper->nblock_z, sweeper->nblock_octant, env );

  const Bool_t is_face_comm_async = Bool_false;

  Faces_create( &(sweeper->faces), sweeper->dims_b,
                sweeper->noctant_per_block, is_face_comm_async, env );

  /*---Allocate arrays---*/

  sweeper->vslocal = malloc_host_P( dims.na * NU * dims.ne * NOCTANT * dims.ncell_x * dims.ncell_y );
  //sweeper->facexy  = malloc_host_P( dims.ncell_x * dims.ncell_y * dims.ne *
  //                       dims.na * NU * NOCTANT);
  //sweeper->facexz  = malloc_host_P( dims.ncell_x * dims.ncell_z * dims.ne *
  //                       dims.na * NU * NOCTANT);
  //sweeper->faceyz  = malloc_host_P( dims.ncell_y * dims.ncell_z * dims.ne *
  //                       dims.na * NU * NOCTANT);

}

/*===========================================================================*/
/*---Pseudo-destructor for Sweeper struct---*/

void Sweeper_destroy( Sweeper* sweeper,
                      Env*     env )
{
  /*---Deallocate arrays---*/

  free_host_P( sweeper->vslocal );
  //free_host_P( sweeper->facexy );
  //free_host_P( sweeper->facexz );
  //free_host_P( sweeper->faceyz );

  sweeper->vslocal = NULL;
  //sweeper->facexy  = NULL;
  //sweeper->facexz  = NULL;
  //sweeper->faceyz  = NULL;

  Faces_destroy( &(sweeper->faces) );
}

/*===========================================================================*/
/*---Quantities_init_face inline routine---*/

#pragma acc routine seq
P Quantities_init_face_inline(int ia, int ie, int iu, int scalefactor_space, int octant)
{
  /*--- Quantities_affinefunction_ inline ---*/
  return ( (P) (1 + ia) ) 

    /*--- Quantities_scalefactor_angle_ inline ---*/
    * ( (P) (1 << (ia & ( (1<<3) - 1))) ) 

    /*--- Quantities_scalefactor_space_ inline ---*/
    * ( (P) scalefactor_space)

    /*--- Quantities_scalefactor_energy_ inline ---*/
    * ( (P) (1 << ((( (ie) * 1366 + 150889) % 714025) & ( (1<<2) - 1))) )

    /*--- Quantities_scalefactor_unknown_ inline ---*/
    * ( (P) (1 << ((( (iu) * 741 + 60037) % 312500) & ( (1<<2) - 1))) )

    /*--- Quantities_scalefactor_octant_ ---*/
    * ( (P) 1 + octant);

}

/*===========================================================================*/
/*---Quantities_scalefactor_space_ inline routine---*/

#pragma acc routine seq
int Quantities_scalefactor_space_inline(int ix_g, int iy_g, int iz_g)
{
  int result = 0;

#ifndef RELAXED_TESTING
  const int im = 134456;
  const int ia = 8121;
  const int ic = 28411;

  result = ( (result+(ix_g+2))*ia + ic ) % im;
  result = ( (result+(iy_g+2))*ia + ic ) % im;
  result = ( (result+(iz_g+2))*ia + ic ) % im;
  result = ( (result+(ix_g+3*iy_g+7*iz_g+2))*ia + ic ) % im;
  result = ix_g+3*iy_g+7*iz_g+2;
  result = result & ( (1<<2) - 1 );
#endif
  result = 1 << result;

  return result;
}

/*===========================================================================*/

#pragma acc routine seq
void Quantities_solve_inline(P* vs_local, Dimensions dims, P* facexy, P* facexz, P* faceyz, 
                             int ix, int iy, int iz, int ie, int ia,
                             int octant, int octant_in_block, int noctant_per_block)
{
  const int dir_x = Dir_x( octant );
  const int dir_y = Dir_y( octant );
  const int dir_z = Dir_z( octant );

  int iu = 0;

  /*---Average the face values and accumulate---*/

  /*---The state value and incoming face values are first adjusted to
    normalized values by removing the spatial scaling.
    They are then combined using a weighted average chosen in a special
    way to give just the expected result.
    Finally, spatial scaling is applied to the result which is then
    stored.
    ---*/

  /*--- Quantities_scalefactor_octant_ inline ---*/
  const P scalefactor_octant = 1 + octant;
  const P scalefactor_octant_r = ((P)1) / scalefactor_octant;

  /*---Quantities_scalefactor_space_ inline ---*/
  const P scalefactor_space = (P)Quantities_scalefactor_space_inline(ix, iy, iz);
  const P scalefactor_space_r = ((P)1) / scalefactor_space;
  const P scalefactor_space_x_r = ((P)1) /
    Quantities_scalefactor_space_inline( ix - dir_x, iy, iz );
  const P scalefactor_space_y_r = ((P)1) /
    Quantities_scalefactor_space_inline( ix, iy - dir_y, iz );
  const P scalefactor_space_z_r = ((P)1) /
    Quantities_scalefactor_space_inline( ix, iy, iz - dir_z );

#pragma acc loop seq
  for( iu=0; iu<NU; ++iu )
    {

      int vs_local_index = ia + dims.na * (
                           iu + NU  * (
                           ie + dims.ne * (
                           ix + dims.ncell_x * (
                           iy + dims.ncell_y * (
                           octant + NOCTANT * (
                           0))))));

      const P result = ( vs_local[vs_local_index] * scalefactor_space_r + 
               (
                /*--- ref_facexy inline ---*/
                facexy[ia + dims.na      * (
                        iu + NU           * (
                        ie + dims.ne      * (
                        ix + dims.ncell_x * (
                        iy + dims.ncell_y * (
                        octant + NOCTANT * (
                        0 )))))) ]

               /*--- Quantities_xfluxweight_ inline ---*/
               * (P) ( 1 / (P) 2 )

               * scalefactor_space_z_r

               /*--- ref_facexz inline ---*/
               + facexz[ia + dims.na      * (
                        iu + NU           * (
                        ie + dims.ne      * (
                        ix + dims.ncell_x * (
                        iz + dims.ncell_z * (
                        octant + NOCTANT * (
                        0 )))))) ]

               /*--- Quantities_yfluxweight_ inline ---*/
               * (P) ( 1 / (P) 4 )

               * scalefactor_space_y_r

               /*--- ref_faceyz inline ---*/
               + faceyz[ia + dims.na      * (
                        iu + NU           * (
                        ie + dims.ne      * (
                        iy + dims.ncell_y * (
                        iz + dims.ncell_z * (
                        octant + NOCTANT * (
                        0 )))))) ]

                        /*--- Quantities_zfluxweight_ inline ---*/
                        * (P) ( 1 / (P) 4 - 1 / (P) (1 << ( ia & ( (1<<3) - 1 ) )) )

               * scalefactor_space_x_r
               ) 
               * scalefactor_octant_r ) * scalefactor_space;

      vs_local[vs_local_index] = result;

      const P result_scaled = result * scalefactor_octant;
      /*--- ref_facexy inline ---*/
      facexy[ia + dims.na      * (
             iu + NU           * (
             ie + dims.ne      * (
             ix + dims.ncell_x * (
             iy + dims.ncell_y * (
             octant + NOCTANT * (
             0 )))))) ] = result_scaled;

      /*--- ref_facexz inline ---*/
      facexz[ia + dims.na      * (
             iu + NU           * (
             ie + dims.ne      * (
             ix + dims.ncell_x * (
             iz + dims.ncell_z * (
             octant + NOCTANT * (
             0 )))))) ] = result_scaled;

      /*--- ref_faceyz inline ---*/
      faceyz[ia + dims.na      * (
             iu + NU           * (
             ie + dims.ne      * (
             iy + dims.ncell_y * (
             iz + dims.ncell_z * (
             octant + NOCTANT * (
             0 )))))) ] = result_scaled;

    } /*---for---*/
}

/*===========================================================================*/
/*---In-gricell computations---*/

#pragma acc routine vector
void Sweeper_sweep_cell_acceldir( Dimensions dims,
                                  int wavefront,
                                  int octant,
                                  int ix, int iy,
                                  int dir_x, int dir_y, int dir_z,
                                  P* __restrict__ facexy,
                                  P* __restrict__ facexz,
                                  P* __restrict__ faceyz,
                                  const P* __restrict__ a_from_m,
                                  const P* __restrict__ m_from_a,
                                  const P* vi,
                                  P* vo,
                                  P* vs_local,
                                  int octant_in_block,
                                  int noctant_per_block
                                  )
{
  /*---Declarations---*/
  int iz = 0;
  int ie = 0;
  int im = 0;
  int ia = 0;
  int iu = 0;
  /* int octant = 0; */

  /*--- Dimensions ---*/
  int dims_ncell_x = dims.ncell_x;
  int dims_ncell_y = dims.ncell_y;
  int dims_ncell_z = dims.ncell_z;
  int dims_ne = dims.ne;
  int dims_na = dims.na;
  int dims_nm = dims.nm;

  /*--- Solve for Z dimension, and check bounds.
    The sum of the dimensions should equal the wavefront number.
    If z < 0 or z > wavefront number, we are out of bounds.
    Z also shouldn't exceed the spacial bound for the z dimension.

    The calculation is adjusted for the direction of each axis
    in a given octant.
  ---*/

  int ixwav, iywav, izwav;
  if (dir_x==DIR_UP) { ixwav = ix; } else { ixwav = (dims_ncell_x-1) - ix; }
  if (dir_y==DIR_UP) { iywav = iy; } else { iywav = (dims_ncell_y-1) - iy; }
  
  if (dir_z==DIR_UP) {
    iz = wavefront - (ixwav + iywav); } 
  else { 
    iz = (dims_ncell_z-1) - (wavefront - (ixwav + iywav));
  }

  /*--- Bounds check ---*/
  if ((iz >= 0 && iz < dims_ncell_z) )// &&
    /* ((dir_z==DIR_UP && iz <= wavefront) || */
    /*  (dir_z==DIR_DN && (dims_ncell_z-1-iz) <= wavefront))) */
    {

   /*---Loop over energy groups---*/
#pragma acc loop independent vector, collapse(3)
      for( ie=0; ie<dims_ne; ++ie )
      {

      /*--------------------*/
      /*---Transform state vector from moments to angles---*/
      /*--------------------*/

      /*---This loads values from the input state vector,
           does the small dense matrix-vector product,
           and stores the result in a relatively small local
           array that is hopefully small enough to fit into
           processor cache.
      ---*/

      for( iu=0; iu<NU; ++iu )
      for( ia=0; ia<dims_na; ++ia )
      { 
        // reset reduction
        P result = (P)0;
#pragma acc loop seq
        for( im=0; im < dims_nm; ++im )
        {
          /*--- const_ref_a_from_m inline ---*/
          result += a_from_m[ ia     + dims.na * (
                              im     +      NM * (
                              octant + NOCTANT * (
                              0 ))) ] * 

            /*--- const_ref_state inline ---*/
            vi[im + dims.nm      * (
                          iu + NU           * (
                          ix + dims.ncell_x * (
                          iy + dims.ncell_y * (
                          ie + dims.ne      * (
                          iz + dims.ncell_z * ( /*---NOTE: This axis MUST be slowest-varying---*/
                          0 ))))))];
        }

        /*--- ref_vslocal inline ---*/
        vs_local[ ia + dims.na * (
                  iu + NU  * (
                  ie + dims.ne * (
                  ix + dims.ncell_x * (
                  iy + dims.ncell_y * (
                  octant + NOCTANT * (
                                       0)))))) ] = result;
      }
      }

      /*--------------------*/
      /*---Perform solve---*/
      /*--------------------*/

   /*---Loop over energy groups---*/
#pragma acc loop independent vector, collapse(2)
      for( ie=0; ie<dims_ne; ++ie )
      for( ia=0; ia<dims_na; ++ia )
      {
        Quantities_solve_inline(vs_local, dims, facexy, facexz, faceyz, 
                             ix, iy, iz, ie, ia,
                             octant, octant_in_block, noctant_per_block);
      }

      /*--------------------*/
      /*---Transform state vector from angles to moments---*/
      /*--------------------*/

      /*---Perform small dense matrix-vector products and store
           the result in the output state vector.
      ---*/

   /*---Loop over energy groups---*/
#pragma acc loop independent vector, collapse(3)
      for( ie=0; ie<dims_ne; ++ie )
      {

      for( iu=0; iu<NU; ++iu )
      for( im=0; im<dims_nm; ++im )
      {
        P result = (P)0;
#pragma acc loop seq
        for( ia=0; ia<dims_na; ++ia )
        {
         /*--- const_ref_m_from_a ---*/
         result += m_from_a[ im     +      NM * (
                             ia     + dims.na * (
                             octant + NOCTANT * (
                             0 ))) ] *

         /*--- const_ref_vslocal ---*/
         vs_local[ ia + dims.na * (
                   iu + NU    * (
                   ie + dims.ne * (
                   ix + dims.ncell_x * (
                   iy + dims.ncell_y * (
                   octant + NOCTANT * (
                   0 )))))) ];
        }

        /*--- ref_state inline ---*/
#pragma acc atomic update
        vo[im + dims.nm      * (
           iu + NU           * (
           ix + dims.ncell_x * (
           iy + dims.ncell_y * (
           ie + dims.ne      * (
           iz + dims.ncell_z * ( /*---NOTE: This axis MUST be slowest-varying---*/
           0 ))))))] += result;
      }

      } /*---ie---*/

    } /*--- iz ---*/
}

/*===========================================================================*/
/*---Perform a sweep---*/

void Sweeper_sweep_block(
  Sweeper*               sweeper,
        P* __restrict__  vo,
  const P* __restrict__  vi,
  int*                   is_block_init,
        P* __restrict__  facexy,
        P* __restrict__  facexz,
        P* __restrict__  faceyz,
  const P* __restrict__  a_from_m,
  const P* __restrict__  m_from_a,
  int                    step,
  const Quantities*      quan,
  Env*                   env )
{
  Assert( sweeper );
  Assert( vi );
  Assert( vo );
//  Assert( is_block_init );
  Assert( facexy );
  Assert( facexz );
  Assert( faceyz );
  Assert( a_from_m );
  Assert( m_from_a );
  Assert( quan );
  Assert( env );

  /*--- Set OpenACC device based on MPI rank ---*/
//#ifdef USE_MPI
//  //int num_devices = acc_get_num_devices(acc_device_default);
//  int num_devices = acc_get_num_devices(acc_device_nvidia);
//  int mpi_rank;
//  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
//  int device_num = mpi_rank % num_devices;
//  printf("%d: num_devices = %d, device_num = %d\n",mpi_rank,num_devices,device_num);
//  //acc_set_device_num( device_num, acc_device_default );
//  acc_set_device_num( device_num, acc_device_nvidia );
//#endif

  const int nstep = 1; //FIX

  /*---Declarations---*/
  int wavefront = 0;
  int ix = 0;
  int iy = 0;
  int iz = 0;
  int ie = 0;
  int im = 0;
  int ia = 0;
  int iu = 0;
  int octant = 0;
  const int noctant_per_block = sweeper->noctant_per_block;
  int octant_in_block = 0;


// TODO: check dims, dims_b, dims_g
  /*--- Dimensions ---*/
  Dimensions dims = sweeper->dims;
  Dimensions dims_b = sweeper->dims_b;
  int dims_b_ncell_x = dims_b.ncell_x;
  int dims_b_ncell_y = dims_b.ncell_y;
  int dims_b_ncell_z = dims_b.ncell_z;
  int dims_ncell_z = dims.ncell_z;
  int dims_b_ne = dims_b.ne;
  int dims_b_na = dims_b.na;
  int dims_b_nm = dims_b.nm;

  /*--- Array Pointers ---*/
  P* vs_local = sweeper->vslocal;

  /*--- Array Sizes ---*/
  int facexy_size = dims_b.ncell_x * dims_b.ncell_y * 
    dims_b.ne * dims_b.na * NU * NOCTANT;
  int facexz_size = dims_b.ncell_x * dims_b.ncell_z * 
    dims_b.ne * dims_b.na * NU * NOCTANT;
  int faceyz_size = dims_b.ncell_y * dims_b.ncell_z * 
    dims_b.ne * dims_b.na * NU * NOCTANT;

  int a_from_m_size = dims_b.nm * dims_b.na * NOCTANT;
  int m_from_a_size = dims_b.nm * dims_b.na * NOCTANT;

  int v_size = dims.ncell_x * dims.ncell_y * dims.ncell_z * 
    dims_b.ne * dims_b.nm * NU;
  int v_b_size = dims_b.ncell_x * dims_b.ncell_y * dims_b.ncell_z * 
    dims_b.ne * dims_b.nm * NU;

  int vs_local_size = dims_b.na * NU * dims_b.ne * NOCTANT * dims_b.ncell_x * dims_b.ncell_y;

  const int proc_x = Env_proc_x_this( env );
  const int proc_y = Env_proc_y_this( env );

  StepInfoAll stepinfoall;  /*---But only use noctant_per_block values---*/

  for( octant_in_block=0; octant_in_block<sweeper->noctant_per_block;
                                                            ++octant_in_block )
  {
    stepinfoall.stepinfo[octant_in_block] = StepScheduler_stepinfo(
      &(sweeper->stepscheduler), step, octant_in_block, proc_x, proc_y );

  }

  octant_in_block = 0; //FIX

# if 0
a_from_m, m_from_a - copyin, delete to caller
vi, vo (global) - initialize on device in caller
vo (global) - copyout at end in caller
vi_b, vo_b - reference as global ptr + int offset, loop ovr only part.
  suggest don't form variables "vi_b", "vo_b"
make sure to say "present"

#endif
 
  /*--- Data transfer to the GPU ---*/
#pragma acc enter data copyin(a_from_m[:a_from_m_size], \
                              m_from_a[:m_from_a_size], \
                              vi[:v_size], \
                              vo[:v_size], \
                              dims_b), \
                       create(facexy[:facexy_size], \
                              facexz[:facexz_size], \
                              faceyz[:faceyz_size])

    /*---Initialize faces---*/

    /*---The semantics of the face arrays are as follows.
         On entering a cell for a solve at the gridcell level,
         the face array is assumed to have a value corresponding to
         "one cell lower" in the relevant direction.
         On leaving the gridcell solve, the face has been updated
         to have the flux at that gridcell.
         Thus, the face is initialized at first to have a value
         "one cell" outside of the domain, e.g., for the XY face,
         either -1 or dims.ncell_x.
         Note also that the face initializer functions now take
         coordinates for all three spatial dimensions --
         the third dimension is used to denote whether it is the
         "lower" or "upper" face and also its exact location
         in that dimension.
    ---*/


// TODO: conditionalize face set on whether proc on boundary, step number, etc.

#pragma acc parallel present(facexy[:facexy_size])
{

#pragma acc loop independent gang collapse(3)
      for( octant=0; octant<NOCTANT; ++octant )
      for( iy=0; iy<dims_b_ncell_y; ++iy )
      for( ix=0; ix<dims_b_ncell_x; ++ix )
#pragma acc loop independent vector collapse(3)
      for( ie=0; ie<dims_b_ne; ++ie )
      for( iu=0; iu<NU; ++iu )
      for( ia=0; ia<dims_b_na; ++ia )
      {

      const int dir_z = Dir_z( octant );
      iz = dir_z == DIR_UP ? -1 : dims_b_ncell_z;

      /*--- Quantities_scalefactor_space_ inline ---*/
      int scalefactor_space = Quantities_scalefactor_space_inline(ix, iy, iz);

      /*--- ref_facexy inline ---*/
      facexy[ia + dims_b.na      * (
                        iu + NU           * (
                        ie + dims_b.ne      * (
                        ix + dims_b.ncell_x * (
                        iy + dims_b.ncell_y * (
                        octant + NOCTANT * (
                        0 )))))) ]

        /*--- Quantities_init_face routine ---*/
        = Quantities_init_face_inline(ia, ie, iu, scalefactor_space, octant);
      }

 } /*--- #pragma acc parallel ---*/

#pragma acc parallel present(facexz[:facexz_size])
{
 
#pragma acc loop independent gang collapse(3)
      for( octant=0; octant<NOCTANT; ++octant )
      for( iz=0; iz<dims_b_ncell_z; ++iz )
      for( ix=0; ix<dims_b_ncell_x; ++ix )
#pragma acc loop independent vector collapse(3)
      for( ie=0; ie<dims_b_ne; ++ie )
      for( iu=0; iu<NU; ++iu )
      for( ia=0; ia<dims_b_na; ++ia )
      {

        const int dir_y = Dir_y( octant );
        iy = dir_y == DIR_UP ? -1 : dims_b_ncell_y;

        /*--- Quantities_scalefactor_space_ inline ---*/
        int scalefactor_space = Quantities_scalefactor_space_inline(ix, iy, iz);

        /*--- ref_facexz inline ---*/
        facexz[ia + dims_b.na      * (
                        iu + NU           * (
                        ie + dims_b.ne      * (
                        ix + dims_b.ncell_x * (
                        iz + dims_b.ncell_z * (
                        octant + NOCTANT * (
                                             0 )))))) ]

          /*--- Quantities_init_face routine ---*/
          = Quantities_init_face_inline(ia, ie, iu, scalefactor_space, octant);
      }

 } /*--- #pragma acc parallel ---*/

#pragma acc parallel present(faceyz[:faceyz_size])
{

#pragma acc loop independent gang collapse(3)
      for( octant=0; octant<NOCTANT; ++octant )
      for( iz=0; iz<dims_b_ncell_z; ++iz )
      for( iy=0; iy<dims_b_ncell_y; ++iy )
#pragma acc loop independent vector collapse(3)
      for( ie=0; ie<dims_b_ne; ++ie )
      for( iu=0; iu<NU; ++iu )
      for( ia=0; ia<dims_b_na; ++ia )
      {

        const int dir_x = Dir_x( octant );
        ix = dir_x == DIR_UP ? -1 : dims_b_ncell_x;

        /*--- Quantities_scalefactor_space_ inline ---*/
        int scalefactor_space = Quantities_scalefactor_space_inline(ix, iy, iz);

        /*--- ref_faceyz inline ---*/
        faceyz[ia + dims_b.na      * (
                        iu + NU           * (
                        ie + dims_b.ne      * (
                        iy + dims_b.ncell_y * (
                        iz + dims_b.ncell_z * (
                        octant + NOCTANT * (
                                             0 )))))) ]

          /*--- Quantities_init_face routine ---*/
          = Quantities_init_face_inline(ia, ie, iu, scalefactor_space, octant);
      }

 } /*--- #pragma acc parallel ---*/

// TODO: conditionalize based on step info, etc.

#pragma acc data present(a_from_m[:a_from_m_size], \
                         m_from_a[:m_from_a_size], \
                         vi[:v_size], \
                         vo[:v_size], \
                         facexy[:facexy_size], \
                         facexz[:facexz_size], \
                         faceyz[:faceyz_size], \
                         dims_b), \
                 create(vs_local[vs_local_size])
 {

/*---Loop over octants---*/
  for( octant=0; octant<NOCTANT; ++octant )
  {

   /*---Decode octant directions from octant number---*/

   const int dir_x = Dir_x( octant );
   const int dir_y = Dir_y( octant );
   const int dir_z = Dir_z( octant );


   const int v_offset = 0; // FIX


   /*--- KBA sweep ---*/

   /*--- Number of wavefronts equals the sum of the dimension sizes
     minus the number of dimensions minus one. In our case, we have
     three total dimensions, so we add the sizes and subtract 2. 
   ---*/
   int num_wavefronts = (dims_b.ncell_z + dims_b.ncell_y + dims_b.ncell_x) - 2;
 
   /*--- Loop over wavefronts ---*/
   for (wavefront = 0; wavefront < num_wavefronts; wavefront++)
     {

/*--- Create an asynchronous queue for each octant ---*/
#pragma acc parallel async(octant)
     {

       /*---Loop over cells, in proper direction---*/

#pragma acc loop independent gang, collapse(2)
       for( int iy_updown=0; iy_updown<dims_b_ncell_y; ++iy_updown )
         for( int ix_updown=0; ix_updown<dims_b_ncell_x; ++ix_updown )
           {

             const int iy = dir_y==DIR_UP ? iy_updown : dims_b_ncell_y - 1 - iy_updown;
             const int ix = dir_x==DIR_UP ? ix_updown : dims_b_ncell_x - 1 - ix_updown;

             /*--- In-gridcell computations ---*/
             Sweeper_sweep_cell_acceldir( dims_b, wavefront, octant, ix, iy,
                                          dir_x, dir_y, dir_z,
                                          facexy, facexz, faceyz,
                                          a_from_m, m_from_a,
                                          &(vi[v_offset]), &(vo[v_offset]), vs_local,
                                          octant_in_block, noctant_per_block );
           } /*---ix/iy---*/

     } /*--- #pragma acc parallel ---*/

     } /*--- wavefront ---*/

  } /*---octant---*/
 
}   /*--- #pragma acc enter data ---*/

#pragma acc wait

  if (step < nstep - 1) {   
  /*--- Data transfer of results to the host ---*/
#pragma acc exit data copyout(vo[:v_size], \
                              facexy[:facexy_size], \
                              facexz[:facexz_size], \
                              faceyz[:faceyz_size]) \
                      delete(a_from_m[:a_from_m_size], \
                             m_from_a[:m_from_a_size], \
                             vi[:v_size], \
                             vs_local[:vs_local_size])
  } else {
#pragma acc exit data copyout(vo[:v_size]), \
                      delete(a_from_m[:a_from_m_size], \
                             m_from_a[:m_from_a_size], \
                             vi[:v_size], \
                             facexy[:facexy_size], \
                             facexz[:facexz_size], \
                             faceyz[:faceyz_size], \
                             vs_local[:vs_local_size])
  }

} /*---sweep---*/

/*===========================================================================*/
/*---Perform a sweep---*/

void Sweeper_sweep(
  Sweeper*               sweeper,
  Pointer*               vo,
  Pointer*               vi,
  const Quantities*      quan,
  Env*                   env )
{
  Assert( sweeper );
  Assert( vi );
  Assert( vo );
  Assert( quan );
  Assert( env );

  int* is_block_init = NULL; // unused

  /*---Initialize result array to zero---*/

  initialize_state_zero( Pointer_h( vo ), sweeper->dims, NU );

  const int nstep = 1; //FIX

  int step = 0;
  for (step = 0; step < nstep; ++step) {

    Sweeper_sweep_block(
      sweeper,
      Pointer_h( vo ),
      Pointer_h( vi ),
      is_block_init,
      (P*) Pointer_h( &(sweeper->faces.facexy0) ),
      (P*) Pointer_h( &(sweeper->faces.facexz0) ),
      (P*) Pointer_h( &(sweeper->faces.faceyz0) ),
      //sweeper->facexy,
      //sweeper->facexz,
      //sweeper->faceyz,
      (P*) Pointer_const_h( & quan->a_from_m),
      (P*) Pointer_const_h( & quan->m_from_a),
      step,
      quan,
      env);

  } // step



}

/*===========================================================================*/

#ifdef __cplusplus
} /*---extern "C"---*/
#endif

#endif /*---_sweeper_acc_c_h_---*/

/*---------------------------------------------------------------------------*/
