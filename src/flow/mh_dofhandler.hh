/*!
 *
﻿ * Copyright (C) 2015 Technical University of Liberec.  All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 3 as published by the
 * Free Software Foundation. (http://www.gnu.org/licenses/gpl-3.0.en.html)
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * 
 * @file    mh_dofhandler.hh
 * @brief   
 */

#ifndef MH_DOFHANDLER_HH_
#define MH_DOFHANDLER_HH_

#include <vector>
#include <memory>
#include <unordered_map>

#include "mesh/mesh_types.hh"
#include "mesh/accessors.hh"
#include "mesh/sides.h"
#include "mesh/region.hh"

#include "la/distribution.hh"
#include "la/local_to_global_map.hh"

using namespace std;

class Mesh;
class Side;
class SideIter;
class MH_DofHandler;

template <int spacedim>
class LocalElementAccessorBase;


#include "fields/field.hh"
#include "fem/xfem_element_data.hh"

class XFEMElementSingularData;
template<unsigned int spacedim> class Singularity0D;
namespace computeintersection{
    class InspectElements;
}

/// temporary solution to provide access to results
/// from DarcyFlowMH independent of mesh
class MH_DofHandler {
public:
    MH_DofHandler();
    ~MH_DofHandler();
    void reinit(Mesh *mesh);

    void prepare_parallel();
    void make_row_numberings();
    void prepare_parallel_bddc();

    void set_solution( double time, double * solution, double precision);

    inline double time_changed() const
        { return time_; }

    unsigned int side_dof(const SideIter side) const;

    /// temporary replacement for DofHandler accessor, flux through given side
    double side_flux(const Side &side) const;

    /// temporary replacement for DofHandler accessor, scalar (pressure) on edge of the side
    double side_scalar(const Side &side) const;

    /// temporary replacement for DofHandler accessor, scalar (pressure) on element
    double element_scalar( ElementFullIter &ele ) const;

    inline double precision() const { return solution_precision; };

    LocalElementAccessorBase<3> accessor(uint local_ele_idx);

//protected:
    vector< vector<unsigned int> > elem_side_to_global;

    Mesh *mesh_;
    int *el_4_loc;              //< array of idexes of local elements (in ordering matching the optimal global)
    int *row_4_el;              //< element index to matrix row
    int *side_id_4_loc;     //< array of ids of local sides
    int *side_row_4_id;     //< side id to matrix row
    int *edge_4_loc;        //< array of indexes of local edges
    int *row_4_edge;        //< edge index to matrix row

    // parallel
    Distribution *edge_ds;          //< optimal distribution of edges
    Distribution *el_ds;            //< optimal distribution of elements
    Distribution *side_ds;          //< optimal distribution of elements
    std::shared_ptr<Distribution> rows_ds;          //< final distribution of rows of MH matrix


    /// Maps mesh index of the edge to the edge index in the mesh portion local to the processor.
    /// Temporary solution until we have parallel mesh which should provide such information.
    std::unordered_map<unsigned int, unsigned int> edge_new_local_4_mesh_idx_;

    /// Necessary only for BDDC solver.
    std::shared_ptr<LocalToGlobalMap> global_row_4_sub_row;           //< global dof index for subdomain index


    double * mh_solution;
    double solution_precision;
    double time_;

    friend LocalElementAccessorBase<3>;
    
    
    
    // XFEM:
public:
    
    typedef typename std::shared_ptr<Singularity0D<3>> SingularityPtr;
    
    void reinit(Mesh *mesh,
                shared_ptr< computeintersection::InspectElements > intersections,
                Field<3, FieldValue<3>::Scalar>& cross_section,
                Field<3, FieldValue<3>::Scalar>& sigma);
    
    template<class T>
    void print_array(T * array, unsigned int length, std::string name = "array"){
        DBGCOUT("print '" << name  << "' (" << length << "): \n");
        for(unsigned int i=0; i < length; i++){
            DBGCOUT(<< "[" << i << "]:  " << array[i] << "\n");
        }
    }
    
    int total_size();
    
    int *row_4_sing;        //< singularity index to matrix row (lagrange multiplier)
    
    unsigned int n_enrichments();
    
    //TODO: move to input xfem settings, make proper setters
    bool enrich_velocity, enrich_pressure;
    
protected:
    static const int empty_node_idx;
    
    void create_enrichment(std::shared_ptr<computeintersection::InspectElements> intersections,
                           std::vector<SingularityPtr> &singularities,
                           Field<3, FieldValue<3>::Scalar>& cross_section,
                           Field<3, FieldValue<3>::Scalar>& sigma);
    
    void find_ele_to_enrich(SingularityPtr sing, std::vector<unsigned int>& ele_to_enrich,
                            ElementFullIter ele, double radius, int& new_enrich_node_idx);
    
    void clear_mesh_flags();
    
    void clear_node_aux();
    
    /// Distribute continuous enriched DoFs.
    void distribute_enriched_dofs(std::vector<std::vector<int>>& temp_dofs, int& offset, Quantity quant);
    
    /// Distribute discontinuous enriched DoFs.
    void distribute_enriched_dofs(int& offset, Quantity quant);
    
    void update_standard_dofs();
    
    std::vector<SingularityPtr> singularities_12d_;
    
    std::vector<XFEMComplementData> xfem_data_1d;
    std::vector<XFEMElementSingularData> xfem_data;
    
    std::vector<std::map<int, double> > node_values;
    
    std::vector<std::map<int, Space<3>::Point> > node_vec_values;
    
    std::vector<bool> mesh_flags_;
    
    unsigned int offset_velocity, offset_pressure, offset_enr_velocity,
                 offset_enr_pressure, offset_edges, offset_enr_lagrange;
};



typedef unsigned int uint;

template <int spacedim>
class LocalElementAccessorBase {
public:
    LocalElementAccessorBase(MH_DofHandler *dh, uint loc_ele_idx)
    : dh(dh), local_ele_idx_(loc_ele_idx), ele(dh->mesh_->element(ele_global_idx()))
    {}

    uint dim() {
        return ele->dim();
    }

    uint n_sides() {
        return ele->n_sides();
    }

    ElementFullIter full_iter() {
        return ele;
    }

    ElementAccessor<3> element_accessor() {
        return ele->element_accessor();
    }

    const arma::vec3 centre() const {
        return ele->centre();
    }

    double measure() const {
        return ele->measure();
    }

    Region region() const {
        return ele->region();
    }

    uint ele_global_idx() {
        return dh->el_4_loc[local_ele_idx_];
    }

    uint ele_local_idx() {
        return local_ele_idx_;
    }

    uint ele_row() {
        return dh->row_4_el[ele_global_idx()];
    }

    uint ele_local_row() {
        return ele_row() - dh->rows_ds->begin(); //  i_loc_el + side_ds->lsize();
    }

    uint edge_global_idx(uint i) {
        return ele->side(i)->edge_idx();
    }

    uint edge_local_idx(uint i) {
        return dh->edge_new_local_4_mesh_idx_[edge_global_idx(i)];
    }

    uint edge_row(uint i) {
        return dh->row_4_edge[edge_global_idx(i)];
    }

    uint edge_local_row( uint i) {
        return edge_row(i) - dh->rows_ds->begin();
    }

    int *edge_rows() {
        for(uint i=0; i< n_sides(); i++) edge_rows_[i] = edge_row(i);
        return edge_rows_;
    }

    SideIter side(uint i) {
        return ele->side(i);
    }

    uint side_global_idx(uint i) {
        return dh->elem_side_to_global[ ele->index() ][ i ];
    }

    uint side_local_idx(uint i) {
        return dh->side_row_4_id[side_global_idx(i)] - dh->rows_ds->begin();
    }

    uint side_row(uint i) {
        return dh->side_row_4_id[side_global_idx(i)];
    }

    uint side_local_row( uint i) {
        return side_row(i) - dh->rows_ds->begin();
    }

    int *side_rows() {
        for(uint i=0; i< n_sides(); i++) side_rows_[i] = side_row(i);
        return side_rows_;
    }

    
    XFEMElementDataBase* xfem_data_pointer(){
        return ele->xfem_data;
    }
    
    XFEMElementSingularData* xfem_data_sing(){
        if(is_enriched()){
            return static_cast<XFEMElementSingularData*>(ele->xfem_data);
        }
        ASSERT_DBG(0).error("Element not enriched with any XFEM data!");
        return nullptr;
    }
    
    bool is_enriched(){
        return ele->xfem_data != nullptr;
    }
    
    int sing_row(uint local_enrichment_index){
        return dh->row_4_sing[xfem_data_sing()->global_enrichment_index(local_enrichment_index)];
    }
    
//     int get_dofs(Quantity quant, int dofs[])
//     {
//         uint i;
//         for(i=0; i< dim(); i++) dofs[i] = side_row(i);
//         
//         if(ele->xfem_data != nullptr)
//             for(uint w=0; w< ele->xfem_data->n_enrichments(); w++)
//                 for(uint j=0; j< dim(); j++, i++){
//                     dofs[i] = ele->xfem_data->global_enriched_dofs(Quantity::velocity, w)[j];
//                 }
//         return i;
//     }
    
    int get_dofs_vel(int dofs[])
    {
        uint i;
        for(i=0; i< n_sides(); i++) dofs[i] = side_row(i);
        
        if(is_enriched() && ! ele->xfem_data->is_complement()){
            XFEMElementSingularData* xd = xfem_data_sing();
            for(uint w=0; w< xd->n_enrichments(); w++)
                for(uint j=0; j< xd->n_enriched_dofs(Quantity::velocity,w); j++, i++){
                    dofs[i] = xd->global_enriched_dofs(Quantity::velocity, w)[j];
                }
        }
        return i;
    }
    
    int get_dofs_press(int dofs[])
    {
        dofs[0] = ele_row();
        uint i = 1;
        
        if(is_enriched() && ! ele->xfem_data->is_complement()){
            XFEMElementSingularData* xd = xfem_data_sing();
            for(uint w=0; w< xd->n_enrichments(); w++)
                for(uint j=0; j< xd->n_enriched_dofs(Quantity::pressure,w); j++, i++){
                    dofs[i] = xd->global_enriched_dofs(Quantity::pressure, w)[j];
                }
        }
        return i;
    }
    
    int get_dofs(int dofs[])
    {
        int d = get_dofs_vel(dofs);
        d += get_dofs_press(dofs+d);
        for(uint i=0; i< n_sides(); i++, d++) dofs[d] = edge_row(i);
        
        return d;
    }
    
    unsigned int n_dofs_vel(){
        unsigned int n = n_sides();
        if(is_enriched() && ! ele->xfem_data->is_complement())
            n += xfem_data_sing()->n_enriched_dofs(Quantity::velocity);
        return n;
    }
    
    unsigned int n_dofs_press(){
        unsigned int n = 1;
        if(is_enriched() && ! ele->xfem_data->is_complement())
            n += xfem_data_sing()->n_enriched_dofs(Quantity::pressure);
        return n;
    }
    
    unsigned int n_dofs()
    {
        // velocity, pressure, edge pressure(lagrange)
        // exclude xfem pressure(lagrange)
        return n_dofs_vel() + n_dofs_press() + n_sides(); // + xfem_data()->n_enrichments();
    }
    
private:
    int side_rows_[4];
    int edge_rows_[4];
    MH_DofHandler *dh;
    uint local_ele_idx_;
    ElementFullIter ele;
};

/**
 * This is prototype of further much more complex and general accessor templated by
 * element dimension. In fact we shall need an accessor for every kind of element interaction integral.
 */
template <int spacedim, int dim>
class LocalElementAccessor : public LocalElementAccessorBase<spacedim> {
public:
    LocalElementAccessor(MH_DofHandler & dh, uint loc_ele_idx)
    : LocalElementAccessorBase<spacedim>(dh, loc_ele_idx)
    {}
};



#endif /* MH_DOFHANDLER_HH_ */
