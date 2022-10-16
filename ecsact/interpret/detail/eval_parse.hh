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

template<typename InputStream>
struct statement_reader {
	InputStream stream;
	fixed_stack<ecsact_statement, 16> statements;
	fixed_stack<std::string, 16> sources;
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
		statements.clear();
		sources.clear();
	}

	bool can_read_next() {
		return !next_statement_sources.empty() || (stream && !stream.eof());
	}

	void read_next() {
		auto previous_statement = try_top(statements);
		auto& next_statement = statements.emplace();
		auto& next_source = sources.emplace();

		if(status.code == ECSACT_PARSE_STATUS_EXPECTED_STATEMENT_END) {
			next_source.reserve(1024);
			ecsact::detail::stream_get_until(
				stream,
				next_source,
				statement_ending_chars
			);
			if(!next_statement_sources.empty()) {
				next_source = next_statement_sources.back() + next_source;
				next_statement_sources.pop_back();
			}
			next_source.shrink_to_fit();
		} else if(next_statement_sources.empty()) {
			next_source.reserve(1024);
			ecsact::detail::stream_get_until(
				stream,
				next_source,
				statement_ending_chars
			);
			next_source.shrink_to_fit();
		} else {
			next_source = next_statement_sources.back();
			next_statement_sources.pop_back();
		}

		auto read_data = next_source.data();
		auto read_size = next_source.size();

		current_context = previous_statement
			? &previous_statement->get()
			: nullptr;
		[[maybe_unused]]
		auto parse_read_amount = ecsact_parse_statement(
			read_data,
			read_size,
			current_context,
			&next_statement,
			&status
		);

		int last_nl_index = -1;
		current_line += count_char(next_source, '\n', last_nl_index);
		if(last_nl_index != -1) {
			current_character = next_source.size() - last_nl_index;
		} else {
			current_character = next_source.size();
		}

		if(!ecsact_is_error_parse_status_code(status.code)) {
			assert(parse_read_amount == next_source.size());
		}
	}

	void pump_status_code() {
		if(status.code == ECSACT_PARSE_STATUS_OK) {
			// We've reached the end of the current statement. Pop it off the stack.
			statements.pop();
			sources.pop();
			current_context = nullptr;
		} else if(status.code == ECSACT_PARSE_STATUS_BLOCK_END) {
			// We've reached the end of the current statement and the current block.
			// Pop the current and pop the block.
			statements.pop();
			sources.pop();
			statements.pop();
			sources.pop();
			current_context = nullptr;
		}
	}

	void pop_rewind() {
		auto& last_source = sources.top();
		next_statement_sources.push_back(last_source);
		statements.pop();
		sources.pop();
		current_context = nullptr;
	}

	void pop_discard() {
		statements.pop();
		sources.pop();
		current_context = nullptr;
	}
};

template<typename InputStream>
struct eval_parse_state {
	std::filesystem::path file_path;
	statement_reader<InputStream> reader;
	std::optional<ecsact_package_id> package_id;
	std::vector<std::string> imports;
};

template<typename InputStream>
void parse_package_statements
	( std::vector<eval_parse_state<InputStream>>&  file_states
	, std::vector<parse_eval_error>&               out_errors
	)
{
	auto source_index = 0;
	for(auto& state : file_states) {
		state.reader.read_next();
		
		out_errors.push_back(parse_eval_error{
			.source_index = source_index,
			.line = state.reader.current_line,
			.character = state.reader.current_character,
			.error_message = "",
		});

		source_index += 1;
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
