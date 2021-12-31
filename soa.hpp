/* soa.hpp
 * Struct-Of-Arrays template class
 * by Haydn V. Harach
 * Created October 2019
 * Modified March 2021
 *
 * Implements a Struct-Of-Arrays container class to store and manage a series
 * of contiguous arrays which are stored back-to-back in memory.
 * The interface is designed to be similar to that of std::vector.
 */

#ifndef HVH_TOOLS_STRUCTOFARRAYS_H
#define HVH_TOOLS_STRUCTOFARRAYS_H


#include <cstdint>
#include <cstring> // For memcpy and memmove
#include <algorithm> // For std::min
#include <tuple>
#include <functional>
#include <vector>


/******************************************************************************
 * Aligned Malloc/Free
 *****************************************************************************/
#include <cstdlib>

// We need access to aligned allocations and deallocations,
// but visual studio doesn't support aligned_alloc from the c11 standard.
// This define lets us have consistent behaviour.
#ifdef _MSC_VER
  #define _soa_aligned_malloc(alignment,size) _aligned_malloc(size,alignment)
#else
  #define _soa_aligned_malloc(alignment,size) aligned_alloc(alignment,size)
#endif

#ifdef _MSC_VER
  #define _soa_aligned_free(mem) _aligned_free(mem)
#else
  #define _soa_aligned_free(mem) free(mem)
#endif


namespace hvh {

	template <typename... Ts>
	class _soa_base {
	public:
		inline size_t constexpr size_per_entry() const { return 0; }
		inline void nullify() {}
		inline void construct_range(size_t, size_t) {}
		inline void destruct_range(size_t, size_t) {}
		inline void divy_buffer(void*) {}
		inline void push_back() {}
		inline void emplace_back() {}
		inline void emplace_back_default() {}
		inline void pop_back() {}
		inline void insert(size_t) {}
		inline void emplace(size_t) {}
		inline void emplace_default(size_t) {}
		inline void erase_swap(size_t) {}
		inline void erase_shift(size_t) {}
		friend inline void swap(_soa_base<Ts...>& lhs, _soa_base<Ts...>& rhs) { std::swap(lhs.mysize, rhs.mysize); std::swap(lhs.mycapacity, rhs.mycapacity); }
		inline void copy(const _soa_base<Ts...>& other) { mysize = other.mysize; }
		inline void swap_entries(size_t, size_t) {}

	protected:
		_soa_base() {}
		size_t mysize = 0;
		size_t mycapacity = 0;
	};

	template <typename FT, typename... RTs>
	class _soa_base<FT, RTs...> : public _soa_base<RTs...> {

		// This stuff is neccesary to hold the list of types.
		// I'm not entirely sure exactly how this works, tbh.

		template <size_t, typename> struct elem_type_holder;

		template <typename T, typename... Ts>
		struct elem_type_holder<0, _soa_base<T, Ts...>> {
			typedef T type;
		};

		template <size_t K, typename T, typename... Ts>
		struct elem_type_holder<K, _soa_base<T, Ts...>> {
			typedef typename elem_type_holder<K - 1, _soa_base<Ts...>>::type type;
		};

	public:

		// data<K>()
		// Gets a constant reference to the pointer to the Kth array.
		// Elements of the array may be modified, but the array itself cannot.
		// As a reference to the internal array, the result will remain valid even after a reallocation.
		template <size_t K>
		typename std::enable_if<K == 0, FT * const&>::type
			inline data() { return mydata; }

		// data<K>()
		// Gets a constant reference to the pointer to the Kth array.
		// Elements of the array may be modified, but the array itself cannot.
		// As a reference to the internal array, the result will remain valid even after a reallocation.
		template <size_t K>
		typename std::enable_if<K != 0, typename elem_type_holder<K, _soa_base<FT, RTs...>>::type* const&>::type
			inline data() { _soa_base<RTs...>& base = *this; return base.data<K - 1>(); }

		// data<K>() const
		// Gets a constant reference to a constant pointer to the Kth array.
		// Neither the array nor its elements can be modified.
		// As a reference to the internal array, the result will remain valid even after a reallocation.
		template <size_t K>
		typename std::enable_if<K == 0, const FT * const&>::type
			inline data() const { return mydata; }

		// data<K>() const
		// Gets a constant reference to a constant pointer to the Kth array.
		// Neither the array nor its elements can be modified.
		// As a reference to the internal array, the result will remain valid even after a reallocation.
		template <size_t K>
		typename std::enable_if<K != 0, const typename elem_type_holder<K, _soa_base<FT, RTs...>>::type* const&>::type
			inline data() const { _soa_base<RTs...>& base = *this; return base.data<K - 1>(); }

		// at<K>(i)
		// Gets a reference to the ith item of the Kth array.
		// Does not perform bounds checking; do not use with an out-of-bounds index!
		template <size_t K>
		typename std::enable_if<K == 0, FT&>::type
			inline at(size_t index) { return mydata[index]; }

		// at<K>(i)
		// Gets a reference to the ith item of the Kth array.
		// Does not perform bounds checking; do not use with an out-of-bounds index!
		template <size_t K>
		typename std::enable_if<K != 0, typename elem_type_holder<K, _soa_base<FT, RTs...>>::type&>::type
			inline at(size_t index) { _soa_base<RTs...>& base = *this; return base.at<K - 1>(index); }

		// at<K>(i) const
		// Gets a constant reference to the ith item of the Kth array.
		// Does not perform bounds checking; do not use with an out-of-bounds index!
		template <size_t K>
		typename std::enable_if<K == 0, const FT&>::type
			inline at(size_t index) const { return mydata[index]; }

		// at<K>(i) const
		// Gets a constant reference to the ith item of the Kth array.
		// Does not perform bounds checking; do not use with an out-of-bounds index!
		template <size_t K>
		typename std::enable_if<K != 0, const typename elem_type_holder<K, _soa_base<FT, RTs...>>::type&>::type
			inline at(size_t index) const { _soa_base<RTs...>& base = *this; return base.at<K - 1>(index); }

		// front<K>()
		// Gets a reference to the item at the front of the Kth array.
		// Does not perform bounds checking; do not call on an empty container!
		template <size_t K>
		typename std::enable_if<K == 0, FT&>::type
			inline front() { return mydata[0]; }

		// front<K>()
		// Gets a reference to the item at the front of the Kth array.
		// Does not perform bounds checking; do not call on an empty container!
		template <size_t K>
		typename std::enable_if<K != 0, typename elem_type_holder<K, _soa_base<FT, RTs...>>::type&>::type
			inline front() { _soa_base<RTs...>& base = *this; return base.front<K - 1>(); }

		// front<K>() const
		// Gets a const reference to the item at the front of the Kth array.
		// Does not perform bounds checking; do not call on an empty container!
		template <size_t K>
		typename std::enable_if<K == 0, const FT&>::type
			inline front() const { return mydata[0]; }

		// front<K>() const
		// Gets a constant reference to the item at the front of the Kth array.
		// Does not perform bounds checking; do not call on an empty container!
		template <size_t K>
		typename std::enable_if<K != 0, const typename elem_type_holder<K, _soa_base<FT, RTs...>>::type&>::type
			inline front() const { _soa_base<RTs...>& base = *this; return base.front<K - 1>(); }

		// back<K>()
		// Gets a reference to the item at the back of the Kth array.
		// Does not perform bounds checking; do not call on an empty container!
		template <size_t K>
		typename std::enable_if<K == 0, FT&>::type
			inline back() { return mydata[this->mysize - 1]; }

		// back<K>()
		// Gets a reference to the item at the back of the Kth array.
		// Does not perform bounds checking; do not call on an empty container!
		template <size_t K>
		typename std::enable_if<K != 0, typename elem_type_holder<K, _soa_base<FT, RTs...>>::type&>::type
			inline back() { _soa_base<RTs...>& base = *this; return base.back<K - 1>(); }

		// back<K>() const
		// Gets a constant reference to the item at the back of the Kth array.
		// Does not perform bounds checking; do not call on an empty container!
		template <size_t K>
		typename std::enable_if<K == 0, const FT&>::type
			inline back() const { return mydata[this->mysize - 1]; }

		// back<K>() const
		// Gets a constant reference to the item at the back of the Kth array.
		// Does not perform bounds checking; do not call on an empty container!
		template <size_t K>
		typename std::enable_if<K != 0, const typename elem_type_holder<K, _soa_base<FT, RTs...>>::type&>::type
			inline back() const { _soa_base<RTs...>& base = *this; return base.back<K - 1>(); }

		// lower_bound<K>(goal)
		// Performs a binary search looking for 'goal' in sorted array 'K'.
		// If 'goal' is in sorted array 'K', returns the index of leftmost element which equals 'goal'.
		// Otherwise, returns the number of items which are less than 'goal'.
		// This behaviour is similar to std::lower_bound.
		// Complexity: O(logn).
		template <size_t K>
		typename std::enable_if<K == 0, size_t>::type
			lower_bound(const FT & goal) const {
			size_t left = 0;
			size_t right = this->mysize;
			while (left < right) {
				size_t middle = (left + right) / 2;
				if (mydata[middle] < goal) left = middle + 1;
				else right = middle;
			}
			return left;
		}

		// lower_bound_row<K>(goal_row)
		// Performs a binary search looking for the Kth entry in 'goal_row' in sorted array 'K'.
		// This behaves like lower_bound, but letting the user give an entire row of data
		// instead of just the key.  Entries in the row other than the Kth are ignored.
		// Complexity: O(logn).
		template <size_t K>
		typename std::enable_if<K == 0, size_t>::type
			lower_bound_row(const FT& goal, const RTs&... rest) const {
			size_t left = 0;
			size_t right = this->mysize;
			while (left < right) {
				size_t middle = (left + right) / 2;
				if (mydata[middle] < goal) left = middle + 1;
				else right = middle;
			}
			return left;
		}

		// lower_bound<K>(goal)
		// Performs a binary search looking for 'goal' in sorted array 'K'.
		// If 'goal' is in the array, returns the index of leftmost element which equals 'goal'.
		// Otherwise, returns the number of items which are less than 'goal'.
		// This behaviour is similar to std::lower_bound.
		// Complexity: O(logn).
		template <size_t K>
		typename std::enable_if<K != 0, size_t>::type
			inline lower_bound(const typename elem_type_holder<K, _soa_base<FT, RTs...>>::type & goal) {
			_soa_base<RTs...>& base = *this; return base.lower_bound<K - 1>(goal);
		}
		// lower_bound_row<K>(goal_row)
		// Performs a binary search looking for the Kth entry in 'goal_row' in sorted array 'K'.
		// This behaves like lower_bound, but letting the user give an entire row of data
		// instead of just the key.  Entries in the row other than the Kth are ignored.
		// Complexity: O(logn).
		template<size_t K>
		typename std::enable_if<K != 0, size_t>::type
			inline lower_bound_row(const FT& first, const RTs&... rest) {
			_soa_base<RTs...>& base = *this; return base.lower_bound<K - 1>(rest...);
		}

		// upper_bound<K>(goal)
		// Performs a binary search looking for 'goal' in sorted array 'K'.
		// If 'goal' is in the array, returns the index of the leftmost element which is greather than 'goal'.
		// Otherwise, returns the number of items which are less than 'goal'.
		// This behaviour is similar to std::upper_bound, and is useful if one array in a container
		// is treated as the key for a binary search table.
		// Complexity: O(logn).
		template <size_t K>
		typename std::enable_if<K == 0, size_t>::type
			upper_bound(const FT & goal) const {
			size_t left = 0;
			size_t right = this->mysize;
			while (left < right) {
				size_t middle = (left + right) / 2;
				if (goal < mydata[middle]) right = middle;
				else left = middle + 1;
			}
			return left;
		}
		// upper_bound<K>(goal)
		// Performs a binary search looking for 'goal' in sorted array 'K'.
		// If 'goal' is in the array, returns the index of the leftmost element which is greather than 'goal'.
		// Otherwise, returns the number of items which are less than 'goal'.
		// This behaviour is similar to std::upper_bound, and is useful if one array in a container
		// is treated as the key for a binary search table.
		// Complexity: O(logn).
		template <size_t K>
		typename std::enable_if<K != 0, size_t>::type
			inline upper_bound(const typename elem_type_holder<K, _soa_base<FT, RTs...>>::type & goal) {
			_soa_base<RTs...>& base = *this; return base.upper_bound<K - 1>(goal);
		}

		///////////////////////////////////////////////////////////////////////////
		// "protected" methods.
		// Not actually protected; child classes need to ban them manually.
		// This is because of the wonky way in which child classes can't access
		// protected members in instances of their base class.
		///////////////////////////////////////////////////////////////////////////

		// size_per_entry gives the total sizeof() of an entire row of data.
		inline constexpr size_t size_per_entry() const {
			const _soa_base<RTs...>& base = *this;
			return sizeof(FT) + base.size_per_entry();
		}

		// nullify sets the data pointer of every column to nullptr.
		inline void nullify() {
			_soa_base<RTs...>& base = *this;
			mydata = nullptr; base.nullify();
		}

		// construct_range calls the default constructor on a range of entries.
		inline void construct_range(size_t begin, size_t end) {
			for (size_t i = begin; i < end; ++i) {
				new (&mydata[i]) FT();
			}
			_soa_base<RTs...>& base = *this;
			base.construct_range(begin, end);
		}

		// construct_range calls the copy constructor on a range of entries.
		inline void construct_range(size_t begin, size_t end, const FT& initval, const RTs& ... restvals) {
			for (size_t i = begin; i < end; ++i) {
				new (&mydata[i]) FT(initval);
			}
			_soa_base<RTs...>& base = *this;
			base.construct_range(begin, end, restvals...);
		}

		// destruct_range calls the destructor on a range of entries.
		inline void destruct_range(size_t begin, size_t end) {
			for (size_t i = begin; i < end; ++i) {
				mydata[i].~FT();
			}
			_soa_base<RTs...>& base = *this;
			base.destruct_range(begin, end);
		}

		// divy_buffer splits a big buffer of memory into a series of column arrays.
		// This also copies existing data into the new memory buffer.
		inline void divy_buffer(void* newmem) {
			if (mydata) { memcpy(newmem, mydata, sizeof(FT) * std::min(this->mysize, this->mycapacity)); }
			mydata = (FT*)newmem;
			_soa_base<RTs...>& base = *this;
			base.divy_buffer(((FT*)newmem) + this->mycapacity);
		}

		// push_back copies a row onto the back of the container.
		template <typename FirstType = FT, typename... RestTypes>
		typename std::enable_if<std::is_copy_constructible<FirstType>::value, void>::type inline push_back(const FirstType& first, RestTypes&&... rest) {
			new (&mydata[this->mysize]) FT(first);
			_soa_base<RTs...>& base = *this;
			base.push_back(rest...);
		}

		// push_back moves a row onto the back of the container.
		template <typename FirstType = FT, typename... RestTypes>
		typename std::enable_if<std::is_move_constructible<FirstType>::value, void>::type inline push_back(FirstType&& first, RestTypes&&... rest) {
			new (&mydata[this->mysize]) FT(std::move(first));
			_soa_base<RTs...>& base = *this;
			base.push_back(rest...);
		}

		// emplace_back using a single argument to copy-construct the object.
		template <typename FirstType, typename... RestTypes>
		typename std::enable_if<std::is_copy_constructible<FirstType>::value, void>::type inline emplace_back(const FirstType& first, RestTypes&& ... rest) {
			new (&mydata[this->mysize]) FT(first);
			_soa_base<RTs...>& base = *this;
			base.emplace_back(rest...);
		}

		// emplace_back using a single argument to move-construct the object.
		template <typename FirstType, typename... RestTypes>
		typename std::enable_if<std::is_move_constructible<FirstType>::value, void>::type inline emplace_back(FirstType&& first, RestTypes&&... rest) {
			new (&mydata[this->mysize]) FT(std::move(first));
			_soa_base<RTs...>& base = *this;
			base.emplace_back(rest...);
		}

		// emplace_back using no arguments to construct the object.
		template <typename... RestTypes>
		inline void emplace_back(decltype(std::ignore), const RestTypes& ... rest) {
			new (&mydata[this->mysize]) FT();
			_soa_base<RTs...>& base = *this;
			base.emplace_back(rest...);
		}

		// emplace_back using multiple arguments to construct the object.
		template <typename... FirstTypes, typename... RestTypes>
		inline void emplace_back(const std::tuple<FirstTypes...>& first, const RestTypes& ... rest) {
			std::apply([=](const FirstTypes& ... args) {new (&mydata[this->mysize]) FT(args...); }, first);
			_soa_base<RTs...>& base = *this;
			base.emplace_back(rest...);
		}

		// emplace_back using all default constructors.
		inline void emplace_back_default() {
			new (&mydata[this->mysize]) FT();
			_soa_base<RTs...>& base = *this;
			base.emplace_back_default();
		}

		// inserts (copies) a row at the specified index, moving later entries back by one.
		template <typename FirstType = FT, typename... RestTypes>
		typename std::enable_if<std::is_copy_constructible<FirstType>::value, void>::type inline insert(size_t location, const FirstType& first, RestTypes&& ... rest) {
			memmove(mydata + (location + 1), mydata + location, sizeof(FT) * (this->mysize - location));
			new (&mydata[location]) FT(first);
			_soa_base<RTs...>& base = *this;
			base.insert(location, rest...);
		}

		// inserts (moves) a row at the specified index, moving later entries back by one.
		template <typename FirstType = FT, typename... RestTypes>
		typename std::enable_if<std::is_move_constructible<FirstType>::value, void>::type inline insert(size_t location, FirstType&& first, RestTypes&& ... rest) {
			memmove(mydata + (location + 1), mydata + location, sizeof(FT) * (this->mysize - location));
			new (&mydata[location]) FT(std::move(first));
			_soa_base<RTs...>& base = *this;
			base.insert(location, rest...);
		}

		// emplace using a single argument to copy-construct the object.
		template <typename FirstType, typename... RestTypes>
		typename std::enable_if<std::is_copy_constructible<FirstType>::value, void>::type inline emplace(size_t location, const FirstType& first, RestTypes&& ... rest) {
			memmove(mydata + (location + 1), mydata + location, sizeof(FT) * (this->mysize - location));
			new (&mydata[location]) FT(first);
			_soa_base<RTs...>& base = *this;
			base.emplace_back(location, rest...);
		}

		// emplace using a single argument to move-construct the object.
		template <typename FirstType, typename... RestTypes>
		typename std::enable_if<std::is_move_constructible<FirstType>::value, void>::type inline emplace(size_t location, FirstType&& first, RestTypes&& ... rest) {
			memmove(mydata + (location + 1), mydata + location, sizeof(FT) * (this->mysize - location));
			new (&mydata[location]) FT(first);
			_soa_base<RTs...>& base = *this;
			base.emplace_back(location, rest...);
		}

		// emplace using no arguments to construct the object.
		template <typename... RestTypes>
		inline void emplace(size_t location, decltype(std::ignore), RestTypes&& ... rest) {
			memmove(mydata + (location + 1), mydata + location, sizeof(FT) * (this->mysize - location));
			new (&mydata[location]) FT();
			_soa_base<RTs...>& base = *this;
			base.emplace_back(location, rest...);
		}

		// emplace using multiple arguments to construct the object.
		template <typename... FirstTypes, typename... RestTypes>
		inline void emplace(size_t location, const std::tuple<FirstTypes...>& first, RestTypes&& ... rest) {
			memmove(mydata + (location + 1), mydata + location, sizeof(FT) * (this->mysize - location));
			std::apply([=](const FirstTypes& ... args) {new (&mydata[location]) FT(args...); }, first);
			_soa_base<RTs...>& base = *this;
			base.emplace_back(location, rest...);
		}

		// emplace using all default constructors.
		inline void emplace_default(size_t location) {
			memmove(mydata + (location + 1), mydata + location, sizeof(FT) * (this->mysize - location));
			new (&mydata[location]) FT();
			_soa_base<RTs...>& base = *this;
			base.emplace_back(location);
		}

		// pop_back removes the last row in the container.
		inline void pop_back() {
			mydata[this->mysize - 1].~FT();
			_soa_base<RTs...>& base = *this;
			base.pop_back();
		}

		// erase_swap swaps the given row with the back of the container, then removes the last row.
		inline void erase_swap(size_t location) {
			using std::swap;
			swap(mydata[location], mydata[this->mysize - 1]);
			mydata[this->mysize - 1].~FT();
			_soa_base<RTs...>& base = *this;
			base.erase_swap(location);
		}

		// erase_shift removes the given row, and move all further rows forward by one.
		inline void erase_shift(size_t location) {
			mydata[location].~FT();
			memmove(mydata + location, mydata + (location + 1), sizeof(FT) * (this->mysize - location));
			_soa_base<RTs...>& base = *this;
			base.erase_shift(location);
		}

		// swaps two containers.
		friend inline void swap(_soa_base<FT, RTs...>& lhs, _soa_base<FT, RTs...>& rhs) {
			using std::swap;
			swap(lhs.mydata, rhs.mydata);
			_soa_base<RTs...>& lhsbase = lhs;
			_soa_base<RTs...>& rhsbase = rhs;
			swap(lhsbase, rhsbase);
		}

		// performs a deep copy.
		inline void copy(const _soa_base<FT, RTs...>& other) {
			memcpy(mydata, other.mydata, sizeof(FT) * other.mysize);
			_soa_base<RTs...>& lhs = *this;
			const _soa_base<RTs...>& rhs = other;
			lhs.copy(rhs);
		}

		// swaps the position of two indicated rows.
		inline void swap_entries(size_t first, size_t second) {
			using std::swap;
			swap(mydata[first], mydata[second]);
			_soa_base<RTs...>& base = *this;
			base.swap_entries(first, second);
		}

	protected:
		_soa_base() {}
		FT* mydata = nullptr;
	};

	template <typename... Ts>
	class soa : public _soa_base<Ts...> {
	public:

		// soa()
		// Default constructor for a Struct-Of-Arrays object.
		// Initial size and capacity will be 0.
		// Complexity: O(1).
		soa() {}
		// soa(initsize)
		// Constructs a Struct-Of-Arrays object.
		// Initial size is set to the input,
		// and initial capacity will be enough to hold that many items.
		// Calls default constructors for each new item.
		// Complexity: O(n).
		soa(size_t initsize) { resize(initsize); }
		// soa(initsize, args...)
		// Constructs a Struct-Of-Arrays object.
		// Initial size is set to the input,
		// and initial capacity will be enough to hold that many items.
		// Calls copy-constructors for each new item using 'args...'.
		// Complexity: O(n).
		soa(size_t initsize, const Ts& ... initvals) { resize(initsize, initvals...); }
		// soa({...})
		// Constructs a Struct-Of-Arrays using a list of tuples.
		// Copies the items from the initializer list into ourselves.
		// Complexity: O(n).
		soa(const std::initializer_list<std::tuple<Ts...>>& initlist) {
			reserve(initlist.size());
			for (auto& entry : initlist) {
				std::apply([=](const Ts& ... args) {this->push_back(args...); }, entry);
			}
		}
		// soa(&& rhs)
		// Move constructor for Struct-Of-Arrays.
		// Moves the contents from the rhs struct-of-arrays into ourselves.
		// Complexity: O(1).
		soa(soa<Ts...>&& other) { swap(other); }
		// soa(const& rhs)
		// Copy constructor for Struct-Of-Arrays.
		// Copies the contents from the rhs struct-of-arrays into ourselves.
		// Complexity: O(n).
		soa(const soa<Ts...>& other) {
			reserve(other.size());
			_soa_base<Ts...>& base = *this;
			_soa_base<Ts...>& otherbase = other;
			base.copy(otherbase);
		}
		// operator = (&& rhs)
		// Move-assignment operator for Struct-Of-Arrays.
		// Moves the contents from the rhs struct-of-arrays into ourselves.
		// Complexity: O(1).
		soa<Ts...>& operator = (soa<Ts...>&& other) { swap(other); return *this; }
		// operator = (const& rhs)
		// Copy-assignment operator for Struct-Of-Arrays.
		// Copies the contents from the rhs struct-of-arrays into ourselves.
		// Complexity: O(n).
		soa<Ts...>& operator = (soa<Ts...> other) { swap(other); return *this; }
		// ~soa()
		// Destructor for Struct-Of-Arrays.
		// Destructs all stored elements and frees held memory.
		// Complexity: O(n).
		~soa() {
			_soa_base<Ts...>& base = *this;
			base.destruct_range(0, this->mysize);
			void* oldmem = this->data<0>();
			if (oldmem) _soa_aligned_free(oldmem);
		}

		// swap(& rhs)
		// Swaps the contents of this container with the other container.
		// Complexity: O(1).
		friend inline void swap(soa<Ts...>& lhs, soa<Ts...>& rhs) {
			_soa_base<Ts...>& lhsbase = lhs;
			_soa_base<Ts...>& rhsbase = rhs;
			swap(lhsbase, rhsbase);
		}

		// clear()
		// Clears and destructs all held items.
		// Does not change capacity.
		// Complexity: O(n).
		inline void clear() {
			_soa_base<Ts...>& base = *this;
			base.destruct_range(0, this->mysize);
			this->mysize = 0;
		}

		// reserve(n)
		// Ensures that the container has enough space to hold n items.
		// If n is greater than the current capacity, this triggers a memory re-allocation.
		// Returns false if a memory allocation error occurs, or true otherwise.
		// Complexity: O(n).
		bool reserve(size_t newsize) {
			// For alignment, we must have a multiple of 16 items.
			if (newsize % 16 != 0)
				newsize += 16 - (newsize % 16);

			// We need at least 16 elements.
			if (newsize == 0) newsize = 16;

			// We can't shrink the actual memory.
			if (newsize <= this->mycapacity) return true;

			// Remember the old memory so we can free it.
			_soa_base<Ts...>& base = *this;
			void* oldmem = this->data<0>();

			// Allocate new memory.
			void* alloc_result = _soa_aligned_malloc(16, base.size_per_entry() * newsize);
			if (!alloc_result) return false;

			// Copy the old data into the new memory.
			this->mycapacity = newsize;
			base.divy_buffer(alloc_result);

			// Free the old memory.
			if (oldmem) _soa_aligned_free(oldmem);
			return true;
		}

		// shrink_to_fit()
		// Shrinks the capacity to the smallest amount that can hold all currently-held items.
		// If the capacity is reduced, this triggers a memory re-allocation.
		// Returns false is a memory allocation error occurs, true otherwise.
		// Complexity: O(n).
		bool shrink_to_fit() {
			// For alignment, we must have a multiple of 16 items.
			size_t newsize = this->mysize;
			if (newsize % 16 != 0)
				newsize += 16 - (newsize % 16);

			// If the container is already as small as it can be, bail out now.
			if (newsize == this->mycapacity) return true;

			// Remember the old memory so we can free it.
			_soa_base<Ts...>& base = *this;
			void* oldmem = this->data<0>();

			if (newsize > 0) {
				// Allocate new memory.
				void* alloc_result = _soa_aligned_malloc(16, base.size_per_entry() * newsize);
				if (!alloc_result) return false;

				// Copy the old data into the new memory.
				this->mycapacity = newsize;
				base.divy_buffer(alloc_result);
			}
			else {
				base.nullify();
				this->mycapacity = 0;
			}

			// Free the old memory.
			if (oldmem) _soa_aligned_free(oldmem);
			return true;
		}

		// resize(n)
		// Resizes the container to contain exactly n items.
		// If n is greater than the current capacity, reserve(n) is called.
		// If n is greater than the current number of items, the new items are default-constructed.
		// If n is less than the current number of items, the lost items are destructed.
		// Returns false if a memory allocation failure occurs in reserve, true otherwise.
		// Complexity: O(n).
		inline bool resize(size_t newsize) {
			_soa_base<Ts...>& base = *this;
			if (newsize > this->mysize) {
				if (newsize > this->mycapacity) {
					if (!reserve(newsize)) return false;
				}
				base.construct_range(this->mysize, newsize);
				this->mysize = newsize;
			}
			else if (newsize < this->mysize) {
				base.destruct_range(newsize, this->mysize);
				this->mysize = newsize;
			}
			return true;
		}

		// resize(n, args...)
		// Resizes the container to contain exactly n items.
		// If n is greater than the current capacity, reserve(n) is called.
		// If n is greater than the current number of items, the new items are copy-constructed using 'args...'.
		// If n is less than the current number of items, the lost items are destructed.
		// Returns false if a memory allocation failure occurs in reserve, true otherwise.
		// Complexity: O(n).
		inline bool resize(size_t newsize, const Ts& ... initvals) {
			_soa_base<Ts...>& base = *this;
			if (newsize > this->mysize) {
				if (newsize > this->mycapacity) {
					if (!reserve(newsize)) return false;
				}
				base.construct_range(this->mysize, newsize, initvals...);
			}
			else if (newsize < this->mysize) {
				base.destruct_range(newsize, this->mysize);
				this->mysize = newsize;
			}
			return true;
		}

		// push_back(args...)
		// Increases the size of the container by 1 and places the given args at the back of each array.
		// If we're out of space, reserve is called to expand the size of the buffer.
		// Returns false if a memory allocation occurs in reserve, true otherwise.
		// Complexity: O(1) unless reserve is called, then O(n).
		template <typename... EntryTypes>
		inline bool push_back(EntryTypes&& ... args) {
			if (this->mysize == this->mycapacity) {
				if (!reserve(this->mycapacity * 2)) return false;
			}
			_soa_base<Ts...>& base = *this;
			base.push_back(args...);
			++this->mysize;
			return true;
		}

		// exmplace_back(args...)
		// Increases the size of the container by 1 and constructs the new items at the back of each array using 'args...'.
		// If we're out of space, reserve is called to expand the size of the buffer.
		// Returns false if a memory allocation occurs in reserve, true otherwise.
		// You can pass std::ignore as an argument to default-construct the corresponding element,
		// or an std::tuple to initialize the corresponding element with multiple arguments.
		// Complexity: O(1) unless reserve is called, then O(n).
		template <typename... CTypes>
		inline bool emplace_back(CTypes&&... args) {
			if (this->mysize == this->mycapacity) {
				if (!reserve(this->mycapacity * 2)) return false;
			}
			_soa_base<Ts...>& base = *this;
			base.emplace_back(args...);
			++this->mysize;
			return true;
		}

		// exmplace_back()
		// Increases the size of the container by 1 and constructs the new items at the back of each array using default constructors.
		// If we're out of space, reserve is called to expand the size of the buffer.
		// Returns false if a memory allocation occurs in reserve, true otherwise.
		// Complexity: O(1) unless reserve is called, then O(n).
		inline bool emplace_back() {
			if (this->mysize == this->mycapacity) {
				if (!reserve(this->mycapacity * 2)) return false;
			}
			_soa_base<Ts...>& base = *this;
			base.emplace_back_default();
			++this->mysize;
			return true;
		}

		// insert(where, args...)
		// Increases the size of the container by 1 and inserts 'args...' into the arrays at location 'where'.
		// If we're out of space, reserve is called to expand the size of the buffer.
		// Returns false if a memory allocation error occurs in reserve or if 'where' is out of bounds, true otherwise.
		// Complexity: O(n).
		template <typename... Args>
		inline bool insert(size_t where, Args&& ... args) {
			if (this->mysize == this->mycapacity) {
				if (!reserve(this->mycapacity * 2)) return false;
			}
			if (where > this->mysize) return false;
			_soa_base<Ts...>& base = *this;
			base.insert(where, args...);
			++this->mysize;
			return true;
		}

		// emplace(where, args...)
		// Increases the size of the container by 1 and inserts newly-constructed objects using 'args...' into the arrays at location 'where'.
		// If we're out of space, reserve is called to expand the size of the buffer.
		// Returns false if a memory allocation error occurs in reserve or if 'where' is out of bounds, true otherwise.
		// You can pass std::ignore as an argument to default-construct the corresponding element,
		// or an std::tuple to initialize the corresponding element with multiple arguments.
		// Complexity: O(n).
		template <typename... CTypes>
		inline bool emplace(size_t where, CTypes&& ... args) {
			if (this->mysize == this->mycapacity) {
				if (!reserve(this->mycapacity * 2)) return false;
			}
			if (where > this->mysize) return false;
			_soa_base<Ts...>& base = *this;
			base.emplace(where, args...);
			++this->mysize;
			return true;
		}

		// emplace(where)
		// Increases the size of the container by 1 and inserts newly-constructed objects using default constructors into the arrays at location 'where'.
		// If we're out of space, reserve is called to expand the size of the buffer.
		// Returns false if a memory allocation error occurs in reserve or if 'where' is out of bounds, true otherwise.
		// Complexity: O(n).
		inline bool emplace(size_t where) {
			if (this->mysize == this->mycapacity) {
				if (!reserve(this->mycapacity * 2)) return false;
			}
			if (where > this->mysize) return false;
			_soa_base<Ts...>& base = *this;
			base.emplace_default(where);
			++this->mysize;
			return true;
		}

		// pop_back()
		// Reduces the size of the container by 1 and destructs the items at the back of the arrays.
		// Complexity: O(1).
		inline void pop_back() {
			if (this->mysize == 0) return;
			_soa_base<Ts...>& base = *this;
			base.pop_back();
			--this->mysize;
		}

		// erase_swap(where)
		// Swaps the items at 'where' in each array with the back of the container,
		// then destructs the rear of the container and recuces the size by 1.
		// Complexity: O(1).
		inline void erase_swap(size_t where) {
			if (where >= this->mysize) return;
			_soa_base<Ts...>& base = *this;
			base.erase_swap(where);
			--this->mysize;
		}

		// erase_shift(where)
		// Destructs the item in each array at 'where', then moves each item after it 1 space forward.
		// Maintains the ordering of a sorted container.
		// Complexity: O(n).
		inline void erase_shift(size_t where) {
			if (where >= this->mysize) return;
			_soa_base<Ts...>& base = *this;
			base.erase_shift(where);
			--this->mysize;
		}

		// swap_entries(first, second)
		// Swaps the 'first' and 'second' entries in each array.
		// Complexity: O(1).
		inline void swap_entries(size_t first, size_t second) {
			if (first >= this->mysize || second >= this->mysize) return;
			_soa_base<Ts...>& base = *this;
			base.swap_entries(first, second);
		}

		// empty()
		// Returns true if the container is empty, false otherwise.
		inline bool empty() const { return (this->mysize == 0); }
		// size()
		// Returns the number of items in the container.
		inline size_t size() const { return this->mysize; }
		// max_size()
		// Returns the maximum number of items that this container could theoretically hold.
		// Does not account for running out of memory.
		inline const size_t max_size() const { return SIZE_MAX; }
		// capacity()
		// Returns the number of items that this container could hold before needing to reserve additional memory.
		inline size_t capacity() const { return this->mycapacity; }

		void* get_raw_data() {
			return this->data<0>();
		}
		size_t get_raw_capacity() {
			return this->size_per_entry() * this->mycapacity;;
		}

		// serialize()
		// Shrinks the container to the smallest capacity that can contain its entries,
		// then returns a pointer to the raw data buffer that stores the container's data.
		// num_bytes is filled with the number of bytes in that buffer.
		// This function should be used in tandem with 'deserialize' to save and load a container to disk.
		void* serialize(size_t& num_bytes) {
			shrink_to_fit();
			num_bytes = this->size_per_entry() * this->mycapacity;
			return this->data<0>();
		}

		// deserialize(n)
		// Reserves just enough space for n elements,
		// then returns a pointer to the place in memory where the container's data should be copied/read into.
		// Immediately after calling deserialize, the user MUST fill the buffer with EXACTLY the data returned from serialize!
		// num_bytes is filled with the number of bytes in that buffer.
		// This function should be used in tandem with 'serialize' to save and load a container to disk.
		void* deserialize(size_t num_elements, size_t& num_bytes) {
			reserve(num_elements);
			num_bytes = this->size_per_entry() * this->mycapacity;
			this->mysize = num_elements;
			return this->data<0>();
		}

		// sort<K>()
		// Sorts the entries in the table according to the Kth array.
		// Returns the exact number of swaps performed while sorting the data.
		// Complexity: O(nlogn).
		template <size_t K>
		size_t sort() {
			return quicksort(this->data<K>(), 0, this->mysize - 1);
		}

	protected:
		// Ban access to certain parent methods.
		using _soa_base<Ts...>::nullify;
		using _soa_base<Ts...>::divy_buffer;
		using _soa_base<Ts...>::construct_range;
		using _soa_base<Ts...>::destruct_range;
		using _soa_base<Ts...>::copy;
		using _soa_base<Ts...>::emplace_back_default;
		using _soa_base<Ts...>::emplace_default;

		template <typename T>
		size_t quicksort(T* arr, size_t low, size_t high) {
			size_t numswaps = 0;
			// Create a stack and initialize the top.
			std::vector<size_t> stack((high - low) + 2);
			//size_t stack[(high - low) + 1];
			int top = -1;
			// Push initial values of low and high to the stack.
			stack[++top] = low;
			stack[++top] = high;
			// Keep popping from the stack while it's not empty.
			while (top >= 0) {
				// pop h and l
				size_t h = stack[top--];
				size_t l = stack[top--];
				// Set pivot element at its correct position.
				size_t p = partition(arr, l, h, numswaps);
				// If there are elements on the left side, push left to the stack.
				if (p > l + 1) {
					stack[++top] = l;
					stack[++top] = p - 1;
				}
				// If there are elements on the right side, push right to the stack.
				if (p + 1 < h) {
					stack[++top] = p + 1;
					stack[++top] = h;
				}
			}
			return numswaps;
		}
		template <typename T>
		size_t partition(T* arr, size_t low, size_t high, size_t& numswaps) {
			T* pivot = &arr[high];
			size_t i = low;
			for (size_t j = low; j < high; ++j) {
				if (arr[j] < *pivot) {
					swap_entries(i, j); ++numswaps;
					++i;
				}
			}
			swap_entries(i, high); ++numswaps;
			return i;
		}

	};

} // namespace hvh

#endif // HVH_TOOLKIT_STRUCTOFARRAYS_H