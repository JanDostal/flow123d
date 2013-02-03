/*!
 *
 * Copyright (C) 2007 Technical University of Liberec.  All rights reserved.
 *
 * Please make a following refer to Flow123d on your project site if you use the program for any purpose,
 * especially for academic research:
 * Flow123d, Research Centre: Advanced Remedial Technologies, Technical University of Liberec, Czech Republic
 *
 * This program is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License version 3 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 021110-1307, USA.
 *
 *
 * $Id$
 * $Revision$
 * $LastChangedBy$
 * $LastChangedDate$
 *
 * @file
 * @brief ???
 *
 */

#ifndef ELEMENTS_H
#define ELEMENTS_H

#include "mesh/nodes.hh"
#include "mesh/region.hh"

#include <materials.hh>

template <int spacedim>
class ElementAccessor;

class Mesh;
class Side;
class SideIter;
struct MaterialDatabase;



//=============================================================================
// STRUCTURE OF THE ELEMENT OF THE MESH
//=============================================================================
class Element
{
public:
    Element();
    Element(unsigned int dim, Mesh *mesh_in, RegionIdx reg);
    void init(unsigned int dim, Mesh *mesh_in, RegionIdx reg);

    inline unsigned int dim() const;
    inline unsigned int index() const;
    unsigned int n_sides() const;    // Number of sides
    unsigned int n_nodes() const; // Number of nodes
    
    ///Gets ElementAccessor of this element
    ElementAccessor<3> element_accessor();
    
    double measure();
    arma::vec3 centre();

    unsigned int n_sides_by_dim(int side_dim);
    inline SideIter side(const unsigned int loc_index);
    Region region() const;
    inline RegionIdx region_idx() const
        { return region_idx_; }
    
    
    


    //int      mid;       // Id # of material
    //int      rid;       // Id # of region
    int      pid;       // Id # of mesh partition

    // Type specific data
    Node** node;    // Element's nodes

//    MaterialDatabase::Iter material; // Element's material

    unsigned int *edge_idx_; // Edges on sides
    unsigned int *boundary_idx_; // Possible boundaries on sides (REMOVE) all bcd assembly should be done through iterating over boundaries
                           // ?? deal.ii has this not only boundary iterators


    int      n_neighs_vb;   // # of neighbours, V-B type (comp.)
                            // only ngh from this element to higher dimension edge
    struct Neighbour **neigh_vb; // List og neighbours, V-B type (comp.)


    Mesh    *mesh_; // should be removed as soon as the element is also an Accessor

protected:
    // Data readed from mesh file
    RegionIdx  region_idx_;
    unsigned int dim_;

    friend class GmshMeshReader;

};




#define FOR_ELEMENT_NODES(i,j)  for((j)=0;(j)<(i)->n_nodes();(j)++)
#define FOR_ELEMENT_SIDES(i,j)  for(unsigned int j=0; j < (i)->n_sides(); j++)
#define FOR_ELM_NEIGHS_VB(i,j)  for((j)=0;(j)<(i)->n_neighs_vb;(j)++)


#endif
//-----------------------------------------------------------------------------
// vim: set cindent:

