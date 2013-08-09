#!/bin/bash
# 
# Copyright (C) 2007 Technical University of Liberec.  All rights reserved.
#
# Please make a following refer to Flow123d on your project site if you use the program for any purpose,
# especially for academic research:
# Flow123d, Research Centre: Advanced Remedial Technologies, Technical University of Liberec, Czech Republic
#
# This program is free software; you can redistribute it and/or modify it under the terms
# of the GNU General Public License version 3 as published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this program; if not,
# write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 021110-1307, USA.
#
# $Id: hydra_kai_tul_cz.sh 1527 2012-02-08 07:30:20Z jiri.hnidek $
# $Revision: 1527 $
# $LastChangedBy: jiri.hnidek $
# $LastChangedDate: 2011-01-02 16:54:35 +0100 (ne, 02 I 2011) $
#

# NP is number of procs used to compute
# MPIEXEC is relative path to bin/mpiexec
# FLOW123D is relative path to bin/flow123d (.exe)
# FLOW_PARAMS is list of parameters of flow123d
# INI_FILE is name of .ini file
# WORKDIR is directory from which flow123d.sh was started
# TIMEOUT is max time to run

# Function that is used for running flow123d at hydra cluster
function run_flow()
{
	# Some important files
	export ERR_FILE="err.log"
	export OUT_FILE="out.log"
	
	QSUB_FILE="/tmp/${USER}-flow123.qsub"
        rm -f ${QSUB_FILE}
	
	if [ -z "${QUEUE}" ]; then QUEUE=normal; fi
	if [ -z "${PPN}" ]; then PPN=2; fi
        if [ -z "${MEM}" ]; then MEM="$( ${PPN} * 2)"; fi
        
			
# Copy following text to the file /tmp/firstname.surname-hydra_flow.qsub
# ======================================================================
cat << xxEOFxx > ${QSUB_FILE}
#!/bin/bash
#
# Specific PBS setting
#
#PBS -S /bin/bash 
#PBS -N flow123d
#PBS -j oe
#################
#PBS -l nodes=${NP}:ppn=${PPN}:x86_64
#PBS -l mem=${MEM}gb
# #PBS -l walltime=24:00:00
# #PBS -l select=1:ncpus=$NP:host=rex
# #PBS -l place=free:shared 

# load modules
module load openmpi-1.6-intel
module load boost-1.49
module load intelcdk-12


cd ${WORKDIR}

# header - info about task
uname -a
echo JOB START: `date` 
pwd

echo mpirun -np $NP "$FLOW123D" $FLOW_PARAMS  

mpirun -np $NP "$FLOW123D" $FLOW_PARAMS  
  
	
# End of flow123d.qsub
xxEOFxx
# ======================================================================

	if [ -f ${QSUB_FILE} ]
	then    
		# Add new PBS job to the queue
		echo "qsub -l nodes=${NP}:ppn=${PPN}:x86_64 -l mem=${MEM}gb -q ${QUEUE} ${QSUB_FILE}"
		
		JOB_NAME=`qsub -q ${QUEUE} ${UNRESOLVED_PARAMS} ${QSUB_FILE}`
                # construct STDOUT_NAME
                # JOB_NAME = Your job 77931 ("jan.brezina-hydra_flow.qsub") has been submitted
                # STDOUT_NAME =  jan.brezina-hydra_flow.po77931
                JOB_NAME=${JOB_NAME%.*}
                echo "job number: ${JOB_NAME}"
                STDOUT_NAME= ${QSUB_SCRIPT}.o${JOB_NAME}
		# Remove obsolete script
		rm ${QSUB_FILE}
	else
		exit 1
	fi		
	
	exit 0
}