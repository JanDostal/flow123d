# 
# Copyright (C) 2007 Technical University of Liberec.  All rights reserved.
#
# Please make a following refer to Flow123d on your project site if you use the program for any purpose,
# especially for academic research:
# Flow123d, Research Centre: Advanced Remedial Technologies, Technical University of Liberec, Czech Republic
#
# This program is free software; you can redistribute it and/or modify it under the terms
# of the GNU General Public License version 3 as published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this program; if not,
# write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 021110-1307, USA.
#
# $Id: CMakeLists.txt 1567 2012-02-28 13:24:58Z jan.brezina $
# $Revision: 1567 $
# $LastChangedBy: jan.brezina $
# $LastChangedDate: 2012-02-28 14:24:58 +0100 (Tue, 28 Feb 2012) $
#

#set(CMAKE_INCLUDE_CURRENT_DIR ON)



set(libs input_lib)
add_test_directory("${libs}")


### define_test(enum_macro)

define_test(type_base)
define_test(type_output)
define_test(type_record)
define_test(type_abstract)
define_test(type_generic)
define_test(type_tuple)
define_test(type_selection)

define_test(storage)
define_test(comment_filter)
define_test(reader_to_storage)
define_test(path_base)
define_test(input_interface)
define_test(type_use_case)
define_test(address)
define_test(reference)
define_test(yaml_include)

#define_test(lazy_types)
define_test(type_repository)

define_test(factory)
define_test(type_dispatch)

define_test(ist_finish)

define_test(yaml_lib)
define_test(json_spirit)






