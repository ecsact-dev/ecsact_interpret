#include "ecsact/runtime/dynamic.h"
#include "ecsact/runtime/meta.h"

#include <string>
#include <atomic>
#include <unordered_map>

struct comp_def {
	std::string name;
};

static std::atomic_int32_t last_id = 0;
static std::unordered_map<ecsact_component_id, comp_def> comp_defs;

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
	auto& comp_def = comp_defs[comp_id];
	comp_def.name = std::string_view(component_name, component_name_len);

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
