#include "htable.hpp"
#include <cstdio>

bool hashtable_test() {
	bool success = true;
	printf("Testing hashtable...\n");


	hvh::htable<std::string, int> stringhash;

	stringhash.insert("apple", 61);
	stringhash.insert("banana", 12);
	stringhash.insert("carrot", 33);
	stringhash.insert("donut", 94);
	stringhash.insert("eggplant", 55);
	stringhash.insert("flowers", 36);
	stringhash.insert("ginger", 17);
	stringhash.insert("hashbrowns", 28);
	stringhash.insert("ice cream", 99);
	stringhash.insert("jello", 10);
	stringhash.insert("kale", 711);
	stringhash.insert("lemon", 112);
	stringhash.insert("melon", 313);
	stringhash.insert("nougat", 614);
	stringhash.insert("onion", 615);
	stringhash.insert("parfait", 716);
	stringhash.insert("quiche", 217);
	stringhash.insert("rice", 318);
	stringhash.insert("steak", 919);
	stringhash.insert("tumeric", 220);
	stringhash.insert("u", 21);
	stringhash.insert("vinegar", 222);
	stringhash.insert("water", 323);
	stringhash.insert("x", 824);
	stringhash.insert("y", 725);
	stringhash.insert("z", 626);

	stringhash.insert("banana", 42);
	stringhash.insert("banana", 9001);

	size_t index = stringhash.find("banana", true);
	if (stringhash.at<1>(index) != 12) {
		printf("Failed to find 'banana' in the hash table.\n");
		success = false;
	}

	index = stringhash.find("banana", false);
	if (stringhash.at<1>(index) != 42) {
		printf("Failed to find a second banana.\n");
		success = false;
	}

	index = stringhash.find("banana", false);
	if (stringhash.at<1>(index) != 9001) {
		printf("Failed to find a third banana.\n");
		success = false;
	}

	index = stringhash.find("banana", false);
	if (index != SIZE_MAX) {
		printf("Failed to fail to find a fourth banana.\n");
		success = false;
	}

	stringhash.erase_all("banana");
	index = stringhash.find("banana", true);
	if (index != SIZE_MAX) {
		printf("Failed to fail to find a deleted banana.\n");
		success = false;
	}

	const uint32_t* hashmap;
	size_t hashcap;
	hashmap = stringhash.see_map(hashcap);
	for (int i = 0; i < hashcap; ++i) {
		if (hashmap[i] == (UINT32_MAX))
			printf("[%i]:\t-\n", i);
		else if (hashmap[i] == (UINT32_MAX - 1))
			printf("[%i]:\tx\n", i);
		else
			printf("[%i]:\t%i\n", i, hashmap[i]);
	}

	for (int i = 0; i < stringhash.size(); ++i) {
		printf("[%s]:[%i]\n", stringhash.at<0>(i).c_str, stringhash.at<1>(i));
	}

	size_t swaps = stringhash.sort<1>();
	printf("Sort performed %zi swaps.\n", swaps);

	for (int i = 0; i < stringhash.size(); ++i) {
		printf("[%i]:[%s]\n", stringhash.at<1>(i), stringhash.at<0>(i).c_str);
	}

	return success;
}
