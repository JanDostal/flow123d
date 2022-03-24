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
 * @file    assembly_observe.hh
 * @brief
 */

#ifndef ASSEMBLY_OBSERVE_HH_
#define ASSEMBLY_OBSERVE_HH_

#include <unordered_map>

#include "coupling/generic_assembly.hh"
#include "coupling/assembly_base.hh"
#include "fem/dofhandler.hh"
//#include "fem/fe_values.hh"
//#include "quadrature/quadrature_lib.hh"
//#include "coupling/balance.hh"
#include "fields/field_value_cache.hh"
//#include "io/element_data_cache.hh"
//#include "mesh/ref_element.hh"


/// Holds data of one eval point on patch (index of element and local coordinations).
struct PatchPointData {
    /// Default constructor
	PatchPointData() {}

    /// Constructor with data mebers initialization
	PatchPointData(unsigned int elm_idx, arma::vec loc_coords, unsigned int point_idx)
    : elem_idx(elm_idx), local_coords(loc_coords), point_cache_idx(point_idx), i_quad(0), i_quad_point(0) {}

    /// Copy constructor
	PatchPointData(const PatchPointData &other)
    : elem_idx(other.elem_idx), local_coords(other.local_coords), point_cache_idx(other.point_cache_idx),
      i_quad(other.i_quad), i_quad_point(other.i_quad_point) {}

    unsigned int elem_idx;        ///< Index of element
    arma::vec local_coords;       ///< Local coords of point
    unsigned int point_cache_idx; ///< Local point time index hold position of point in output element data cache
    unsigned int i_reg;           ///< Index of region (use during patch creating)
    unsigned int i_quad;          ///< Index of quadrature (use during patch creating), i_quad = dim-1
	unsigned int i_quad_point;    ///< Index of point in quadrature (use during patch creating)
};
typedef std::vector<PatchPointData> PatchPointVec;


/**
 * Helper structure holds data of cell (bulk) integral
 *
 * Data is specified by cell, subset index in EvalPoint object and index of point in subset
 */
struct BulkIntegralPointData {
	/// Default constructor
	BulkIntegralPointData() {}

    /// Constructor with data mebers initialization
	BulkIntegralPointData(DHCellAccessor dhcell, unsigned int subset_idx, unsigned int point_idx, unsigned int i_p)
    : cell(dhcell), subset_index(subset_idx), point_cache_idx(point_idx), i_point(i_p) {}

    /// Copy constructor
	BulkIntegralPointData(const BulkIntegralPointData &other)
    : cell(other.cell), subset_index(other.subset_index), point_cache_idx(other.point_cache_idx), i_point(other.i_point) {}

    DHCellAccessor cell;          ///< Specified cell (element)
    unsigned int subset_index;    ///< Index (order) of subset in EvalPoints object
    unsigned int point_cache_idx; ///< Local point time index hold position of point in output element data cache
    unsigned int i_point;         ///< Index of point in subset
};


/**
 * @brief Generic class of observe output assemblation.
 *
 * Class
 *  - holds assemblation structures (EvalPoints, Integral objects, Integral data table).
 *  - associates assemblation objects specified by dimension
 *  - provides general assemble method
 *  - provides methods that allow construction of element patches
 */
template < template<IntDim...> class DimAssembly>
class GenericAssemblyObserve : public GenericAssemblyBase
{
public:
    /// Constructor
	GenericAssemblyObserve( typename DimAssembly<1>::EqFields *eq_fields, typename DimAssembly<1>::EqData *eq_data)
    : multidim_assembly_(eq_fields, eq_data), bulk_integral_data_(20, 10)
    {
        eval_points_ = std::make_shared<EvalPoints>();
    }

    /// Getter to set of assembly objects
    inline MixedPtr<DimAssembly, 1> multidim_assembly() const {
        return multidim_assembly_;
    }

	/**
	 * @brief General assemble methods.
	 *
	 * Loops through local cells and calls assemble methods of assembly
	 * object of each cells over space dimension.
	 */
    void assemble(std::shared_ptr<DOFHandlerMultiDim> dh) override {
        START_TIMER( DimAssembly<1>::name() );
        eval_points_->clear();
        element_cache_map_.eval_point_data_.reset();
        this->create_bulk_integrals(dh);

        this->reallocate_cache();
        element_cache_map_.create_patch();
        multidim_assembly_[1_d]->eq_fields_->cache_update(element_cache_map_);

        multidim_assembly_[1_d]->assemble_cell_integrals(bulk_integral_data_);
        multidim_assembly_[2_d]->assemble_cell_integrals(bulk_integral_data_);
        multidim_assembly_[3_d]->assemble_cell_integrals(bulk_integral_data_);
        bulk_integral_data_.reset();
        END_TIMER( DimAssembly<1>::name() );
    }

    inline PatchPointVec &patch_point_data() {
        return patch_point_data_;
    }


private:
    /// Calls cache_reallocate method on set of used fields
    inline void reallocate_cache() {
        multidim_assembly_[1_d]->eq_fields_->cache_reallocate(this->element_cache_map_, multidim_assembly_[1_d]->used_fields_);
        // DebugOut() << "Order of evaluated fields (" << DimAssembly<1>::name() << "):" << multidim_assembly_[1_d]->eq_fields_->print_dependency();
    }

    void create_bulk_integrals(std::shared_ptr<DOFHandlerMultiDim> dh) {
        MeshBase *dh_mesh = dh->mesh();
        std::array< std::vector<arma::vec>, 3 > reg_points;

        for(auto & p_data : patch_point_data_) {
            auto el_acc = dh_mesh->element_accessor(p_data.elem_idx);
            p_data.i_reg = el_acc.region_idx().idx();
            p_data.i_quad = el_acc.dim() - 1;
            p_data.i_quad_point = reg_points[p_data.i_quad].size();
            reg_points[p_data.i_quad].push_back(p_data.local_coords);
        }

        if (reg_points[0].size() > 0) { // dim 1
            Quadrature q(1, reg_points[0].size());
            for (uint j=0; j<reg_points[0].size(); j++) {
                arma::vec::fixed<1> fix_p1 = reg_points[0][j].subvec(0, 0);
                q.set(j) = fix_p1;
            }
            bulk_integrals_[0] = eval_points_->add_bulk<1>(q);
            multidim_assembly_[1_d]->set_bulk_integral(bulk_integrals_[0]);
        }
        if (reg_points[1].size() > 0) { // dim 1
            Quadrature q(2, reg_points[1].size());
            for (uint j=0; j<reg_points[1].size(); j++) {
                arma::vec::fixed<2> fix_p2 = reg_points[1][j].subvec(0, 1);
                q.set(j) = fix_p2;
            }
            bulk_integrals_[1] = eval_points_->add_bulk<2>(q);
            multidim_assembly_[2_d]->set_bulk_integral(bulk_integrals_[1]);
        }
        if (reg_points[2].size() > 0) { // dim 1
            Quadrature q(3, reg_points[2].size());
            for (uint j=0; j<reg_points[2].size(); j++) {
                arma::vec::fixed<3> fix_p3 = reg_points[2][j].subvec(0, 2);
                q.set(j) = fix_p3;
            }
            bulk_integrals_[2] = eval_points_->add_bulk<3>(q);
            multidim_assembly_[3_d]->set_bulk_integral(bulk_integrals_[2]);
        }
        element_cache_map_.init(eval_points_); // should be made only once
        multidim_assembly_[1_d]->initialize(&element_cache_map_);
        multidim_assembly_[2_d]->initialize(&element_cache_map_);
        multidim_assembly_[3_d]->initialize(&element_cache_map_);

        unsigned int i_ep, subset_begin, subset_idx;
        for(auto & p_data : patch_point_data_) {
            subset_idx = bulk_integrals_[p_data.i_quad]->get_subset_idx();
        	subset_begin = eval_points_->subset_begin(p_data.i_quad+1, subset_idx);
            i_ep = subset_begin + p_data.i_quad_point;
            DHCellAccessor dh_cell = dh->cell_accessor_from_element(p_data.elem_idx);
            bulk_integral_data_.emplace_back(dh_cell, subset_idx, p_data.point_cache_idx, p_data.i_quad_point);
            element_cache_map_.eval_point_data_.emplace_back(p_data.i_reg, p_data.elem_idx, i_ep, 0);
        }
        bulk_integral_data_.make_permanent();
        element_cache_map_.eval_point_data_.make_permanent();
    }

    std::shared_ptr<EvalPoints> eval_points_;                     ///< EvalPoints object
    ElementCacheMap element_cache_map_;                           ///< ElementCacheMap according to EvalPoints
    MixedPtr<DimAssembly, 1> multidim_assembly_;                  ///< Assembly object
    PatchPointVec patch_point_data_;                              ///< Holds data of eval points on patch
    std::array<std::shared_ptr<BulkIntegral>, 3> bulk_integrals_; ///< Set of bulk integrals of dimensions 1, 2, 3
    RevertableList<BulkIntegralPointData> bulk_integral_data_;    ///< Holds data for computing bulk integrals.
};


template <unsigned int dim>
class AssemblyObserveOutput : public AssemblyBase<dim>
{
public:
    // TODO set used_fields: same as in AssemblyOutputBase
    typedef EquationOutput EqFields;
    typedef EquationOutput EqData;

    static constexpr const char * name() { return "AssemblyObserveOutput"; }

    /// Constructor.
    AssemblyObserveOutput(EqFields *eq_fields, EqData *eq_data)
    : AssemblyBase<dim>(), eq_fields_(eq_fields), eq_data_(eq_data) {
        offsets_.resize(1.1 * CacheMapElementNumber::get());
    }

    /// Destructor.
    ~AssemblyObserveOutput() {}

    /// Initialize auxiliary vectors and other data members
    void initialize(ElementCacheMap *element_cache_map) {
        this->element_cache_map_ = element_cache_map;
    }

    /// Sets observe output data members (set of used fields).
    void set_observe_data(const FieldSet &used) {
    	used_fields_ = FieldSet();
    	used_fields_ += used;
    }

    /// Assembles the cell integrals for the given dimension.
    inline void assemble_cell_integrals(const RevertableList<BulkIntegralPointData> &bulk_integral_data) {
        unsigned int element_patch_idx, field_value_cache_position, val_idx;
        this->reset_offsets();
        for (unsigned int i=0; i<bulk_integral_data.permanent_size(); ++i) {
            if (bulk_integral_data[i].cell.dim() != dim) continue;
            element_patch_idx = this->element_cache_map_->position_in_cache(bulk_integral_data[i].cell.elm_idx());
            auto p = *( this->bulk_points(element_patch_idx).begin()); // evaluation point
            field_value_cache_position = this->element_cache_map_->element_eval_point(element_patch_idx, p.eval_point_idx() + bulk_integral_data[i].i_point);
            val_idx = bulk_integral_data[i].point_cache_idx;
            this->offsets_[field_value_cache_position] = val_idx;
        }
        for (FieldListAccessor f_acc : this->used_fields_.fields_range()) {
            f_acc->fill_observe_value(this->offsets_);
        }
    }


    /// Assembles the cell integrals for the given dimension.
    inline void set_bulk_integral(std::shared_ptr<BulkIntegral> bulk_integral) {
        this->integrals_.bulk_ = bulk_integral;
    }

private:
    void reset_offsets() {
        std::fill(offsets_.begin(), offsets_.end(), -1);
    }

    /// Data objects shared with EquationOutput
    EqFields *eq_fields_;
    EqData *eq_data_;

    FieldSet used_fields_;                                    ///< Sub field set contains fields performed to output
    std::vector<int> offsets_;                                ///< Holds indices (offsets) of cached data to output data vector


    template < template<IntDim...> class DimAssembly>
    friend class GenericAssemblyObserve;
};




#endif /* ASSEMBLY_OBSERVE_HH_ */
