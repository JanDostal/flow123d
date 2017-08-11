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
 * @file    msh_basereader.hh
 * @brief
 * @author  dalibor
 */

#ifndef MSH_BASE_READER_HH
#define	MSH_BASE_READER_HH


#include <string>
#include <vector>
#include <istream>
#include <memory>

#include "io/element_data_cache.hh"
#include "mesh/mesh.h"
#include "input/accessors.hh"
#include "system/system.hh"
#include "system/tokenizer.hh"



/// Types of VTK data (value 'undefined' for empty value)
enum DataType {
	int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, float64, undefined
};



/**
 * Abstract parent of mesh readers.
 *
 * Supported are:
 *  - GMSH reader (class GmshMeshReader)
 *  - VTK reader (class VtkMeshReader)
 */
class BaseMeshReader {
public:
	TYPEDEF_ERR_INFO(EI_FieldName, std::string);
	TYPEDEF_ERR_INFO(EI_Time, double);
	TYPEDEF_ERR_INFO(EI_MeshFile, std::string);
	TYPEDEF_ERR_INFO(EI_Type, std::string);
	TYPEDEF_ERR_INFO(EI_TokenizerMsg, std::string);
	TYPEDEF_ERR_INFO(EI_FileExtension, std::string);
	DECLARE_INPUT_EXCEPTION(ExcFieldNameNotFound,
			<< "No data for field: "<< EI_FieldName::qval
			<< " and time: "<< EI_Time::val
			<< " in the input file: "<< EI_MeshFile::qval);
	DECLARE_INPUT_EXCEPTION(ExcMissingFieldDiscretization,
			<< "Missing data type specification for field: "<< EI_FieldName::qval
			<< " and time: "<< EI_Time::val
			<< " in the input file: "<< EI_MeshFile::qval
			<< "\nPlease, add value of key 'input_discretization'.");
	DECLARE_EXCEPTION(ExcWrongFormat,
			<< "Wrong format of " << EI_Type::val << ", " << EI_TokenizerMsg::val << "\n"
			<< "in the input file: " << EI_MeshFile::qval);
	DECLARE_EXCEPTION(ExcWrongExtension,
			<< "Unsupported extension " << EI_FileExtension::qval << " of the input file: " << EI_MeshFile::qval);


    /**
     * Specify section where to find the field, some sections are specific to file format.
     */
	enum Discretization {
		node_data,         ///< point_data (VTK) / node_data (GMSH)
		element_data,      ///< cell_data (VTK) / element_data (GMSH)
		element_node_data, ///< element_node_data (only for VTK)
		native_data,       ///< native_data (only for GMSH)
		mesh_definition,   ///< special case of section (of VTK) that defines mesh
		undefined          ///< section is not specified by user (requires control specific for concrete usage)
	};

	/**
	 * Store base data that determines native data of VTK file.
	 *
	 * Structure is filled in get_element data and has effect only for VTK files.
	 */
	struct DiscretizationParams {
		Discretization discretization; ///< Flag marks native data of VTK file
        std::size_t dof_handler_hash;  ///< Hash of DOF handler object
	};

	/// Constructor
	BaseMeshReader(const FilePath &file_name);

    /**
     * This static method gets file path object of reader,
     * dispatch to correct constructor and initialize appropriate function object from the input.
     * Returns shared pointer to BaseMeshReader.
     */
    static std::shared_ptr< BaseMeshReader > reader_factory(const FilePath &file_name);

    /**
     * This static method gets accessor to record with function input,
     * dispatch to correct constructor and initialize appropriate function object from the input.
     * Returns pointer to Mesh.
     */
    static Mesh * mesh_factory(const Input::Record &input_mesh_rec);

    /**
     * Reads @p raw data of mesh (only nodes and elements) from the GMSH or VTKfile.
     * Input of the mesh allows changing regions within the input file.
     *
     */
    void read_raw_mesh(Mesh * mesh);

    /**
     * Read regions from the mesh file and save the physical sections as regions in the RegionDB.
     */
    virtual void read_physical_names(Mesh * mesh)=0;

    /**
     * Prepare data header for reading ElementData sections of opened mesh file. The file is searched for the \\$ElementData
     * (GMSH) or DataArray (VTK) section with header that match the given @p field_name, @p time and discretization of the
     * next section is the first greater then that given in input parameters).
     *
     * This method must be call before \p get_element_data method!
     *
     *  @param field_name field name
     *  @param time searched time
     *  @param disc_params struct with parameters 'discretization' (can be define on input) and 'dof_handler_hash' used only for native data
     */
    void set_actual_data_header( std::string field_name, double time, DiscretizationParams &disc_params);

    /**
     *  Reads ElementData sections of opened mesh file. Method must be call after \p set_data_header method. If such section
     *  has not been yet read, we read the data section into raw buffer @p data. The buffer must have size at least
     *  @p n_components * @p n_entities. Indexes in the map must be smaller then @p n_entities.
     *
     *  Possible optimizations:
     *  If the map ID lookup seem slow, we may assume that IDs are in increasing order, use simple array of IDs instead of map
     *  and just check that they comes in in correct order.
     *
     *  @param n_entities count of entities (elements)
     *  @param n_components count of components (size of returned data is given by n_entities*n_components)
     *  @param boundary_domain flag determines that data is read for boundary or bulk elements
     *  @param component_idx component index of MultiField
	 */
    template<typename T>
    typename ElementDataCache<T>::ComponentDataPtr get_element_data( unsigned int n_entities, unsigned int n_components,
    		bool boundary_domain, unsigned int component_idx);

    /**
     * Check if nodes and elements of reader mesh is compatible with \p mesh.
     */
    virtual void check_compatible_mesh(Mesh &mesh)=0;

    /**
     * Returns vector of boundary or bulk element ids by parameter boundary_domain
     */
    std::vector<int> const & get_element_vector(bool boundary_domain);

protected:
    typedef std::shared_ptr<ElementDataCacheBase> ElementDataPtr;
    typedef std::map< string, ElementDataPtr > ElementDataFieldMap;

    /***********************************
     * Structure to store the information from a header of \\$ElementData (GMSH file) or DataArray (VTK file) section.
     *
     * Format of GMSH ASCII data sections
     *
       number-of-string-tags (== 2)
         field_name
         interpolation_scheme_name
       number-of-real-tags (==1)
         time_of_dataset
       number-of-integer-tags
         time_step_index (starting from zero)
         number_of_field_components (1, 3, or 9 - i.e. 3d scalar, vector or tensor data)
         number_of entities (nodes or elements)
         partition_index (0 == no partition, not clear if GMSH support reading different partition from different files)
       elm-number value ...
    *
    */
    struct MeshDataHeader {
    	/// Set field_name value to empty string, that signs invalid header (using after reading data)
    	void reset() { field_name = ""; }
        /// Name of field
    	std::string field_name;
        /// Currently d'ont used
        std::string interpolation_scheme;
        /// Time of field data (used only for GMSH reader)
        double time;
        /// Currently d'ont used
        unsigned int time_index;
        /// Number of values on one row
        unsigned int n_components;
        /// Number of rows
        unsigned int n_entities;
        /// Currently d'ont used
        unsigned int partition_index;
        /// Position of data in mesh file
        Tokenizer::Position position;
        /// Type of data (used only for VTK reader)
        DataType type;
        /// Flag marks input discretization of data of VTK file
        Discretization discretization;
        /// Hash of DOF handler object (only for native data of VTK file)
        std::size_t dof_handler_hash;
    };

	/// Constructor
	BaseMeshReader(const FilePath &file_name, std::shared_ptr<ElementDataFieldMap> element_data_values);

	/**
     * private method for reading of nodes
     */
    virtual void read_nodes(Mesh * mesh)=0;

    /**
     * Method for reading of elements.
     */
    virtual void read_elements(Mesh * mesh)=0;

    /**
	 * Find data header for given time and field.
	 */
	virtual MeshDataHeader & find_header(double time, std::string field_name, DiscretizationParams &disc_params)=0;

    /**
     * Reads table of data headers specific for each descendants.
     */
    virtual void make_header_table()=0;

    /**
     * Read element data to data cache
     */
    virtual void read_element_data(ElementDataCacheBase &data_cache, MeshDataHeader actual_header, unsigned int n_components,
    		bool boundary_domain)=0;

    /**
     * Flag stores that check of compatible mesh was performed.
     *
     * This flag has effect only for VTK reader.
     */
    bool has_compatible_mesh_;

    /// Store name of field data section specify for type of mesh file.
    std::string data_section_name_;

    /// Cache with last read element data
    std::shared_ptr<ElementDataFieldMap> element_data_values_;

    /// Tokenizer used for reading ASCII file format.
    Tokenizer tok_;

    /// Vector of both bulk and boundary IDs. Bulk elements come first, then boundary elements, but only the portion that appears
    /// in input mesh file and has ID assigned.
    vector<int> bulk_elements_id_, boundary_elements_id_;

    /// Header of actual loaded data.
    MeshDataHeader actual_header_;
};

#endif	/* MSH_BASE_READER_HH */
