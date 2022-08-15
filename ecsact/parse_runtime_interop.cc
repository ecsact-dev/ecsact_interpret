#include "ecsact/parse_runtime_interop.h"

#include <concepts>
#include <unordered_map>
#include <string_view>
#include <vector>
#include "ecsact/parse.h"
#include "ecsact/runtime/dynamic.h"
#include "ecsact/runtime/meta.h"

#include "detail/visit_statement.hh"

struct parse_interop_object {
	// key is statement ID, value is declaration
	std::unordered_map<int32_t, ecsact_component_id> _components;

	void statement_interop
		( ecsact_statement            statement
		, ecsact_component_statement  data
		)
	{
		std::string_view comp_name(
			data.component_name.data,
			data.component_name.length
		);

		std::vector<ecsact_component_id> component_ids;
		component_ids.resize(ecsact_meta_count_components());
		ecsact_meta_get_component_ids(
			component_ids.size(),
			component_ids.data(),
			nullptr
		);

		for(auto comp_id : component_ids) {
			if(comp_name == ecsact_meta_component_name(comp_id)) {
				return;
			}
		}

		auto comp_id = ecsact_create_component(
			data.component_name.data,
			data.component_name.length
		);

		_components[statement.id] = comp_id;
	}
};

void ecsact_parse_runtime_interop
	( const char**  file_paths
	, int           file_paths_count
	)
{
	parse_interop_object obj;

	ecsact_parse_with_cpp_callback(
		file_paths,
		file_paths_count,
		[&](ecsact_parse_callback_params params) {
			// This calls one of 3 overloads of obj.statement_interop
			// 1. obj.statement_interop(
			//      context_statement,
			//      context_statement_data,
			//      statement,
			//      statement_data
			//    );
			// 2. obj.statement_interop(
			//      context_statement,
			//      statement,
			//      statement_data
			//    );
			// 3. obj.statement_interop(
			//      statement,
			//      statement_data
			//    );
			ecsact::detail::parse_statement_data_visit(
				*params.statement,
				[&]<typename Data>(Data&& data) {
					constexpr bool can_invoke_no_context = std::invocable
						< decltype(&parse_interop_object::statement_interop)
						, parse_interop_object*
						, ecsact_statement
						, Data
						>;

					if constexpr(can_invoke_no_context) {
						obj.statement_interop(
							*params.statement,
							std::forward<Data>(data)
						);
					} else if(params.context_statement) {
						ecsact::detail::parse_statement_data_visit(
							*params.context_statement,
							[&]<typename CtxData>(CtxData&& ctx_data) {
								constexpr bool can_invoke_with_context_data = std::invocable
									< decltype(&parse_interop_object::statement_interop)
									, parse_interop_object*
									, ecsact_statement
									, CtxData
									, ecsact_statement
									, Data
									>;

								if constexpr(can_invoke_with_context_data) {
									obj.statement_interop(
										*params.context_statement,
										ctx_data,
										*params.statement,
										data
									);
								} else {
									constexpr bool can_invoke_with_context = std::invocable
										< decltype(&parse_interop_object::statement_interop)
										, parse_interop_object*
										, ecsact_statement
										, ecsact_statement
										, Data
										>;
									
									if constexpr(can_invoke_with_context) {
										obj.statement_interop(
											*params.context_statement,
											*params.statement,
											data
										);
									}
								}
							}
						);
					}
				}
			);

			return ECSACT_PARSE_CALLBACK_CONTINUE;
		}
	);
}
