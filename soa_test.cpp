#include "soa.hpp"
#include <string>
using namespace std;

#include <cstdio>

int testdata0[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
string testdata1[] = { "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen", "twenty" };

bool structofarrays_test() {
	printf("Testing structofarrays...\n");

	bool success = true;
	hvh::soa<int, string, short, double> soa;
	int* const& ints = soa.data<0>();
	string* const& strings = soa.data<1>();
	short* const& shorts = soa.data<2>();
	double* const& doubles = soa.data<3>();

	if (soa.capacity() != 0) {
		printf("Capacity of a newly created struct-of-arrays should be 0.\n");
		success = false;
	}
	if (soa.size() != 0) {
		printf("Size of a newly created struct-of-arrays should be 0.\n");
		success = false;
	}

	if (ints != nullptr || strings != nullptr || shorts != nullptr || doubles != nullptr) {
		printf("Data of newly-constructed soa should all be nullptr.\n");
		return false;
	}

	if (soa.push_back(0, "zero", 0, 0.0) == false) {
		printf("push_back failed.\n");
		success = false;
	}

	if (soa.size() != 1) {
		printf("Size should be 1, instead it's %zi.\n", soa.size());
		success = false;
	}
	if (soa.capacity() != 16) {
		printf("Capacity should 16, instead it's %zi.\n", soa.capacity());
		success = false;
	}

	for (int i = 1; i < 16; ++i) {
		soa.push_back(i, testdata1[i], (short)-i, (double)i);
	}

	if (soa.size() != 16) {
		printf("Size should be 16, instead it's %zi.\n", soa.size());
		success = false;
	}

	if (soa.capacity() != 16) {
		printf("Capacity should be 16, instead it's %zi.\n", soa.size());
		success = false;
	}

	size_t ints_raw = (size_t)ints;
	size_t strings_raw = (size_t)strings;
	size_t shorts_raw = (size_t)shorts;
	size_t doubles_raw = (size_t)doubles;

	if (strings_raw != (ints_raw + (sizeof(int) * soa.capacity())) ||
		shorts_raw != (strings_raw + (sizeof(string) * soa.capacity())) ||
		doubles_raw != (shorts_raw + (sizeof(short) * soa.capacity())))
	{
		printf("Arrays are not contiguous!\n");
		success = false;
	}

	for (int i = 16; i < 21; ++i) {
		soa.push_back(i, testdata1[i], (short)-i, (double)i);
	}

	if (soa.size() != 21) {
		printf("Size should be 21, instead it's %zi.\n", soa.size());
		success = false;
	}

	if (soa.capacity() != 32) {
		printf("Capacity should be 32, instead it's %zi.\n", soa.capacity());
		success = false;
	}

	for (int i = 0; i < 21; ++i) {
		if (ints[i] != soa.at<0>(i)) {
			printf("Array access does not match 'at'.\n");
			success = false;
		}
		if (strings[i] != soa.at<1>(i)) {
			printf("Array access does not match 'at'.\n");
			success = false;
		}
		if (shorts[i] != soa.at<2>(i)) {
			printf("Array access does not match 'at'.\n");
			success = false;
		}
		if (doubles[i] != soa.at<3>(i)) {
			printf("Array access does not match 'at'.\n");
			success = false;
		}

		if (ints[i] != i || strings[i] != testdata1[i] || shorts[i] != (short)-i || doubles[i] != (double)i) {
			printf("soa contents do not match expected value.\n");
			printf("Expected: %i, %s, %i, %f.\n", i, testdata1[i].c_str(), -i, (double)i);
			printf("Got: %i, %s, %i, %f.\n", ints[i], strings[i].c_str(), shorts[i], doubles[i]);
			success = false;
		}
	}

	soa.reserve(1010);
	if (soa.capacity() != 1024) {
		printf("Capacity should be 1024, instead it's %zi.\n", soa.capacity());
		success = false;
	}
	if (soa.size() != 21) {
		printf("Size should still be 21, instead it's %zi.\n", soa.size());
		success = false;
	}

	size_t index = soa.lower_bound<0>(10);
	if (index >= soa.size() || soa.at<0>(index) != 10) {
		printf("lower_bound failed to find '10'.\n");
		success = false;
	}

	soa.insert(index, 10, "10", 1010, 1010.0);
	soa.insert(index, 10, "TEN", -1010, -1010.0);
	soa.insert(index, 10, "TEEEEEEEN", 11010, 101010.0);

	size_t begin = soa.lower_bound<0>(10);
	size_t end = soa.upper_bound<0>(10);
	if ((end - begin) != 4) {
		printf("There should be 4 entries with key '10' in the list.\n");
		printf("begin = %zi, end = %zi.\n", begin, end);
		success = false;
	}
	if (soa.at<1>(begin) != "TEEEEEEEN") {
		printf("First '10' entry should have string 'TEEEEEEEN'.\n");
		printf("Instead, it's '%s'.\n", soa.at<1>(begin).c_str());
		success = false;
	}
	++begin;
	if (soa.at<1>(begin) != "TEN") {
		printf("Second '10' entry should have string 'TEN'.\n");
		printf("Instead, it's '%s'.\n", soa.at<1>(begin).c_str());
		success = false;
	}
	++begin;
	if (soa.at<1>(begin) != "10") {
		printf("Third '10' entry should have string '10'.\n");
		printf("Instead, it's '%s'.\n", soa.at<1>(begin).c_str());
		success = false;
	}
	++begin;
	if (soa.at<1>(begin) != "ten") {
		printf("Fourth '10' entry should have string 'ten'.\n");
		printf("Instead, it's '%s'.\n", soa.at<1>(begin).c_str());
		success = false;
	}

	if (soa.lower_bound<0>(42) != soa.size()) {
		printf("Searching for an item beyond the list should return soa.size().\n");
		success = false;
	}

	if (soa.front<1>() != "zero") {
		printf("Front of the strings array should be 'zero'.\n");
		printf("Instead, it's '%s'.\n", soa.front<1>().c_str());
	}
	if (soa.back<1>() != "twenty") {
		printf("Back of the strings array should be 'twenty'.\n");
		printf("Instead, it's '%s'.\n", soa.back<1>().c_str());
	}

	//soa.emplace_back(21, "twenty one", std::ignore, 21.0);
	//soa.emplace_back(22, std::make_tuple( 2, '2' ), -22, 22.0);

	//if (soa.back<1>() != "22") {
	//	printf("Emplace_back failed to put a tuple in spot 22.");
	//	success = false;
	//}

	index = soa.lower_bound<0>(10);
	soa.erase_shift(index);
	soa.erase_shift(index);
	soa.erase_shift(index);

	soa.pop_back();
	soa.pop_back();

	for (int i = 0; i < soa.size(); ++i) {
		if (ints[i] != i || strings[i] != testdata1[i] || shorts[i] != (short)-i || doubles[i] != (double)i) {
			printf("soa contents do not match expected value.\n");
			printf("Expected: %i, %s, %i, %f.\n", i, testdata1[i].c_str(), -i, (double)i);
			printf("Got: %i, %s, %i, %f.\n", ints[i], strings[i].c_str(), shorts[i], doubles[i]);
			success = false;
		}
	}

	soa.clear();
	if (soa.size() != 0) {
		printf("Size after clear should be 0.\n");
		success = false;
	}
	if (soa.capacity() != 1024) {
		printf("Capacity after clear should still be 1024.\n");
		success = false;
	}

	soa.shrink_to_fit();
	if (soa.capacity() != 0) {
		printf("Capacity after shrink_to_fit should be 0.\n");
		success = false;
	}

	if (ints != nullptr || strings != nullptr || shorts != nullptr || doubles != nullptr) {
		printf("Array pointers should be null after shrinking capacity to 0.\n");
		success = false;
	}

	return success;
}