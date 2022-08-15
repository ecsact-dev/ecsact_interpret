#include "gtest/gtest.h"

#include <array>
#include <filesystem>
#include "ecsact/parse_runtime_interop.h"
#include "ecsact/runtime/meta.h"
#include "bazel_sundry/runfiles.hh"

namespace fs = std::filesystem;

TEST(EcsactParseRuntimeInterop, Simple) {
	auto runfiles = bazel_sundry::CreateDefaultRunfiles();
	ASSERT_TRUE(runfiles);

	auto test_ecsact = runfiles->Rlocation(
		"ecsact_parse_runtime_interop/test/test.ecsact"
	);
	ASSERT_FALSE(test_ecsact.empty());
	ASSERT_TRUE(fs::exists(test_ecsact));

	auto files = std::array{test_ecsact.c_str()};

	// Run the interop
	ecsact_parse_runtime_interop(files.data(), files.size());

	// Our test file only contains 1 component
	ASSERT_EQ(ecsact_meta_count_components(), 1);

	// Running it again should result in there still being only 1 component
	// because the component name hasn't changed.
	ecsact_parse_runtime_interop(files.data(), files.size());
	ASSERT_EQ(ecsact_meta_count_components(), 1);
}
