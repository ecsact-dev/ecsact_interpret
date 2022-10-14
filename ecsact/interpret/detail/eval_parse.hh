#pragma once

#include <filesystem>
#include <array>
#include "ecsact/interpret/parse_eval_error.hh"

#include "./fixed_stack.hh"
#include "./stack_util.hh"
#include "./string_util.hh"
#include "./read_util.hh"

namespace ecsact::detail {

constexpr std::array statement_ending_chars{';', '{', '}', '\n'};

struct statement_and_source {
	ecsact_statement statement;
	std::string source;
};

template<typename InputStream>
struct statement_reader {
	InputStream stream;
	fixed_stack<statement_and_source, 16> stack;
	std::vector<std::string> next_statement_sources;
	ecsact_statement* current_context = nullptr;
	ecsact_parse_status status = {};
	int current_line = 0;
	int current_character = 0;

	void reset() {
		current_line = 0;
		current_character = 0;
		status = {};
		current_context = nullptr;
		next_statement_sources.clear();
		stack.clear();
	}

	bool can_read_next() {
		return !next_statement_sources.empty() || (stream && !stream.eof());
	}

	void read_next() {
		auto previous = try_top(stack);
		auto& next = stack.emplace();

		if(status.code == ECSACT_PARSE_STATUS_EXPECTED_STATEMENT_END) {
			next.source.reserve(1024);
			ecsact::detail::stream_get_until(
				stream,
				next.source,
				statement_ending_chars
			);
			if(!next_statement_sources.empty()) {
				next.source = next_statement_sources.back() + next.source;
				next_statement_sources.pop_back();
			}
			next.source.shrink_to_fit();
		} else if(next_statement_sources.empty()) {
			next.source.reserve(1024);
			ecsact::detail::stream_get_until(
				stream,
				next.source,
				statement_ending_chars
			);
			next.source.shrink_to_fit();
		} else {
			next.source = next_statement_sources.back();
			next_statement_sources.pop_back();
		}

		auto read_data = next.source.data();
		auto read_size = next.source.size();

		current_context = previous ? &previous->get().statement : nullptr;
		[[maybe_unused]]
		auto parse_read_amount = ecsact_parse_statement(
			read_data,
			read_size,
			current_context,
			&next.statement,
			&status
		);

		int last_nl_index = -1;
		current_line += count_char(next.source, '\n', last_nl_index);
		if(last_nl_index != -1) {
			current_character = next.source.size() - last_nl_index;
		} else {
			current_character = next.source.size();
		}

		if(!ecsact_is_error_parse_status_code(status.code)) {
			assert(parse_read_amount == next.source.size());
		}
	}

	void pump_status_code() {
		if(status.code == ECSACT_PARSE_STATUS_OK) {
			// We've reached the end of the current statement. Pop it off the stack.
			stack.pop();
			current_context = nullptr;
		} else if(status.code == ECSACT_PARSE_STATUS_BLOCK_END) {
			// We've reached the end of the current statement and the current block.
			// Pop the current and pop the block.
			stack.pop();
			stack.pop();
			current_context = nullptr;
		}
	}

	void pop_rewind() {
		auto& last = stack.top();
		next_statement_sources.push_back(last.source);
		stack.pop();
		current_context = nullptr;
	}

	void pop_discard() {
		stack.pop();
		current_context = nullptr;
	}

	void eval() {
		auto& last = stack.top();
		auto eval_error = ecsact_eval_statement(
			current_context,
			&last.statement
		);
	}
};

template<typename InputStream>
struct eval_parse_state {
	std::filesystem::path file_path;
	statement_reader<InputStream> reader;
};

template<typename InputStream>
void open_and_parse_package_statement
	( std::vector<eval_parse_state<InputStream>>&  file_states
	, std::vector<parse_eval_error>&               out_errors
	)
{

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
