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
 * $Id: bounding_box.hh 1567 2012-02-28 13:24:58Z jan.brezina $
 * $Revision: 1567 $
 * $LastChangedBy: jan.brezina $
 * $LastChangedDate: 2012-02-28 14:24:58 +0100 (Tue, 28 Feb 2012) $
 *
 *
 */

#ifndef BOX_ELEMENT_HH_
#define BOX_ELEMENT_HH_

#include "system/system.hh"
#include "mesh/point.hh"
#include <armadillo>

/**
 * Contains data of bounding box.
 * Used for areas and elements.
 *
 *
 */
class BoundingBox {
public:

	/**
	 * Empty constructor
	 */
	BoundingBox();

	/**
	 * Constructor.
	 *
	 * Set class members
	 * @param minCoor Set value to minCoordinates_
	 * @param maxCoor Set value to maxCoordinates_
	 */
	BoundingBox(arma::vec3 minCoor, arma::vec3 maxCoor);



	BoundingBox(const vector<arma::vec3> &points);




    /// get minimal coordinates of bounding box
    const arma::vec3 get_min() const;
    /// get maximal coordinates of bounding box
    const arma::vec3 get_max() const;
    /// get center coordinates of bounding box
    arma::vec3 get_center() const;

    /**
     * Detects if box element contains point
     *
     * @param point Testing point
     * @return True if box element contains point
     */
    bool contains_point(const Space<3>::Point &point) const;

    /**
     * Returns true if two bounding boxes have intersection.
     * This serves as an estimate of intersection of elements.
     * To make it safe (do not exclude possible intersection) for
     * 1d and 2d elements aligned with axes, we use some tolerance.
     * Since this tolerance is fixed, there could be problem with
     * highly refined meshes (get false positive result).
     */
    bool intersection(const BoundingBox &b2) const;

    /**
     * Set class members
     *
	 * @param minCoor Set value to minCoordinates_
	 * @param maxCoor Set value to maxCoordinates_
     */
    void set_bounds(arma::vec3 minCoor, arma::vec3 maxCoor);

private:
    /// count of dimensions
    static const unsigned int dimension = 3;
    /// stabilization parameter
    static const double epsilon;
    /// minimal coordinates of bounding box
    arma::vec3 minCoordinates_;
    /// maximal coordinates of bounding box
    arma::vec3 maxCoordinates_;
};

#endif /* BOX_ELEMENT_HH_ */
