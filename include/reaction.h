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

#ifndef REACTION_H_
#define REACTION_H_

//=============================================================================
// STRUCTURE OF THE REACTION
/*! @brief STRUCTURE OF THE REACTION (ONE SPECIE)
 *
 *
 */
//=============================================================================
struct Reaction
{
    int                     id; // reaction ID
    int                     sbi; // substance ID
    int                     type; // type of reaction
    double                  *coef; // type dependent coefficent set
};

void read_reaction_list( struct Transport *transport );
void transport_reaction(struct Transport *transport, int elm_pos, int sbi);

#endif /* REACTION_H_ */
