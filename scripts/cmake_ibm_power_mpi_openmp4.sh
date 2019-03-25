#!/bin/bash -l
#------------------------------------------------------------------------------

# CLEANUP
rm -rf CMakeCache.txt
rm -rf CMakeFiles

if [ "$COMPILER" = "xl" ] ; then
  module load xl
  #CC="xlc_r"
  #CXX="xlC_r"
  CC="mpicc"
  CXX="mpicxx"
  OMP_ARGS="-qsmp -qoffload"
  OPT_ARGS=""
elif [ "$COMPILER" = "pgi" ] ; then
  module load pgi
  CC="pgcc"
  CXX="pgc++"
  OMP_ARGS="-mp"
  OPT_ARGS=""
elif [ "$COMPILER" = "clang" ] ; then
  module load clang
  module load cuda
  CC="clang"
  CXX="clang++"
  OMP_ARGS="-fopenmp -I$CLANG_OMP_INCLUDE -L$CLANG_OMP_LIB"
  OPT_ARGS=""
else
  CC="gcc"
  CXX="g++"
  OMP_ARGS="-fopenmp"
  OPT_ARGS="-O3 -fomit-frame-pointer -funroll-loops -finline-limit=10000000"
fi

if [ "$NO_OPENMP" != "" ] ; then
  OMP_ARGS=""
else
  true
  #OMP_ARGS="$OMP_ARGS -DUSE_OPENMP -DUSE_OPENMP_THREADS"
fi


# SOURCE AND INSTALL
if [ "$SOURCE" = "" ] ; then
  SOURCE=../
fi
if [ "$INSTALL" = "" ] ; then
  INSTALL=../install
fi

if [ "$BUILD" = "" ] ; then
  BUILD=Debug
  #BUILD=Release
fi

if [ "$NM_VALUE" = "" ] ; then
  NM_VALUE=4
fi


#------------------------------------------------------------------------------

cmake \
  -DCMAKE_BUILD_TYPE:STRING="$BUILD" \
  -DCMAKE_INSTALL_PREFIX:PATH="$INSTALL" \
 \
  -DCMAKE_C_COMPILER:STRING="$CC" \
  -DCMAKE_CXX_COMPILER:STRING="$CXX" \
  -DCMAKE_C_FLAGS:STRING="-DNM_VALUE=$NM_VALUE -I$MPI_INCLUDE_DIR $OMP_ARGS" \
  -DCMAKE_C_FLAGS_DEBUG:STRING="-g" \
  -DCMAKE_C_FLAGS_RELEASE:STRING="$OPT_ARGS" \
 \
  -DUSE_MPI:BOOL=ON \
  -DUSE_OPENMP4:BOOL=ON \
 \
  -DCMAKE_EXE_LINKER_FLAGS:STRING=$MPI_LIB \
 \
  $SOURCE

#------------------------------------------------------------------------------

#  -DMPI_C_COMPILER:STRING=xlc_r \

