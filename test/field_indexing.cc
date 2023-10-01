#include "gtest/gtest.h"
#include "ecsact/interpret/eval.h"
#include "ecsact/runtime/meta.hh"

#include "test_lib.hh"

TEST(FieldIndexing, NoErrors) {
	auto errs = ecsact_interpret_test_files({"field_indexing.ecsact"});
	EXPECT_EQ(errs.size(), 0) //
		<< "Expected no errors. Instead got: " << errs[0].error_message << "\n";

	auto pkg_id = ecsact::meta::get_package_ids().at(0);

	auto comp_ids = ecsact::meta::get_component_ids(pkg_id);
	auto example_comp_id = comp_ids.at(0);
	auto example_with_index_field_id = comp_ids.at(1);

	auto example_with_index_field_id_field_ids =
		ecsact::meta::get_field_ids(example_with_index_field_id);
	auto example_with_index_field_id_field_id =
		example_with_index_field_id_field_ids.at(0);

	auto example_comp_field_ids = ecsact::meta::get_field_ids(example_comp_id);
	auto example_comp_num_field_id = example_comp_field_ids.at(0);

	auto act_id = ecsact::meta::get_action_ids(pkg_id).at(0);

	auto act_field_ids = ecsact::meta::get_field_ids(act_id);
	ASSERT_EQ(act_field_ids.size(), 1);
	auto act_field_id = act_field_ids.at(0);

	auto act_field_type = ecsact_meta_field_type(
		ecsact_id_cast<ecsact_composite_id>(act_id),
		act_field_id
	);

	ASSERT_EQ(act_field_type.kind, ECSACT_TYPE_KIND_FIELD_INDEX);
	ASSERT_EQ(
		act_field_type.type.field_index.composite_id,
		ecsact_id_cast<ecsact_composite_id>(example_comp_id)
	);
	ASSERT_EQ(
		act_field_type.type.field_index.field_id,
		example_comp_num_field_id
	);
	ASSERT_EQ(act_field_type.length, 0);

	auto comp_field_type = ecsact_meta_field_type(
		ecsact_id_cast<ecsact_composite_id>(example_with_index_field_id),
		example_with_index_field_id_field_id
	);

	ASSERT_EQ(comp_field_type.kind, ECSACT_TYPE_KIND_FIELD_INDEX);
	ASSERT_EQ(
		comp_field_type.type.field_index.composite_id,
		ecsact_id_cast<ecsact_composite_id>(example_comp_id)
	);
	ASSERT_EQ(
		comp_field_type.type.field_index.field_id,
		example_comp_num_field_id
	);
	ASSERT_EQ(comp_field_type.length, 0);
}
