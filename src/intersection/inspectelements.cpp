/*
 * inspectelements.cpp
 *
 *  Created on: 13.4.2014
 *      Author: viktor
 */

#include "inspectelements.h"
namespace computeintersection {

InspectElements::InspectElements(){};

InspectElements::InspectElements(Mesh* _mesh):mesh(_mesh){
	//projeti.assign(sit->n_elements(),false);

	unsigned int elementLimit = 20;
	unsigned int el2_idx;
	unsigned int el3_idx;
	BIHTree bt(mesh, elementLimit);

		//Profiler::initialize(MPI_COMM_WORLD);
		 //START_TIMER("Test_bez_BIHtree");


		FOR_ELEMENTS(mesh, elm) {
			// Pouze pro dvojice 2d - 3d
			if(elm->dim() == 3){
				el3_idx = elm.index();
				xprintf(Msg, "-----Nalezen 3D element idx(%d)------ \n", el3_idx);

				this->UpdateTetrahedron(elm);

			}

			if (elm->dim() == 2) {
				el2_idx = elm.index();
				xprintf(Msg, "-----Nalezen 2D element idx(%d)------ \n",el2_idx);
				this->UpdateTriangle(elm);

			 }
		}

		IntersectionLocal il(el2_idx, el3_idx);
		ComputeIntersection<Simplex<2>,Simplex<3> > CI_23(triangle, tetrahedron);
		CI_23.init();
		//CI_23.toStringPluckerCoordinatesTree();
		CI_23.compute(il);
		//CI_23.toStringPluckerCoordinatesTree();
		il.printTracingTable();
		il.tracePolygon();
		all_intersections.push_back(il);

		cout << "Velikost pole s pruseciky:" << all_intersections.size() << endl;
		//END_TIMER("Test_bez_BIHtree");

		//Profiler::instance()->output(cout);
		//Profiler::uninitialize();

};

InspectElements::~InspectElements(){};


void InspectElements::UpdateTriangle(const ElementFullIter &element_2D){

			arma::vec3 pole_bodu[3] = {element_2D->node[0]->point(),element_2D->node[1]->point(),element_2D->node[2]->point()};
		    triangle = Simplex<2>(pole_bodu);

};

void InspectElements::UpdateTetrahedron(const ElementFullIter &element_3D){

			arma::vec3 pole_bodu[4] = {element_3D->node[0]->point(),element_3D->node[1]->point(),element_3D->node[2]->point(),element_3D->node[3]->point()};
		    tetrahedron = Simplex<3>(pole_bodu);

};

void InspectElements::print(unsigned int vyber){


	FILE * soubor;
	if(vyber == 0){
		soubor = fopen("pokusTriangle.msh","w");

	}else{
		soubor = fopen("pokusTetrahedron.msh","w");

	}

	fprintf(soubor, "$MeshFormat\n");
	fprintf(soubor, "2.2 0 8\n");
	fprintf(soubor, "$EndMeshFormat\n");
	fprintf(soubor, "$Nodes\n");



	int pocet_pruseciku = 0;
	int pocet_predtim = 0;

	FOR_NODES(mesh, nod){
		pocet_predtim++;
	}

		for(unsigned int i = 0; i < all_intersections.size(); i++){
			pocet_pruseciku += all_intersections[i].getIPsize();
		}
	fprintf(soubor, "%d\n", (pocet_pruseciku + pocet_predtim));

	int velikost = 0;
	int pocet_prus = pocet_predtim;
	int velikost_pruseciku = all_intersections.size();

	FOR_NODES(mesh, nod){
		arma::vec3 bod = nod->point();
		fprintf(soubor,"%d %f %f %f\n", nod.id(), bod[0], bod[1], bod[2]);

	}

	for(unsigned int i = 0; i < velikost_pruseciku; i++){

		IntersectionLocal il = all_intersections[i];
		velikost = il.getIPsize();

			unsigned int idx2D = all_intersections[i].idx_2D();
			unsigned int idx3D = all_intersections[i].idx_3D();

			ElementFullIter el2D = mesh->element(idx2D);
			ElementFullIter el3D = mesh->element(idx3D);


			for(unsigned int j = 0; j < velikost; j++){
				pocet_prus++;

				IntersectionPoint<2,3> IP23 = il.get_point(j);
				arma::vec3 T_globalni;
				if(vyber == 0){
					T_globalni = (IP23.getLocalCoords1())[0] * el2D->node[0]->point()
													   +(IP23.getLocalCoords1())[1] * el2D->node[1]->point()
													   +(IP23.getLocalCoords1())[2] * el2D->node[2]->point();
				}else{
					T_globalni = (IP23.getLocalCoords2())[0] * el3D->node[0]->point()
													   +(IP23.getLocalCoords2())[1] * el3D->node[1]->point()
													   +(IP23.getLocalCoords2())[2] * el3D->node[2]->point()
													   +(IP23.getLocalCoords2())[3] * el3D->node[3]->point();
				}
				fprintf(soubor,"%d %f %f %f\n", pocet_prus, T_globalni[0], T_globalni[1], T_globalni[2]);
				//fprintf(soubor,"%d %f %f %f\n", pocet_prus, C_globalni[0], C_globalni[1], C_globalni[2]);

			}
	}




	fprintf(soubor,"$EndNodes\n");
	fprintf(soubor,"$Elements\n");
	fprintf(soubor,"%d\n", (pocet_pruseciku + mesh->n_elements()) );

	FOR_ELEMENTS(mesh, elee){
		// 1 4 2 30 26 1 2 3 4
		// 2 2 2 2 36 5 6 7
		if(elee->dim() == 3){
			int id1 = mesh->node_vector.index(elee->node[0]) + 1;
			int id2 = mesh->node_vector.index(elee->node[1]) + 1;
			int id3 = mesh->node_vector.index(elee->node[2]) + 1;
			int id4 = mesh->node_vector.index(elee->node[3]) + 1;

			fprintf(soubor,"%d 4 2 30 26 %d %d %d %d\n", elee.id(), id1, id2, id3, id4);
		}else if(elee->dim() == 2){
			int id1 = mesh->node_vector.index(elee->node[0]) + 1;
			int id2 = mesh->node_vector.index(elee->node[1]) + 1;
			int id3 = mesh->node_vector.index(elee->node[2]) + 1;
			fprintf(soubor,"%d 2 2 2 36 %d %d %d\n", elee.id(), id1, id2, id3);

		}else{

		}
		}


	pocet_prus = mesh->n_elements();
	int posledni = 0;
	int vrcholy = pocet_predtim;
		for(unsigned int i = 0; i < all_intersections.size(); i++){


			IntersectionLocal il = all_intersections[i];
			velikost = il.getIPsize();

				for(unsigned int j = 0; j < velikost; j++){
					pocet_prus++;
					vrcholy++;
					if(j == 0){
						posledni = vrcholy;
					}

					if((j+1) == velikost){
						fprintf(soubor,"%d 1 2 18 7 %d %d\n", pocet_prus, vrcholy, posledni);
					}else{
						fprintf(soubor,"%d 1 2 18 7 %d %d\n", pocet_prus, vrcholy, vrcholy+1);
					}

				}
		}

	fprintf(soubor,"$EndElements\n");
	fclose(soubor);

}

} // END namespace
