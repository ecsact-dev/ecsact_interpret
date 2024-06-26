#include "gtest/gtest.h"
#include "ecsact/interpret/eval.h"

#include "test_lib.hh"

TEST(InvalidAssocField, NonEntityBuiltin) {
	auto errs =
		ecsact_interpret_test_files({"errors/invalid_assoc_field.ecsact"});
	ASSERT_EQ(errs.size(), 1);
	ASSERT_EQ(errs[0].eval_error, ECSACT_EVAL_ERR_INVALID_ASSOC_FIELD_TYPE);
}
