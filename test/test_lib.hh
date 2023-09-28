#pragma once

#include <iostream>
#include <vector>
#include <filesystem>
#include "gtest/gtest.h"
#include "ecsact/interpret/eval.hh"
#include "bazel_sundry/runfiles.hh"

inline auto ecsact_interpret_test_files(std::vector<std::string> relative_file_paths)
	-> std::vector<ecsact::parse_eval_error> {
	auto runfiles = bazel_sundry::CreateDefaultRunfiles();
	[&] { ASSERT_TRUE(runfiles); }();

	std::vector<std::filesystem::path> file_paths;
	file_paths.reserve(relative_file_paths.size());

	for(auto rel_p : relative_file_paths) {
		auto path_rloc = "ecsact_interpret_test/" + rel_p;
		auto path = runfiles->Rlocation(path_rloc);
		[&] {
			ASSERT_FALSE(path.empty()) << "couldn't find: " << path_rloc << "\n";
			ASSERT_TRUE(std::filesystem::exists(path)) << path << " does not exist\n";
		}();
		file_paths.emplace_back(path);
	}

	[&] { ASSERT_FALSE(file_paths.empty()) << "cannot interpret 0 files\n"; }();

	auto errs = ecsact::eval_files(file_paths);

	for(auto& err : errs) {
		std::cerr //
			<< "[ERROR] " << file_paths[err.source_index].generic_string() << ":"
			<< err.line << ":" << err.character << " " << err.error_message << "\n";
	}

	return errs;
}
