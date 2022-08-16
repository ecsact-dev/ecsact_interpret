#include "ecsact/parse_runtime_interop.h"

#include <concepts>
#include <unordered_map>
#include <string_view>
#include <vector>
#include <string>
#include "ecsact/parse.h"
#include "ecsact/runtime/dynamic.h"
#include "ecsact/runtime/meta.h"

#include "detail/visit_statement.hh"

static ecsact_component_id ensure_component(ecsact_statement statement) {
	auto& data = statement.data.component_statement;
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
			return comp_id;
		}
	}

	return ecsact_create_component(
		data.component_name.data,
		data.component_name.length
	);
}

struct parse_interop_object {
	// key is statement ID, value is declaration ID
	std::unordered_map<int32_t, ecsact_component_id> _components;
	std::unordered_map<int32_t, ecsact_composite_id> _composites;

	void component_interop(ecsact_statement statement) {
		auto comp_id = ensure_component(statement);
		_components[statement.id] = comp_id;
		_composites[statement.id] = ecsact_id_cast<ecsact_composite_id>(comp_id);
	}

	void field_interop(ecsact_statement context, ecsact_statement statement) {
		auto& composite_id = _composites.at(context.id);
		auto& data = statement.data.field_statement;

		std::string field_name(data.field_name.data, data.field_name.length);
		
		ecsact_add_field(
			composite_id,
			field_name.c_str(),
			data.field_type,
			data.length
		);
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
			switch(params.statement->type) {
				case ECSACT_STATEMENT_COMPONENT:
					obj.component_interop(*params.statement);
					break;
				case ECSACT_STATEMENT_BUILTIN_TYPE_FIELD:
					obj.field_interop(*params.context_statement, *params.statement);
					break;
			}

			return ECSACT_PARSE_CALLBACK_CONTINUE;
		}
	);
}
