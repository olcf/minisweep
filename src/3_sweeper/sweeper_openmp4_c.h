/*---------------------------------------------------------------------------*/
/*!
 * \file   sweeper_acc_c.h
 * \author Robert Searles, Wayne Joubert
 * \date   Wed Apr 11 9:12:00 EST 2018
 * \brief  Definitions for performing a sweep, OpenACC/KBA version.
 * \note   Copyright (C) 2018 Oak Ridge National Laboratory, UT-Battelle, LLC.
 */
/*---------------------------------------------------------------------------*/

#ifndef _sweeper_openmp4_c_h_
#define _sweeper_openmp4_c_h_

#include "env.h"
#include "definitions.h"
#include "quantities.h"
#include "array_accessors.h"
#include "array_operations.h"
#include "sweeper_openmp4.h"

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
  /* Insist( Env_nproc( env ) == 1 &&  */
  /*                            "This sweeper version runs only with one proc." ); */

  /*---Allocate arrays---*/

  sweeper->vslocal = malloc_host_P( dims.na * NU * dims.ne * NOCTANT * dims.ncell_x * dims.ncell_y );
  sweeper->facexy  = malloc_host_P( dims.ncell_x * dims.ncell_y * dims.ne *
                         dims.na * NU * NOCTANT);
  sweeper->facexz  = malloc_host_P( dims.ncell_x * dims.ncell_z * dims.ne *
                         dims.na * NU * NOCTANT);
  sweeper->faceyz  = malloc_host_P( dims.ncell_y * dims.ncell_z * dims.ne *
                         dims.na * NU * NOCTANT);

  sweeper->dims = dims;
}

/*===========================================================================*/
/*---Pseudo-destructor for Sweeper struct---*/

void Sweeper_destroy( Sweeper* sweeper,
                      Env*     env )
{
  /*---Deallocate arrays---*/

  free_host_P( sweeper->vslocal );
  free_host_P( sweeper->facexy );
  free_host_P( sweeper->facexz );
  free_host_P( sweeper->faceyz );

  sweeper->vslocal = NULL;
  sweeper->facexy  = NULL;
  sweeper->facexz  = NULL;
  sweeper->faceyz  = NULL;
}

/*===========================================================================*/
/*---Quantities_init_face inline routine---*/

#ifdef USE_OPENMP4
#pragma omp declare target
#endif
#ifdef USE_ACC
#pragma acc routine seq
#endif
P Quantities_init_face(int ia, int ie, int iu, int scalefactor_space, int octant)
{
  /*--- Quantities_init_facexy inline ---*/

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
#ifdef USE_OPENMP4
#pragma omp end declare target
#endif

/*===========================================================================*/
/*---Quantities_scalefactor_space_ inline routine---*/

#ifdef USE_OPENMP4
#pragma omp declare target
#endif
#ifdef USE_ACC
#pragma acc routine seq
#endif
int Quantities_scalefactor_space_inline(int ix, int iy, int iz)
{
  /*--- Quantities_scalefactor_space_ inline ---*/
  int scalefactor_space = 0;

#ifndef RELAXED_TESTING
  scalefactor_space = ( (scalefactor_space+(ix+2))*8121 + 28411 ) % 134456;
  scalefactor_space = ( (scalefactor_space+(iy+2))*8121 + 28411 ) % 134456;
  scalefactor_space = ( (scalefactor_space+(iz+2))*8121 + 28411 ) % 134456;
  scalefactor_space = ( (scalefactor_space+(ix+3*iy+7*iz+2))*8121 + 28411 ) % 134456;
  scalefactor_space = ix+3*iy+7*iz+2;
  scalefactor_space = scalefactor_space & ( (1<<2) - 1 );
#endif
  scalefactor_space = 1 << scalefactor_space;

  return scalefactor_space;
}
#ifdef USE_OPENMP4
#pragma omp end declare target
#endif

#ifdef USE_OPENMP4
#pragma omp declare target
#endif
#ifdef USE_ACC
#pragma acc routine seq
#endif
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

  int facexy_size = dims.ncell_x * dims.ncell_y * 
    dims.ne * dims.na * NU * NOCTANT;
  int facexz_size = dims.ncell_x * dims.ncell_z * 
    dims.ne * dims.na * NU * NOCTANT;
  int faceyz_size = dims.ncell_y * dims.ncell_z * 
    dims.ne * dims.na * NU * NOCTANT;
  int v_size = dims.nm * dims.na * NOCTANT;
  int vi_h_size = dims.ncell_x * dims.ncell_y * dims.ncell_z * 
    dims.ne * dims.nm * NU;
  int vo_h_size = dims.ncell_x * dims.ncell_y * dims.ncell_z * 
    dims.ne * dims.nm * NU;
  int vs_local_size = dims.na * NU * dims.ne * NOCTANT * dims.ncell_x * dims.ncell_y;

#ifdef USE_OPENMP4
// no equivalent
#endif
#ifdef USE_ACC
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
#ifdef USE_OPENMP4
#pragma omp end declare target
#endif

/*===========================================================================*/
/*---In-gricell computations---*/
#ifdef USE_OPENMP4
//#pragma omp declare target
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
			     )
{
  /*---Declarations---*/
  int iz = 0;
  int ie = 0;
  int im = 0;
  int ia = 0;
  int iu = 0;
  /* int octant = 0; */
  int noctant_per_block = 1;
  const int octant_in_block = 0;

  /*--- Dimensions ---*/
  int dim_x = dims.ncell_x;
  int dim_y = dims.ncell_y;
  int dim_z = dims.ncell_z;
  int dim_ne = dims.ne;
  int dim_na = dims.na;
  int dim_nm = dims.nm;
  int facexy_size = dims.ncell_x * dims.ncell_y * 
    dims.ne * dims.na * NU * NOCTANT;
  int facexz_size = dims.ncell_x * dims.ncell_z * 
    dims.ne * dims.na * NU * NOCTANT;
  int faceyz_size = dims.ncell_y * dims.ncell_z * 
    dims.ne * dims.na * NU * NOCTANT;
  int v_size = dims.nm * dims.na * NOCTANT;
  int vi_h_size = dims.ncell_x * dims.ncell_y * dims.ncell_z * 
    dims.ne * dims.nm * NU;
  int vo_h_size = dims.ncell_x * dims.ncell_y * dims.ncell_z * 
    dims.ne * dims.nm * NU;
  int vs_local_size = dims.na * NU * dims.ne * NOCTANT * dims.ncell_x * dims.ncell_y;

  /*--- Solve for Z dimension, and check bounds.
    The sum of the dimensions should equal the wavefront number.
    If z < 0 or z > wavefront number, we are out of bounds.
    Z also shouldn't exceed the spacial bound for the z dimension.

    The calculation is adjusted for the direction of each axis
    in a given octant.
  ---*/

  int ixwav, iywav, izwav;
  if (dir_x==DIR_UP) { ixwav = ix; } else { ixwav = (dim_x-1) - ix; }
  if (dir_y==DIR_UP) { iywav = iy; } else { iywav = (dim_y-1) - iy; }
  
  if (dir_z==DIR_UP) {
    iz = wavefront - (ixwav + iywav); } 
  else { 
    iz = (dim_z-1) - (wavefront - (ixwav + iywav));
  }

  /*--- Bounds check ---*/
  if ((iz >= 0 && iz < dim_z) ) /* && */
    /* ((dir_z==DIR_UP && iz <= wavefront) || */
    /*  (dir_z==DIR_DN && (dim_z-1-iz) <= wavefront))) */
    {

   /*---Loop over energy groups---*/
#ifdef USE_OPENMP4
//#pragma omp parallel for collapse(3)
//#pragma omp target teams distribute parallel for collapse(3)
#pragma omp target teams distribute parallel for simd collapse(3)
#endif
#ifdef USE_ACC
#pragma acc loop independent vector, collapse(3)
#endif
/* FIX: come back to this */
      for( ie=0; ie<dim_ne; ++ie )
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
      for( ia=0; ia<dim_na; ++ia )
      { 
	/* reset reduction */
	P result = (P)0;
#ifdef USE_OPENMP4
// no equivalent
#endif
#ifdef USE_ACC
#pragma acc loop seq
#endif
        for( im=0; im < dim_nm; ++im )
        {
	  /*--- const_ref_a_from_m inline ---*/
	  result += v_a_from_m[ ia     + dims.na * (
         	  	        im     +      NM * (
	  	                octant + NOCTANT * (
						    0 ))) ] * 

	    /*--- const_ref_state inline ---*/
	    vi_h[im + dims.nm      * (
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
#ifdef USE_OPENMP4
//#pragma omp parallel for collapse(2)
#pragma omp target teams distribute parallel for simd collapse(2)
#endif
#ifdef USE_ACC
#pragma acc loop independent vector, collapse(2)
#endif
      for( ie=0; ie<dim_ne; ++ie )
      for( ia=0; ia<dim_na; ++ia )
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
#ifdef USE_OPENMP4
#pragma omp target teams distribute parallel for collapse(3)
//#pragma omp parallel for collapse(3)
#endif
#ifdef USE_ACC
#pragma acc loop independent vector, collapse(3)
#endif
      for( ie=0; ie<dim_ne; ++ie )
      {

      for( iu=0; iu<NU; ++iu )
      for( im=0; im<dim_nm; ++im )
      {
        P result = (P)0;
#ifdef USE_OPENMP4
// no equivalent
#endif
#ifdef USE_ACC
#pragma acc loop seq
#endif
        for( ia=0; ia<dim_na; ++ia )
        {
	  /*--- const_ref_m_from_a ---*/
	  result += v_m_from_a[ im     +      NM * (
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
/* FIX: use omp critical ??*/
#ifdef USE_OPENMP4
#pragma omp atomic update
#endif
#ifdef USE_ACC
#pragma acc atomic update
#endif
	vo_h[im + dims.nm      * (
             iu + NU           * (
             ix + dims.ncell_x * (
             iy + dims.ncell_y * (
             ie + dims.ne      * (
             iz + dims.ncell_z * ( /*---NOTE: This axis MUST be slowest-varying---*/
             0 ))))))] += result;
      }
      } /*---ie---*/
//#pragma omp target update from(vo_h[0:vo_h_size])
	} /*--- iz ---*/
} /*--- end of Sweeper_in_gridcell */
#ifdef USE_OPENMP4
//#pragma omp end declare target
#endif

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

  /*--- Set OpenACC device based on MPI rank ---*/
#ifdef USE_MPI
#ifdef USE_OPENMP4
  int num_devices = omp_get_num_devices();
  int mpi_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  int device_num = mpi_rank % num_devices;
  omp_set_default_device(device_num);
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
  int noctant_per_block = 1;

  /*--- Dimensions ---*/
  Dimensions dims = sweeper->dims;
  int dim_x = dims.ncell_x;
  int dim_y = dims.ncell_y;
  int dim_z = dims.ncell_z;
  int dim_ne = dims.ne;
  int dim_na = dims.na;
  int dim_nm = dims.nm;

  /*--- Array Pointers ---*/
  P* __restrict__ facexy = sweeper->facexy;
  P* __restrict__ facexz = sweeper->facexz;
  P* __restrict__ faceyz = sweeper->faceyz;
  P* v_a_from_m = (P*) Pointer_const_h( & quan->a_from_m);
  P* v_m_from_a = (P*) Pointer_const_h( & quan->m_from_a);
  P* vi_h = Pointer_h( vi );
  P* vo_h = Pointer_h( vo );
  P* vs_local = sweeper->vslocal;

  /*--- Array Sizes ---*/
  int facexy_size = dims.ncell_x * dims.ncell_y * 
    dims.ne * dims.na * NU * NOCTANT;
  int facexz_size = dims.ncell_x * dims.ncell_z * 
    dims.ne * dims.na * NU * NOCTANT;
  int faceyz_size = dims.ncell_y * dims.ncell_z * 
    dims.ne * dims.na * NU * NOCTANT;
  int v_size = dims.nm * dims.na * NOCTANT;
  int vi_h_size = dims.ncell_x * dims.ncell_y * dims.ncell_z * 
    dims.ne * dims.nm * NU;
  int vo_h_size = dims.ncell_x * dims.ncell_y * dims.ncell_z * 
    dims.ne * dims.nm * NU;
  int vs_local_size = dims.na * NU * dims.ne * NOCTANT * dims.ncell_x * dims.ncell_y;

  /*---Initialize result array to zero---*/

  initialize_state_zero( Pointer_h( vo ), dims, NU );

  /*--- Data transfer to the GPU ---*/
#ifdef USE_OPENMP4
#pragma omp target enter data \
  map(to: v_a_from_m[:v_size], \
          v_m_from_a[:v_size], \
          vi_h[:vi_h_size], \
          vo_h[:vo_h_size], \
          dims), \
  map(alloc: facexy[:facexy_size], \
             facexz[:facexz_size], \
             faceyz[:faceyz_size])
#endif
#ifdef USE_ACC
#pragma acc enter data copyin(v_a_from_m[:v_size], \
			      v_m_from_a[:v_size], \
			      vi_h[:vi_h_size], \
			      vo_h[:vo_h_size], \
                              dims),	\
  create(facexy[:facexy_size], \
	 facexz[:facexz_size], \
	 faceyz[:faceyz_size])
#endif

  const int octant_in_block = 0;

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

#ifdef USE_OPENMP4
/*#pragma omp target map(to:facexy[:facexy_size])*/
#endif
#ifdef USE_ACC
#pragma acc parallel present(facexy[:facexy_size])
#endif
{

/* FIX: maybe collapse(6) */
#ifdef USE_OPENMP4
#pragma omp target teams distribute collapse(3)
#endif
#ifdef USE_ACC
#pragma acc loop independent gang collapse(3)
#endif
      for( octant=0; octant<NOCTANT; ++octant )
      for( iy=0; iy<dim_y; ++iy )
      for( ix=0; ix<dim_x; ++ix )

#ifdef USE_OPENMP4
#pragma omp parallel for collapse(3)
#endif
#ifdef USE_ACC
#pragma acc loop independent vector collapse(3)
#endif
      for( ie=0; ie<dim_ne; ++ie )
      for( iu=0; iu<NU; ++iu )
      for( ia=0; ia<dim_na; ++ia )
      {

	const int dir_z = Dir_z( octant );
	iz = dir_z == DIR_UP ? -1 : dim_z;

	/*--- Quantities_scalefactor_space_ inline ---*/
	int scalefactor_space = Quantities_scalefactor_space_inline(ix, iy, iz);

	/*--- ref_facexy inline ---*/
	facexy[ia + dims.na      * (
			iu + NU           * (
                        ie + dims.ne      * (
                        ix + dims.ncell_x * (
                        iy + dims.ncell_y * (
	                octant + NOCTANT * (
					     0 )))))) ]

	  /*--- Quantities_init_face routine ---*/
	  = Quantities_init_face(ia, ie, iu, scalefactor_space, octant);
      }

/*--- #pragma acc parallel ---*/
 }

#ifdef USE_OPENMP4
//#pragma omp target update from(facexy[0:facexy_size])
#endif
#ifdef USE_ACC
#pragma acc parallel present(facexz[:facexz_size])
#endif
{
 
#ifdef USE_OPENMP4
#pragma omp target teams distribute collapse(3)
#endif
#ifdef USE_ACC
#pragma acc loop independent gang collapse(3)
#endif
      for( octant=0; octant<NOCTANT; ++octant )
      for( iz=0; iz<dim_z; ++iz )
      for( ix=0; ix<dim_x; ++ix )
#ifdef USE_OPENMP4
#pragma omp parallel for collapse(3)
#endif
#ifdef USE_ACC
#pragma acc loop independent vector collapse(3)
#endif
      for( ie=0; ie<dim_ne; ++ie )
      for( iu=0; iu<NU; ++iu )
      for( ia=0; ia<dim_na; ++ia )
      {

	const int dir_y = Dir_y( octant );
	iy = dir_y == DIR_UP ? -1 : dim_y;

	/*--- Quantities_scalefactor_space_ inline ---*/
	int scalefactor_space = Quantities_scalefactor_space_inline(ix, iy, iz);

	/*--- ref_facexz inline ---*/
	facexz[ia + dims.na      * (
			iu + NU           * (
                        ie + dims.ne      * (
                        ix + dims.ncell_x * (
                        iz + dims.ncell_z * (
	                octant + NOCTANT * (
					     0 )))))) ]

	  /*--- Quantities_init_face routine ---*/
	  = Quantities_init_face(ia, ie, iu, scalefactor_space, octant);
      }

/*--- #pragma acc parallel ---*/
 }

#ifdef USE_OPENMP4
//#pragma omp target update from(facexz[0:facexz_size])
#endif
#ifdef USE_ACC
#pragma acc parallel present(faceyz[:faceyz_size])
#endif
{

#ifdef USE_OPENMP4
#pragma omp target teams distribute collapse(3)
#endif
#ifdef USE_ACC
#pragma acc loop independent gang collapse(3)
#endif
      for( octant=0; octant<NOCTANT; ++octant )
      for( iz=0; iz<dim_z; ++iz )
      for( iy=0; iy<dim_y; ++iy )
#ifdef USE_OPENMP4
#pragma omp parallel for collapse(3)
#endif
#ifdef USE_ACC
#pragma acc loop independent vector collapse(3)
#endif
      for( ie=0; ie<dim_ne; ++ie )
      for( iu=0; iu<NU; ++iu )
      for( ia=0; ia<dim_na; ++ia )
      {

	const int dir_x = Dir_x( octant );
	ix = dir_x == DIR_UP ? -1 : dim_x;

	/*--- Quantities_scalefactor_space_ inline ---*/
	int scalefactor_space = Quantities_scalefactor_space_inline(ix, iy, iz);

	/*--- ref_faceyz inline ---*/
	faceyz[ia + dims.na      * (
			iu + NU           * (
                        ie + dims.ne      * (
                        iy + dims.ncell_y * (
                        iz + dims.ncell_z * (
	                octant + NOCTANT * (
					     0 )))))) ]

	  /*--- Quantities_init_face routine ---*/
	  = Quantities_init_face(ia, ie, iu, scalefactor_space, octant);
      }

/*--- #pragma acc parallel ---*/
 }
     
#ifdef USE_OPENMP4
#pragma omp target enter data map(alloc: vs_local[:vs_local_size])
#endif
#ifdef USE_ACC
#pragma acc data present(v_a_from_m[:v_size], \
			     v_m_from_a[:v_size],   \
			     vi_h[:vi_h_size],	    \
			     vo_h[:vo_h_size], \
                             facexy[:facexy_size],				    \
	                     facexz[:facexz_size],			    \
	                     faceyz[:faceyz_size], \
			     dims),			   \
  create(vs_local[vs_local_size])
#endif
 {

/* FIX: do we need an omp parallel for nowait here with 8 CPU threads ??*/
/*---Loop over octants---*/
for( octant=0; octant<NOCTANT; ++octant )
  {

   /*---Decode octant directions from octant number---*/

   const int dir_x = Dir_x( octant );
   const int dir_y = Dir_y( octant );
   const int dir_z = Dir_z( octant );

   /*--- KBA sweep ---*/

   /*--- Number of wavefronts equals the sum of the dimension sizes
     minus the number of dimensions minus one. In our case, we have
     three total dimensions, so we add the sizes and subtract 2. 
   ---*/
   int num_wavefronts = (dims.ncell_z + dims.ncell_y + dims.ncell_x) - 2;
 
   /*--- Loop over wavefronts ---*/
   for (wavefront = 0; wavefront < num_wavefronts; wavefront++)
     {

/*--- Create an asynchronous queue for each octant ---*/
#ifdef USE_OPENMP4

/* FIX: possibly CPU threads here? or CPU tasks? ? parallel nowait ?*/

/*#pragma omp target nowait depend(out:octant)*/
#endif
#ifdef USE_ACC
#pragma acc parallel async(octant)
#endif
     {

//====================
#if 1
//#ifdef REVISION_OF_ROBBIE_OSCAR_CODE
//====================

#ifdef USE_OPENMP4
//#pragma omp target teams distribute parallel for collapse(2)
#endif
#ifdef USE_ACC
#pragma acc loop independent gang, collapse(2)
#endif
	 for( int iy_updown=0; iy_updown<dim_y; ++iy_updown )
	   for( int ix_updown=0; ix_updown<dim_x; ++ix_updown )
	     {

               const int iy = dir_y==DIR_UP ? iy_updown : dim_y - 1 - iy_updown;
               const int ix = dir_x==DIR_UP ? ix_updown : dim_x - 1 - ix_updown;

	       /*--- In-gridcell computations ---*/
	       Sweeper_in_gridcell( dims, wavefront, octant, ix, iy,
				    dir_x, dir_y, dir_z,
				    facexy, facexz, faceyz,
				    v_a_from_m, v_m_from_a,
				    vi_h, vo_h, vs_local );
	     } /*---ix/iy---*/

#pragma omp target update from(vo_h[0:vo_h_size])
//====================
#else // original Robbie / Oscar code
//====================

       /*---Loop over cells, in proper direction---*/
       if (dir_y==DIR_UP && dir_x==DIR_UP) {
#ifdef USE_OPENMP4
#pragma omp target update from(vo_h[0:vo_h_size],vs_local[0:vs_local_size])
#pragma omp target teams distribute parallel for collapse(2) 
#endif
#ifdef USE_ACC
#pragma acc loop independent gang, collapse(2)
#endif
	 for( iy=0; iy<dim_y; ++iy )
	   for( ix=0; ix<dim_x; ++ix )
	     {
	       /*--- In-gridcell computations ---*/
	       Sweeper_in_gridcell( dims, wavefront, octant, ix, iy,
				    dir_x, dir_y, dir_z,
				    facexy, facexz, faceyz,
				    v_a_from_m, v_m_from_a,
				    vi_h, vo_h, vs_local );
	     } /*---ix/iy---*/
       } else if (dir_y==DIR_UP && dir_x==DIR_DN) {
#ifdef USE_OPENMP4
#pragma omp target teams distribute parallel for collapse(2)
#endif
#ifdef USE_ACC
#pragma acc loop independent gang, collapse(2)
#endif
	 for( iy=0; iy<dim_y; ++iy )
	   for( ix=dim_x-1; ix>=0; --ix )
	     {
	       /*--- In-gridcell computations ---*/
	       Sweeper_in_gridcell( dims, wavefront, octant, ix, iy,
				    dir_x, dir_y, dir_z,
				    facexy, facexz, faceyz,
				    v_a_from_m, v_m_from_a,
				    vi_h, vo_h, vs_local );	 
	     } /*---ix/iy---*/
       } else if (dir_y==DIR_DN && dir_x==DIR_UP) {
#ifdef USE_OPENMP4
#pragma omp target teams distribute parallel for collapse(2)
#endif
#ifdef USE_ACC
#pragma acc loop independent gang, collapse(2)
#endif
	 for( iy=dim_y-1; iy>=0; --iy )
	   for( ix=0; ix<dim_x; ++ix )
	     {
	       /*--- In-gridcell computations ---*/
	       Sweeper_in_gridcell( dims, wavefront, octant, ix, iy,
				    dir_x, dir_y, dir_z,
				    facexy, facexz, faceyz,
				    v_a_from_m, v_m_from_a,
				    vi_h, vo_h, vs_local );	 
	     } /*---ix/iy---*/
       } else {
#ifdef USE_OPENMP4
#pragma omp target teams distribute parallel for collapse(2)
#endif
#ifdef USE_ACC
#pragma acc loop independent gang, collapse(2)
#endif
	 for( iy=dim_y-1; iy>=0; --iy )
	   for( ix=dim_x-1; ix>=0; --ix )
	     {
	       /*--- In-gridcell computations ---*/
	       Sweeper_in_gridcell( dims, wavefront, octant, ix, iy,
				    dir_x, dir_y, dir_z,
				    facexy, facexz, faceyz,
				    v_a_from_m, v_m_from_a,
				    vi_h, vo_h, vs_local );	 
	     } /*---ix/iy---*/
       } // if

//====================
#endif // Robbie / Oscar code
//====================

     } /*--- #pragma acc parallel ---*/

     } /*--- wavefront ---*/

  } /*---octant---*/
 
 }   /*--- #pragma acc enter data ---*/

#ifdef USE_OPENMP4
#pragma omp taskwait
#endif

#ifdef USE_ACC
#pragma acc wait
#endif

   
  /*--- Data transfer of results to the host ---*/
#ifdef USE_OPENMP4
#pragma omp target exit data \
  map(from: vo_h[0:vo_h_size]), \
  map(delete: v_a_from_m[0:v_size], \
              v_m_from_a[0:v_size], \
              vi_h[0:vi_h_size], \
              facexy[0:facexy_size], \
              facexz[0:facexz_size], \
              faceyz[0:faceyz_size], \
              vs_local[0:vs_local_size])
#endif
#ifdef USE_ACC
#pragma acc exit data copyout(vo_h[:vo_h_size]), delete(v_a_from_m[:v_size], \
                                                        v_m_from_a[:v_size], \
			                                vi_h[:vi_h_size], \
                                                        facexy[:facexy_size], \
	                                                facexz[:facexz_size], \
	                                                faceyz[:faceyz_size], \
							vs_local[:vs_local_size])
#endif
			      

} /*---sweep---*/

/*===========================================================================*/

#ifdef __cplusplus
} /*---extern "C"---*/
#endif

#endif /*---_sweeper_openmp4_c_h_---*/

/*---------------------------------------------------------------------------*/
