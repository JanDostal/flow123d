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
 * @file    mesh.h
 * @brief   
 */

#ifndef MAKE_MESH_H
#define MAKE_MESH_H

#include <mpi.h>                             // for MPI_Comm, MPI_COMM_WORLD
#include <boost/exception/info.hpp>          // for error_info::~error_info<...
//#include <boost/range.hpp>
#include <memory>                            // for shared_ptr
#include <string>                            // for string
#include <vector>                            // for vector, vector<>::iterator
#include "input/accessors.hh"                // for Record, Array (ptr only)
#include "input/accessors_impl.hh"           // for Record::val
#include "input/storage.hh"                  // for ExcStorageTypeMismatch
#include "input/type_record.hh"              // for Record (ptr only), Recor...
#include "mesh/region.hh"                    // for RegionDB, RegionDB::MapE...
#include "mesh/elements.h"
#include "mesh/bounding_box.hh"              // for BoundingBox
#include "mesh/range_wrapper.hh"
#include "mesh/mesh_data.hh"
#include "tools/bidirectional_map.hh"
#include "tools/general_iterator.hh"
#include "system/index_types.hh"             // for LongIdx
#include "system/exceptions.hh"              // for operator<<, ExcStream, EI
#include "system/file_path.hh"               // for FilePath
#include "system/sys_vector.hh"              // for FullIterator, VectorId<>...
#include "system/armor.hh"


class BIHTree;
class Distribution;
class Partitioning;
class MixedMeshIntersections;
class Neighbour;
class SideIter;
class Boundary;
class Edge;
class BCMesh;
class DuplicateNodes;
template <int spacedim> class ElementAccessor;
template <int spacedim> class NodeAccessor;



#define ELM  0
#define BC  1
#define NODE  2

/**
 *  This parameter limits volume of elements from below.
 */
#define MESH_CRITICAL_VOLUME 1.0E-12

class BoundarySegment {
public:
    static Input::Type::Record input_type;
};


/// Base class for Mesh and BCMesh.
class MeshBase {
public:

    MeshBase()
    :  nodes_(3, 1, 0)
    {}

    virtual ~MeshBase() {};

    virtual unsigned int n_edges() const = 0;
    
    unsigned int n_elements() const
    { return element_vec_.size(); }

    virtual unsigned int n_nodes() const = 0;
    virtual unsigned int n_vb_neighbours() const = 0;

    virtual LongIdx *get_el_4_loc() const = 0;
    virtual LongIdx *get_row_4_el() const = 0;
    virtual Distribution *get_el_ds() const = 0;

    const Element &element(unsigned idx) const
    { return element_vec_[idx]; }

    virtual NodeAccessor<3> node(unsigned int idx) const = 0;
    virtual Edge edge(uint edge_idx) const = 0;
    virtual Boundary boundary(uint edge_idx) const = 0;
    virtual const Neighbour &vb_neighbour(unsigned int nb) const = 0;
    virtual ElementAccessor<3> element_accessor(unsigned int idx) const = 0;
    virtual Range<Edge> edge_range() const = 0;
    virtual Range<ElementAccessor<3>> elements_range() const = 0;

    virtual void check_element_size(unsigned int elem_idx) const = 0;
    
    /// Return element id (in GMSH file) of element of given position in element vector.
    int find_elem_id(unsigned int pos) const
    { return element_ids_[pos]; }
    
    virtual const std::vector<unsigned int> &get_side_nodes(unsigned int dim, unsigned int side) const = 0;
    virtual BCMesh *bc_mesh() const = 0;
    virtual const RegionDB &region_db() const = 0;
    virtual const DuplicateNodes *duplicate_nodes() const = 0;


    /**
     * Returns pointer to partitioning object. Partitioning is created during setup_topology.
     */
    virtual Partitioning *get_part() = 0;
    virtual const LongIdx *get_local_part() = 0;

    /*
     * Check if nodes and elements are compatible with \p mesh.
     */
    virtual bool check_compatible_mesh( Mesh & mesh, vector<LongIdx> & bulk_elements_id, vector<LongIdx> & boundary_elements_id ) = 0;

    /**
     * Find intersection of element lists given by Mesh::node_elements_ for elements givne by @p nodes_list parameter.
     * The result is placed into vector @p intersection_element_list. If the @p node_list is empty, and empty intersection is
     * returned.
     */
    void intersect_element_lists(vector<unsigned int> const &nodes_list, vector<unsigned int> &intersection_element_list);

    /// For element of given elem_id returns index in element_vec_ or (-1) if element doesn't exist.
    inline int elem_index(int elem_id) const;

        /// Initialize element_vec_, set size and reset counters of boundary and bulk elements.
    void init_element_vector(unsigned int size);

    /// Initialize node_vec_, set size
    void init_node_vector(unsigned int size);


    /// For each node the vector contains a list of elements that use this node
    vector<vector<unsigned int> > node_elements_;

protected:

    /**
     * Create element lists for nodes in Mesh::nodes_elements.
     */
    void create_node_element_lists();

    /// Adds element to mesh data structures (element_vec_, element_ids_), returns pointer to this element.
    Element * add_element_to_vector(int id, bool is_boundary = false);

    /**
     * Vector of elements of the mesh.
     *
     * Store all elements of the mesh in order bulk elements - boundary elements
     */
    vector<Element> element_vec_;

    /// Maps element ids to indexes into vector element_vec_
    BidirectionalMap<int> element_ids_;

    /**
     * Vector of nodes of the mesh.
     */
    Armor::Array<double> nodes_;

    /// Maps node ids to indexes into vector node_vec_
    BidirectionalMap<int> node_ids_;

};

//=============================================================================
// STRUCTURE OF THE MESH
//=============================================================================

class Mesh : public MeshBase {
public:
    TYPEDEF_ERR_INFO( EI_ElemLast, int);
    TYPEDEF_ERR_INFO( EI_ElemNew, int);
    TYPEDEF_ERR_INFO( EI_RegLast, std::string);
    TYPEDEF_ERR_INFO( EI_RegNew, std::string);
    DECLARE_EXCEPTION(ExcDuplicateBoundary,
            << "Duplicate boundary elements! \n"
            << "Element id: " << EI_ElemLast::val << " on region name: " << EI_RegLast::val << "\n"
            << "Element id: " << EI_ElemNew::val << " on region name: " << EI_RegNew::val << "\n");


    /**
     * \brief Types of search algorithm for finding intersection candidates.
     */
    typedef enum IntersectionSearch {
        BIHsearch  = 1,
        BIHonly = 2,
        BBsearch = 3
    } IntersectionSearch;
    
    /**
     * \brief The definition of input record for selection of variant of file format
     */
    static const Input::Type::Selection & get_input_intersection_variant();
    
    static const unsigned int undef_idx=-1;
    static const Input::Type::Record & get_input_type();


    /** Labels for coordinate indexes in arma::vec3 representing vectors and points.*/
    enum {x_coord=0, y_coord=1, z_coord=2};

    /**
     * Empty constructor.
     *
     * Use only for unit tests!!!
     */
    Mesh();
    /**
     * Constructor from an input record.
     * Do not process input record. That is done in init_from_input.
     */
    Mesh(Input::Record in_record, MPI_Comm com = MPI_COMM_WORLD);

    /// Destructor.
    ~Mesh() override;

    inline unsigned int n_nodes() const override {
        return nodes_.size();
    }

    inline unsigned int n_boundaries() const {
        return boundary_.size();
    }

    inline unsigned int n_edges() const override {
        return edges.size();
    }

    Edge edge(uint edge_idx) const override;
    Boundary boundary(uint edge_idx) const override;

    unsigned int n_corners();

    inline const RegionDB &region_db() const override {
        return region_db_;
    }

    Partitioning *get_part() override;

    const LongIdx *get_local_part() override;

    Distribution *get_el_ds() const override
    { return el_ds; }

    LongIdx *get_row_4_el() const override
    { return row_4_el; }

    LongIdx *get_el_4_loc() const override
    { return el_4_loc; }

    Distribution *get_node_ds() const
    { return node_ds_; }

    LongIdx *get_node_4_loc() const
    { return node_4_loc_; }

    unsigned int n_local_nodes() const
	{ return n_local_nodes_; }

    /**
     * Returns MPI communicator of the mesh.
     */
    inline MPI_Comm get_comm() const { return comm_; }


    MixedMeshIntersections &mixed_intersections();

    unsigned int n_sides() const;

    unsigned int n_vb_neighbours() const override;

    const Neighbour &vb_neighbour(unsigned int nb) const override;

    /**
     * Returns maximal number of sides of one edge, which connects elements of dimension @p dim.
     * @param dim Dimension of elements sharing the edge.
     */
    unsigned int max_edge_sides(unsigned int dim) const { return max_edge_sides_[dim-1]; }

    /**
     * Reads mesh from stream.
     *
     * Method is especially used in unit tests.
     */
    void read_gmsh_from_stream(istream &in);
    /**
     * Reads input record, creates regions, read the mesh, setup topology. creates region sets.
     */
    void init_from_input();


    /**
     * Initialize all mesh structures from raw information about nodes and elements (including boundary elements).
     * Namely: create remaining boundary elements and Boundary objects, find edges and compatible neighborings.
     */
    void setup_topology();
    
    /**
     * Returns vector of ID numbers of elements, either bulk or bc elemnts.
     */
    void elements_id_maps( vector<LongIdx> & bulk_elements_id, vector<LongIdx> & boundary_elements_id) const;

    bool check_compatible_mesh( Mesh & mesh, vector<LongIdx> & bulk_elements_id, vector<LongIdx> & boundary_elements_id ) override;

    /// Create and return ElementAccessor to element of given idx
    ElementAccessor<3> element_accessor(unsigned int idx) const override;

    /// Create and return NodeAccessor to node of given idx
    NodeAccessor<3> node(unsigned int idx) const override;

    /**
     * Reads elements and their affiliation to regions and region sets defined by user in input file
     * Format of input record is defined in method RegionSetBase::get_input_type()
     *
     * @param region_list Array input AbstractRecords which define regions, region sets and elements
     */
    void read_regions_from_input(Input::Array region_list);

    /**
     * Returns nodes_elements vector, if doesn't exist creates its.
     */
    vector<vector<unsigned int> > const & node_elements();

    /// Vector of boundary sides where is prescribed boundary condition.
    /// TODO: apply all boundary conditions in the main assembling cycle over elements and remove this Vector.
    mutable vector<BoundaryData> boundary_;

    //flow::VectorId<int> bcd_group_id; // gives a index of group for an id

    /**
     * Vector of individual intersections of two elements.
     * This is enough for local mortar.
     */
    std::shared_ptr<MixedMeshIntersections>  intersections;

    /**
     * For every element El we have vector of indices into @var intersections array for every intersection in which El is master element.
     * This is necessary for true mortar.
     */
    vector<vector<unsigned int> >  master_elements;
    
    

    /**
     * Vector of compatible neighbourings.
     */
    vector<Neighbour> vb_neighbours_;

    int n_insides; // # of internal sides
    int n_exsides; // # of external sides
    mutable int n_sides_; // total number of sides (should be easy to count when we have separated dimensions

    int n_lines; // Number of line elements
    int n_triangles; // Number of triangle elements
    int n_tetrahedras; // Number of tetrahedra elements

    // Temporary solution for numbering of nodes on sides.
    // The data are defined in RefElement<dim>::side_nodes,
    // Mesh::side_nodes can be removed as soon as Element
    // is templated by dimension.
    //
    // side_nodes[dim][elm_side_idx][side_node_idx]
    // for every side dimension D = 0 .. 2
    // for every element side 0 .. D+1
    // for every side node 0 .. D
    // index into element node array
    vector< vector< vector<unsigned int> > > side_nodes;

    /**
     * Check usage of regions, set regions to elements defined by user, close RegionDB
     */
    void check_and_finish();
    
    /// Compute bounding boxes of elements contained in mesh.
    std::vector<BoundingBox> get_element_boxes();

    /// Getter for BIH. Creates and compute BIH at first call.
    const BIHTree &get_bih_tree();\

    /// Add new node of given id and coordinates to mesh
    void add_node(unsigned int node_id, arma::vec3 coords);

    /// Add new element of given id to mesh
    void add_element(unsigned int elm_id, unsigned int dim, unsigned int region_id, unsigned int partition_id,
    		std::vector<unsigned int> node_ids);

    /// Add new node of given id and coordinates to mesh
    void add_physical_name(unsigned int dim, unsigned int id, std::string name);

    /// Return FilePath object representing "mesh_file" input key
    inline FilePath mesh_file() {
    	return in_record_.val<FilePath>("mesh_file");
    }

    /// Getter for input type selection for intersection search algorithm.
    IntersectionSearch get_intersection_search();

    /// Maximal distance of observe point from Mesh relative to its size
    double global_snap_radius() const;

    /// Returns range of bulk elements
    Range<ElementAccessor<3>> elements_range() const override;

    /// Returns range of nodes
    Range<NodeAccessor<3>> node_range() const;

    /// Returns range of edges
    Range<Edge> edge_range() const override;

    /// For node of given node_id returns index in element_vec_ or (-1) if node doesn't exist.
    inline int node_index(int node_id) const
    {
        return node_ids_.get_position(node_id);
    }

    /// Return node id (in GMSH file) of node of given position in node vector.
    inline int find_node_id(unsigned int pos) const
    {
        return node_ids_[pos];
    }

    /// Check if given index is in element_vec_
    void check_element_size(unsigned int elem_idx) const override;

    /// Permute nodes of 3D elements of given elm_idx
    void permute_tetrahedron(unsigned int elm_idx, std::vector<unsigned int> permutation_vec);

    /// Permute nodes of 2D elements of given elm_idx
    void permute_triangle(unsigned int elm_idx, std::vector<unsigned int> permutation_vec);

    /// Create boundary mesh if doesn't exist and return it.
    BCMesh *bc_mesh() const override;

    const std::vector<unsigned int> &get_side_nodes(unsigned int dim, unsigned int side) const override
    { return side_nodes[dim][side]; }

    const DuplicateNodes *duplicate_nodes() const override
    { return duplicate_nodes_; }

protected:

    /**
     * Part of the constructor whichdoes not depedn on input record.
     * Initializes node-side numbering according to RefElement.
     */
    void init();

    /**
     *  This replaces read_neighbours() in order to avoid using NGH preprocessor.
     *
     *  TODO:
     *  - Avoid maps:
     *
     *    5) need not to have temporary array for Edges, only postpone setting pointers in elements and set them
     *       after edges are found; we can temporary save Edge index instead of pointer in Neigbours and elements
     *
     *    6) Try replace Edge * by indexes in Neigbours and elements (anyway we have mesh pointer in elements so it is accessible also from Neigbours)
     *
     */
    void make_neighbours_and_edges();

    /**
     * On edges sharing sides of many elements it may happen that each side has its nodes ordered in a different way.
     * This method finds the permutation for each side so as to obtain the ordering of side 0.
     */
    void make_edge_permutations();

    /**
     * Remove elements with dimension not equal to @p dim from @p element_list. Index of the first element of dimension @p dim-1,
     * is returned in @p element_idx. If no such element is found the method returns false, if one such element is found the method returns true,
     * if more elements are found we report an user input error.
     */
    bool find_lower_dim_element(vector<unsigned int> &element_list, unsigned int dim, unsigned int &element_idx);

    /**
     * Returns true if side @p si has same nodes as in the list @p side_nodes.
     */
    bool same_sides(const SideIter &si, vector<unsigned int> &side_nodes);


    void element_to_neigh_vb();

    void count_element_types();
    void count_side_types();

    /**
     * Check the element quality and remove unused nodes.
     */
    void check_mesh_on_read();

    /**
     * Possibly modify region id of elements sets by user in "regions" part of input file.
     *
     * TODO: This method needs check in issue 'Review mesh setting'.
     * Changes have been done during generalized region key and may be causing problems
     * during the further development.
     */
    void modify_element_ids(const RegionDB::MapElementIDToRegionID &map);

    /// Initialize element
    void init_element(Element *ele, unsigned int elm_id, unsigned int dim, RegionIdx region_idx, unsigned int partition_id,
    		std::vector<unsigned int> node_ids);

    unsigned int n_bb_neigh, n_vb_neigh;

    /// Maximal number of sides per one edge in the actual mesh (set in make_neighbours_and_edges()).
    unsigned int max_edge_sides_[3];

    /// Output of neighboring data into raw output.
    void output_internal_ngh_data();
    
    /**
     * Database of regions (both bulk and boundary) of the mesh. Regions are logical parts of the
     * domain that allows setting of different data and boundary conditions on them.
     */
    RegionDB region_db_;
    /**
     * Mesh partitioning. Created in setup_topology.
     */
    std::shared_ptr<Partitioning> part_;

    /**
     * BIH Tree for intersection and observe points lookup.
     */
    std::shared_ptr<BIHTree> bih_tree_;


    /**
     * Accessor to the input record for the mesh.
     */
    Input::Record in_record_;

    /**
     * MPI communicator used for partitioning and ...
     */
    MPI_Comm comm_;

    /// Vector of MH edges, this should not be part of the geometrical mesh
    std::vector<EdgeData> edges;


    friend class Edge;
    friend class Side;
    friend class RegionSetBase;
    friend class Element;
    friend class BIHTree;
    friend class Boundary;
    friend class BCMesh;
    template <int spacedim> friend class ElementAccessor;
    template <int spacedim> friend class NodeAccessor;



private:

    /// Fill array node_4_loc_ and create object node_ds_ according to element distribution.
    void distribute_nodes();

    /// Index set assigning to global element index the local index used in parallel vectors.
    LongIdx *row_4_el;
	/// Index set assigning to local element index its global index.
    LongIdx *el_4_loc;
	/// Parallel distribution of elements.
	Distribution *el_ds;
	/// Index set assigning to local node index its global index.
    LongIdx *node_4_loc_;
    /// Parallel distribution of nodes. Depends on elements distribution.
    Distribution *node_ds_;
    /// Hold number of local nodes (own + ghost), value is equal with size of node_4_loc array.
    unsigned int n_local_nodes_;
	/// Boundary mesh, object is created only if it's necessary
	BCMesh *bc_mesh_;
        
    ofstream raw_ngh_output_file;

    DuplicateNodes *duplicate_nodes_;
};

#endif
//-----------------------------------------------------------------------------
// vim: set cindent:
