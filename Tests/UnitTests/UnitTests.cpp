#include "stdafx.h"

#undef trace
#include <gtest/gtest.h>


// example usage:
// UnitTests.exe --gtest_list_tests
// UnitTests.exe --gtest_filter=Formatter_tests*
// UnitTests.exe --gtest_filter=*Dispatch_level*
int main(int argc, char **argv){
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}