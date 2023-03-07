#include "gtest/gtest.h"
#include "ecsact/interpret/eval.h"

#include "test/test_lib.hh"

TEST(NoPackageStatementFirst, NoPackageStatementFirst) {
	auto errs =
		ecsact_interpret_test_files({"errors/no_package_statement_first.ecsact"});
	ASSERT_EQ(errs.size(), 1);
	ASSERT_EQ(errs[0].eval_error, ECSACT_EVAL_ERR_EXPECTED_PACKAGE_STATEMENT);
}
