#pragma once

#include <string>

namespace ecsact {

struct parse_eval_error {
	int         source_index = -1;
	int         line = -1;
	int         character = -1;
	std::string error_message;
};

} // namespace ecsact
