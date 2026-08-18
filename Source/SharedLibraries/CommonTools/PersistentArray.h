#pragma once

#ifdef _WINDOWS
#include <afxtempl.h>		// MFC support for template container classes (CArray).
#else
#include "Exceptions.h"
#endif

#include "PersistentObject.h"

////////////////////////////////////////////////////////////////////////////////////////////
// PersistentArray:
////////////////////////////////////////////////////////////////////////////////////////////


template<class Type>
class PersistentArray : public CArray<Type, Type&>, public PersistentObject
{
	const int cur_schema = 1;	// Current class version number.

public:
	using value_type = Type;
	using reference = Type&;
	using const_reference = const Type&;
	using difference_type = std::ptrdiff_t;

	PersistentArray() {  } //Cld it seems not necessary, but the C++ compiler crashes without them (SetupMenuDlg on "config->groups[i].buttons.GetSize()"
	PersistentArray(const PersistentArray& arg) { *this=arg; } //Cld it seems not necessary, but the C++ compiler crashes without them (SetupMenuDlg on "config->groups[i].buttons.GetSize()"
	virtual void Serialize(CArchive& ar); // The default behaviour for CArray is a memcpy of each element.

	Type& LastElement() { return this->ElementAt(this->GetUpperBound()); }
	const Type& LastElement() const { return this->ElementAt(this->GetUpperBound()); }

	PersistentArray<Type>& operator=(const PersistentArray<Type>& rhs) {
		if (&rhs!=this) this->Copy(rhs); return *this;
	}

	/**
	 * Iterator class for the PersistentArray.
	 * This is needed for compatibility with std::range::ranges concepts
	 */
	class iterator
	{
	private:
		static const int no_index = -1;
		PersistentArray<Type>* m_instance;
		int m_index;

	public:
      using iterator_category = std::random_access_iterator_tag;
      using value_type = Type;
      using reference = Type&;
      using pointer = Type*;
      using difference_type = int;

		iterator()
			: m_instance(nullptr), m_index(no_index) {}
		iterator(PersistentArray<Type>* instance, int index)
			: m_instance(instance), m_index(index) {}
		iterator(const PersistentArray<Type>* instance, int index)
			: m_instance(const_cast<PersistentArray<Type>*>(instance)), m_index(index) {}
		iterator(const iterator& rhs)
			: m_instance(rhs.m_instance), m_index(rhs.m_index) {}

    
      inline iterator& operator+=(difference_type rhs) {
			if (this->m_instance == nullptr) return *this;

			this->m_index += rhs; 
			if (this->m_index < 0 || this->m_index >= this->m_instance->GetSize()) {
				*this = iterator();
			}

			return *this; 
		}

      inline iterator& operator-=(difference_type rhs) {
			if (this->m_instance == nullptr) return *this;

			this->m_index -= rhs;
			if (this->m_index < 0 || this->m_index >= this->m_instance->GetSize()) {
				*this = iterator();
			}

			return *this;
		}

		inline Type& operator*() const { return (*this->m_instance)[this->m_index]; }
      inline Type* operator->() const { return &(*this->m_instance)[this->m_index]; }
      inline Type& operator[](difference_type rhs) const { return (*this->m_instance)[rhs]; }
        
		inline iterator& operator++() {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");

			this->m_index++;

			if (this->m_index < 0 || this->m_index >= this->m_instance->GetSize()) {
				*this = iterator();
			}

			return *this;
		}

      inline iterator& operator--() {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");

			this->m_index--;

			if (this->m_index < 0 || this->m_index >= this->m_instance->GetSize()) {
				*this = iterator();
			}

			return *this;
		}

      inline iterator operator++(int) const { iterator tmp(*this); ++(*this); return tmp; }
      inline iterator operator--(int) const { iterator tmp(*this); --(*this); return tmp; }
        
      inline difference_type operator-(const iterator& rhs) const {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");
			if (this->m_instance != rhs.m_instance) throw std::invalid_argument("different instances");

			int index1 = this->m_index;
			int index2 = rhs.m_index;

			if (index1 == -1) {
				index1 = this->m_instance->GetSize();
			}

			if (index2 == -1) {
				index2 = rhs.m_instance->GetSize();
			}

			return index1 - index2;
		}

      inline iterator operator+(difference_type rhs) const {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");

			iterator result;

			result.m_index = this->m_index + rhs;

			if (result.m_index < 0 || result.m_index >= this->m_instance->GetSize()) {
				result = iterator();
			}

			return result;
		}

      inline iterator operator-(difference_type rhs) const {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");

			iterator result;

			result.m_index = this->m_index - rhs;

			if (result.m_index < 0 || result.m_index >= this->m_instance->GetSize()) {
				result = iterator();
			}

			return result;
		}
      
		friend inline iterator operator+(difference_type lhs, const iterator& rhs) {
			iterator result(rhs);
			result += lhs;
			return result;
		}

		friend inline iterator operator-(difference_type lhs, const iterator& rhs) {
			iterator result(rhs);
			result -= lhs;
			return result;
		}
      
		inline bool operator==(const iterator& rhs) const { return this->m_instance == rhs.m_instance && this->m_index == rhs.m_index; }
      inline bool operator!=(const iterator& rhs) const { return !(*this == rhs); }
		inline bool operator>(const iterator& rhs) const {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");
			if (this->m_instance != rhs.m_instance) throw std::invalid_argument("iterator from different instance");

			return this->m_index > rhs.m_index;
		}

      inline bool operator<(const iterator& rhs) const {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");
			if (this->m_instance != rhs.m_instance) throw std::invalid_argument("iterator from different instance");

			return this->m_index < rhs.m_index;
		}

      inline bool operator>=(const iterator& rhs) const {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");
			if (this->m_instance != rhs.m_instance) throw std::invalid_argument("iterator from different instance");

			return this->m_index >= rhs.m_index;
		}

      inline bool operator<=(const iterator& rhs) const {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");
			if (this->m_instance != rhs.m_instance) throw std::invalid_argument("iterator from different instance");

			return this->m_index <= rhs.m_index;
		}

		friend PersistentArray<Type>;
	};

	/**
	 * Retrieves size of this array.
	 * 
	 * @return size of this array
	 */
	std::size_t size() const {
		return this->GetSize();
	}

	/**
	 * Get an iterator to the first element of this PersistentArray
	 * 
	 * @return the iterator starting on first element of array, or end() if empty
	 */
	iterator begin() noexcept {
		if (this->GetSize() > 0) {
			return typename PersistentArray<Type>::iterator(this, 0);
		} else {
			return this->end();
		}
	}

	/**
	 * Get an iterator to the first element of this PersistentArray
	 *
	 * @return the iterator starting on first element of array, or end() if empty
	 */
	const iterator begin() const noexcept {
		if (this->GetSize() > 0) {
			return typename PersistentArray<Type>::iterator(this, 0);
		} else {
			return this->end();
		}
	}

	/**
	 * Get an iterator to the after-last element of this PersistentArray
	 *
	 * @return the iterator after-last element of array
	 */
	iterator end() noexcept {
		return typename PersistentArray<Type>::iterator();
	}

	/**
	 * get an iterator to the after-last element of this persistentarray
	 *
	 * @return the iterator after-last element of array
	 */
	const iterator end() const noexcept {
		return typename PersistentArray<Type>::iterator();
	}

	iterator erase(iterator pos) {
		this->RemoveAt(pos.m_index);

		if (pos.m_index >= this->GetSize()) {
			pos.m_index = iterator::no_index;
		}

		return pos;
	}

	void push_back(const Type& value) {
		this->Add(const_cast<Type&>(value));
	}

};


template<class T> CArchive& operator<<(CArchive& ar, PersistentArray<T>& rhs) {
	rhs.Serialize(ar); return ar; 
}

template<class T> CArchive& operator>>(CArchive& ar, PersistentArray<T>& rhs) {
	rhs.Serialize(ar); return ar;
}



/**
 * Get an iterator to the first element of this PersistentArray
 *
 * @return the iterator starting on first element of array, or end() if empty
 */
template<typename Type>
PersistentArray<Type>::iterator begin(PersistentArray<Type>& instance) noexcept {
	if (instance.GetSize() > 0) {
		return PersistentArray<Type>::iterator(instance, 0);
	} else {
		return end(instance);
	}
}

/**
 * Get an iterator to the first element of this PersistentArray
 *
 * @return the iterator starting on first element of array, or end() if empty
 */
template<typename Type>
const typename PersistentArray<Type>::iterator begin(const PersistentArray<Type>& instance) noexcept {
	if (instance.GetSize() > 0) {
		return typename PersistentArray<Type>::iterator(instance, 0);
	} else {
		return end(instance);
	}
}

/**
 * Get an iterator to the after-last element of this PersistentArray
 *
 * @return the iterator after-last element of array
 */
template<typename Type>
PersistentArray<Type>::iterator end(PersistentArray<Type>& instance) noexcept {
	return typename PersistentArray<Type>::iterator(
		instance, PersistentArray<Type>::iterator::no_index);
}

/**
 * Get an iterator to the after-last element of this PersistentArray
 *
 * @return the iterator after-last element of array
 */
template<typename Type>
const typename PersistentArray<Type>::iterator end(const PersistentArray<Type>& instance) noexcept {
	return typename PersistentArray<Type>::iterator(
		instance, PersistentArray<Type>::iterator::no_index);
}


////////////////////////////////////////////////////////////////////////////////////////////
// SortedArray: For when you need sorting, array comparison, concatenation etc...
////////////////////////////////////////////////////////////////////////////////////////////

template<class Type> class SortedArray : public PersistentArray<Type>
{
public:
	SortedArray() { }
	SortedArray(const SortedArray& arg) { *this=arg; }

	virtual ~SortedArray() { }

#ifdef _WINDOWS //Cld Can't compile StationId due to functions Add and InsertAt
	void Add_sorted(Type& element, bool reverse_order=false); // add an element (in a sorted array) if not present.// #### Richard: consting - StationId
#else
	void Add_sorted(const Type& element, bool reverse_order=false); // add an element (in a sorted array) if not present.// #### Richard: consting - StationId
#endif
	bool operator==(const SortedArray<Type>& rhs) const;
	bool operator!=(const SortedArray<Type>& rhs) const { return !(*this==rhs); }
	SortedArray<Type>& operator+=(SortedArray<Type>&rhs); // #### Richard: consting - StationId
};


template<class T> CArchive& operator<<(CArchive& ar, SortedArray<T>& rhs) { rhs.Serialize(ar); return ar; }
template<class T> CArchive& operator>>(CArchive& ar, SortedArray<T>& rhs) { rhs.Serialize(ar); return ar; }


////////////////////////////////////////////////////////////////////////////////////////////
// PersistentPtrArray:
////////////////////////////////////////////////////////////////////////////////////////////

template<class Type> class PersistentPtrArray : public CArray<Type*, Type*>, public PersistentObject
{
protected:
	bool manage_memory;
	bool deep_copy;

public:
	PersistentPtrArray(bool manage_memory, bool deep_copy) : manage_memory(manage_memory), deep_copy(deep_copy) { }
	PersistentPtrArray(PersistentPtrArray& arg) { *this=arg; }
	~PersistentPtrArray();
	virtual void Serialize(CArchive& ar); // The default behaviour for CArray is a memcpy of each element.

	void Manage_memory(bool manage) { manage_memory=manage; }
	void Deep_copy(bool deep) { deep_copy=deep; }
	Type* LastElement() { return PersistentPtrArray<Type>::ElementAt(PersistentPtrArray<Type>::GetUpperBound()); }

	// CArray overides to manage ptr memory:
	virtual int Add(Type* newElement, bool activate=false);
	virtual void RemoveAt(int nIndex, int nCount=1);
	virtual void RemoveAll();
	virtual void SetSize(int nNewSize, int nGrowBy=-1);

	PersistentPtrArray<Type>& operator=(const PersistentPtrArray<Type>& rhs);
	bool operator==(const PersistentPtrArray<Type>& rhs) const;
	bool operator!=(const PersistentPtrArray<Type>& rhs) const { return !(*this==rhs); }
	PersistentPtrArray<Type>& operator+=(const PersistentPtrArray<Type>&rhs);

};


template<class T> CArchive& operator<<(CArchive& ar, PersistentPtrArray<T>& rhs) { rhs.Serialize(ar); return ar; }
template<class T> CArchive& operator>>(CArchive& ar, PersistentPtrArray<T>& rhs) { rhs.Serialize(ar); return ar; }


////////////////////////////////////////////////////////////////////////////////////////////
// PersistentEnumArray: Just for fucking DWORD cast in serialization
////////////////////////////////////////////////////////////////////////////////////////////

template<class Type>
class PersistentEnumArray : public CArray<Type, Type>, public PersistentObject
{
public:
	using value_type = Type;
	using reference = Type&;
	using const_reference = const Type&;
	using difference_type = std::ptrdiff_t;

	PersistentEnumArray() {  }
	PersistentEnumArray(const PersistentEnumArray& arg) { *this=arg; }
	virtual void Serialize(CArchive& ar); // The default behaviour for CArray is a memcpy of each element.

	Type& LastElement() { return PersistentEnumArray<Type>::ElementAt(PersistentEnumArray<Type>::GetUpperBound()); }
	int Find(Type n) const;	// -1..PersistentArray<Type>::GetUpperBound(); -1 if not found
	void Add_sorted(Type element, bool reverse_order=false);

	PersistentEnumArray<Type>& operator=(const PersistentEnumArray<Type>& rhs) { if (&rhs!=this) this->Copy(rhs); return *this; }
	bool operator==(const PersistentEnumArray<Type>& rhs) const;
	bool operator!=(const PersistentEnumArray<Type>& rhs) const { return !(*this==rhs); }
	PersistentEnumArray<Type>& operator+=(const PersistentEnumArray<Type>&rhs);

	/**
	 * Iterator class for the PersistentEnumArray.
	 * This is needed for compatibility with std::range::ranges concepts
	 */
	class iterator
	{
	private:
		static const int no_index = -1;
		PersistentEnumArray<Type>* m_instance;
		int m_index;

	public:
      using iterator_category = std::random_access_iterator_tag;
      using value_type = Type;
      using reference = Type&;
      using pointer = Type*;
      using difference_type = int;

		iterator()
			: m_instance(nullptr), m_index(no_index) {}
		iterator(PersistentEnumArray<Type>* instance, int index)
			: m_instance(instance), m_index(index) {}
		iterator(const PersistentEnumArray<Type>* instance, int index)
			: m_instance(const_cast<PersistentEnumArray<Type>*>(instance)), m_index(index) {}
		iterator(const iterator& rhs)
			: m_instance(rhs.m_instance), m_index(rhs.m_index) {}

    
      inline iterator& operator+=(difference_type rhs) {
			if (this->m_instance == nullptr) return *this;

			this->m_index += rhs; 
			if (this->m_index < 0 || this->m_index >= this->m_instance->GetSize()) {
				*this = iterator();
			}

			return *this; 
		}

      inline iterator& operator-=(difference_type rhs) {
			if (this->m_instance == nullptr) return *this;

			this->m_index -= rhs;
			if (this->m_index < 0 || this->m_index >= this->m_instance->GetSize()) {
				*this = iterator();
			}

			return *this;
		}

		inline Type& operator*() const { return (*this->m_instance)[this->m_index]; }
      inline Type* operator->() const { return &(*this->m_instance)[this->m_index]; }
      inline Type& operator[](difference_type rhs) const { return (*this->m_instance)[rhs]; }
        
		inline iterator& operator++() {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");

			this->m_index++;

			if (this->m_index < 0 || this->m_index >= this->m_instance->GetSize()) {
				*this = iterator();
			}

			return *this;
		}

      inline iterator& operator--() {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");

			this->m_index--;

			if (this->m_index < 0 || this->m_index >= this->m_instance->GetSize()) {
				*this = iterator();
			}

			return *this;
		}

      inline iterator operator++(int) const { iterator tmp(*this); ++(*this); return tmp; }
      inline iterator operator--(int) const { iterator tmp(*this); --(*this); return tmp; }
        
      inline difference_type operator-(const iterator& rhs) const {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");
			if (this->m_instance != rhs.m_instance) throw std::invalid_argument("different instances");

			int index1 = this->m_index;
			int index2 = rhs.m_index;

			if (index1 == -1) {
				index1 = this->m_instance->GetSize();
			}

			if (index2 == -1) {
				index2 = rhs.m_instance->GetSize();
			}

			return index1 - index2;
		}

      inline iterator operator+(difference_type rhs) const {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");

			iterator result;

			result.m_index = this->m_index + rhs;

			if (result.m_index < 0 || result.m_index >= this->m_instance->GetSize()) {
				result = iterator();
			}

			return result;
		}

      inline iterator operator-(difference_type rhs) const {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");

			iterator result;

			result.m_index = this->m_index - rhs;

			if (result.m_index < 0 || result.m_index >= this->m_instance->GetSize()) {
				result = iterator();
			}

			return result;
		}
      
		friend inline iterator operator+(difference_type lhs, const iterator& rhs) {
			iterator result(rhs);
			result += lhs;
			return result;
		}

		friend inline iterator operator-(difference_type lhs, const iterator& rhs) {
			iterator result(rhs);
			result -= lhs;
			return result;
		}
      
		inline bool operator==(const iterator& rhs) const { return this->m_instance == rhs.m_instance && this->m_index == rhs.m_index; }
      inline bool operator!=(const iterator& rhs) const { return !(*this == rhs); }
		inline bool operator>(const iterator& rhs) const {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");
			if (this->m_instance != rhs.m_instance) throw std::invalid_argument("iterator from different instance");

			return this->m_index > rhs.m_index;
		}

      inline bool operator<(const iterator& rhs) const {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");
			if (this->m_instance != rhs.m_instance) throw std::invalid_argument("iterator from different instance");

			return this->m_index < rhs.m_index;
		}

      inline bool operator>=(const iterator& rhs) const {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");
			if (this->m_instance != rhs.m_instance) throw std::invalid_argument("iterator from different instance");

			return this->m_index >= rhs.m_index;
		}

      inline bool operator<=(const iterator& rhs) const {
			if (this->m_instance == nullptr) throw std::out_of_range("null instance");
			if (this->m_instance != rhs.m_instance) throw std::invalid_argument("iterator from different instance");

			return this->m_index <= rhs.m_index;
		}

		friend PersistentEnumArray<Type>;
	};

	/**
	 * Retrieve size of this array.
	 * 
	 * @return size of this array
	 */
	std::size_t size() const {
		return this->GetSize();
	}

	/**
	 * Get an iterator to the first element of this PersistentEnumArray
	 * 
	 * @return the iterator starting on first element of array, or end() if empty
	 */
	iterator begin() noexcept {
		if (this->GetSize() > 0) {
			return typename PersistentEnumArray<Type>::iterator(this, 0);
		} else {
			return this->end();
		}
	}

	/**
	 * Get an iterator to the first element of this PersistentEnumArray
	 *
	 * @return the iterator starting on first element of array, or end() if empty
	 */
	const iterator begin() const noexcept {
		if (this->GetSize() > 0) {
			return typename PersistentEnumArray<Type>::iterator(this, 0);
		} else {
			return this->end();
		}
	}

	/**
	 * Get an iterator to the after-last element of this PersistentEnumArray
	 *
	 * @return the iterator after-last element of array
	 */
	iterator end() noexcept {
		return typename PersistentEnumArray<Type>::iterator();
	}

	/**
	 * get an iterator to the after-last element of this PersistentEnumArray
	 *
	 * @return the iterator after-last element of array
	 */
	const iterator end() const noexcept {
		return typename PersistentEnumArray<Type>::iterator();
	}

	iterator erase(iterator pos) {
		this->RemoveAt(pos.m_index);

		if (pos.m_index >= this->GetSize()) {
			pos.m_index = iterator::no_index;
		}

		return pos;
	}

	void push_back(const Type& value) {
		this->Add(const_cast<Type&>(value));
	}


};

template<class T> CArchive& operator<<(CArchive& ar, PersistentEnumArray<T>& rhs);
template<class T> CArchive& operator>>(CArchive& ar, PersistentEnumArray<T>& rhs);

/**
 * Get an iterator to the first element of this PersistentEnumArray
 *
 * @return the iterator starting on first element of array, or end() if empty
 */
template<typename Type>
PersistentEnumArray<Type>::iterator begin(PersistentEnumArray<Type>& instance) noexcept {
	if (instance.GetSize() > 0) {
		return PersistentEnumArray<Type>::iterator(instance, 0);
	} else {
		return end(instance);
	}
}

/**
 * Get an iterator to the first element of this PersistentEnumArray
 *
 * @return the iterator starting on first element of array, or end() if empty
 */
template<typename Type>
const typename PersistentEnumArray<Type>::iterator begin(const PersistentEnumArray<Type>& instance) noexcept {
	if (instance.GetSize() > 0) {
		return typename PersistentEnumArray<Type>::iterator(instance, 0);
	} else {
		return end(instance);
	}
}

/**
 * Get an iterator to the after-last element of this PersistentEnumArray
 *
 * @return the iterator after-last element of array
 */
template<typename Type>
PersistentEnumArray<Type>::iterator end(PersistentEnumArray<Type>& instance) noexcept {
	return typename PersistentEnumArray<Type>::iterator(
		instance, PersistentEnumArray<Type>::iterator::no_index);
}

/**
 * Get an iterator to the after-last element of this PersistentEnumArray
 *
 * @return the iterator after-last element of array
 */
template<typename Type>
const typename PersistentEnumArray<Type>::iterator end(const PersistentEnumArray<Type>& instance) noexcept {
	return typename PersistentEnumArray<Type>::iterator(
		instance, PersistentEnumArray<Type>::iterator::no_index);
}


/******************************************************************************************\
|*                                                                                        *|
|* Definitions:                                                                           *|
|*                                                                                        *|
\******************************************************************************************/


////////////////////////////////////////////////////////////////////////////////////////////
// PersistentArray:
////////////////////////////////////////////////////////////////////////////////////////////


template<class Type>
void PersistentArray<Type>::Serialize(CArchive& ar)
{
	if (ar.IsLoading())   { // Reading code.
		int file_schema;
		ar >> file_schema;
		if (this->cur_schema == file_schema) {
			PersistentArray<Type>::RemoveAll();	// Make sure that the items release resources.
			int size;
			ar >> size;
			PersistentArray<Type>::SetSize(size);
			for (int i=0; i<size; i++)
                ar >> PersistentArray<Type>::ElementAt(i);
		} else {
			PersistentArray<Type>::RemoveAll();	// Preferable to avoid further problem.
			TraceException(SerializationFailure());
		}
	} else { // Storing code.  
		ar << cur_schema << PersistentArray<Type>::GetSize();
		for (int i=0; i<PersistentArray<Type>::GetSize(); i++) {
			ar << PersistentArray<Type>::ElementAt(i);
		}
	}
}

//template<class Type>
//void PersistentArray<Type>::Serialize(JSONSerializer& serializer) {
//
//	serializer.m_json = nlohmann::json::array();
//
//	for (int i = 0; i < PersistentArray<Type>::GetSize(); i++) {
//		JSONSerializer tmpSerializer;
//		tmpSerializer << PersistentArray<Type>::ElementAt(i);
//		serializer.m_json.push_back(tmpSerializer.m_json);
//	}
//}
//
//template<class Type>
//void PersistentArray<Type>::Deserialize(JSONSerializer& serializer) {
//
//	for (int i = 0; i < serializer.m_json.size(); i++) {
//		Type tmpObj;
//		serializer >> tmpObj;
//		this->Add(tmpObj);
//	}
//}

////////////////////////////////////////////////////////////////////////////////////////////
// SortedArray:
////////////////////////////////////////////////////////////////////////////////////////////

template<class Type> bool SortedArray<Type>::operator==(const SortedArray<Type>& rhs) const
{
	if (SortedArray<Type>::GetSize()!=rhs.SortedArray<Type>::GetSize()) return false;
	for (int i=0; i<SortedArray<Type>::GetSize(); i++) { 
		if (SortedArray<Type>::ElementAt(i)!=rhs.SortedArray<Type>::ElementAt(i))//[i])
			return false; 
	} 
	return true;
}


template<class Type> SortedArray<Type>& SortedArray<Type>::operator+=(SortedArray<Type>& rhs)
{
	for (int i = 0 ; i < rhs.SortedArray<Type>::GetSize() ; i++) {
		bool allready_in = false;
		for (int j = 0 ; j < SortedArray<Type>::GetSize() && !allready_in; j++) {
			if (!(SortedArray<Type>::ElementAt(j) != rhs[i])) 
				allready_in = true;
		}
		if (!allready_in) 
			this->Add(rhs.SortedArray<Type>::ElementAt(i));
	}
	return *this;
}


#ifdef _WINDOWS //Cld Can't compile StationId due to functions Add and InsertAt
template<class Type> void SortedArray<Type>::Add_sorted(Type& element, bool reverse_order)
#else
template<class Type> void SortedArray<Type>::Add_sorted(const Type& element, bool reverse_order)
#endif
{
	int pos = 0;
	while (pos < SortedArray<Type>::GetSize()) {
		if (SortedArray<Type>::ElementAt(pos) == element)
			return;
		if (!reverse_order && SortedArray<Type>::ElementAt(pos) > element) {
			this->InsertAt(pos,element);
			return;
		}
		if (reverse_order && SortedArray<Type>::ElementAt(pos) < element) {
			this->InsertAt(pos,element);
			return;
		}
		pos++;
	}

	this->Add(element);
}


////////////////////////////////////////////////////////////////////////////////////////////
// PersistentPtrArray:
////////////////////////////////////////////////////////////////////////////////////////////

// We take responsibility for memory so free all pointers before destructing:
template<class Type> PersistentPtrArray<Type>::~PersistentPtrArray()
{
    if (manage_memory)   
		for (int i=0; i<PersistentPtrArray<Type>::GetSize(); i++)
            delete PersistentPtrArray<Type>::ElementAt(i);
}


// Serialize what the pointer points to not just the pointer:
template<class Type> void PersistentPtrArray<Type>::Serialize(CArchive& ar)
{
	const int cur_schema=1;	// Current class version number.
	if (ar.IsLoading())   { // Reading code.
		int file_schema; ar >> file_schema;
		switch(file_schema) {
		case cur_schema: {
			RemoveAll();
			int size;
			ar >> size;
			for (int i=0; i<size; i++) {
				Type* object = new Type;
				Add(object); // Manages pointer setup.
				ar >> *object;
			}
			break; }
		default: 
			TraceException(SerializationFailure());
		} 
	} 
	else { // Storing code.  
		ar << cur_schema << PersistentPtrArray<Type>::GetSize();
        for (int i=0; i<PersistentPtrArray<Type>::GetSize(); i++)
            ar << *PersistentPtrArray<Type>::ElementAt(i);
	}
}


// Compare actual element not just pointers:
template<class Type> bool PersistentPtrArray<Type>::operator==(const PersistentPtrArray<Type>& rhs) const
{
	if (PersistentPtrArray<Type>::GetSize()!=rhs.PersistentPtrArray<Type>::GetSize()) return false; 
    for (int i=0; i<PersistentPtrArray<Type>::GetSize(); i++) { 
        if ((*PersistentPtrArray<Type>::ElementAt(i))!=*rhs[i]) return false; 
	} 
	return true;
}


template<class Type> PersistentPtrArray<Type>& PersistentPtrArray<Type>::operator+=(const PersistentPtrArray<Type>& rhs)
{
	for (int i=0; i<rhs.PersistentPtrArray<Type>::GetSize(); i++) {
		bool allready_in = false;
		for (int j=0; j<PersistentPtrArray<Type>::GetSize() && !allready_in; j++) { 
			if (PersistentPtrArray<Type>::ElementAt(j)==rhs[i])
				allready_in=true;
		}
		if (!allready_in) Add(rhs.PersistentPtrArray<Type>::ElementAt(i));
	}
	return *this;
}


template<class Type> PersistentPtrArray<Type>& PersistentPtrArray<Type>::operator=(const PersistentPtrArray<Type>& rhs)
{
	if (&rhs!=this) {
		// #### Richard (MMDC change):

		// This objects copy constructor calls this assignment while "this" has not be initialize (normally ok as
		// its going to be assigned to).  Therefore we need to be careful this code depends only on rhs and not
		// on the state of this::manage_memory and this::deep copy that in the copy constructor case have random
		// values.  This reasoning overrides the reasoning below which I leave for reference but logic no longer
		// applies.

		// Deep copy:
		// If we are assigning from a deep copy object make this object deep copy too. This is partially
		// motifated by cases like Route that set deep_copy true in default constructor but if assignment
		// constructed by compiler defined copy constructor then deep copy will not be correctly set with
		// out this.

		// Manage memory:
		// If we do a deep copy then we must manage memory (regardless of current manage_memory in this object
		// or rhs object.
		// If we are not deep copying then:
		//     If the rhs manage memory we must not (otherwise both will free)
		//     If the rhs does doesnt manage memory we should either?  This is not really clear because we might
		//     be transfering ownership to this array?	Best just to do nothing and leave mangage_mem as it is.

		deep_copy = rhs.deep_copy;

		// Whether or not we do a deep copy depend on the settings of the array being copied not the array that
		// is copied too:
		if (rhs.deep_copy) {
			manage_memory = true;

			// Copy what pointers point to:
			RemoveAll();
			PersistentPtrArray<Type>::SetSize(rhs.PersistentPtrArray<Type>::GetSize()); 
			for (int i=0; i<rhs.PersistentPtrArray<Type>::GetSize(); i++) {
				PersistentPtrArray<Type>::ElementAt(i)=new Type;
				*PersistentPtrArray<Type>::ElementAt(i) = *(rhs[i]); //*rhs.PersistentArray<Type>::ElementAt(i); <- Causes const problems.
			}
		}
		else {
			//if (rhs.manage_memory) // Rhs is managing memory so we should not:
			//	manage_memory = false;
			//else // If rhs is not managing memory we should not either: 
			//	manage_memory = false;
			manage_memory = false;

			this->Copy(rhs);	// Copy pointers:
		}
	}
	return *this;
}


template<class Type> int PersistentPtrArray<Type>::Add(Type* object, bool activate)
{
	return CArray<Type*, Type*>::Add(object);
}


template<class Type> void PersistentPtrArray<Type>::SetSize(int nNewSize, int nGrowBy)
{
	int old_size=PersistentPtrArray<Type>::GetSize();
	if (nNewSize==old_size)
		return;
	else if (nNewSize<old_size && manage_memory) {
		for (int i=nNewSize; i<old_size; i++)
			delete PersistentPtrArray<Type>::ElementAt(i);
	}
	CArray<Type*, Type*>::SetSize(nNewSize, nGrowBy);
	// Already done in CArray ConstructElements
	//if (nNewSize>old_size) {
	//	for (int i=old_size; i<GetSize(); i++)
	//		ElementAt(i)=NULL;
	//}
}


template<class Type> void PersistentPtrArray<Type>::RemoveAt(int nIndex, int nCount) //, bool free_mem)
{
	if (manage_memory) 
		for (int i=0; i<nCount; i++)
			delete PersistentPtrArray<Type>::ElementAt(nIndex+i);
	CArray<Type*, Type*>::RemoveAt(nIndex, nCount);
}


template<class Type> void PersistentPtrArray<Type>::RemoveAll() //bool free_mem)
{
	if (manage_memory) // Free memory that pointer points to.
		for (int i=0; i<PersistentPtrArray<Type>::GetSize(); i++)
            delete PersistentPtrArray<Type>::ElementAt(i);
	CArray<Type*, Type*>::RemoveAll();
}


////////////////////////////////////////////////////////////////////////////////////////////
// PersistentEnumArray:
////////////////////////////////////////////////////////////////////////////////////////////

template<class T> CArchive& operator<<(CArchive& ar, PersistentEnumArray<T>& rhs)
{
	rhs.Serialize(ar);
	return ar;
}


template<class T> CArchive& operator>>(CArchive& ar, PersistentEnumArray<T>& rhs)
{
	rhs.Serialize(ar);
	return ar;
}


template<class Type> void PersistentEnumArray<Type>::Serialize(CArchive& ar)
{
	const int cur_schema=1;	// Current class version number.
	if (ar.IsLoading())   { // Reading code.
		int file_schema; ar >> file_schema;
		switch(file_schema) {
		case cur_schema: {
			int size;
			ar >> size;
			PersistentEnumArray<Type>::SetSize(size);
			ar.Read(PersistentEnumArray<Type>::GetData(), size*sizeof(Type));
			break; }
		default:
			TraceException(SerializationFailure());
		} 
	} 
	else { // Storing code.  
		ar << cur_schema << PersistentEnumArray<Type>::GetSize();
		ar.Write(PersistentEnumArray<Type>::GetData(), PersistentEnumArray<Type>::GetSize()*sizeof(Type));
	}
}


template<class Type> bool PersistentEnumArray<Type>::operator==(const PersistentEnumArray<Type>& rhs) const
{
	return PersistentEnumArray<Type>::GetSize()==rhs.PersistentEnumArray<Type>::GetSize() && memcmp(PersistentEnumArray<Type>::GetData(), 
			rhs.PersistentEnumArray<Type>::GetData(), PersistentEnumArray<Type>::GetSize()*sizeof(Type)) == 0;
}


template<class Type> int PersistentEnumArray<Type>::Find(Type n) const
{
	for (int i = 0 ; i < PersistentEnumArray<Type>::GetSize() ; i++) 
		if (PersistentEnumArray<Type>::ElementAt(i) == n)
			return i;
	return -1;
}


template<class Type> void PersistentEnumArray<Type>::Add_sorted(Type element, bool reverse_order)
{
	for (int pos = 0; pos < this->GetSize(); pos++) {
		if (this->ElementAt(pos) == element)
			return;
		if (!reverse_order && this->ElementAt(pos) > element) {
			this->InsertAt(pos, element);
			return;
		}
		if (reverse_order && this->ElementAt(pos) < element) {
			this->InsertAt(pos, element);
			return;
		}
	}

	this->Add(element);
}


template<class Type> PersistentEnumArray<Type>& PersistentEnumArray<Type>::operator+=(const PersistentEnumArray<Type>& rhs)
{
	for (int i = 0 ; i < rhs.PersistentEnumArray<Type>::GetSize() ; i++) {
		if (Find(rhs[i]) < 0)
			this->Add(rhs[i]);
	}
	return *this;
}
