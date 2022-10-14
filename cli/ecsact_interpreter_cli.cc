#include <type_traits>
#include <iostream>
#include "ecsact/parse.h"
#include "ecsact/interpret/eval.h"

static void stream_get_until(auto& stream, auto& output, auto&& delimiters) {
	while(stream) {
		std::remove_reference_t<decltype(output.back())> c;
		stream.get(c);
		if(stream.eof()) {
			break;
		}

		output.push_back(c);

		for(auto& delim : delimiters) {
			if(c == delim) {
				return;
			}
		}
	}
}

int main(int argc, char* argv[]) {

	

	return 0;
}
