#include "gtest/gtest.h"

#include <array>
#include <filesystem>
#include <unordered_map>
#include <functional>
#include "ecsact/parse_runtime_interop.h"
#include "ecsact/runtime/meta.h"
#include "bazel_sundry/runfiles.hh"

namespace fs = std::filesystem;

template<typename... CallbackPairs>
void for_each_field
	( ecsact_composite_id  compo_id
	, CallbackPairs&&...   callback_pairs
	)
{
	auto field_count = ecsact_meta_count_fields(compo_id);
	std::vector<ecsact_field_id> field_ids;
	field_ids.resize(field_count);
	ecsact_meta_get_field_ids(
		compo_id,
		field_ids.size(),
		field_ids.data(),
		&field_count
	);
	ASSERT_EQ(field_count, field_ids.size());

	using pairs_t = std::unordered_map
		< std::string
		, std::function<void(ecsact_field_id)>
		>;
	pairs_t pairs{std::forward<CallbackPairs>(callback_pairs)...};

	for(auto& field_id : field_ids) {
		std::string field_name = ecsact_meta_field_name(compo_id, field_id);
		ASSERT_TRUE(pairs.contains(field_name))
			<< "Missing test body for field '" << field_name << "'";
		pairs.at(field_name)(field_id);
	}

	ASSERT_EQ(field_ids.size(), pairs.size());
}

TEST(EcsactParseRuntimeInterop, Simple) {
	auto runfiles = bazel_sundry::CreateDefaultRunfiles();
	ASSERT_TRUE(runfiles);

	auto test_ecsact = runfiles->Rlocation(
		"ecsact_parse_runtime_interop/test/test.ecsact"
	);
	ASSERT_FALSE(test_ecsact.empty());
	ASSERT_TRUE(fs::exists(test_ecsact));

	auto files = std::array{test_ecsact.c_str()};

	auto test_interop = [&] {
		ecsact_parse_runtime_interop(files.data(), files.size());

		// Only 1 package
		ASSERT_EQ(ecsact_meta_count_packages(), 1);
		auto package_id = (ecsact_package_id)-1;
		ecsact_meta_get_package_ids(1, &package_id, nullptr);
		ASSERT_NE(package_id, (ecsact_package_id)-1);

		ASSERT_STREQ(ecsact_meta_package_name(package_id), "test");

		// Our test file only contains 1 component
		ASSERT_EQ(ecsact_meta_count_components(), 1);

		auto comp_id = ecsact_invalid_component_id;
		ecsact_meta_get_component_ids(1, &comp_id, nullptr);
		ASSERT_NE(comp_id, ecsact_invalid_component_id);
		auto compo_id = ecsact_id_cast<ecsact_composite_id>(comp_id);

		ASSERT_STREQ("TestComponent", ecsact_meta_component_name(comp_id));
		for_each_field(
			compo_id,
			std::make_pair("num", [&](ecsact_field_id field_id) {
				auto type = ecsact_meta_field_type(compo_id, field_id);
				ASSERT_EQ(type.type, ECSACT_I32);
				ASSERT_EQ(type.length, 1);
			}),
			std::make_pair("test_entity", [&](ecsact_field_id field_id) {
				auto type = ecsact_meta_field_type(compo_id, field_id);
				ASSERT_EQ(type.type, ECSACT_ENTITY_TYPE);
				ASSERT_EQ(type.length, 1);
			})
		);
		
		// Here we're creating a function for each system in our test file to make
		// sure we have the expected capabilities
		using test_system_fns_t = std::unordered_map
			< std::string
			, std::function<void(ecsact_system_id)>
			>;
		test_system_fns_t test_system_fns{
			{"TestSystem", [&](ecsact_system_id sys_id) {
				auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(sys_id);

				ASSERT_EQ(ecsact_meta_system_capabilities_count(sys_like_id), 1);

				std::array sys_cap_comp_ids{ecsact_invalid_component_id};
				std::array sys_caps{ecsact_system_capability{}};
				ecsact_meta_system_capabilities(
					sys_like_id,
					1,
					sys_cap_comp_ids.data(),
					sys_caps.data(),
					nullptr
				);

				ASSERT_EQ(sys_cap_comp_ids[0], comp_id);
				ASSERT_EQ(sys_caps[0], ECSACT_SYS_CAP_READWRITE);
			}},
			{"TestNestedSystem", [&](ecsact_system_id sys_id) {
				auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(sys_id);

				ASSERT_EQ(ecsact_meta_system_capabilities_count(sys_like_id), 1);

				std::array sys_cap_comp_ids{ecsact_invalid_component_id};
				std::array sys_caps{ecsact_system_capability{}};
				ecsact_meta_system_capabilities(
					sys_like_id,
					1,
					sys_cap_comp_ids.data(),
					sys_caps.data(),
					nullptr
				);

				ASSERT_EQ(sys_cap_comp_ids[0], comp_id);
				ASSERT_EQ(sys_caps[0], ECSACT_SYS_CAP_READWRITE);
			}},
		};

		ASSERT_EQ(ecsact_meta_count_systems(), test_system_fns.size());
		std::vector<ecsact_system_id> sys_ids;
		sys_ids.resize(test_system_fns.size());

		int32_t out_count = 0;
		ecsact_meta_get_system_ids(sys_ids.size(), sys_ids.data(), &out_count);
		ASSERT_EQ(sys_ids.size(), out_count);

		for(auto sys_id : sys_ids) {
			std::string sys_name = ecsact_meta_system_name(sys_id);
			ASSERT_TRUE(test_system_fns.contains(sys_name))
				<< "No test function for the '"
				<< sys_name << "' system";
			test_system_fns.at(sys_name)(sys_id);
		}
	};

	test_interop();
	// Running the interop a second time should be no different since all defs are
	// removed.
	test_interop();
}
