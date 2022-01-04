/* htable.hpp
 * A Hash Table implementation utilizing a Struct-Of-Arrays
 * by Haydn V. Harach
 * Created October 2019
 * Modified March 2021
 *
 * By storing a lightweight hashmap alongside a struct-of-arrays, memory
 * efficiency is improved compared to a traditional hash table.
 * In addition, this table can store more than 1 item types.
 */
#ifndef HVH_TOOLS_HASHTABLESOA_H
#define HVH_TOOLS_HASHTABLESOA_H

#include "soa.hpp"

namespace hvh {

	template <typename KeyT, typename... ItemTs>
	class htable : public soa<KeyT, ItemTs...> {
	public:

		// htable()
		// Default constructor for a hash table.
		// Initial size, capacity, and hashmap size are 0.
		// Complexity: O(1).
		htable() {}
		// htable(...)
		// Constructs a hash table using a list of tuples.
		// Initializes the table with the entries from the list; the leftmost item is the key.
		// Complexity: O(n).
		htable(const std::initializer_list<std::tuple<KeyT, ItemTs...>>& initlist) {
			reserve(initlist.size());
			for (auto& entry : initlist) {
				std::apply([=](const KeyT& key, const ItemTs& ... items) {this->insert(key, items...); }, entry);
			}
		}
		// htable(&& rhs)
		// Move constructor for a hash table.
		// Moves the entries from the rhs hash table into ourselves.
		// Complexity: O(1).
		htable(htable<KeyT, ItemTs...>&& other) { swap(*this, other); }
		// htable(const& rhs)
		// Copy constructor for a hash table.
		// Initializes the hash table as a copy of rhs.
		// Complexity: O(n).
		htable(const htable<KeyT, ItemTs...>& other) {
			reserve(other.capacity());
			memcpy(hashmap, other.hashmap, sizeof(uint32_t) * hashcapacity);
			_soa_base<KeyT, ItemTs...>& base = *this;
			const _soa_base<KeyT, ItemTs...>& otherbase = other;
			base.copy(otherbase);
		}
		// operator = (&& rhs)
		// Move-assignment operator for a hash table.
		// Moves the entries from the rhs hash table into ourselves, replacing old contents.
		// Complexity: O(1).
		htable<KeyT, ItemTs...>& operator = (htable&& other) { swap(*this, other); return *this; }
		// operator = (& rhs)
		// Copy-assignment operator for a hash table.
		// Copies the entries from the rhs hash table into ourselves, replacing old contents.
		// Complexity: O(n).
		htable<KeyT, ItemTs...>& operator = (htable other) { swap(*this, other); return *this; }
		// ~htable()
		// Destructor for a hash table.
		// Calls the destructor for all contained keys and items, then frees held memory.
		// Complexity: O(n).
		~htable() {
			_soa_base<KeyT, ItemTs...>& base = *this;
			base.destruct_range(0, this->mysize);
			base.nullify();
			this->mysize = 0;
			this->mycapacity = 0;
			if (hashmap) _soa_aligned_free(hashmap);
		}

		// swap(lhs, rhs)
		// swaps the contents of two htables.
		// Complexity: O(1).
		friend inline void swap(htable<KeyT, ItemTs...>& lhs, htable<KeyT, ItemTs...>& rhs) {
			std::swap(lhs.hashmap, rhs.hashmap);
			std::swap(lhs.hashcapacity, rhs.hashcapacity);
			std::swap(lhs.hashcursor, rhs.hashcursor);
			soa<KeyT, ItemTs...>& lhsbase = lhs;
			soa<KeyT, ItemTs...>& rhsbase = rhs;
			swap(lhsbase, rhsbase);
		}

		// clear()
		// Erases all entries from the htable, destructing all keys and items.
		// The capacity of the hash table is unchanged.
		// Complexity: O(n).
		inline void clear() {
			memset(hashmap, INDEXNUL, sizeof(uint32_t) * hashcapacity);
			soa<KeyT, ItemTs...>& base = *this;
			base.clear();
			hashcursor = SIZE_MAX;
		}

		// rehash()
		// Recalculates the hash for all keys in the table.
		// Called automatically if the table is resized, and can be used to clear up deleted indices in the map.
		// Complexity: O(n).
		void rehash() {
			memset(hashmap, INDEXNUL, sizeof(uint32_t) * hashcapacity);
			for (size_t i = 0; i < this->mysize; ++i) {
				// Get the hash for this key.
				size_t hash = std::hash<KeyT>{}(this->template at<0>(i)) % hashcapacity;
				// Figure out where to put it.
				while (1) {
					// If this spot is NULL or DELETED, we can put our reference here.
					uint32_t index = hashmap[hash];
					if (index == INDEXNUL || index == INDEXDEL) {
						hashmap[hash] = (uint32_t)i;
						break;
					}
					// Otherwise, keep looking.
					hash_inc(hash);
				}
			}
			hashcursor = SIZE_MAX;
		}

		// reserve(n)
		// Ensures that the hash table is large enough to hold at least n entries.
		// Called auomatically when trying to insert an entry into a full table.
		// Returns false if a memory allocation error occurs, true otherwise.
		// Complexity: O(n).
		bool reserve(size_t newsize) {
			// For alignment, we must have a multiple of 16 items.
			if (newsize % 16 != 0)
				newsize += 16 - (newsize % 16);

			// We need at least 16 elements.
			if (newsize == 0) newsize = 16;

			// We can't shrink the actual memory.
			if (newsize <= this->mycapacity) return true;

			// Hash capacity needs to be odd and just greater than double the list capacity,
			// but it also needs to conform to 16-byte alignment.
			hashcapacity = newsize + newsize + 4;
			size_t htable_size = hashcapacity * sizeof(uint32_t);
			--hashcapacity;

			// Remember the old memory so we can free it.
			void* oldmem = hashmap;

			// Allocate new memory.
			_soa_base<KeyT, ItemTs...>& base = *this;
			void* alloc_result = _soa_aligned_malloc(16, (base.size_per_entry() * newsize) + htable_size);
			if (!alloc_result) return false;

			hashmap = (uint32_t*)alloc_result;

			// Copy the old data into the new memory.
			this->mycapacity = newsize;
			base.divy_buffer(((char*)alloc_result) + htable_size);

			// Free the old memory.
			if (oldmem) _soa_aligned_free(oldmem);
			rehash();
			return true;
		}

		// shrink_to_fit()
		// Shrinks the capacity of the hash table to the smallest possible size capable of holding the existing entries.
		// Returns false if a memory allocation error occurs, true otherwise.
		// Complexity: O(n).
		bool shrink_to_fit() {
			// For alignment, we must have a multiple of 16 items.
			size_t newsize = this->mysize;
			if (newsize % 16 != 0)
				newsize += 16 - (newsize % 16);

			// If the container is already as small as it can be, bail out now.
			if (newsize == this->mycapacity) return true;

			// Remember the old memory so we can free it.
			_soa_base<KeyT, ItemTs...>& base = *this;
			void* oldmem = hashmap;

			if (newsize > 0) {
				// Hash capacity needs to be odd and just greater than double the list capacity,
				// but it also needs to conform to 16-byte memory alignment.
				hashcapacity = newsize + newsize + 4;
				size_t htable_size = hashcapacity * sizeof(uint32_t);
				--hashcapacity;

				// Allocate new memory.
				void* alloc_result = _soa_aligned_malloc(16, (base.size_per_entry() * newsize) + htable_size);
				if (!alloc_result) return false;

				hashmap = (uint32_t*)alloc_result;

				// Copy the old data into the new memory.
				this->mycapacity = newsize;
				base.divy_buffer(((char*)alloc_result) + htable_size);
			}
			else {
				base.nullify();
				this->mycapacity = 0;
				hashcapacity = 0;
				hashmap = nullptr;
			}

			// Free the old memory.
			if (oldmem) _soa_aligned_free(oldmem);
			rehash();
			return true;
		}

		// insert(key, items...)
		// Inserts a new entry into the hash table.
		// Hash tables may store multiple entries with the same key.
		// Returns false if a memory allocation failure occurs in reserve(), true otherwise.
		// Complexity: O(1) amortized.
		template <typename... Ts>
		bool insert(const KeyT& key, Ts&&... items) {
			if (this->mysize == max_size()) return false;
			if (this->mysize == this->mycapacity) {
				if (!reserve(this->mycapacity * 2)) return false;
			}
			// Get the hash for the key.
			size_t hash = std::hash<KeyT>{}(key) % hashcapacity;
			// Look through the table for a place to put it...
			while (1) {
				uint32_t index = hashmap[hash];
				if (index == INDEXNUL || index == INDEXDEL) {
					index = (uint32_t)this->mysize;
					soa<KeyT, ItemTs...>& base = *this;
					base.push_back(key, std::forward<Ts>(items)...);
					hashmap[hash] = index;
					break;
				}
				hash_inc(hash);
			}
			return true;
		}

		// emplace(key, args...)
		// Constructs a new entry in the hash table in-place.
		// Hash tables may store multiple entries with the same key.
		// Returns false if a memory allocation failure occurs in reserve(), true otherwise.
		// Complexity: O(1) amortized.
		template <typename... CTypes>
		bool emplace(const KeyT& key, CTypes&&... cargs) {
			if (this->mysize == max_size()) return false;
			if (this->mysize == this->mycapacity) {
				if (!reserve(this->mycapacity * 2)) return false;
			}
			// Get the hash for the key.
			size_t hash = std::hash<KeyT>{}(key) % hashcapacity;
			// Look through the table for a place to put it...
			while (1) {
				uint32_t index = hashmap[hash];
				if (index == INDEXNUL || index == INDEXDEL) {
					index = (uint32_t)this->mysize;
					soa<KeyT, ItemTs...>& base = *this;
					base.emplace_back(key, std::forward<CTypes>(cargs)...);
					hashmap[hash] = index;
					break;
				}
				hash_inc(hash);
			}
			return true;
		}

		// insert_sorted<K>(key, items...)
		// Inserts a new entry into the hash table sorted according to the Kth array.
		// Possibly useful if the data needs to be sorted for some reason other than searching.
		// Returns false if a memory allocation failure occurs in reserve(), true otherwise.
		// Complexity: O(n).
		template <size_t K>
		bool insert_sorted(const KeyT& key, const ItemTs&... items) {
			if (this->mysize == max_size()) return false;
			if (this->mysize == this->mycapacity) {
				if (!reserve(this->mycapacity * 2)) return false;
			}
			soa<KeyT, ItemTs...>& base = *this;
			size_t where = base.lower_bound_row<K>(key, items...);
			base.insert(where, key, items...);
			rehash();
		}

		// find(key, restart)
		// Searches for the entry with the indicated key.
		// Returns the index of the found entry, or SIZE_MAX if the key could not be found.
		// If 'restart' is true, find will get the first entry with the matching key.
		// If 'restart' is false, find will get the next entry with the matching key after the entry previously found by 'find'.
		// To iterate over every entry with a given key in the table, use the following loop template:
		// `for (size_t i = find(key, true); i != SIZE_MAX; i = find(key, false)) { ... }`
		// The returned index can be used with 'at' or 'data' to access the items inside the table.
		// Complexity: O(1) amortized.
		size_t find(const KeyT& key, bool restart = true) {
			if (this->mysize == 0) return SIZE_MAX;
			if (restart) hashcursor = std::hash<KeyT>{}(key) % hashcapacity;
			else {
				if (hashcursor >= hashcapacity) return SIZE_MAX;
				hash_inc(hashcursor);
			}
			while (1) {
				uint32_t index = hashmap[hashcursor];
				if (index == INDEXNUL) return SIZE_MAX;
				if (index != INDEXDEL && this->template at<0>(index) == key) return (size_t)index;
				hash_inc(hashcursor);
			}
			return SIZE_MAX;
		}

		// find(key) const
		// Searches for the entry with the indicated key.
		// Returns the index of the found entry, or SIZE_MAX if the key could not be found.
		// The returned index can be used with 'at' or 'data' to access the items inside the table.
		// Complexity: O(1) amortized.
		size_t find(const KeyT& key) const {
			if (this->mysize == 0) return SIZE_MAX;
			size_t hash = std::hash<KeyT>{}(key) % hashcapacity;
			while (1) {
				uint32_t index = hashmap[hash];
				if (index == INDEXNUL) return SIZE_MAX;
				if (index != INDEXDEL && this->template at<0>(index) == key) return (size_t)index;
				hash_inc(hash);
			}
		}

		// find(key, restart, hashc) const
		// Searches for the entry with the indicated key.
		// Returns the index of the found entry, or SIZE_MAX if the key could not be found.
		// The returned index can be used with 'at' or 'data' to access the items inside the table.
		// This version allows iterating over multiple items like the regular 'find',
		// but uses an external hash cursor to maintain const-compatible.
		// Complexity: O(1) amortized.
		size_t find(const KeyT& key, bool restart, size_t& hashc) const {
			if (this->mysize == 0) return SIZE_MAX;
			if (restart) hashc = std::hash<KeyT>{}(key) % hashcapacity;
			else {
				if (hashc >= hashcapacity) return SIZE_MAX;
				hash_inc(hashc);
			}
			while (1) {
				uint32_t index = hashmap[hashc];
				if (index == INDEXNUL) return SIZE_MAX;
				if (index != INDEXDEL && this->template at<0>(index) == key) return (size_t)index;
				hash_inc(hashc);
			}
		}

		// count(key)
		// Returns the number of entries in the table which have the indicated key.
		// If no entries in the table have the indicated key, 0 is returned.
		// Complexity: O(1) amortized.
		inline size_t count(const KeyT& key) const {
			size_t result = 0;
			size_t hashc = SIZE_MAX;
			for (size_t index = find(key, true, hashc); index != SIZE_MAX; index = find(key, false, hashc)) {
				++result;
			}
			return result;
		}

		// swap_entries(first, second)
		// Swaps the position of two entries and repairs the hashes for each.
		// Complexity: O(1) amortized.
		void swap_entries(size_t first, size_t second) {
			// Swap the two entries.
			soa<KeyT, ItemTs...>& base = *this;
			base.swap_entries(first, second);

			// Find the hash position for the first entry.
			size_t hash = std::hash<KeyT>{}(this->at<0>(first)) % hashcapacity;
			size_t first_hashpos = SIZE_MAX;
			while (1) {
				uint32_t index = hashmap[hash];
				if (index == (uint32_t)first) {
					first_hashpos = hash;
					break;
				}
				if (index == INDEXNUL) {
					// ERROR! We can't repair the link!
					break;
				}
				hash_inc(hash);
			}

			// Find the hash position for the second entry.
			hash = std::hash<KeyT>{}(this->at<0>(second)) % hashcapacity;
			size_t second_hashpos = SIZE_MAX;
			while (1) {
				uint32_t index = hashmap[hash];
				if (index == (uint32_t)second) {
					second_hashpos = hash;
					break;
				}
				if (index == INDEXNUL) {
					// ERROR! We can't repair the link!
					break;
				}
				hash_inc(hash);
			}

			// Swap the hash positions.
			hashmap[first_hashpos] = (uint32_t)second;
			hashmap[second_hashpos] = (uint32_t)first;
		}

		// erase_found()
		// Erases the entry which was found by the last call to 'find'.
		// If the last call to 'find' did not find its goal, the table is unchanged.
		// Returns the number of items erased (0 or 1).
		// Complexity: O(1) amortized.
		size_t erase_found() {
			if (hashcursor >= hashcapacity) return 0;
			uint32_t index = hashmap[hashcursor];
			if (index == INDEXNUL || index == INDEXDEL) return 0;
			soa<KeyT, ItemTs...>& base = *this;
			base.erase_swap(index);
			hashmap[hashcursor] = INDEXDEL;

			// Get the hash of the key that we just moved into the deleted item's place.
			size_t hash = std::hash<KeyT>{}(this->template at<0>(index)) % hashcapacity;
			// Scan through looking for the reference so we can repair it.
			while (1) {
				uint32_t newindex = hashmap[hash];
				if (newindex == this->mysize) {
					// Repair the link.
					hashmap[hash] = index;
					break;
				}
				if (newindex == INDEXNUL) {
					// ERROR! We can't repair the link!
					break;
				}
				hash_inc(hash);
			}
			return 1;
		}

		// erase(key)
		// Finds the entry with the indicated key and erases it.
		// If no entries have the indicated key, the table is unchanged.
		// Returns the number of items erased (0 or 1).
		// Complexity: O(1) amortized.
		inline size_t erase(const KeyT& key) {
			find(key, true);
			return erase_found();
		}
		// erase_all(key)
		// Erases all entries with the indicated key from the table.
		// If no entries have the indicated key, the table is unchanged.
		// Returns the number of items erased.
		// Complexity: O(1) amortized.
		inline size_t erase_all(const KeyT& key) {
			size_t result = 0;
			for (size_t index = find(key, true); index != SIZE_MAX; index = find(key, false)) {
				result += erase_found();
			}
			return result;
		}

		// erase_found_sorted()
		// Erases the entry which was found by the last call to 'find',
		// maintaining the order of the data.
		// Returns the number of items erased (0 or 1).
		// Complexity: O(n).
		size_t erase_found_sorted() {
			if (hashcursor >= hashcapacity) return 0;
			uint32_t index = hashmap[hashcursor];
			if (index == INDEXNUL || index == INDEXDEL) return 0;
			soa<KeyT, ItemTs...>& base = *this;
			base.erase_shift(index);
			hashmap[hashcursor] = INDEXDEL;
			rehash();
			return 1;
		}
		// erase_sorted(key)
		// Finds the entry with the indicated key and erases it, maintaining the order of the data.
		// If no entries have the indicated key, the table is unchanged.
		// Returns the number of items erased (0 or 1).
		// Complexity: O(n).
		inline size_t erase_sorted(const KeyT& key) {
			find(key, true);
			return erase_found_sorted();
		}


		// max_size()
		// Returns the greatest number of entries that this hash table could theoretically hold.
		// Does not account for running out of memory.
		inline constexpr size_t max_size() const {
			return UINT_MAX - 2;
		}

		// see_map()
		// Used for debugging to see if there are any big clumps in the hash map.
		const uint32_t* see_map(size_t& cap) { cap = hashcapacity; return hashmap; }

		// serialize()
		// Shrinks the container to the smallest capacity that can contain its entries,
		// then returns a pointer to the raw data buffer that stores the container's data.
		// num_bytes is filled with the number of bytes in that buffer.
		// This function should be used in tandem with 'deserialize' to save and load a container to disk.
		void* serialize(size_t& num_bytes) {
			shrink_to_fit();
			num_bytes = (this->size_per_entry() * this->mycapacity) + (sizeof(uint32_t) * hashcapacity);
			return hashmap;
		}

		// deserialize(n)
		// Reserves just enough space for n elements,
		// then returns a pointer to the place in memory where the container's data should be copied/read into.
		// Immediately after calling deserialize, the user MUST fill the buffer with EXACTLY the data returned from serialize!
		// num_bytes is filled with the number of bytes in that buffer.
		// This function should be used in tandem with 'serialize' to save and load a container to disk.
		void* deserialize(size_t num_elements, size_t& num_bytes) {
			reserve(num_elements);
			num_bytes = (this->size_per_entry() * this->mycapacity) + (sizeof(uint32_t) * hashcapacity);
			this->mysize = num_elements;
			return hashmap;
		}

		// sort<K>()
		// Sorts the entries in the table according to the Kth array, then does a rehash.
		// Potentially useful if the data needs to be sorted for some reason other than searching.
		// Returns the exact number of swaps performed while sorting the data.
		// Complexity: O(nlogn).
		template <size_t K>
		size_t sort() {
			size_t result = this->quicksort(this->template data<K>(), 0, this->mysize-1);
			rehash();
			return result;
		}

	protected:

		inline void hash_inc(size_t& h) const { h = ((h + 2) % hashcapacity); }

		static const uint32_t INDEXNUL = UINT_MAX;
		static const uint32_t INDEXDEL = UINT_MAX - 1;

		uint32_t* hashmap = nullptr;
		size_t hashcapacity = 0;
		size_t hashcursor = SIZE_MAX;

		// Ban certain inherited methods.
	//	using soa<KeyT, ItemTs...>::clear;
	//	using soa<KeyT, ItemTs...>::reserve;
	//	using soa<KeyT, ItemTs...>::shrink_to_fit;
		using soa<KeyT, ItemTs...>::resize;
		using soa<KeyT, ItemTs...>::push_back;
		using soa<KeyT, ItemTs...>::emplace_back;
		using soa<KeyT, ItemTs...>::pop_back;
	//	using soa<KeyT, ItemTs...>::insert;
		using soa<KeyT, ItemTs...>::erase_swap;
		using soa<KeyT, ItemTs...>::erase_shift;
	//	using soa<KeyT, ItemTs...>::swap;
	//	using soa<KeyT, ItemTs...>::swap_entries;
	};

} // namespace hvh

#endif // HVH_TOOLS_HASHTABLESOA_H
