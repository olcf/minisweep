#!/bin/bash -l
#------------------------------------------------------------------------------

# CLEANUP
rm -rf CMakeCache.txt
rm -rf CMakeFiles

# SOURCE AND INSTALL
if [ "$SOURCE" = "" ] ; then
  SOURCE=../
fi
if [ "$INSTALL" = "" ] ; then
  INSTALL=./install
fi

if [ "$BUILD" = "" ] ; then
  #BUILD=Debug
  BUILD=Release
fi

if [ "$NM_VALUE" = "" ] ; then
  NM_VALUE=4
fi

#------------------------------------------------------------------------------

MPI_INCLUDE_DIR=${OLCF_SPECTRUM_MPI_ROOT}/include
MPI_LIB=${OLCF_SPECTRUM_MPI_ROOT}/lib/libmpi_ibm.so

cmake \
  -DCMAKE_BUILD_TYPE:STRING="$BUILD" \
  -DCMAKE_INSTALL_PREFIX:PATH="$INSTALL" \
 \
  -DCMAKE_C_COMPILER:STRING=xlc \
\
  -DCMAKE_C_FLAGS:STRING="-O3 -DNM_VALUE=$NM_VALUE $ALG_OPTIONS -DUSE_OPENMP_TARGET -DUSE_ACCELDIR -DUSE_KBA -I$MPI_INCLUDE_DIR -qsmp=omp -qoffload" \
  -DCMAKE_C_FLAGS_DEBUG:STRING="-g" \
  -DCMAKE_C_FLAGS_RELEASE:STRING="" \
\
  -DUSE_MPI:BOOL=ON \
  -DMPI_C_INCLUDE_PATH:STRING=$MPI_INCLUDE_DIR \
  -DMPI_C_LIBRARIES:STRING=$MPI_LIB \
  -DCMAKE_EXE_LINKER_FLAGS:STRING=$MPI_LIB \
 \
  $SOURCE

#------------------------------------------------------------------------------
