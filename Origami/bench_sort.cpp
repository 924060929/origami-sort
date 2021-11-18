#include "commons.h"
#include "Writer.h"
#include "utils.h"
#include "merge_utils.h"
#include "sorter.h"
#include <iostream>

template<typename Reg, typename Item>
void sort_bench(ui writer_type = MT) {
//#define STD_CORRECTNESS
	print_size<Reg, Item>();
	const ui Itemsize = sizeof(Item);
	ui64 size = GB(1LLU);
	int repeat = 3;
	ui64 n_items = size / Itemsize;

	printf("Running origami-sort --> n: %llu ...\n", n_items);

	Item* data = (Item*)VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
	Item* data_back = (Item*)VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
	Item* end = data + n_items;
	Item* output = (Item*)VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);

	datagen::Writer<Item> writer;
	writer.generate(data, n_items, writer_type);
	memset(output, 0, size);
	memcpy(data_back, data, size);


#ifdef STD_CORRECTNESS
	printf("Sorting with std::sort ... ");
	Item* sorted = (Item*)VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE); 
	memcpy(sorted, data, size);
	hrc::time_point st1 = hrc::now();
	SortEvery(sorted, n_items, n_items);
	hrc::time_point en1 = hrc::now();
	printf("done, ");
	double el1 = ELAPSED_MS(st1, en1);
	printf("elapsed: %.2f ms, Speed: %.2f M/s\n", el1, (n_items / el1 / 1e3));
#endif	

	double avgS = 0;
	FOR(i, repeat, 1) {
		memcpy(data, data_back, size);
		Item* data2 = data;
		Item* end2 = data2 + n_items;
		Item* output2 = output;
		Item* o = data;

		hrc::time_point st1 = hrc::now();
		o = origami_sorter::sort_single_thread<Item, Reg>(data2, output2, end2, n_items);
		hrc::time_point en1 = hrc::now();

		printf("\r                               \r");

		// fix output ptr
		double el = ELAPSED(st1, en1);
		double sp = double(n_items) / el / 1e6;
		avgS += sp;

#ifdef STD_CORRECTNESS
		printf("Iter %3lu done, checking correctness w/ std::sort ... ", i);
		if (!SortCorrectnessCheckerSTD(o, sorted, n_items)) {
			printf("Correctness error @ %llu\n", i);
			exit(1);
		}
		printf("done\r                                                                    \r");
#else 
		if (!SortCorrectnessChecker(o, n_items)) {
			printf("Correctness error @ %llu\n", i);
			break;
			//system("pause");
			//exit(1);
		}
#endif
		printf("\rIter: %llu / %llu", i + 1, repeat);
	}
	avgS /= repeat;
	printf("\nSpeed: %.2f M keys/sec\n", avgS);
	// prints to prevent compiler optimizations
	if (data[13] & 0x123 == output[13]) printf("%u %u\n", data[13], output[13]);
	PRINT_DASH(50);

	VirtualFree(data, 0, MEM_RELEASE);
	VirtualFree(data_back, 0, MEM_RELEASE);			
	VirtualFree(output, 0, MEM_RELEASE);
#ifdef STD_CORRECTNESS
	VirtualFree(sorted, 0, MEM_RELEASE);
#endif 
#undef STD_CORRECTNESS
}

int main() {

	// single thread sort test
	sort_bench<Regtype, Itemtype>(MT);
	system("pause");

	return 0;
}