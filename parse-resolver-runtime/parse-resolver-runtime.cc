#include "ecsact/runtime/dynamic.h"
#include "ecsact/runtime/meta.h"

#include <string>
#include <atomic>
#include <variant>
#include <vector>
#include <exception>
#include <stdexcept>
#include <unordered_map>
#include <map>
#include <cassert>

struct field {
	std::string name;
	ecsact_builtin_type type;
	int32_t length;
};

class composite {
	int32_t _last_field_id = -1;
public:

	std::map<ecsact_field_id, field> fields;

	inline ecsact_field_id next_field_id() {
		return static_cast<ecsact_field_id>(++_last_field_id);
	}
};

struct comp_def : composite {
	std::string name;
};

struct system_like {
	using cap_comp_map_t = std::unordered_map
		< ecsact_component_id
		, ecsact_system_capability
		>;

	struct cap_entry {
		ecsact_system_capability cap;
		std::unordered_map<ecsact_field_id, cap_comp_map_t> assoc;
	};

	std::unordered_map<ecsact_component_id, cap_entry> caps;
	ecsact_system_like_id parent_system_id = (ecsact_system_like_id)-1;
	std::vector<ecsact_system_id> nested_systems;
};

struct action_def : composite, system_like {
	std::string name;
};

struct system_def : system_like {
	std::string name;
};

using any_def = std::variant<comp_def, action_def, system_def>;

struct package_def {
	std::string name;
	bool main;
};

static std::atomic_int32_t last_id = 0;
static std::unordered_map<ecsact_package_id, package_def> package_defs;
static std::unordered_map<ecsact_decl_id, ecsact_package_id> def_owner_map;
static std::unordered_map<ecsact_component_id, comp_def> comp_defs;
static std::unordered_map<ecsact_system_id, system_def> sys_defs;

template<typename T>
static ecsact_package_id owner_package_id(T id) {
	ecsact_decl_id decl_id = ecsact_id_cast<ecsact_decl_id>(id);
	if(!def_owner_map.contains(decl_id)) {
		return (ecsact_package_id)-1;
	}
	return def_owner_map.at(decl_id);
}

template<typename Callback>
void visit_each_def_collection
	( Callback&& callback
	)
{
	callback(comp_defs);
	callback(sys_defs);
}

template<typename T>
static void set_package_owner(T id, ecsact_package_id owner) {
	assert(package_defs.contains(owner));
	def_owner_map[ecsact_id_cast<ecsact_decl_id>(id)] = owner;
}

static composite& get_composite(ecsact_composite_id id) {
	if(comp_defs.contains((ecsact_component_id)id)) {
		return comp_defs.at((ecsact_component_id)id);
	}

	throw std::invalid_argument("Invalid composite ID");
}

static system_like& get_system_like(ecsact_system_like_id id) {
	if(sys_defs.contains((ecsact_system_id)id)) {
		return sys_defs.at((ecsact_system_id)id);
	}

	throw std::invalid_argument("Invalid system-like ID");
}

template<typename T>
static T next_id() {
	return static_cast<T>(last_id++);
}

ecsact_package_id ecsact_create_package
	( bool         main_package
	, const char*  package_name
	, int32_t      package_name_len
	)
{
	auto pkg_id = next_id<ecsact_package_id>();
	auto& pkg = package_defs[pkg_id];
	pkg.main = main_package;
	pkg.name = std::string_view(package_name, package_name_len);
	return pkg_id;
}

void ecsact_destroy_package
	( ecsact_package_id package_id
	)
{
	auto itr = package_defs.find(package_id);
	if(itr == package_defs.end()) {
		return;
	}

	package_defs.erase(itr);

	visit_each_def_collection([&](auto& defs) {
		for(auto itr = defs.begin(); itr != defs.end();) {
			if(owner_package_id(itr->first) != package_id) {
				++itr;
			} else {
				itr = defs.erase(itr);
			}
		}
	});

	auto owner_itr = def_owner_map.begin();
	while(owner_itr != def_owner_map.end()) {
		if(owner_itr->second != package_id) {
			++owner_itr;
		} else {
			owner_itr = def_owner_map.erase(owner_itr);
		}
	}
}

int32_t ecsact_meta_count_packages() {
	return static_cast<int32_t>(package_defs.size());
}

void ecsact_meta_get_package_ids
	( int32_t             max_package_count
	, ecsact_package_id*  out_package_ids
	, int32_t*            out_package_count
	)
{

	auto itr = package_defs.begin();
	for(int i=0; max_package_count > i; ++i, ++itr) {
		if(itr == package_defs.end()) {
			break;
		}
		out_package_ids[i] = itr->first;
	}

	if(out_package_count != nullptr) {
		*out_package_count = static_cast<int32_t>(package_defs.size());
	}
}

const char* ecsact_meta_package_name
	( ecsact_package_id package_id
	)
{
	if(package_defs.contains(package_id)) {
		return package_defs.at(package_id).name.c_str();
	}

	return nullptr;
}

ecsact_component_id ecsact_create_component
	( ecsact_package_id  owner
	, const char*        component_name
	, int32_t            component_name_len
	)
{
	auto comp_id = next_id<ecsact_component_id>();
	set_package_owner(comp_id, owner);
	auto& def = comp_defs[comp_id];
	def.name = std::string_view(component_name, component_name_len);

	return comp_id;
}

ecsact_system_id ecsact_create_system
	( ecsact_package_id  owner
	, const char*        system_name
	, int32_t            system_name_len
	)
{
	auto sys_id = next_id<ecsact_system_id>();
	set_package_owner(sys_id, owner);
	auto& def = sys_defs[sys_id];
	def.name = std::string_view(system_name, system_name_len);

	return sys_id;
}

int32_t ecsact_meta_count_systems() {
	return static_cast<int32_t>(sys_defs.size());
}

void ecsact_meta_get_system_ids
	( int32_t            max_system_count
	, ecsact_system_id*  out_system_ids
	, int32_t*           out_system_count
	)
{
	auto itr = sys_defs.begin();
	for(int32_t i=0; max_system_count > i && itr != sys_defs.end(); ++i) {
		out_system_ids[i] = itr->first;
		++itr;
	}

	if(out_system_count != nullptr) {
		*out_system_count = static_cast<int32_t>(sys_defs.size());
	}
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

const char* ecsact_meta_system_name
	( ecsact_system_id sys_id
	)
{
	return sys_defs.at(sys_id).name.c_str();
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

	def.fields[field_id] = {
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
		++itr;
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

void ecsact_set_system_capability
	( ecsact_system_like_id     sys_id
	, ecsact_component_id       comp_id
	, ecsact_system_capability  cap
	)
{
	auto& def = get_system_like(sys_id);
	def.caps[comp_id].cap = cap;
}

void ecsact_unset_system_capability
	( ecsact_system_like_id  sys_id
	, ecsact_component_id    comp_id
	)
{
	auto& def = get_system_like(sys_id);
	def.caps.erase(comp_id);
}

int32_t ecsact_meta_system_capabilities_count
	( ecsact_system_like_id system_id
	)
{
	auto& def = get_system_like(system_id);
	return static_cast<int32_t>(def.caps.size());
}

void ecsact_meta_system_capabilities
	( ecsact_system_like_id      system_id
	, int32_t                    max_capabilities_count
	, ecsact_component_id*       out_capability_component_ids
	, ecsact_system_capability*  out_capabilities
	, int32_t*                   out_capabilities_count
	)
{
	auto& def = get_system_like(system_id);
	auto itr = def.caps.begin();
	for(int i=0; max_capabilities_count > i; ++i) {
		if(i >= def.caps.size() || itr == def.caps.end()) {
			break;
		}

		out_capability_component_ids[i] = itr->first;
		out_capabilities[i] = itr->second.cap;

		++itr;
	}

	if(out_capabilities_count != nullptr) {
		*out_capabilities_count = static_cast<int32_t>(def.caps.size());
	}
}

void ecsact_set_system_association_capability
	( ecsact_system_like_id     sys_id
	, ecsact_component_id       comp_id
	, ecsact_field_id           with_entity_field_id
	, ecsact_component_id       with_comp_id
	, ecsact_system_capability  with_comp_cap
	)
{
	auto& def = get_system_like(sys_id);
	auto& assoc_field = def.caps.at(comp_id).assoc[with_entity_field_id];
	assoc_field[with_comp_id] = with_comp_cap;
}

void ecsact_unset_system_association_capability
	( ecsact_system_like_id   sys_id
	, ecsact_component_id     comp_id
	, ecsact_field_id         with_entity_field_id
	, ecsact_component_id     with_comp_id
	)
{
	auto& def = get_system_like(sys_id);
	assert(def.caps.contains(comp_id));
	assert(def.caps.at(comp_id).assoc.contains(with_entity_field_id));
	auto& assoc_field = def.caps.at(comp_id).assoc.at(with_entity_field_id);
	assoc_field.erase(with_comp_id);
}

void ecsact_remove_child_system
	( ecsact_system_like_id  parent
	, ecsact_system_id       child
	)
{
	auto& child_def = sys_defs.at(child);
	if(child_def.parent_system_id != parent) {
		return;
	}

	auto& parent_def = get_system_like(parent);
	auto itr = std::find(
		parent_def.nested_systems.begin(),
		parent_def.nested_systems.end(),
		child
	);

	child_def.parent_system_id = (ecsact_system_like_id)-1;
	if(itr != parent_def.nested_systems.end()) {
		parent_def.nested_systems.erase(itr);
	}
}

void ecsact_add_child_system
	( ecsact_system_like_id  parent
	, ecsact_system_id       child
	)
{
	auto& child_def = sys_defs.at(child);
	auto& parent_def = get_system_like(parent);

	if((int32_t)child_def.parent_system_id != -1) {
		auto& prev_parent_def = get_system_like(child_def.parent_system_id);
		ecsact_remove_child_system(parent, child);
	}
	
	child_def.parent_system_id = parent;
	parent_def.nested_systems.push_back(child);
}
