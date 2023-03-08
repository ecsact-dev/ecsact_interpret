#include "gtest/gtest.h"
#include "ecsact/interpret/eval.h"
#include "ecsact/runtime/meta.hh"

#include "test/test_lib.hh"

TEST(MultiPkgTest, NoErrors) {
	auto errs = ecsact_interpret_test_files({
		"multi_pkg_main.ecsact",
		"multi_pkg_a.ecsact",
		"multi_pkg_b.ecsact",
		"multi_pkg_c.ecsact",
	});
	EXPECT_EQ(errs.size(), 0) //
		<< "Expected no errors. Instead got: " << errs[0].error_message << "\n";

	EXPECT_EQ(ecsact_meta_count_packages(), 4);

	auto pkg_ids = ecsact::meta::get_package_ids();
	for(auto pkg_id : pkg_ids) {
		auto pkg_name = ecsact::meta::package_name(pkg_id);

		if(pkg_name == "multi.pkg") {
			EXPECT_EQ(ecsact_meta_count_dependencies(pkg_id), 3) //
				<< "Expected main package to have 3 dependencies. One for each import";
		} else if(pkg_name == "multi.pkg.a") {

		} else if(pkg_name == "multi.pkg.b") {

		} else if(pkg_name == "multi.pkg.c") {
			
		} else {
			EXPECT_TRUE(false) << "No tests for package: " << pkg_name;
		}
	}
}
