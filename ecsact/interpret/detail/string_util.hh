#pragma once

namespace ecsact::detail {
	int count_char(const auto& string_like, char c, int& out_last_index) {
		auto len = string_like.size();
		int count = 0;
		int index = 0;
		for(; len > index; ++index) {
			if(string_like[index] == c) {
				count += 1;
				out_last_index = index;
			}
		}
		return count;
	}
}
