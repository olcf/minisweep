/*---------------------------------------------------------------------------*/
/*!
 * \file   sweeper_gpu_c.h
 * \author Robert Searles, Wayne Joubert, Veronica G. Melesse Vergara
 * \date   Wed Apr 11 9:12:00 EST 2018
 * \brief  Definitions for performing a sweep, OpenACC|OpenMP/KBA version.
 * \note   Copyright (C) 2018 Oak Ridge National Laboratory, UT-Battelle, LLC.
 */
/*---------------------------------------------------------------------------*/

#ifndef _sweeper_gpu_c_h_
#define _sweeper_gpu_c_h_

#include "env.h"
#include "definitions.h"
#include "dimensions.h"
#include "quantities.h"
#include "array_accessors.h"
#include "array_operations.h"
#include "sweeper_gpu.h"

#ifdef USE_ACC
#include "openacc.h"
#endif


#ifdef __cplusplus
extern "C"
{
#endif

/*===========================================================================*/
/*---Quantities_init_face inline routine---*/
#ifdef USE_OPENMP_TARGET
#pragma omp declare target
#elif USE_ACC
#pragma acc routine seq
#endif
P Quantities_init_face_acceldir(int ia, int ie, int iu, int scalefactor_space, int octant)
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
#ifdef USE_OPENMP_TARGET
#pragma omp end declare target
#endif

/*===========================================================================*/
/*---Quantities_scalefactor_space_ inline routine---*/

#ifdef USE_OPENMP_TARGET
#pragma omp declare target
#elif USE_ACC
#pragma acc routine seq
#endif
int Quantities_scalefactor_space_acceldir(int ix_g, int iy_g, int iz_g)
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
#ifdef USE_OPENMP_TARGET
#pragma omp end declare target
#endif

/*===========================================================================*/

#ifdef USE_OPENMP_TARGET
#pragma omp declare target
#elif USE_ACC
#pragma acc routine seq
#endif
void Quantities_solve_acceldir(P* vs_local, Dimensions dims, P* facexy, P* facexz, P* faceyz,
                             int ix, int iy, int iz,
                             int ix_g, int iy_g, int iz_g,
                             int ie, int ia,
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
  const P scalefactor_space = (P)Quantities_scalefactor_space_acceldir(ix_g, iy_g, iz_g);
  const P scalefactor_space_r = ((P)1) / scalefactor_space;
  const P scalefactor_space_x_r = ((P)1) /
    Quantities_scalefactor_space_acceldir( ix_g - dir_x, iy_g, iz_g );
  const P scalefactor_space_y_r = ((P)1) /
    Quantities_scalefactor_space_acceldir( ix_g, iy_g - dir_y, iz_g );
  const P scalefactor_space_z_r = ((P)1) /
    Quantities_scalefactor_space_acceldir( ix_g, iy_g, iz_g - dir_z );

#ifdef USE_OPENMP_TARGET
// no equivalent
#elif USE_ACC
#pragma acc loop seq
#endif
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
#ifdef USE_OPENMP_TARGET
#pragma omp end declare target
#endif

/*===========================================================================*/
/*---In-gricell computations---*/

#ifdef USE_OPENMP_TARGET
// no equivalent
#elif USE_ACC
#pragma acc routine vector
#endif
void Sweeper_sweep_cell_acceldir( Dimensions dims,
                                  int wavefront,
                                  int octant,
                                  int ix, int iy,
                                  int ix_g, int iy_g, int iz_g,
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
//  int iz = 0;
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

  const int ixwav = dir_x==DIR_UP ? ix : (dims_ncell_x-1) - ix;
  const int iywav = dir_y==DIR_UP ? iy : (dims_ncell_y-1) - iy;
  const int izwav = wavefront - ixwav - iywav;
  const int iz = dir_z==DIR_UP ? izwav : (dims_ncell_z-1) - izwav;

//  int ixwav, iywav, izwav;
//  if (dir_x==DIR_UP) { ixwav = ix; } else { ixwav = (dims_ncell_x-1) - ix; }
//  if (dir_y==DIR_UP) { iywav = iy; } else { iywav = (dims_ncell_y-1) - iy; }
  
//  if (dir_z==DIR_UP) {
//    iz = wavefront - (ixwav + iywav); } 
//  else { 
//    iz = (dims_ncell_z-1) - (wavefront - (ixwav + iywav));
//  }

  /*--- Bounds check ---*/
  if ((iz >= 0 && iz < dims_ncell_z) )// &&
    /* ((dir_z==DIR_UP && iz <= wavefront) || */
    /*  (dir_z==DIR_DN && (dims_ncell_z-1-iz) <= wavefront))) */
    {

   /*---Loop over energy groups---*/
#ifdef USE_OPENMP_TARGET
#pragma omp target teams distribute parallel for simd collapse(3) 
#elif USE_ACC
#pragma acc loop independent vector, collapse(3)
#endif
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
#ifdef USE_OPENMP_TARGET
// no equivalent
#elif USE_ACC
#pragma acc loop seq
#endif
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
#ifdef USE_OPENMP_TARGET
#pragma omp target teams distribute parallel for simd collapse(2) 
#elif USE_ACC
#pragma acc loop independent vector, collapse(2)
#endif
      for( ie=0; ie<dims_ne; ++ie )
      for( ia=0; ia<dims_na; ++ia )
      {
        Quantities_solve_acceldir(vs_local, dims, facexy, facexz, faceyz, 
                             ix, iy, iz,
                             ix_g, iy_g, iz_g,
                             ie, ia,
                             octant, octant_in_block, noctant_per_block);
      }

      /*--------------------*/
      /*---Transform state vector from angles to moments---*/
      /*--------------------*/

      /*---Perform small dense matrix-vector products and store
           the result in the output state vector.
      ---*/

   /*---Loop over energy groups---*/
#ifdef USE_OPENMP_TARGET
#pragma omp target teams distribute parallel for simd collapse(3) 
#elif USE_ACC
#pragma acc loop independent vector, collapse(3)
#endif
      for( ie=0; ie<dims_ne; ++ie )
      {

      for( iu=0; iu<NU; ++iu )
      for( im=0; im<dims_nm; ++im )
      {
        P result = (P)0;
#ifdef USE_OPENMP_TARGET
// no equivalent
#elif USE_ACC
#pragma acc loop seq
#endif
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
#ifdef USE_OPENMP_TARGET
//#pragma omp atomic update
#elif USE_ACC
#pragma acc atomic update
#endif
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
/*---Perform a sweep on a block---*/

void Sweeper_sweep_block_acceldir(
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
#ifdef USE_MPI
#ifdef USE_OPENMP_TARGET
  int num_devices = omp_get_num_devices();
#elif USE_ACC
  int num_devices = acc_get_num_devices(acc_device_default);
  //int num_devices = acc_get_num_devices(acc_device_nvidia);
#endif

  int mpi_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  int device_num = mpi_rank % num_devices;
  printf("%d: num_devices = %d, device_num = %d\n",mpi_rank,num_devices,device_num);

#ifdef USE_OPENMP_TARGET
  omp_set_default_device( device_num );
#elif USE_ACC
  acc_set_device_num( device_num, acc_device_default );
  //acc_set_device_num( device_num, acc_device_nvidia );
#endif
#endif

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
  int octant_in_block = 0;
  const int noctant_per_block = sweeper->noctant_per_block; // = 8

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
  P* vs_local = sweeper->vslocal_host_;

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
    dims.ne * dims.nm * NU;
  int v_b_size = dims_b.ncell_x * dims_b.ncell_y * dims_b.ncell_z * 
    dims_b.ne * dims_b.nm * NU;

  int vs_local_size = dims_b.na * NU * dims_b.ne * NOCTANT * dims_b.ncell_x * dims_b.ncell_y;

  const int proc_x = Env_proc_x_this( env );
  const int proc_y = Env_proc_y_this( env );

  const Bool_t proc_x_min = 0 == proc_x;
  const Bool_t proc_x_max = Env_nproc_x( env ) - 1 == proc_x;
  const Bool_t proc_y_min = 0 == proc_y;
  const Bool_t proc_y_max = Env_nproc_y( env ) - 1 == proc_y;

  StepInfoAll stepinfoall;  /*---But only use noctant_per_block values---*/

  for( octant_in_block=0; octant_in_block<sweeper->noctant_per_block;
                                                            ++octant_in_block )
  {
    stepinfoall.stepinfo[octant_in_block] = StepScheduler_stepinfo(
      &(sweeper->stepscheduler), step, octant_in_block, proc_x, proc_y );
  }

  /*
  Insist( dims_b.ncell_x * proc_x == quan->ix_base);
  Insist( dims_b.ncell_y * proc_y == quan->iy_base);
  */

  const int ix_base = quan->ix_base;
  const int iy_base = quan->iy_base;

  const int nstep = StepScheduler_nstep( &(sweeper->stepscheduler) );
  const int is_first_step = 0 == step;
  const int is_last_step = nstep - 1 == step;

  /*--- Data transfer to the GPU ---*/
  if (is_first_step) {

#ifdef USE_OPENMP_TARGET
#pragma omp target enter data \
  map(to: vi[0:v_size], \
          vo[0:v_size], \
          a_from_m[0:a_from_m_size], \
          m_from_a[0:m_from_a_size]), \
  map(alloc: facexy[0:facexy_size], \
             facexz[0:facexz_size], \
             faceyz[0:faceyz_size], \
             vs_local[0:vs_local_size])
#elif USE_ACC
    #pragma acc enter data \
      copyin(vi[:v_size]), \
      copyin(vo[:v_size]), \
      copyin(a_from_m[:a_from_m_size]), \
      copyin(m_from_a[:m_from_a_size]), \
      create(facexy[:facexy_size]), \
      create(facexz[:facexz_size]), \
      create(faceyz[:faceyz_size]), \
      create(vs_local[:vs_local_size])
#endif
  } else {
#ifdef USE_OPENMP_TARGET
#pragma omp target enter data \
  map(to: facexz[0:facexz_size], \
          faceyz[0:faceyz_size])
#elif USE_ACC
    #pragma acc enter data \
      copyin(facexz[:facexz_size]), \
      copyin(faceyz[:faceyz_size])
#endif
  }

#ifdef USE_OPENMP_TARGET
#pragma omp target data \
  map(to: dims_b, stepinfoall)
#elif USE_ACC
  #pragma acc enter data copyin(dims_b)
  #pragma acc enter data copyin(stepinfoall)
#endif

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

  /*---FACE XY---*/

  if (is_first_step) {

#ifdef USE_OPENMP_TARGET
//#pragma omp target update from(facexy[0:facexy_size], stepinfoall)
#elif USE_ACC
    #pragma acc parallel present(facexy[:facexy_size], stepinfoall)
#endif
    {

#ifdef USE_OPENMP_TARGET
#pragma omp target teams distribute collapse(3) 
#elif USE_ACC
      #pragma acc loop independent gang collapse(3)
#endif
      for( octant=0; octant<NOCTANT; ++octant )
      for( iy=0; iy<dims_b_ncell_y; ++iy )
      for( ix=0; ix<dims_b_ncell_x; ++ix )
#ifdef USE_OPENMP_TARGET
#pragma omp parallel for collapse(3)
#elif USE_ACC
      #pragma acc loop independent vector collapse(3)
#endif
      for( ie=0; ie<dims_b_ne; ++ie )
      for( iu=0; iu<NU; ++iu )
      for( ia=0; ia<dims_b_na; ++ia )
      {
        const int dir_z = Dir_z( octant );
        const int iz = dir_z == DIR_UP ? -1 : dims_b_ncell_z;

        const int ix_g = ix + ix_base; // dims_b_ncell_x * proc_x;
        const int iy_g = iy + iy_base; // dims_b_ncell_y * proc_y;
        const int iz_g = iz + (dir_z == DIR_UP ? 0 : dims_ncell_z - dims_b_ncell_z);
        //const int iz_g = iz + stepinfoall.stepinfo[octant].block_z * dims_b_ncell_z;

        /*--- Quantities_scalefactor_space_ inline ---*/
        const int scalefactor_space
          = Quantities_scalefactor_space_acceldir(ix_g, iy_g, iz_g);

        /*--- ref_facexy inline ---*/
        facexy[ia + dims_b_na      * (
               iu + NU           * (
               ie + dims_b_ne      * (
               ix + dims_b_ncell_x * (
               iy + dims_b_ncell_y * (
               octant + NOCTANT * (
               0 )))))) ]

        /*--- Quantities_init_face routine ---*/
          = Quantities_init_face_acceldir(ia, ie, iu, scalefactor_space, octant);
    } /*---for---*/

    } /*--- #pragma acc parallel ---*/

  } // is_first_step

  /*---FACE XZ---*/

#ifdef USE_OPENMP_TARGET
//#pragma omp target update from(facexz[0:facexz_size], stepinfoall)
#elif USE_ACC
  #pragma acc parallel present(facexz[:facexz_size], stepinfoall)
#endif
  {

#ifdef USE_OPENMP_TARGET
#pragma omp target teams distribute collapse(3) 
#elif USE_ACC
    #pragma acc loop independent gang collapse(3)
#endif
    for( octant=0; octant<NOCTANT; ++octant )
    for( iz=0; iz<dims_b_ncell_z; ++iz )
    for( ix=0; ix<dims_b_ncell_x; ++ix )
#ifdef USE_OPENMP_TARGET
// review this pragma
#pragma omp parallel for collapse(3)
#elif USE_ACC
    #pragma acc loop independent vector collapse(3)
#endif
    for( ie=0; ie<dims_b_ne; ++ie )
    for( iu=0; iu<NU; ++iu )
    for( ia=0; ia<dims_b_na; ++ia )
    {
      const int dir_y = Dir_y( octant );
      const int iy = dir_y == DIR_UP ? -1 : dims_b_ncell_y;

      const int ix_g = ix + ix_base; // dims_b_ncell_x * proc_x;
      const int iy_g = iy + iy_base; // dims_b_ncell_y * proc_y;
      const int iz_g = iz + stepinfoall.stepinfo[octant].block_z * dims_b_ncell_z;

      if ((dir_y == DIR_UP && proc_y_min) || (dir_y == DIR_DN && proc_y_max)) {

        /*--- Quantities_scalefactor_space_ inline ---*/
        const int scalefactor_space
          = Quantities_scalefactor_space_acceldir(ix_g, iy_g, iz_g);

        /*--- ref_facexz inline ---*/
        facexz[ia + dims_b_na      * (
               iu + NU           * (
               ie + dims_b_ne      * (
               ix + dims_b_ncell_x * (
               iz + dims_b_ncell_z * (
               octant + NOCTANT * (
               0 )))))) ]

          /*--- Quantities_init_face routine ---*/
          = Quantities_init_face_acceldir(ia, ie, iu, scalefactor_space, octant);

      } /*---if---*/
    } /*---for---*/

  } /*--- #pragma acc parallel ---*/

  /*---FACE YZ---*/
#ifdef USE_OPENMP_TARGET
//#pragma omp target update from(faceyz[0:faceyz_size], stepinfoall)
#elif USE_ACC
  #pragma acc parallel present(faceyz[:faceyz_size], stepinfoall)
#endif
  {

#ifdef USE_OPENMP_TARGET
#pragma omp target teams distribute collapse(3) 
#elif USE_ACC
    #pragma acc loop independent gang collapse(3)
#endif
    for( octant=0; octant<NOCTANT; ++octant )
    for( iz=0; iz<dims_b_ncell_z; ++iz )
    for( iy=0; iy<dims_b_ncell_y; ++iy )
#ifdef USE_OPENMP_TARGET
// review this pragma
#pragma omp parallel for collapse(3)
#elif USE_ACC
    #pragma acc loop independent vector collapse(3)
#endif
    for( ie=0; ie<dims_b_ne; ++ie )
    for( iu=0; iu<NU; ++iu )
    for( ia=0; ia<dims_b_na; ++ia )
    {

      const int dir_x = Dir_x( octant );
      const int ix = dir_x == DIR_UP ? -1 : dims_b_ncell_x;

      const int ix_g = ix + ix_base; // dims_b_ncell_x * proc_x;
      const int iy_g = iy + iy_base; // dims_b_ncell_y * proc_y;
      const int iz_g = iz + stepinfoall.stepinfo[octant].block_z * dims_b_ncell_z;

      if ((dir_x == DIR_UP && proc_x_min) || (dir_x == DIR_DN && proc_x_max)) {

        /*--- Quantities_scalefactor_space_ inline ---*/
        const int scalefactor_space
          = Quantities_scalefactor_space_acceldir(ix_g, iy_g, iz_g);

        /*--- ref_faceyz inline ---*/
        faceyz[ia + dims_b_na      * (
               iu + NU           * (
               ie + dims_b_ne      * (
               iy + dims_b_ncell_y * (
               iz + dims_b_ncell_z * (
               octant + NOCTANT * (
               0 )))))) ]

          /*--- Quantities_init_face routine ---*/
          = Quantities_init_face_acceldir(ia, ie, iu, scalefactor_space, octant);
      } /*---if---*/
    } /*---for---*/

  } /*--- #pragma acc parallel ---*/

#ifdef USE_OPENMP_TARGET
//#pragma omp target update from(a_from_m[0:a_from_m_size], \
                               m_from_a[0:m_from_a_size], \
                               vi[0:v_size], \
                               vo[0:v_size], \
                               facexy[0:facexy_size], \
                               facexz[0:facexz_size], \
                               faceyz[0:faceyz_size], \
                               dims_b, stepinfoall, \
                               vs_local[0:vs_local_size])
#elif USE_ACC
  #pragma acc data \
    present(a_from_m[:a_from_m_size]), \
    present(m_from_a[:m_from_a_size]), \
    present(vi[:v_size]), \
    present(vo[:v_size]), \
    present(facexy[:facexy_size]), \
    present(facexz[:facexz_size]), \
    present(faceyz[:faceyz_size]), \
    present(dims_b), \
    present(stepinfoall), \
    present(vs_local[:vs_local_size])
#endif
  {

  /*---Loop over octants---*/
  for( octant=0; octant<NOCTANT; ++octant )
  {

    if (!stepinfoall.stepinfo[octant].is_active)
      continue;

    /*---Decode octant directions from octant number---*/

    const int dir_x = Dir_x( octant );
    const int dir_y = Dir_y( octant );
    const int dir_z = Dir_z( octant );

    const int v_offset = stepinfoall.stepinfo[octant].block_z * v_b_size;

    const int octant_in_block = octant;

    /*--- KBA sweep ---*/

    /*--- Number of wavefronts equals the sum of the dimension sizes
      minus the number of dimensions minus one. In our case, we have
      three total dimensions, so we add the sizes and subtract 2. 
    ---*/
    const int num_wavefronts = dims_b_ncell_z + dims_b_ncell_y + dims_b_ncell_x - 2;
 
    /*--- Loop over wavefronts ---*/
    for (wavefront = 0; wavefront < num_wavefronts; wavefront++)
    {

    /*--- Create an asynchronous queue for each octant ---*/
#ifdef USE_OPENMP_TARGET
/* FIX: possibly CPU threads here? or CPU tasks? ? parallel nowait ?*/
//#pragma omp target nowait depend(out:octant)
#elif USE_ACC
    #pragma acc parallel async(octant)
#endif
    {

      /*---Loop over cells, in proper direction---*/

#ifdef USE_OPENMP_TARGET
//#pragma omp target teams distribute parallel for collapse(2) // Sweeper_sweep_cell_acceldir has its own target constructs
#elif USE_ACC
      #pragma acc loop independent gang, collapse(2)
#endif
      for( int iywav=0; iywav<dims_b_ncell_y; ++iywav )
      for( int ixwav=0; ixwav<dims_b_ncell_x; ++ixwav )
      {

        const int ix = dir_x==DIR_UP ? ixwav : dims_b_ncell_x - 1 - ixwav;
        const int iy = dir_y==DIR_UP ? iywav : dims_b_ncell_y - 1 - iywav;
        const int izwav = wavefront - ixwav - iywav;
        const int iz = dir_z==DIR_UP ? izwav : (dims_b_ncell_z-1) - izwav;

        const int ix_g = ix + ix_base; // dims_b_ncell_x * proc_x;
        const int iy_g = iy + iy_base; // dims_b_ncell_y * proc_y;
        const int iz_g = iz + stepinfoall.stepinfo[octant].block_z * dims_b_ncell_z;

        /*--- In-gridcell computations ---*/
        Sweeper_sweep_cell_acceldir( dims_b, wavefront, octant, ix, iy,
                                     ix_g, iy_g, iz_g,
                                     dir_x, dir_y, dir_z,
                                     facexy, facexz, faceyz,
                                     a_from_m, m_from_a,
                                     &(vi[v_offset]), &(vo[v_offset]), vs_local,
                                     octant_in_block, noctant_per_block );

      } /*---ix/iy---*/

    } /*--- #pragma acc parallel ---*/

    } /*--- wavefront ---*/

  } /*---octant---*/
 
  }   /*--- #pragma acc data present ---*/

#ifdef USE_OPENMP_TARGET
#pragma omp taskwait
#elif USE_ACC
  #pragma acc wait
#endif



  /*--- Data transfer of results to the host ---*/
  if (is_last_step) {
#ifdef USE_OPENMP_TARGET
#pragma omp target exit data \
  map(from: vo[0:v_size]), \
  map(delete: vi[0:v_size], \
              a_from_m[0:a_from_m_size], \
              m_from_a[0:m_from_a_size], \
              vs_local[0:vs_local_size], \
              facexy[0:facexy_size], \
              facexz[0:facexz_size], \
              faceyz[0:faceyz_size])
#elif USE_ACC
    #pragma acc exit data \
      copyout(vo[:v_size]), \
      delete(vi[:v_size]), \
      delete(a_from_m[:a_from_m_size]), \
      delete(m_from_a[:m_from_a_size]), \
      delete(vs_local[:vs_local_size]), \
      delete(facexy[:facexy_size]), \
      delete(facexz[:facexz_size]), \
      delete(faceyz[:faceyz_size])
#endif
  } else {
#ifdef USE_OPENMP_TARGET
#pragma omp target exit data \
  map(from: facexz[0:facexz_size], \
            faceyz[0:faceyz_size])
#elif USE_ACC
    #pragma acc exit data \
      copyout(facexz[:facexz_size]), \
      copyout(faceyz[:faceyz_size])
#endif
  }
#ifdef USE_OPENMP_TARGET
#pragma omp target exit data map(delete: dims_b, stepinfoall)
#elif USE_ACC
  #pragma acc exit data delete(dims_b)
  #pragma acc exit data delete(stepinfoall)
#endif

} /*---sweep---*/

/*===========================================================================*/

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
  Env*                   env ) {

  Sweeper_sweep_block_acceldir(
    sweeper,
    Pointer_h( vo ),
    Pointer_const_h( vi ),
    is_block_init,
    Pointer_h( facexy ),
    Pointer_h( facexz ),
    Pointer_h( faceyz ),
    Pointer_const_h( a_from_m),
    Pointer_const_h( m_from_a),
    step,
    quan,
    env);
}

/*===========================================================================*/

#ifndef SWEEPER_KBA_ACC
#ifndef SWEEPER_KBA_OPENMP_TARGET

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
  sweeper->nblock_z = 1; //NOTE: will not work efficiently in parallel.
  sweeper->noctant_per_block = NOCTANT;
  sweeper->nblock_octant     = NOCTANT / sweeper->noctant_per_block;

  const int dims_b_ncell_z = dims.ncell_z / sweeper->nblock_z;

  sweeper->dims = dims;
  sweeper->dims_b = sweeper->dims;
  sweeper->dims_b.ncell_z = dims_b_ncell_z;

  Insist( Env_nproc_x(env) == 1 );
  Insist( Env_nproc_y(env) == 1 );

  StepScheduler_create( &(sweeper->stepscheduler),
                              sweeper->nblock_z, sweeper->nblock_octant, env );

  const Bool_t is_face_comm_async = Bool_false;

  Faces_create( &(sweeper->faces), sweeper->dims_b,
                sweeper->noctant_per_block, is_face_comm_async, env );

  /*---Allocate arrays---*/

  sweeper->vslocal_host_
    = malloc_host_P( dims.na * NU * dims.ne * NOCTANT * dims.ncell_x * dims.ncell_y );
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

  free_host_P( sweeper->vslocal_host_ );
  //free_host_P( sweeper->facexy );
  //free_host_P( sweeper->facexz );
  //free_host_P( sweeper->faceyz );

  sweeper->vslocal_host_ = NULL;
  //sweeper->facexy  = NULL;
  //sweeper->facexz  = NULL;
  //sweeper->faceyz  = NULL;

  Faces_destroy( &(sweeper->faces) );
}

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

  const int nstep = StepScheduler_nstep( &(sweeper->stepscheduler) );

  int step = 0;
  for (step = 0; step < nstep; ++step) {

    Sweeper_sweep_block(
      sweeper,
      vo,
      vi,
      is_block_init,
      & sweeper->faces.facexy0,
      & sweeper->faces.facexz0,
      & sweeper->faces.faceyz0,
      & quan->a_from_m,
      & quan->m_from_a,
      step,
      quan,
      env);

  } // step

}

/*===========================================================================*/

#endif /*---SWEEPER_KBA_OPENMP_TARGET---*/
#endif /*---SWEEPER_KBA_ACC---*/

#ifdef __cplusplus
} /*---extern "C"---*/
#endif

#endif /*---_sweeper_gpu_c_h_---*/

/*---------------------------------------------------------------------------*/
