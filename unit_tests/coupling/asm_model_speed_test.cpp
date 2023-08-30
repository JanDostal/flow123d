/*
 * assembly_speed_test.cpp
 *
 *  Created on: Aug 16, 2023
 *      Author: David Flanderka
 *
 *  Speed tests of assembly
 */

#define TEST_USE_PETSC
#define FEAL_OVERRIDE_ASSERTS
#include <flow_gtest_mpi.hh>

#include "assembly_benchmark.impl.hh"


/****************************************************************************************
 *                 Speed test of Assembly
 *
 * Results:
 * Mesh 27936 elements, 100 assemblation loops - TODO not actual
 * Checked GenericAssembly with active bulk integral only vs. with all active integrals
 *
 *                           bulk      all
 * add_integrals_to_patch   19.12    43.95
 * create_patch              3.00    18.59
 * cache_update              8.43    26.38
 *
 ****************************************************************************************/


TEST_F(AssemblyBenchmarkTest, simple_asm) {
    string eq_data_input = R"YAML(
    solver: !Petsc
      a_tol: 1.0e-12
      r_tol: 1.0e-12
    input_fields:
      - region: .BOUNDARY
        bc_type: diffusive_flux
      - region: BULK
        porosity: !FieldFormula
          value: 0.1*X[0]*X[1]
        init_conc:
          - !FieldConstant
            value: 0.31
          - !FieldFormula
            value: 1-X[0]*X[1]
        diff_m:
          - !FieldConstant
            value: [ [ 0.04, 0.02, 0 ], [ 0.02, 0.01, 0 ], [ 0, 0, 0 ] ]
          - !FieldFormula
            value: "[ [ 0.01*X[0], 0.2*X[1], 1 ], [ 0.2*X[1], 0.01*X[0], 2 ], [ 1, 2, 3 ] ]"
    )YAML";

    this->create_and_set_mesh("mesh/cube_2x1.msh");
    this->initialize( eq_data_input, {"A", "B"} );
    this->eq_fields_->init_field_models();
    this->run_simulation();
    this->profiler_output("model_simple");
}
