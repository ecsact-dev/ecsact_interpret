#include "gtest/gtest.h"
#include "ecsact/interpret/eval.h"
#include "ecsact/runtime/meta.hh"

#include "test/test_lib.hh"

TEST(EcsactInterpret, AllowAnonymousSystem) {
	auto errs = ecsact_interpret_test_files({"anonymous_system.ecsact"});
	ASSERT_EQ(errs.size(), 0) //
		<< "Expected no errors. Instead got: " << errs[0].error_message << "\n";

	auto package_ids = ecsact::meta::get_package_ids();
	ASSERT_EQ(package_ids.size(), 1);

	auto sys_ids = ecsact::meta::get_system_ids(package_ids[0]);
	ASSERT_EQ(sys_ids.size(), 1);

	auto sys_name = ecsact::meta::system_name(sys_ids[0]);
	ASSERT_TRUE(sys_name.empty());

	auto sys_caps = ecsact::meta::system_capabilities(sys_ids[0]);
	ASSERT_EQ(sys_caps.size(), 1);

	auto comp_ids = ecsact::meta::get_component_ids(package_ids[0]);
	ASSERT_EQ(comp_ids.size(), 1);

	ASSERT_EQ(sys_caps.begin()->second, ECSACT_SYS_CAP_REMOVES);
	ASSERT_EQ(
		sys_caps.begin()->first,
		ecsact_id_cast<ecsact_component_like_id>(comp_ids[0])
	);
}
