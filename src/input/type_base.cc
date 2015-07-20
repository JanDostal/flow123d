/*
 * input_type.cc
 *
 *  Created on: Mar 29, 2012
 *      Author: jb
 */



#include <limits>
#include <ios>
#include <map>
#include <vector>
#include <string>
#include <iomanip>

#include "system/system.hh"

#include <boost/type_traits.hpp>
#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/functional/hash.hpp>


#include "type_base.hh"
#include "type_record.hh"
#include "type_output.hh"
#include "type_repository.hh"
#include "type_generic.hh"
#include "json_spirit/json_spirit.h"
#include <boost/algorithm/string.hpp>


namespace Input {
namespace Type {

using namespace std;



/*******************************************************************
 * implementation of TypeBase
 */



TypeBase::TypeBase()
: attributes_( boost::make_shared<attribute_map>() ), generic_(false) {}



TypeBase::TypeBase(const TypeBase& other)
: attributes_(other.attributes_), generic_(other.generic_) {}



TypeBase::~TypeBase() {}


bool TypeBase::is_valid_identifier(const string& key) {
  namespace ba = boost::algorithm;
  return ba::all( key, ba::is_lower() || ba::is_digit() || ba::is_any_of("_") );
}


string TypeBase::desc() const {
    stringstream ss;
    ss << OutputText(this,1);
    return ss.str();
}



void TypeBase::lazy_finish() {
	Input::TypeRepository<Record>::get_instance().finish();
	Input::TypeRepository<AbstractRecord>::get_instance().finish();
	Input::TypeRepository<Selection>::get_instance().finish();
}



void TypeBase::add_attribute(std::string name, json_string val) {
	ASSERT( !this->is_closed(), "Attribute can be add only to non-closed type: '%s'.\n", this->type_name().c_str());
	if (validate_json(val)) {
		(*attributes_)[name] = val;
	} else {
		xprintf(PrgErr, "Invalid JSON format of attribute '%s'.\n", name.c_str());
	}
}


void TypeBase::write_attributes(ostream& stream) const {
	stream << "\"attributes\" : {" << endl;
	for (std::map<std::string, json_string>::iterator it=attributes_->begin(); it!=attributes_->end(); ++it) {
        if (it != attributes_->begin()) {
        	stream << "," << endl;
        }
		stream << "\"" << it->first << "\" : " << it->second;
	}
	stream << endl << "}";
}


bool TypeBase::validate_json(json_string str) const {
    try {
    	json_spirit::mValue node;
    	json_spirit::read_or_throw( str, node);
    	return true;
    } catch (json_spirit::Error_position &e ) {
        return false;
    }
}


void TypeBase::attribute_content_hash(std::size_t &seed) const {
	for (attribute_map::iterator it=attributes_->begin(); it!=attributes_->end(); it++) {
		boost::hash_combine(seed, (*it).first );
		boost::hash_combine(seed, (*it).second );
	}

}


void TypeBase::add_to_parameter_map(ParameterMap other) {
	parameter_map_.insert(other.begin(), other.end());
}


TypeBase::json_string TypeBase::print_parameter_map_to_json() {
	std::stringstream ss;
	ss << "[";
	for (ParameterMap::iterator it=parameter_map_.begin(); it!=parameter_map_.end(); it++) {
		if (it != parameter_map_.begin()) ss << "," << endl;
		ss << "{ \"" << (*it).first << "\" : \"" << (*it).second << "\" }";
	}
	ss << "]";
	return ss.str();
}







std::ostream& operator<<(std::ostream& stream, const TypeBase& type) {
    return ( stream << OutputText(&type, 1) );
}



/**********************************************************************************
 * implementation of Type::Array
 */

TypeBase::TypeHash Array::content_hash() const
{
	TypeHash seed=0;
    boost::hash_combine(seed, type_name());
    boost::hash_combine(seed, data_->lower_bound_);
    boost::hash_combine(seed, data_->upper_bound_);
    boost::hash_combine(seed, data_->type_of_values_->content_hash() );
    attribute_content_hash(seed);
    return seed;
}


bool Array::finish() {
	return data_->finish();
}



bool Array::ArrayData::finish()
{
	if (finished) return true;

    ASSERT(typeid( *type_of_values_ ) != typeid(Parameter), "Finished Array can't be of type Parameter '%s'.\n",
    		type_of_values_->type_name().c_str());

	return (finished = type_of_values_->finish() );
}



string Array::type_name() const {
    return "array_of_" + data_->type_of_values_->type_name();
}

string Array::full_type_name() const {
	return "array_of_" + data_->type_of_values_->type_name();
}



bool Array::operator==(const TypeBase &other) const    {
    return  typeid(*this) == typeid(other) &&
              (*data_->type_of_values_ == static_cast<const Array *>(&other)->get_sub_type() );
}



bool Array::valid_default(const string &str) const {
    if ( this->match_size( 1 ) ) {
        return get_sub_type().valid_default( str );
    } else {
        THROW( ExcWrongDefault() << EI_DefaultStr( str ) << EI_TypeName(type_name()));
    }
}


boost::shared_ptr<TypeBase> Array::make_instance(std::vector<ParameterPair> vec) const {
	// Replace only if type_of_values_ is parameter
	if ( typeid( *(data_->type_of_values_) ) == typeid(Parameter) ) {
		// Create copy of array, we can't set type from parameter vector directly (it's TypeBase that is not allowed)
		Array arr = Array(Integer(), this->data_->lower_bound_, this->data_->upper_bound_);
		// Replace parameter stored in type_of_values_
		bool found = false;
		for (std::vector<ParameterPair>::iterator vec_it=vec.begin(); vec_it!=vec.end(); vec_it++) {
			if ( (*vec_it).first == data_->type_of_values_->type_name() ) {
				found = true;
				arr.data_->type_of_values_ = (*vec_it).second;
				arr.parameter_map_[(*vec_it).first] = (*vec_it).second->content_hash();
			}
		}
		ASSERT(found, "Parameterized type_of_values_ in make_instance method of '%s' Array wasn't replaced!\n",
				this->type_name().c_str());
		// Copy attributes
		arr.attributes_ = boost::make_shared<attribute_map>(*attributes_);
		// Set parameters as attribute
		json_string val = arr.print_parameter_map_to_json();
		ASSERT( this->validate_json(val), "Invalid JSON format of attribute 'parameters'.\n" );
		(*arr.attributes_)["parameters"] = val;
		std::stringstream type_stream;
		type_stream << "\"" << this->content_hash() << "\"";
		(*arr.attributes_)["generic_type"] = type_stream.str();

		return boost::make_shared<Array>(arr);
	}

	return boost::make_shared<Array>(*this);
}


/**********************************************************************************
 * implementation and explicit instantiation of Array constructor template
 */

template <class ValueType>
Array::Array(const ValueType &type, unsigned int min_size, unsigned int max_size)
: data_(boost::make_shared<ArrayData>(min_size, max_size))
{
    // ASSERT MESSAGE: The type of declared keys has to be a class derived from TypeBase.
    BOOST_STATIC_ASSERT( (boost::is_base_of<TypeBase, ValueType >::value) );
    ASSERT( min_size <= max_size, "Wrong limits for size of Input::Type::Array, min: %d, max: %d\n", min_size, max_size);
    ASSERT( type.is_closed(), "Sub-type '%s' of Input::Type::Array must be closed!", type.type_name().c_str());

    // for Parameter type sets that array is generic
    if (typeid(type) == typeid(Parameter)) {
    	this->generic_ = true;
    }

	boost::shared_ptr<TypeBase> type_copy = boost::make_shared<ValueType>(type);
	data_->type_of_values_ = type_copy;
}

// explicit instantiation

#define ARRAY_CONSTRUCT(TYPE) \
template Array::Array(const TYPE &type, unsigned int min_size, unsigned int max_size)

ARRAY_CONSTRUCT(String);
ARRAY_CONSTRUCT(Integer);
ARRAY_CONSTRUCT(Double);
ARRAY_CONSTRUCT(Bool);
ARRAY_CONSTRUCT(FileName);
ARRAY_CONSTRUCT(Selection);
ARRAY_CONSTRUCT(Array);
ARRAY_CONSTRUCT(Record);
ARRAY_CONSTRUCT(AbstractRecord);
ARRAY_CONSTRUCT(Parameter);
ARRAY_CONSTRUCT(Instance);


/**********************************************************************************
 * implementation of Type::Scalar ... and descendants.
 */

string Scalar::full_type_name() const {
    return type_name();
}

/**********************************************************************************
 * implementation of Type::Bool
 */


TypeBase::TypeHash Bool::content_hash() const
{
	TypeHash seed=0;
    boost::hash_combine(seed, type_name());
    return seed;
}


bool Bool::valid_default(const string &str) const {
    from_default(str);
    return true;
}



bool Bool::from_default(const string &str) const {
    if (str == "true" )  {
        return true;
    } else
    if (str == "false") {
        return false;
    } else {
        THROW( ExcWrongDefault() << EI_DefaultStr( str ) << EI_TypeName(type_name()));
    }
}


string Bool::type_name() const {
    return "Bool";
}


boost::shared_ptr<TypeBase> Bool::make_instance(std::vector<ParameterPair> vec) const {
	return boost::make_shared<Bool>(*this);
}

/**********************************************************************************
 * implementation of Type::Integer
 */

TypeBase::TypeHash Integer::content_hash() const
{
	TypeHash seed=0;
    boost::hash_combine(seed, type_name());
    boost::hash_combine(seed, lower_bound_);
    boost::hash_combine(seed, upper_bound_);
    return seed;
}



bool Integer::match(std::int64_t value) const {
    return ( value >=lower_bound_ && value <= upper_bound_);
}



int Integer::from_default(const string &str) const {
    std::istringstream stream(str);
    int value;
    stream >> value;

    if (stream && stream.eof() && match(value)) {
        return value;
    } else {
        THROW( ExcWrongDefault() << EI_DefaultStr( str ) << EI_TypeName(type_name()));
    }
}



bool Integer::valid_default(const string &str) const
{
    from_default(str);
    return true;
}



string Integer::type_name() const {
    return "Integer";
}


boost::shared_ptr<TypeBase> Integer::make_instance(std::vector<ParameterPair> vec) const {
	return boost::make_shared<Integer>(*this);
}


/**********************************************************************************
 * implementation of Type::Double
 */


TypeBase::TypeHash Double::content_hash() const
{
	TypeHash seed=0;
    boost::hash_combine(seed, type_name());
    boost::hash_combine(seed, lower_bound_);
    boost::hash_combine(seed, upper_bound_);
    return seed;
}



bool Double::match(double value) const {
    return ( value >=lower_bound_ && value <= upper_bound_);
}



double Double::from_default(const string &str) const {
    std::istringstream stream(str);
    double value;
    stream >> value;

    if (stream && stream.eof() && match(value)) {
        return value;
    } else {
        THROW( ExcWrongDefault() << EI_DefaultStr( str ) << EI_TypeName(type_name()));
    }
}



bool Double::valid_default(const string &str) const
{
    from_default(str);
    return true;
}




string Double::type_name() const {
    return "Double";
}


boost::shared_ptr<TypeBase> Double::make_instance(std::vector<ParameterPair> vec) const {
	return boost::make_shared<Double>(*this);
}


/**********************************************************************************
 * implementation of Type::FileName
 */

TypeBase::TypeHash FileName::content_hash() const
{
	TypeHash seed=0;
    boost::hash_combine(seed, type_name());
    boost::hash_combine(seed, type_);
    return seed;
}





string FileName::type_name() const {
    switch (type_) {
    case ::FilePath::input_file:
        return "FileName_input";
    case ::FilePath::output_file:
        return "FileName_output";
    default:
        return "FileName";
    }
}



bool FileName::match(const string &str) const {
    return (type_ == ::FilePath::input_file) || (str[0] != DIR_DELIMITER); // output files can not be absolute
}


/**********************************************************************************
 * implementation of Type::String
 */


TypeBase::TypeHash String::content_hash() const
{
	TypeHash seed=0;
    boost::hash_combine(seed, type_name());
    return seed;
}



string String::type_name() const {
    return "String";
}




bool String::valid_default(const string &str) const {
    if (! match(str)) {
        THROW( ExcWrongDefault() << EI_DefaultStr( str ) << EI_TypeName(type_name()));
    }
    return true;
}



string String::from_default(const string &str) const {
    valid_default(str);
    return str;
}



bool String::match(const string &str) const {
    return true;
}



boost::shared_ptr<TypeBase> String::make_instance(std::vector<ParameterPair> vec) const {
	return boost::make_shared<String>(*this);
}



} // closing namespace Type
} // closing namespace Input



