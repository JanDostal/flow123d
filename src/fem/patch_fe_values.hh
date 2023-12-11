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
 * @file    patch_fe_values.hh
 * @brief   Class FEValues calculates finite element data on the actual
 *          cells such as shape function values, gradients, Jacobian of
 *          the mapping from the reference cell etc.
 * @author  Jan Stebel, David Flanderka
 */

#ifndef PATCH_FE_VALUES_HH_
#define PATCH_FE_VALUES_HH_


#include <string.h>                           // for memcpy
#include <algorithm>                          // for swap
#include <new>                                // for operator new[]
#include <string>                             // for operator<<
#include <vector>                             // for vector
#include "fem/element_values.hh"              // for ElementValues
#include "fem/fe_values.hh"                   // for FEValuesBase
#include "fem/fe_values_views.hh"             // for FEValuesViews
#include "mesh/ref_element.hh"                // for RefElement
#include "mesh/accessors.hh"
#include "fem/update_flags.hh"                // for UpdateFlags
#include "quadrature/quadrature_lib.hh"
#include "fields/eval_subset.hh"

template<unsigned int spacedim> class PatchFEValues;



using Scalar = double;
using Vector = arma::vec3;
using Tensor = arma::mat33;

template <class ValueType>
class ElQ {
public:
    /// Forbidden default constructor
    ElQ() = delete;

    /// Constructor
    ElQ(PatchFEValues<3> *fe_values, unsigned int begin)
    : fe_values_(fe_values), begin_(begin) {}

    ValueType operator()(FMT_UNUSED const BulkPoint &point);

    ValueType operator()(FMT_UNUSED const SidePoint &point);

private:
    // attributes:
    PatchFEValues<3> *fe_values_;
    unsigned int begin_;    /// Index of the first component of the Quantity. Size is given by ValueType
};


template <class ValueType>
class FeQ {
public:
    /// Forbidden default constructor
    FeQ() = delete;

    // Class similar to current FeView
    FeQ(PatchFEValues<3> *fe_values, unsigned int begin)
    : fe_values_(fe_values), begin_(begin) {}


    ValueType operator()(FMT_UNUSED unsigned int shape_idx, FMT_UNUSED const BulkPoint &point);

    ValueType operator()(FMT_UNUSED unsigned int shape_idx, FMT_UNUSED const SidePoint &point);

    // Implementation for EdgePoint, SidePoint, and JoinPoint shoud have a common implementation
    // resolving to side values

private:
    // attributes:
    PatchFEValues<3> *fe_values_;
    unsigned int begin_;    /// Index of the first component of the Quantity. Size is given by ValueType
};


template<unsigned int spacedim = 3>
class PatchFEValues {
private:
    enum MeshObjectType {
        ElementFE = 0,
		SideFE = 1
    };


//    /// Structure for storing the precomputed finite element data.
//    class FEInternalData
//    {
//    public:
//
//        FEInternalData(unsigned int np, unsigned int nd);
//
//        /// Create a new instance of FEInternalData for a FESystem component or subvector.
//        FEInternalData(const FEInternalData &fe_system_data,
//                       const std::vector<unsigned int> &dof_indices,
//                       unsigned int first_component_idx,
//                       unsigned int ncomps = 1);
//
//        /**
//         * @brief Precomputed values of basis functions at the quadrature points.
//         *
//         * Dimensions:   (no. of quadrature points)
//         *             x (no. of dofs)
//         *             x (no. of components in ref. cell)
//         */
//        std::vector<std::vector<arma::vec> > ref_shape_values;
//
//        /**
//         * @brief Precomputed gradients of basis functions at the quadrature points.
//         *
//         * Dimensions:   (no. of quadrature points)
//         *             x (no. of dofs)
//         *             x ((dim_ of. ref. cell)x(no. of components in ref. cell))
//         */
//        std::vector<std::vector<arma::mat> > ref_shape_grads;
//
//        /// Number of quadrature points.
//        unsigned int n_points;
//
//        /// Number of dofs (shape functions).
//        unsigned int n_dofs;
//    };


    class ElementFEData
    {
    public:
        ElementFEData() {}

        /// Shape functions evaluated at the quadrature points.
        std::vector<std::vector<double> > shape_values_;

        /// Gradients of shape functions evaluated at the quadrature points.
        /// Each row of the matrix contains the gradient of one shape function.
        std::vector<std::vector<arma::vec::fixed<spacedim> > > shape_gradients_;

        /// Auxiliary object for calculation of element-dependent data.
        std::shared_ptr<ElementValues<spacedim> > elm_values_;

    };


    /// Subobject holds FE data of one dimension (0,1,2,3)
    class DimPatchFEValues {
    public:
        /// Constructor
        DimPatchFEValues(unsigned int max_size=0)
        : used_size_(0), max_n_elem_(max_size) {}


    	inline unsigned int used_size() const {
    	    return used_size_;
    	}

    	inline unsigned int max_size() const {
    	    return element_data_.size();
    	}

        void reinit(PatchElementsList patch_elements);

        /**
    	 * @brief Initialize structures and calculates cell-independent data.
    	 *
    	 * @param _quadrature The quadrature rule for the cell associated
         *                    to given finite element or for the cell side.
    	 * @param _fe The finite element.
    	 * @param _flags The update flags.
    	 */
        template<unsigned int DIM>
        void initialize(Quadrature &_quadrature,
                        FiniteElement<DIM> &_fe,
                        UpdateFlags _flags);

        /**
         * @brief Allocates space for computed data.
         *
         * @param n_points    Number of quadrature points.
         * @param _fe         The finite element.
         * @param flags       The update flags.
         */
        template<unsigned int DIM>
        void allocate(Quadrature &_quadrature,
                      FiniteElement<DIM> &_fe,
                      UpdateFlags flags);

        /// Precompute finite element data on reference element.
        template<unsigned int DIM>
        std::shared_ptr<FEInternalData> init_fe_data(const FiniteElement<DIM> &fe, const Quadrature &q);

        /**
         * @brief Computes the shape function values and gradients on the actual cell
         * and fills the FEValues structure.
         *
         * @param fe_data Precomputed finite element data.
         */
        void fill_data(const ElementValues<spacedim> &elm_values, const FEInternalData &fe_data);

        /**
         * @brief Computes the shape function values and gradients on the actual cell
         * and fills the FEValues structure. Specialized variant of previous method for
         * different FETypes given by template parameter.
         */
        template<class MapType>
        void fill_data_specialized(const ElementValues<spacedim> &elm_values, const FEInternalData &fe_data);

        /**
         * !! Temporary function. Use in fill_data.
         * Set shape value @p val of the @p i_point and @p i_func_comp.
         */
        inline void set_shape_value(unsigned int i_point, unsigned int i_func_comp, double val)
        {
            element_data_[patch_data_idx_].shape_values_[i_point][i_func_comp] = val;
        }

        /**
         * !! Temporary function. Use in fill_data.
         * Set shape gradient @p val of the @p i_point and @p i_func_comp.
         */
        inline void set_shape_gradient(unsigned int i_point, unsigned int i_func_comp, arma::vec::fixed<spacedim> val)
        {
            element_data_[patch_data_idx_].shape_gradients_[i_point][i_func_comp] = val;
        }

        /**
         * !! Temporary function. Use in fill_data.
         * @brief Return the value of the @p function_no-th shape function at
         * the @p point_no-th quadrature point.
         *
         * @param function_no Number of the shape function.
         * @param point_no Number of the quadrature point.
         */
        inline double shape_value(const unsigned int function_no, const unsigned int point_no) const
        {
            ASSERT_LT(function_no, this->n_dofs_);
            ASSERT_LT(point_no, this->n_points_);
            return element_data_[patch_data_idx_].shape_values_[point_no][function_no];
        }

        /**
         * !! Temporary function. Use in fill_data.
         * @brief Return the gradient of the @p function_no-th shape function at
         * the @p point_no-th quadrature point.
         *
         * @param function_no Number of the shape function.
         * @param point_no Number of the quadrature point.
         */
        inline arma::vec::fixed<spacedim> shape_grad(const unsigned int function_no, const unsigned int point_no) const
    	{
            ASSERT_LT(function_no, this->n_dofs_);
            ASSERT_LT(point_no, this->n_points_);
            return element_data_[patch_data_idx_].shape_gradients_[point_no][function_no];
        }

        /**
         * @brief Return the product of Jacobian determinant and the quadrature
         * weight at given quadrature point.
         *
         * @param p BulkPoint corresponds to the quadrature point.
         */
        inline double JxW(const BulkPoint &p)
        {
            unsigned int patch_data_idx = element_patch_map_.find(p.elem_patch_idx())->second;
            return element_data_[patch_data_idx].elm_values_->JxW(p.eval_point_idx());
        }

        /**
         * @brief Return the product of Jacobian determinant and the quadrature
         * weight at given quadrature point.
         *
         * @param p SidePoint corresponds to the quadrature point.
         */
        inline double JxW(const SidePoint &p)
        {
            unsigned int patch_data_idx = element_patch_map_.find(p.elem_patch_idx())->second + p.side_idx();
            return element_data_[patch_data_idx].elm_values_->side_JxW(p.local_point_idx());
        }

        /**
         * @brief Return the value of the @p function_no-th shape function at
         * the @p p quadrature point.
         *
         * @param function_no Number of the shape function.
         * @param p BulkPoint corresponds to the quadrature point.
         */
        inline double shape_value(const unsigned int function_no, const BulkPoint &p) const
        {
            ASSERT_LT(function_no, this->n_dofs_);
            unsigned int patch_data_idx = element_patch_map_.find(p.elem_patch_idx())->second;
            return element_data_[patch_data_idx].shape_values_[p.eval_point_idx()][function_no];
        }

        /**
         * @brief Return the value of the @p function_no-th shape function at
         * the @p p quadrature point.
         *
         * @param function_no Number of the shape function.
         * @param p SidePoint corresponds to the quadrature point.
         */
        inline double shape_value(const unsigned int function_no, const SidePoint &p) const
        {
            ASSERT_LT(function_no, this->n_dofs_);
            unsigned int patch_data_idx = element_patch_map_.find(p.elem_patch_idx())->second + p.side_idx();
            return element_data_[patch_data_idx].shape_values_[p.local_point_idx()][function_no];
        }

        /**
         * @brief Return the gradient of the @p function_no-th shape function at
         * the @p p quadrature point.
         *
         * @param function_no Number of the shape function.
         * @param p BulkPoint corresponds to the quadrature point.
         */
        inline arma::vec::fixed<spacedim> shape_grad(const unsigned int function_no, const BulkPoint &p) const
    	{
            ASSERT_LT(function_no, this->n_dofs_);
            unsigned int patch_data_idx = element_patch_map_.find(p.elem_patch_idx())->second;
            return element_data_[patch_data_idx].shape_gradients_[p.eval_point_idx()][function_no];;
        }

        /**
         * @brief Return the gradient of the @p function_no-th shape function at
         * the @p p quadrature point.
         *
         * @param function_no Number of the shape function.
         * @param p SidePoint corresponds to the quadrature point.
         */
        inline arma::vec::fixed<spacedim> shape_grad(const unsigned int function_no, const SidePoint &p) const
    	{
            ASSERT_LT(function_no, this->n_dofs_);
            unsigned int patch_data_idx = element_patch_map_.find(p.elem_patch_idx())->second + p.side_idx();
            return element_data_[patch_data_idx].shape_gradients_[p.local_point_idx()][function_no];;
        }

        /**
         * @brief Returns the normal vector to a side at given quadrature point.
         *
         * @param p SidePoint corresponds to the quadrature point.
         */
    	inline arma::vec::fixed<spacedim> normal_vector(const SidePoint &p)
    	{
            unsigned int patch_data_idx = element_patch_map_.find(p.elem_patch_idx())->second + p.side_idx();
            return element_data_[patch_data_idx].elm_values_->normal_vector(p.local_point_idx());
    	}

        /// Set size of ElementFEData. Important: Use only during the initialization of FESystem !
        void resize(unsigned int max_size) {
            element_data_.resize(max_size);
        }


        /// Dimension of reference space.
        unsigned int dim_;

        /// Number of integration points.
        unsigned int n_points_;

        /// Number of finite element dofs.
        unsigned int n_dofs_;

        /// Type of finite element (scalar, vector, tensor).
        FEType fe_type_;

        /// Dof indices of FESystem sub-elements.
        std::vector<std::vector<unsigned int>> fe_sys_dofs_;

        /// Numbers of components of FESystem sub-elements in reference space.
        std::vector<unsigned int> fe_sys_n_components_;

        /// Numbers of components of FESystem sub-elements in real space.
        std::vector<unsigned int> fe_sys_n_space_components_;

        /// Flags that indicate which finite element quantities are to be computed.
        UpdateFlags update_flags;

        /// Vector of FEValues for sub-elements of FESystem.
        std::vector<PatchFEValues<spacedim>::DimPatchFEValues> fe_values_vec;

        /// Number of components of the FE.
        unsigned int n_components_;

//        /// Auxiliary storage of FEValuesViews accessors.
//        ViewsCache views_cache_;

        /// Precomputed finite element data.
        std::shared_ptr<FEInternalData> fe_data_;

        /// Precomputed FE data (shape functions on reference element) for all side quadrature points.
        std::vector<shared_ptr<FEInternalData> > side_fe_data_;

        /// Patch index of processed element / side.
        unsigned int patch_data_idx_;

        /// Map of element patch indexes to element_data_.
        std::map<unsigned int, unsigned int> element_patch_map_;

        /// Data of elements / sides on patch
        std::vector<ElementFEData> element_data_;

        /// Number of elements / sides on patch. Must be less or equal to size of element_data vector
        unsigned int used_size_;

        /// Maximal number of elements on patch.
        unsigned int max_n_elem_;

        /// Distinguishes using of PatchFEValues for storing data of elements or sides.
        MeshObjectType object_type_;
    };

    /// Temporary helper class used in step between usage old a new implementation
	class FuncDef {
    public:
    	FuncDef() {}
        FuncDef(DimPatchFEValues *cell_data, DimPatchFEValues *side_data, string func_name)
        : cell_data_(cell_data), side_data_(side_data), func_name_(func_name) {}
        DimPatchFEValues *cell_data_;
        DimPatchFEValues *side_data_;
	    string func_name_;
    };
public:

    PatchFEValues(unsigned int n_quad_points)
    : dim_fe_vals_({DimPatchFEValues(n_quad_points), DimPatchFEValues(n_quad_points), DimPatchFEValues(n_quad_points)}),
	  dim_fe_side_vals_({DimPatchFEValues(n_quad_points), DimPatchFEValues(n_quad_points), DimPatchFEValues(n_quad_points)}),
	  n_columns_(0) {
        used_quads_[0] = false; used_quads_[1] = false;
    }

    /**
	 * @brief Initialize structures and calculates cell-independent data.
	 *
	 * @param _quadrature The quadrature rule for the cell associated
     *                    to given finite element or for the cell side.
	 * @param _fe The finite element.
	 * @param _flags The update flags.
	 */
    template<unsigned int DIM>
    void initialize(Quadrature &_quadrature,
                    FiniteElement<DIM> &_fe,
                    UpdateFlags _flags)
    {
        if ( _quadrature.dim() == DIM ) {
            dim_fe_vals_[DIM-1].initialize(_quadrature, _fe, _flags);
            used_quads_[0] = true;
        } else {
            dim_fe_side_vals_[DIM-1].initialize(_quadrature, _fe, _flags);
            used_quads_[1] = true;
        }
    }

    /// Reinit data.
    void reinit(std::array<PatchElementsList, 4> patch_elements)
    {
        for (unsigned int i=0; i<3; ++i) {
            if (used_quads_[0]) dim_fe_vals_[i].reinit(patch_elements[i+1]);
            if (used_quads_[1]) dim_fe_side_vals_[i].reinit(patch_elements[i+1]);
        }
    }

    /**
     * @brief Returns the number of shape functions.
     */
    inline unsigned int n_dofs(unsigned int dim) const
    {
        ASSERT( (dim>0) && (dim<=3) )(dim).error("Invalid dimension!");
        return dim_fe_vals_[dim-1].n_dofs_;
    }

    /**
     * @brief Return the product of Jacobian determinant and the quadrature
     * weight at given quadrature point.
     *
     * @param quad_list List of quadratures.
     */
    inline ElQ<Scalar> JxW(std::vector<Quadrature *> quad_list)
    {
        uint begin = this->n_columns_;
        n_columns_++; // scalar needs one column
        // storing to temporary map
        DimPatchFEValues *cell_data = (quad_list[0] == nullptr) ? nullptr : &dim_fe_vals_[quad_list[0]->dim()-1];
        DimPatchFEValues *side_data = (quad_list[1] == nullptr) ? nullptr : &dim_fe_side_vals_[quad_list[1]->dim()];
        func_map_[begin] = FuncDef(cell_data, side_data, "JxW");
        return ElQ<Scalar>(this, begin);
    }

    /**
     * @brief Returns the normal vector to a side at given quadrature point.
     *
     * @param quad_list List of quadratures.
     */
	inline ElQ<Vector> normal_vector(std::vector<Quadrature *> quad_list)
	{
        uint begin = this->n_columns_;
        n_columns_ += 3; // Vector needs 3 columns
        // storing to temporary map
        DimPatchFEValues *side_data = (quad_list[0] == nullptr) ? nullptr : &dim_fe_side_vals_[quad_list[0]->dim()];
        func_map_[begin] = FuncDef(nullptr, side_data, "normal_vector");
        return ElQ<Vector>(this, begin);
	}

    /**
     * @brief Return the value of the @p function_no-th shape function at
     * the @p p quadrature point.
     *
     * @param quad_list List of quadratures.
     * @param function_no Number of the shape function.
     */
    inline FeQ<Scalar> scalar_shape(std::vector<Quadrature *> quad_list, unsigned int n_comp)
    {
        uint begin = this->n_columns_;
        n_columns_ += n_comp; // scalar needs one column x n_comp
        // storing to temporary map
        DimPatchFEValues *cell_data = (quad_list[0] == nullptr) ? nullptr : &dim_fe_vals_[quad_list[0]->dim()-1];
        DimPatchFEValues *side_data = (quad_list[1] == nullptr) ? nullptr : &dim_fe_side_vals_[quad_list[1]->dim()];
        func_map_[begin] = FuncDef(cell_data, side_data, "shape_value");
        return FeQ<Scalar>(this, begin);
    }

    inline FeQ<Vector> grad_scalar_shape(std::vector<Quadrature *> quad_list, unsigned int n_comp)
    {
        uint begin = this->n_columns_;
        n_columns_ += 3 * n_comp; // scalar needs one column x n_comp
        // storing to temporary map
        DimPatchFEValues *cell_data = (quad_list[0] == nullptr) ? nullptr : &dim_fe_vals_[quad_list[0]->dim()-1];
        DimPatchFEValues *side_data = (quad_list[1] == nullptr) ? nullptr : &dim_fe_side_vals_[quad_list[1]->dim()];
        func_map_[begin] = FuncDef(cell_data, side_data, "shape_grad");
        return FeQ<Vector>(this, begin);
    }

private:
    /// Sub objects of dimensions 1,2,3
    std::array<DimPatchFEValues, 3> dim_fe_vals_;
    std::array<DimPatchFEValues, 3> dim_fe_side_vals_;

    uint n_columns_;  ///< Number of columns

    ///< Temporary helper objects used in step between usage old a new implementation
    bool used_quads_[2];
    std::map<unsigned int, FuncDef> func_map_;

    template <class ValueType>
    friend class ElQ;
    template <class ValueType>
    friend class FeQ;
};


template <class ValueType>
ValueType ElQ<ValueType>::operator()(const BulkPoint &point) {
	auto it = fe_values_->func_map_.find(begin_);
    if (it->second.func_name_ == "JxW") {
        return it->second.cell_data_->JxW(point);
    } else {
        //ASSERT_PERMANENT(false).error("Should not happen.");
        return 0.0;
    }
}

template <>
inline Vector ElQ<Vector>::operator()(FMT_UNUSED const BulkPoint &point) {
    Vector vect; vect.zeros();
    return vect;
}

template <>
inline Tensor ElQ<Tensor>::operator()(FMT_UNUSED const BulkPoint &point) {
	Tensor tens; tens.zeros();
    return tens;
}

template <class ValueType>
ValueType ElQ<ValueType>::operator()(const SidePoint &point) {
	auto it = fe_values_->func_map_.find(begin_);
    if (it->second.func_name_ == "JxW") {
        return it->second.side_data_->JxW(point);
    } else {
        //ASSERT_PERMANENT(false).error("Should not happen.");
        return 0.0;
    }
}

template <>
inline Vector ElQ<Vector>::operator()(const SidePoint &point) {
	auto it = fe_values_->func_map_.find(begin_);
    if (it->second.func_name_ == "normal_vector") {
        return it->second.side_data_->normal_vector(point);
    } else {
        //ASSERT_PERMANENT(false).error("Should not happen.");
        Vector vect; vect.zeros();
        return vect;
    }
}

template <>
inline Tensor ElQ<Tensor>::operator()(FMT_UNUSED const SidePoint &point) {
	Tensor tens; tens.zeros();
    return tens;
}

template <class ValueType>
ValueType FeQ<ValueType>::operator()(unsigned int shape_idx, const BulkPoint &point) {
	auto it = fe_values_->func_map_.find(begin_);
    if (it->second.func_name_ == "shape_value") {
        return it->second.cell_data_->shape_value(shape_idx, point);
    } else {
        //ASSERT_PERMANENT(false).error("Should not happen.");
        return 0.0;
    }
}

template <>
inline Vector FeQ<Vector>::operator()(unsigned int shape_idx, const BulkPoint &point) {
	auto it = fe_values_->func_map_.find(begin_);
    if (it->second.func_name_ == "shape_grad") {
        return it->second.cell_data_->shape_grad(shape_idx, point);
    } else {
        //ASSERT_PERMANENT(false).error("Should not happen.");
        Vector vect; vect.zeros();
        return vect;
    }
}

template <>
inline Tensor FeQ<Tensor>::operator()(FMT_UNUSED unsigned int shape_idx, FMT_UNUSED const BulkPoint &point) {
	Tensor tens; tens.zeros();
    return tens;
}

template <class ValueType>
ValueType FeQ<ValueType>::operator()(unsigned int shape_idx, const SidePoint &point) {
	auto it = fe_values_->func_map_.find(begin_);
    if (it->second.func_name_ == "shape_value") {
        return it->second.side_data_->shape_value(shape_idx, point);
    } else {
        //ASSERT_PERMANENT(false).error("Should not happen.");
        return 0.0;
    }
}

template <>
inline Vector FeQ<Vector>::operator()(unsigned int shape_idx, const SidePoint &point) {
	auto it = fe_values_->func_map_.find(begin_);
    if (it->second.func_name_ == "shape_grad") {
        return it->second.side_data_->shape_grad(shape_idx, point);
    } else {
        //ASSERT_PERMANENT(false).error("Should not happen.");
        Vector vect; vect.zeros();
        return vect;
    }
}

template <>
inline Tensor FeQ<Tensor>::operator()(FMT_UNUSED unsigned int shape_idx, FMT_UNUSED const SidePoint &point) {
	Tensor tens; tens.zeros();
    return tens;
}


#endif /* PATCH_FE_VALUES_HH_ */
