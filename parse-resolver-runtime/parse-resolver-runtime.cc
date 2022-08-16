#include "ecsact/runtime/dynamic.h"
#include "ecsact/runtime/meta.h"

#include <string>
#include <atomic>
#include <variant>
#include <vector>
#include <exception>
#include <unordered_map>
#include <map>

struct field {
	std::string name;
	ecsact_builtin_type type;
	int32_t length;
};

struct composite {
	std::map<ecsact_field_id, field> fields;

	ecsact_field_id next_field_id() {
		if(fields.empty()) {
			return static_cast<ecsact_field_id>(0);
		}

		auto last = --fields.end();
		return static_cast<ecsact_field_id>(
			static_cast<int32_t>(last->first) + 1
		);
	}
};

struct comp_def : composite {
	std::string name;
};

struct system_like {

};

struct action_def : composite, system_like {
	std::string name;
};

struct system_def : system_like {
	std::string name;
};

using any_def = std::variant<comp_def, action_def, system_def>;

static std::atomic_int32_t last_id = 0;
static std::unordered_map<ecsact_component_id, comp_def> comp_defs;

static composite& get_composite(ecsact_composite_id id) {
	if(comp_defs.contains((ecsact_component_id)id)) {
		return comp_defs.at((ecsact_component_id)id);
	}

	throw std::invalid_argument("Invalid composite ID");
}

template<typename T>
static T next_id() {
	return static_cast<T>(last_id++);
}

ecsact_component_id ecsact_create_component
	( const char*  component_name
	, int32_t      component_name_len
	)
{
	auto comp_id = next_id<ecsact_component_id>();
	auto& def = comp_defs[comp_id];
	def.name = std::string_view(component_name, component_name_len);

	return comp_id;
}

int32_t ecsact_meta_count_components() {
	return static_cast<int32_t>(comp_defs.size());
}

const char* ecsact_meta_component_name
	( ecsact_component_id comp_id
	)
{
	return comp_defs.at(comp_id).name.c_str();
}

void ecsact_meta_get_component_ids
	( int32_t               max_component_count
	, ecsact_component_id*  out_component_ids
	, int32_t*              out_component_count
	)
{
	auto itr = comp_defs.begin();
	for(int32_t i=0; max_component_count > i && itr != comp_defs.end(); ++i) {
		out_component_ids[i] = itr->first;
		++itr;
	}

	if(out_component_count != nullptr) {
		*out_component_count = static_cast<int32_t>(comp_defs.size());
	}
}

ecsact_field_id ecsact_add_field
	( ecsact_composite_id  composite_id
	, const char*          field_name
	, ecsact_builtin_type  field_type
	, int32_t              length
	)
{
	auto& def = get_composite(composite_id);
	auto field_id = def.next_field_id();

	def.fields[def.next_field_id()] = {
		.name = field_name,
		.type = field_type,
		.length = length,
	};

	return field_id;
}

int32_t ecsact_meta_count_fields
	( ecsact_composite_id composite_id
	)
{
	auto& def = get_composite(composite_id);
	return static_cast<int32_t>(def.fields.size());
}

void ecsact_meta_get_field_ids
	( ecsact_composite_id  composite_id
	, int32_t              max_field_count
	, ecsact_field_id*     out_field_ids
	, int32_t*             out_field_ids_count
	)
{
	auto& def = get_composite(composite_id);

	auto itr = def.fields.begin();
	for(int32_t i=0; max_field_count > i && itr != def.fields.end(); ++i) {
		out_field_ids[i] = itr->first;
	}

	if(out_field_ids_count != nullptr) {
		*out_field_ids_count = static_cast<int32_t>(def.fields.size());
	}
}

const char* ecsact_meta_field_name
	( ecsact_composite_id  composite_id
	, ecsact_field_id      field_id
	)
{
	auto& def = get_composite(composite_id);
	return def.fields.at(field_id).name.c_str();
}

ecsact_field_type ecsact_meta_field_type
	( ecsact_composite_id  composite_id
	, ecsact_field_id      field_id
	)
{
	auto& def = get_composite(composite_id);
	auto& field = def.fields.at(field_id);

	return {
		.type = field.type,
		.length = field.length,
	};
}
