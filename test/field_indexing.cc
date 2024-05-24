#include "gtest/gtest.h"

#include "ecsact/runtime/meta.hh"
#include "test_lib.hh"

TEST(FieldIndexing, NoErrors) {
	auto errs = ecsact_interpret_test_files({"field_indexing.ecsact"});
	EXPECT_EQ(errs.size(), 0) //
		<< "Expected no errors. Instead got: " << errs[0].error_message << "\n";

	auto pkg_id = ecsact::meta::get_package_ids().at(0);

	auto example_comp = get_component_by_name(pkg_id, "Example");
	ASSERT_TRUE(example_comp);

	auto single_field_index_comp =
		get_component_by_name(pkg_id, "SingleFieldIndex");
	ASSERT_TRUE(single_field_index_comp);

	auto field_index_action = get_action_by_name(pkg_id, "FieldIndexAction");
	ASSERT_TRUE(field_index_action);

	auto multi_field_comp = get_component_by_name(pkg_id, "MultiField");
	ASSERT_TRUE(multi_field_comp);

	auto indexed_multi_field = get_component_by_name(pkg_id, "IndexedMultiField");
	ASSERT_TRUE(indexed_multi_field);

	auto multi_field_index_system =
		get_system_by_name(pkg_id, "MultiFieldIndexSystem");
	ASSERT_TRUE(multi_field_index_system);

	auto multi_field_index_system_assoc_ids =
		ecsact::meta::system_assoc_ids(*multi_field_index_system);
	ASSERT_EQ(multi_field_index_system_assoc_ids.size(), 1);

	ASSERT_EQ(
		ecsact::meta::system_assoc_component_id(
			*multi_field_index_system,
			multi_field_index_system_assoc_ids.at(0)
		),
		ecsact_id_cast<ecsact_component_like_id>(*indexed_multi_field)
	);
}
