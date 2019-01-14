# Flow123d &middot; [![Jenkins release](https://img.shields.io/jenkins/s/http/ciflow.nti.tul.cz:8080/Flow123d-ci2runner-release-multijob.svg?style=flat-square&label=release)](http://ciflow.nti.tul.cz:8080/view/multijob-list/job/Flow123d-ci2runner-release-multijob/) [![Jenkins debug](https://img.shields.io/jenkins/s/http/ciflow.nti.tul.cz:8080/Flow123d-ci2runner-debug-multijob.svg?style=flat-square&label=debug)](http://ciflow.nti.tul.cz:8080/view/multijob-list/job/Flow123d-ci2runner-debug-multijob/) [![Coveralls master](https://img.shields.io/coveralls/github/flow123d/flow123d.svg?style=flat-square&label=coverage)](https://coveralls.io/github/flow123d/flow123d) [![Docker hub](https://img.shields.io/badge/docker-hub-blue.svg?colorA=2271b8&colorB=dc750d&logo=docker&style=flat-square)](https://hub.docker.com/u/flow123d/) [![CI-HPC](https://img.shields.io/badge/ci--hpc-performace-green.svg?style=flat-square&logo=google%20analytics&logoColor=white)](http://flowdb.nti.tul.cz/ci-hpc/)

Flow123d is a simulator of underground water flow and transport processes in fractured
porous media. Novelty of this software is support of computations on complex
meshes consisting of simplicial elements of different dimensions. Therefore
we can combine continuum models and discrete fracture network models.
For more information see the project pages:
[flow123d.github.com](http://flow123d.github.com).


[![Jenkins release](https://img.shields.io/jenkins/s/http/ciflow.nti.tul.cz:8080/Flow123d-ci2runner-release-multijob.svg?style=flat-square&label=Jenkins-release)](http://ciflow.nti.tul.cz:8080/view/multijob-list/job/Flow123d-ci2runner-release-multijob/)

[![Jenkins debug](https://img.shields.io/jenkins/s/http/ciflow.nti.tul.cz:8080/Flow123d-ci2runner-debug-multijob.svg?style=flat-square&label=Jenkins-debug)](http://ciflow.nti.tul.cz:8080/view/multijob-list/job/Flow123d-ci2runner-debug-multijob/)

## Docker

Link to docker images: https://hub.docker.com/u/flow123d/

## License ##

The source code of Flow123d is licensed under GPL3 license. For details look
at files doc/LICENSE and doc/GPL3.

## Build Flow123d ##

The sources of released versions can be downloaded from the project [web page](http://flow123d.github.com). The sources of
development branches can be obtained by 

    > git clone https://github.com/flow123d/flow123d.git
    
Flow123d now uses [Docker](https://www.docker.com/) tool. Docker containers wrap
a piece of software in a complete filesystem that contains everything needed to run: code, runtime, system tools,
system libraries – anything that can be installed on a server.

### Windows OS prerequisities ###

If you are running Windows, you have to install [Docker Toolbox](https://www.docker.com/products/docker-toolbox). If Docker Toolbox is not installed it can be automatically installed
by Windows Flow123d installator.

On Windows system, you have to:

* install Docker Toolbox
* make sure powershell is in the system path


### Linux OS prerequisities ###

On Linux system, you have to install:

* docker
* make, cmake

## Linux OS prerequisities without docker ##

**This approach is not recommended but it is still possible**

It is also possible to use Flow123d without docker. This however requires
much greater effort and complex prerequisities:

Requested packages are: 

* gcc, g++                        C/C++ compiler in version 4.7 and higher (we use C++11)
* gfortran                        Fortran compiler for compilation of BLAS library for PETSc.
* python, perl                    Scripting languages 
* make, cmake (>2.8.8), patch       Building tools
* libboost                        General purpose C++ library 
* libboost-program-options-dev 
* libboost-serialize-dev
* libboost-regex-dev
* libboost-filesystem-dev 

Requested libraries are:
* Yamlcpp library 
* Armadillo
* PETSc
* BDDCML and Blopex

these libraries can be installed using cmake projects in our other
 [repository](https://github.com/flow123d/docker-config/tree/master/cmakefiles) 
 but you must edit config.cmake properly so the libraries are found

 Optionally you may need:

* doxygen, graphviz     for source generated documentation and its dependency diagrams 
* texlive-latex         or other Latex package to build reference manual, we use also some extra packages:
                        on RedHat type distributions you may need texlive-cooltooltips, on Debian/Ubuntu 
			texlive-latex-extra
* imagemagick		tool is used to generate some graphics for the reference manual	
* Python 2.7 and python libraries - markdown, json for HTML manual

### Compile Flow123d ###

To compile Flow123d use docker image `flow-libs-dev-rel` for *release* or
`flow-libs-dev-dbg` for *debug* version. Images can be found [here](https://github.com/flow123d/docker-config/tree/master/dockerfiles).

In order ro compile Flow123d:

1) Clone [docker-config](https://github.com/flow123d/docker-config) repository:
```sh
 $ git clone https://github.com/flow123d/docker-config.git
```     

2) Install Docker images (this may take a while):
```sh
$ cd docker-config/bin
$ ./configure
```
3) Enter docker image:
```sh
$ docker run -ti --rm -u $(id -u):$(id -g) flow-libs-dev-rel:user
```

4) Clone Flow123d repository and copy config file
```sh
$ git clone https://github.com/flow123d/flow123d.git
$ cd flow123d
$ cp config/config-jenkins-docker-release.cmake config.cmake
```

5) Run compilation:
```sh
$ make -j 2 all
```

This runs configuration and then the build process. If the configuration
fails and you need to correct your `config.cmake` or other system setting
you have to cleanup all files generated by unsuccessful cmake configuration by:
```sh
$ make clean-all
```

Try this every time if your build doesn't work and you don't know why.

** Parallel builds ** 

Build can take quite long time when running on single processor. Use make's parameter '-j N' to 
use N parallel processes. Developers can set appropriate alias in '.bashrc', e.g. :
```sh
export NUMCPUS=`grep -c '^processor' /proc/cpuinfo`
alias pmake='time nice make -j$NUMCPUS --load-average=$NUMCPUS'
```


### Manual PETSc installation (optional) ###

Flow versions 1.8.x depends on the PETSC library 3.4.x. It is installed automatically 
if you do not set an existing installation in `config.cmake` file. However the manual installation 
is necessary if
- you want to switch between more configurations (debugging, release, ...) using `PETSC_ARCH` variable.
- you want to achieve best performance configuring with system-wide MPI and/or BLAS and LAPACK libraries.


1.  download PETSc 3.4.x from:
    http://www.mcs.anl.gov/petsc/petsc-as/documentation/installation.html

2.  unpack to any working directory 
    and go to it, eg. :
 
        > cd /home/jb/local/petsc

3.  Set variables:

        > export PETSC_DIR=`pwd`

    For development you will need at least debugging build of the
    library. Set the name of configuration, eg. :

        > export PETSC_ARCH=linux-gcc-dbg

4. Run the configuration script, for example with following options:

        > ./config/configure.py --with-debugging=1 --CFLAGS-O=-g --FFLAGS-O=-g \
          --download-mpich=yes --download-metis=yes --download-parmetis --download-f-blas-lapack=1

    This also force cofigurator to install BLAS, Lapack, MPICH, and ParMetis so it takes
    a while (it could take about 15 min). If everything is OK, you obtain table with
    used compilers and libraries. 
    
5.  Finally compile PETSC with this configuration:

        > make all

    To test the compilation run:

        > make test

    To obtain PETSC configuration for the production version you can use e.g.

    ```
    > export PETSC_ARCH=linux-gcc-dbg
    > ./config/configure.py --with-debugging=0 --CFLAGS-O=-O3 --FFLAGS-O=-O3 \
       --download-mpich=yes --download-metis=yes --download-f-blas-lapack=1
    > make all
    > make test
    ```

### Notes: ###
* You can have several PETSC configuration using different `PETSC_ARCH` value.
* For some reasons if you let PETSc to download and install its own MPICH it
  overrides your optimization flags for compiler. Workaround is to edit
  file `${PETSC_DIR}/${PETSC_ARCH}/conf/petscvariables` and modify variables
  `<complier>_FLAGS_O` back to the values you wish.
* PETSc configuration should use system wide MPI implementation for efficient parallel computations.
* You have to compile PETSc at least with Metis support.

### Support for other libraries ##
PETSc supports lot of other software packages. Use `configure.py --help` for the full list. Here we
highlight the packages we are familiar with.

* **MKL** is implementation of BLAS and LAPACK libraries provided by Intel for their processors. 
  Usually gives the best performance. Natively supported by PETSc.

* **ATLAS library** PETSC use BLAS and few LAPACK functions for its local vector and matrix
  operations. The speed of BLAS and LAPACK have dramatic impact on the overall
  performance. There is a sophisticated implementation of BLAS called ATLAS.
  ATLAS performs extensive set of performance tests on your hardware then make
  an optimized implementation of  BLAS code for you. According to our
  measurements the Flow123d is about two times faster with ATLAS compared to
  usual --download-f-blas-lapack (on x86 architecture and usin GCC).
   
  In order to use ATLAS, download it from ... and follow their instructions.
  The key point is that you have to turn off the CPU throttling. To this end
  install 'cpufreq-set' or `cpu-freq-selector` and use it to set your processor
  to maximal performance:
  
    cpufreq-set -c 0 -g performance
    cpufreq-set -c 1 -g performance

   ... this way I have set performance mode for both cores of my Core2Duo.

   Then you need not to specify any special options, just run default configuration and make. 
   
   Unfortunately, there is one experimental preconditioner in PETSC (PCASA) which use a QR decomposition Lapack function, that is not
   part of ATLAS. Although it is possible to combine ATLAS with full LAPACK from Netlib, we rather provide an empty QR decomposition function
   as a part of Flow123d sources.
   See. HAVE_ATTLAS_ONLY_LAPACK in ./makefile.in

* PETSC provides interface to many useful packages. You can install them 
  adding further configure options:

  --download-superlu=yes         # parallel direct solver
  --download-hypre=yes           # Boomer algebraic multigrid preconditioner, many preconditioners
  --download-spools=yes          # parallel direc solver
  --download-blacs=ifneeded      # needed by MUMPS
  --download-scalapack=ifneeded  # needed by MUMPS
  --download-mumps=yes           # parallel direct solver
  --download-umfpack=yes         # MATLAB solver

  For further information about use of these packages see:

  http://www.mcs.anl.gov/petsc/petsc-2/documentation/linearsolvertable.html

  http://www.mcs.anl.gov/petsc/petsc-as/snapshots/petsc-current/docs/manualpages/PC/PCFactorSetMatSolverPackage.html#PCFactorSetMatSolverPackage
  http://www.mcs.anl.gov/petsc/petsc-as/snapshots/petsc-current/docs/manualpages/Mat/MAT_SOLVER_SPOOLES.html#MAT_SOLVER_SPOOLES
  http://www.mcs.anl.gov/petsc/petsc-as/snapshots/petsc-current/docs/manualpages/Mat/MAT_SOLVER_MUMPS.html#MAT_SOLVER_MUMPS
  http://www.mcs.anl.gov/petsc/petsc-as/snapshots/petsc-current/docs/manualpages/Mat/MAT_SOLVER_SUPERLU_DIST.html
  http://www.mcs.anl.gov/petsc/petsc-as/snapshots/petsc-current/docs/manualpages/Mat/MAT_SOLVER_UMFPACK.html

  http://www.mcs.anl.gov/petsc/petsc-as/snapshots/petsc-dev/docs/manualpages/PC/PCHYPRE.html

## Build the reference manual ##

The reference manual can be built by while in docker container
```sh
$ make ref-doc
```
To copy out reference manual from docker use command `docker cp`.


## Troubleshooting ##

* **Petsc Detection Problem:**

  CMake try to detect type of your PETSc installation and then test it
  by compiling and running simple program. However, this can fail if the 
  program has to be started under 'mpiexec'. In such a case, please, set:

    > set (PETSC_EXECUTABLE_RUNS YES)

  in your makefile.in.cmake file, and perform: `make clean-all; make all`

  For further information about program usage see reference manual `doc/flow_doc`.


* ** Shared libraries ** By default PETSC will create dynamically linked libraries, which can be shared be more applications. But on some systems 
  (in particular we have problems under Windows) this doesn't work, so one is forced to turn off dynamic linking by:

    --with-shared=0

* ** No MPI ** If you want only serial version of PETSc (and Flow123d)
  add --with-mpi=0 to the configure command line.


* ** Windows line ends**  If you use a shell script for PETSC configuration under cygwin,
  always check if you use UNIX line ends. It can be specified in the notepad
  of Windows 7.

  or other errors usually related to DLL conflicts.
  (see http://www.tishler.net/jason/software/rebase/rebase-2.4.2.README)
  To fix DLL libraries you should perform:

 -# shutdown all Cygwin processes and services
 -# start `ash` (do not use bash or rxvt)
 -# execute `/usr/bin/rebaseall` (in the ash window)

  Possible problem with 'rebase':
    /usr/lib/cygicudata.dll: skipped because nonexist
    .
    .
    .
    FixImage (/usr/x86_64-w64-mingw32/sys-root/mingw/bin/libgcc_s_sjlj-1.dll) failed with last error = 13

   Solution (ATTENTION, depends on Cygwin version): 
    add following to line 110 in the rebase script:
    -e '/\/sys-root\/mingw\/bin/d'



