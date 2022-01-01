# SOA / htable
A struct-of-arrays template container plus a hashtable built upon it.

This is a header-only library consisting of two files: `soa.hpp`, which contains the struct-of-arrays container, and `htable.hpp`, which contains the hash table.  The two `_test.cpp` files contain tests to ensure that the containers work properly, and are not required for the library to be used.

### Installation

Just place `soa.hpp` and `htable.hpp` anywhere that your project can include them, and `#include` them as needed.

### Usage

Both `soa` and `htable` are in the `hvh` namespace in order to avoid name collisions.  

### SOA

`soa` is a template container, like `std::vector`, but uses variadic templates to accept multiple types.  To create an `soa` which stores floats, ints, and strings, one would use `hvh::soa<float, int, std::string> my_soa;`.  The data stored in an `soa` is contained within a single contiguous block of memory, arranged such that elements of the same 'array' are adjascent to each other; see [this wikipedia article](https://en.wikipedia.org/wiki/AoS_and_SoA) for an overview of what a struct-of-arrays container entails.  Most functions which access the data of an `soa` take a template parameter indicating which 'column' to look in (with 0 being the first column); for our above example, `my_soa.at<0>(i)` would access a float, and `my_soa.at<2>(i)` would access a string. `soa` contains the following methods:

- `data<K>()` returns a reference to a pointer to the Kth array.  Elements of the array can be modified, but the array itself cannot.  As a reference, it remains valid even after reallocation.
- `at<K>(i)` returns a reference to the ith element of the Kth array.
- `front<K>()` returns a reference to the element at the front of the Kth array.
- `back<K>()` returns a reference to the element at the back of the Kth array.
- `lower_bound<K>(goal)` performs a binary search looking for 'goal' in the Kth array, which is assumed to be sorted (ensuring this is the responsibility of the user).  Returns the index of the leftmost element which equals 'goal', or the number of items which are less than 'goal' if 'goal' cannot be found.  Behaves similarly to `std::lower_bound`.
- `lower_bound_row<K>(goal...)` Like lower_bound, but the user provides an entire row of data instead of just the key.
- `upper_bound<K>(goal)` performs a binary search looking for 'goal' in the Kth array, which is assumed to be sorted.  If 'goal' is found, returns the index of the leftmost element which is greater than 'goal'; otherwise, returns the number of items which are less than 'goal'.  This behaviour is similar to `std::upper_bound`.
- `swap(lhs, rhs)` swaps the contents of two soa's.
- `clear()` clears and destructs all held items; does not change capacity.
- `reserve(n)` reserves at least enough memory to store n items without needing to resize.  Returns false if a memory allocation error occurs.
- `shrink_to_fit()` shrinks the capacity to the smallest amount that can hold all currently held items.  If the capacity is reduced, this triggers a memory re-allocation.  Returns false if a memory allocation error occurs, true otherwise.
- `resize(n)` Resizes the container to contain exactly n items.  New items are default-constructed, lost items are destructed.  Returns false if a memory allocation error occurs, true otherwise.
- `resize(n, args...)` As 'resize', but uses 'args' to initialize new entries.
- `push_back(args...)` Pushes a new row onto the back of the container and copies 'args' into place.
- `emplace_back(args...)` Pushes a new row onto the back of the container and uses 'args' to call constructors.
- `insert(where, args...)` Inserts a new row at 'where', shifting everything after 'where' forward by 1 spot.
- `emplace(where, args...)` As 'insert', but uses 'args' to call constructors.
- `pop_back()` removes the row at the back of the container.
- `erase_swap(where)` erases the entry at 'where' by swapping it with the entry at the back of the container and then removing the entry at the back.
- `erase_shift(where)` erases the entry at 'where' and then shifts all entries beyond it backwards by 1, maintaining order.
- `empty()` Returns true if the container is empty, false otherwise.
- `size()` returns the number of entries in the container.
- `max_size()` returns the number of items which could hypothetically be stored in the container in a universe with infinite memory.
- `capacity()` returns the number of items that this container could hold before needing to allocate additional memory.
- `sort<K>()` performs a quicksort on the entire table according to the elements in the Kth array.
- `serialize(num_bytes&)` Calls 'shrink_to_fit', fills out 'bytes' with the total number of bytes used by the container, then returns a pointer to the raw data buffer that stores the container's data.  This data can then be saved to disc, if needed.
- `deserialize(num_elements, num_bytes&)` Reserves just enough space for 'num_elements' elements, then returns a pointer to the raw data buffer where a serialized table can be copied into.

### htable

`htable` inherits from `soa`, and shares much of its functionality.  The 'hash table' is an array of indices stored alongside the 'soa' data which stores the actual keys and data.  The 0th array is used as the key for the table, and multiple entries may have the same key.  Many of the methods provided by 'soa' are available here, and should "just work", with the following important additions/changes:

- `resize`, `push_back`, `emplace_back`, `pop_back`, `erase_swap`, and `erase_shift` are not available.
- `insert(args...)` No longer has a 'where' parameter; it performs a hashtable insert and places the row at the back of the container.
- `emplace(args...)` Like 'Insert', no longer has a 'where' parameter.
- `find(key, restart)` searches for the indicated 'key', and if found, returns the index to the associated entry.  If 'key' cannot be found, returns `SIZE_MAX`.  If 'restart' is true (default), this will always be the first entry with the matching key; if 'restart' is false, this will be the next entry with the matching key after the entry returned by the previous call to 'find', or `SIZE_MAX` if there are no more entries with that key.
- `count(key)` Returns the number of entries in the table with the indicated key.
- `erase_found()` Erases the entry which was returned by the most recent call to 'find'.
- `erase(key)` Finds the key, then erases it if it can.
- `erase_all(key)` Erases every entry with the given 'key'. 
- `erase_found_sorted()` As 'erase_found', but maintains the order of the table.
- `erase_sorted()` as 'erase', but maintains the order of the table.
- `insert_sorted<K>(args)` Inserts a row while maintaining the ordering of the Kth array.
- `rehash()` Recalculates the hash for all keys in the table.
- `swap_entries(first, second)` Swaps the position of two entries and repairs the hashes for each.
