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
 * @file    accessors_impl.hh
 * @brief   
 */

#ifndef ACCESSORS_IMPL_HH_
#define ACCESSORS_IMPL_HH_

#include "input/factory.hh"

namespace Input {

using std::string;

/******************************************************************************************
 * Implementation of Input::Record
 */
template <class Ret>
inline const Ret Record::val(const string &key) const {
    try {
        Type::Record::KeyIter key_it = get_type_key_iterator(key);

        ASSERT(key_it->default_.is_obligatory() || key_it->default_.has_value_at_declaration())(key)
        		.error("You have to use Record::find instead.");

        Iterator<Ret> it = Iterator<Ret>( *(key_it->type_), address_, key_it->key_index);
        return *it;
    }
    // we catch all possible exceptions
    catch (Type::Record::ExcRecordKeyNotFound & e) {
        throw;
    }
    catch (ExcTypeMismatch & e) {
        e << EI_CPPRequiredType(typeid(Ret).name()) << EI_KeyName(key);
        throw;
    }
    catch (ExcStorageTypeMismatch &e) {
        throw;
    }
    catch (ExcAccessorForNullStorage &e) {
        throw;
    }
}



template <class Ret>
inline const Ret Record::val(const string &key, const Ret default_val ) const {
    try {
        Type::Record::KeyIter key_it = get_type_key_iterator(key);

        ASSERT(key_it->default_.has_value_at_read_time())(key).error("You have to use Record::val or Record::find instead.");

        Iterator<Ret> it = Iterator<Ret>( *(key_it->type_), address_, key_it->key_index);
        if (it)
            return *it;
        else
            return default_val;
    }
    // we catch all possible exceptions
    catch (Type::Record::ExcRecordKeyNotFound & e) {
        throw;
    }
    catch (ExcTypeMismatch & e) {
        e << EI_CPPRequiredType(typeid(Ret).name()) << EI_KeyName(key);
        throw;
    }
    catch (ExcStorageTypeMismatch &e) {
        throw;
    }
    catch (ExcAccessorForNullStorage &e) {
        throw;
    }
}



template <class Ret>
inline Iterator<Ret> Record::find(const string &key) const {
    try {
        Type::Record::KeyIter key_it = get_type_key_iterator(key);
        return Iterator<Ret>( *(key_it->type_), address_, key_it->key_index);
    }
    // we catch all possible exceptions
    catch (Type::Record::ExcRecordKeyNotFound & e) {
        throw;
    }
    catch (ExcTypeMismatch & e) {
        e << EI_CPPRequiredType(typeid(Ret).name()) << EI_KeyName(key);
        throw;
    }
}

template <class Ret>
inline bool Record::opt_val(const string &key, Ret &value) const {
    try {
        Type::Record::KeyIter key_it = get_type_key_iterator(key);
        Iterator<Ret> it=Iterator<Ret>( *(key_it->type_), address_, key_it->key_index);
        if (it) {
            value = *it;
        } else {
            return false;
        }
    }
    // we catch all possible exceptions
    catch (Type::Record::ExcRecordKeyNotFound & e) {
        throw;
    }
    catch (ExcTypeMismatch & e) {
        e << EI_CPPRequiredType(typeid(Ret).name()) << EI_KeyName(key);
        throw;
    }

    return true;
}


/******************************************************************************************
 * Implementation of Input::AbstractRecord
 */

template<class Type, class... Arguments>
const std::shared_ptr<Type> AbstractRecord::factory(Arguments... arguments) const {
	return Input::Factory<Type, Arguments...>::instance()->create(this->type().type_name(), arguments...);
}


/******************************************************************************************
 * Implementation of Input::Array
 */

template <class ValueType>
inline Iterator<ValueType> Array::begin() const {
    try {
        return Iterator<ValueType>(array_type_.get_sub_type(), address_, 0);
    }
    catch (ExcTypeMismatch & e) {
        e << EI_CPPRequiredType(typeid(ValueType).name()) << EI_KeyName("begin()");
        throw e;
    }
}



inline IteratorBase Array::end() const {
	return IteratorBase(address_, address_.storage_head()->get_array_size());
}



inline unsigned int Array::size() const {
    return address_.storage_head()->get_array_size();
}



template <class Container>
void Array::copy_to(Container &out) const {
    out.clear();
    Iterator<typename Container::value_type> it = begin<typename Container::value_type>();

    for(;it != end(); ++ it) {
        out.push_back(*it);
    }
}


/******************************************************************************************
 * Implementation of Input::IteratorBase
 */

inline bool IteratorBase::operator == (const IteratorBase &that) const
        { return ( address_.storage_head()  == that.address_.storage_head()  && index_ == that.index_); }



inline bool IteratorBase::operator != (const IteratorBase &that) const
        { return ! ( *this == that ); }



inline IteratorBase::operator bool() const {
	StorageBase *s = address_.storage_head()->get_item(index_);
    return ( s && ! s->is_null() );
}



inline unsigned int IteratorBase::idx() const {
    return index_;
}


/******************************************************************************************
 * Implementation of Input::Iterator<Type>
 */


template<class T>
inline Iterator<T> & Iterator<T>::operator ++() {
    index_++;
    return *this;
}



template<class T>
inline Iterator<T> & Iterator<T>::operator --() {
    index_--;
    return *this;
}



template<class T>
inline typename Iterator<T>::OutputType Iterator<T>::operator *() const {

    auto new_address =address_.down(index_);

    ASSERT_PTR(new_address->storage_head()).error();

    return internal::TypeDispatch < DispatchType > ::value(*new_address, type_);
}

template<class T>
inline typename Iterator<T>::OutputType * Iterator<T>::operator ->() const {
	// OutputType has to be an accessor to call its method, e.g. iter->val("xyz"). Variable iter has to be e.g. Iterator.
    BOOST_STATIC_ASSERT(
            (boost::is_same < Record, OutputType > ::value || boost::is_same < AbstractRecord, OutputType > ::value
                    || boost::is_same < Array, OutputType > ::value || boost::is_same < Tuple, OutputType > ::value));

    // we have to make save temporary
    temporary_value_ = this->operator*();
    return &(temporary_value_);

}


template<class T>
typename Iterator<T>::InputType Iterator<T>::type_check_and_convert(const Input::Type::TypeBase &type) {
    if (typeid(type) == typeid(InputType)) {
        return static_cast<const InputType &>(type);
    } else {
        THROW(ExcTypeMismatch() << EI_InputType(type.type_name()) << EI_RequiredType(typeid(InputType).name()));
    }
}


} // namespace Input

#endif /* ACCESSORS_IMPL_HH_ */
