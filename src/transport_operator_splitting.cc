/*
 * transport_operator_splitting.cc
 *
 *  Created on: May 21, 2011
 *      Author: jiri
 */

#include "transport_operator_splitting.hh"
#include <petscmat.h>
#include "system/sys_vector.hh"
#include <time_governor.hh>
#include <materials.hh>
//#include "equation.hh"
#include "transport.h"
#include "mesh/mesh.h"
#include "reaction/linear_reaction.hh"
#include "semchem/semchem_interface.hh"


TransportOperatorSplitting::TransportOperatorSplitting(MaterialDatabase *material_database, Mesh *init_mesh)
{
	mat_base = material_database;
	mesh = init_mesh;
    ConvectionTransport *convection;
    Linear_reaction *decayRad;
    Semchem_interface *Semchem_reactions;

    double problem_save_step = OptGetDbl("Global", "Save_step", "1.0");
    double problem_stop_time = OptGetDbl("Global", "Stop_time", "1.0");

    //temporary variables for chemistry
    double time_step = 0.5;
    int n_substances = OptGetInt("Transport", "N_substances", NULL );

	convection = new ConvectionTransport(mat_base, mesh);

	// Chemistry initialization
	decayRad = new Linear_reaction(time_step);
	decayRad->set_nr_of_species(convection->get_n_substances());
	Semchem_reactions = new Semchem_interface();

	time_marks = new TimeMarks();
	time = new TimeGovernor(time_marks, problem_stop_time, problem_stop_time);
	time->set_constrain(convection->get_cfl_time_constrain());

}

void TransportOperatorSplitting::compute_one_step(){
	//following declarations are here just to enable compilation without errors
	bool porTyp = true;
	ElementIter ppelm;
	int poradi;
	double **conc_mob_arr, **conc_immob_arr, ***conc; //could be handled as **conc[MOBILE], **conc[IMMOBILE]
	double time_step = 0.05;
	int cheat = 10; // I need to get nr of elements instead of this variable

	convection->compute_one_step();
    // Calling linear reactions and Semchem
	decayRad->compute_one_step(conc, cheat);
	Semchem_reactions->compute_one_step(porTyp, ppelm, conc);
	//Semchem_reactions->compute_one_step(dual_porosity, time_step, mesh->element(el_4_loc[loc_el]), loc_el, pconc[MOBILE], pconc[IMMOBILE]);
	time->next_time();
}

void TransportOperatorSplitting::compute_until_save_time(){

	//while(time->output_time())
		compute_one_step();

	//call output

}




void TransportOperatorSplitting::get_parallel_solution_vector(Vec &vec){
	convection->compute_one_step();
};

void TransportOperatorSplitting::get_solution_vector(double * &x, unsigned int &a){
	convection->compute_one_step();
};

