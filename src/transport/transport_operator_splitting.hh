#ifndef TRANSPORT_OPERATOR_SPLITTING_HH_
#define TRANSPORT_OPERATOR_SPLITTING_HH_

#include "equation.hh"
#include "./reaction/linear_reaction.hh"
#include "./semchem/semchem_interface.hh"
#include <limits>


/// external types:
//class LinSys;
//struct Solver;
class Mesh;
//class SchurComplement;
//class Distribution;
//class SparseGraph;
class ConvectionTransport;
class MaterialDatabase;

/**
 * @brief Specification of transport model interface.
 *
 * Here one has to specify methods for setting or getting data particular to
 * transport equations.
 */
class TransportBase : public EquationBase{
public:
    /**
     * This method takes sequantial PETSc vector of side velocities and update
     * transport matrix. The ordering is same as ordering of sides in the mesh.
     *
     * TODO: We should pass whole velocity field object (description of base functions and dof numbering) and vector.
     */
    virtual void set_velocity_field(Vec &velocity_vector) =0;
};



/**
 * @brief Empty transport class.
 */
class TransportNothing : public TransportBase {
public:
    TransportNothing()
    {
        // make module solved for ever
        time_=new TimeGovernor();
        solved = true;
    };

    virtual void get_solution_vector(double * &vector, unsigned int &size) {
        ASSERT( 0 , "Empty transport class do not provide solution!");
    }

    virtual void get_parallel_solution_vector(Vec &vector) {
        ASSERT( 0 , "Empty transport class do not provide solution!");
    };

    virtual void set_velocity_field(Vec &velocity_field) {};
};



/**
 * @brief Reaction transport implemented by operator splitting.
 */

class TransportOperatorSplitting : public TransportBase {
public:
	TransportOperatorSplitting(TimeMarks *marks, MaterialDatabase *material_database, Mesh *init_mesh);
	virtual void set_velocity_field(Vec &velocity_vector);
	virtual void update_solution();
	//virtual void compute_one_step();
	//virtual void compute_until();

//	~TransportOperatorSplitting();
	 virtual void get_parallel_solution_vector(Vec &vc);
	 virtual void get_solution_vector(double* &vector, unsigned int &size);
	 void compute_until_save_time();
protected:

private:

    ConvectionTransport *convection;
    Linear_reaction *decayRad;
    Semchem_interface *Semchem_reactions;
   // Mesh *mesh;
   // MaterialDatabase *mat_base;
   // TimeGovernor *time;
 //   Chemistry *chemistry;
};

#endif // TRANSPORT_OPERATOR_SPLITTING_HH_
