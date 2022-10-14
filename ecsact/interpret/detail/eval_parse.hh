#pragma once

#include <filesystem>
#include <array>
#include "ecsact/interpret/parse_eval_error.hh"

namespace ecsact::detail {

constexpr std::array statement_ending_chars{';', '{', '}'};

template<typename InputStream>
struct eval_parse_state {
	std::filesystem::path file_path;
	InputStream file_stream;
	int current_line = 0;
	int current_character = 0;
};

template<typename InputStream>
void open_and_parse_package_statement
	( std::vector<eval_parse_state<InputStream>>&  file_states
	, std::vector<parse_eval_error>&               out_errors
	)
{
	std::string curr_line_str;

	for(auto& state : file_states) {
		state.file_stream.open(state.file_path);

		curr_line_str.reserve(1024);
		stream_get_until(state.file_stream, curr_line_str, statement_ending_chars);
		curr_line_str.shrink_to_fit();

		ecsact_statement statement{};
		ecsact_parse_status parse_status{};
		auto parse_read_amount = ecsact_parse_statement(
			curr_line_str.c_str(),
			curr_line_str.size(),
			nullptr,
			&statement,
			&parse_status
		);

		int last_nl_index = -1;
		state.current_line += count_char(curr_line_str, '\n', last_nl_index);
		if(last_nl_index != -1) {
			state.current_line = curr_line_str.size() - last_nl_index;
		} else {
			state.current_line = -1;
		}

		if(parse_read_amount != curr_line_str.size()) {
			// TODO(zaucy): Report unexpected read length
		}
	}
}

template<typename InputStream>
void parse_imports
	( std::vector<eval_parse_state<InputStream>>&  file_states
	, std::vector<parse_eval_error>&               out_errors
	)
{

}

template<typename InputStream>
void check_unknown_imports
	( std::vector<eval_parse_state<InputStream>>&  file_states
	, std::vector<parse_eval_error>&               out_errors
	)
{

}

template<typename InputStream>
void check_cyclic_imports
	( std::vector<eval_parse_state<InputStream>>&  file_states
	, std::vector<parse_eval_error>&               out_errors
	)
{

}

template<typename InputStream>
void eval_package_statements
	( std::vector<eval_parse_state<InputStream>>&  file_states
	, std::vector<parse_eval_error>&               out_errors
	)
{

}

template<typename InputStream>
void eval_imports
	( eval_parse_state<InputStream>&  file_states
	, std::vector<parse_eval_error>&  out_errors
	)
{

}

template<typename InputStream>
void parse_eval_declarations
	( eval_parse_state<InputStream>&  file_state
	, std::vector<parse_eval_error>&  out_errors
	)
{

}

template<typename InputStream>
std::vector<std::reference_wrapper<eval_parse_state<InputStream>>> get_sorted_states
	( std::vector<eval_parse_state<InputStream>>&  file_states
	)
{
	return {};
}

}
