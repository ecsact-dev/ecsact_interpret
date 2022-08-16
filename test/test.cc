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

	auto comp_id = ecsact_invalid_component_id;
	ecsact_meta_get_component_ids(1, &comp_id, nullptr);
	ASSERT_NE(comp_id, ecsact_invalid_component_id);
	auto compo_id = ecsact_id_cast<ecsact_composite_id>(comp_id);

	ASSERT_STREQ("TestComponent", ecsact_meta_component_name(comp_id));
	ASSERT_EQ(ecsact_meta_count_fields(compo_id), 1);

	ecsact_field_id field_id = (ecsact_field_id)-1;
	ecsact_meta_get_field_ids(compo_id, 1, &field_id, nullptr);
	ASSERT_NE((int)field_id, -1);

	ASSERT_STREQ(ecsact_meta_field_name(compo_id, field_id), "num");
	ASSERT_EQ(ecsact_meta_field_type(compo_id, field_id).type, ECSACT_I32);
	ASSERT_EQ(ecsact_meta_field_type(compo_id, field_id).length, 1);

	// Running it again should result in there still being only 1 component
	// because the component name hasn't changed.
	ecsact_parse_runtime_interop(files.data(), files.size());
	ASSERT_EQ(ecsact_meta_count_components(), 1);
}
