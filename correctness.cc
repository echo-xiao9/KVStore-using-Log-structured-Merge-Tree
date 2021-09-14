#include <iostream>
#include <cstdint>
#include <string>
#include <ctime>
#include <iomanip>
#include <chrono>
#include "test.h"

class CorrectnessTest : public Test {
private:
	const uint64_t SIMPLE_TEST_MAX = 512;
    const uint64_t LARGE_TEST_MAX = 1024 * 64;

	void regular_test(uint64_t max)
	{
		uint64_t i;
//        store.reset();
		// Test a single key
		EXPECT(not_found, store.get(1));
		store.put(1, "SE");
		EXPECT("SE", store.get(1));
		EXPECT(true, store.del(1));
		EXPECT(not_found, store.get(1));
		EXPECT(false, store.del(1));

		phase();

		// Test multiple key-value pairs
		for (i = 0; i < max; ++i) {
			store.put(i, string(i+1, 's'));
			EXPECT(string(i+1, 's'), store.get(i));
		}
		phase();

		// Test after all insertions
		for (i = 0; i < max; ++i)
			EXPECT(string(i+1, 's'), store.get(i));
		phase();

		// Test deletions
		for (i = 0; i < max; i+=2)
			EXPECT(true, store.del(i));

		for (i = 0; i < max; ++i)
			EXPECT((i & 1) ? string(i+1, 's') : not_found,
			       store.get(i));

		for (i = 1; i < max; ++i)
			EXPECT(i & 1, store.del(i));

		phase();

		report();
	}

public:
	CorrectnessTest(const string &dir, bool v=true) : Test(dir, v)
	{
	}

	void start_test(void *args = NULL) override
	{
		cout << "KVStore Correctness Test" << endl;

		cout << "[Simple Test]" << endl;
		regular_test(SIMPLE_TEST_MAX);

        cout << "[Large Test]" << endl;
        regular_test(LARGE_TEST_MAX);
	}
};

int main(int argc, char *argv[])
{
    bool verbose = (argc == 2 && string(argv[1]) == "-v");

	cout << "Usage: " << argv[0] << " [-v]" << endl;
	cout << "  -v: print extra info for failed tests [currently ";
	cout << (verbose ? "ON" : "OFF")<< "]" << endl;
	cout << endl;
	cout.flush();

	CorrectnessTest test("./data", verbose);

    test.start_test();

	return 0;
}
