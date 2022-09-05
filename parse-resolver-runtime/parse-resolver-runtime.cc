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
#include <limits>
#include <cassert>

struct field {
	std::string name;
	ecsact_field_type type;
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

	/** in execution order */
	std::vector<ecsact_system_id> nested_systems;
};

struct action_def : composite, system_like {
	std::string name;
};

struct system_def : system_like {
	std::string name;
};

struct enum_value {
	std::string name;
	int32_t value;
};

class enum_def {
	int32_t _last_enum_value_id = -1;

public:
	std::string name;
	std::map<ecsact_enum_value_id, enum_value> enum_values;

	inline ecsact_enum_value_id next_enum_value_id() {
		return static_cast<ecsact_enum_value_id>(++_last_enum_value_id);
	}
};

using any_def = std::variant<comp_def, action_def, system_def>;

struct package_def {
	std::string name;
	bool main;
	std::string source_file_path;

	std::vector<ecsact_package_id> dependencies;
	std::vector<ecsact_system_id> systems;
	std::vector<ecsact_action_id> actions;
	std::vector<ecsact_component_id> components;
	std::vector<ecsact_enum_id> enums;

	/** in execution order */
	std::vector<ecsact_system_like_id> top_level_systems;
};

static std::atomic_int32_t last_id = 0;
static std::unordered_map<ecsact_decl_id, std::string> full_names;
static std::unordered_map<ecsact_package_id, package_def> package_defs;
static std::unordered_map<ecsact_decl_id, ecsact_package_id> def_owner_map;
static std::unordered_map<ecsact_component_id, comp_def> comp_defs;
static std::unordered_map<ecsact_system_id, system_def> sys_defs;
static std::unordered_map<ecsact_action_id, action_def> act_defs;
static std::unordered_map<ecsact_enum_id, enum_def> enum_defs;

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

	if(act_defs.contains((ecsact_action_id)id)) {
		return act_defs.at((ecsact_action_id)id);
	}

	throw std::invalid_argument("Invalid composite ID");
}

static system_like& get_system_like(ecsact_system_like_id id) {
	if(sys_defs.contains((ecsact_system_id)id)) {
		return sys_defs.at((ecsact_system_id)id);
	}

	if(act_defs.contains((ecsact_action_id)id)) {
		return act_defs.at((ecsact_action_id)id);
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
	auto& pkg_def = package_defs.at(owner);
	auto comp_id = next_id<ecsact_component_id>();
	auto decl_id = ecsact_id_cast<ecsact_decl_id>(comp_id);
	pkg_def.components.push_back(comp_id);
	set_package_owner(comp_id, owner);
	auto& def = comp_defs[comp_id];
	def.name = std::string_view(component_name, component_name_len);
	full_names[decl_id] = pkg_def.name + "." + def.name;

	return comp_id;
}

ecsact_system_id ecsact_create_system
	( ecsact_package_id  owner
	, const char*        system_name
	, int32_t            system_name_len
	)
{
	auto& pkg_def = package_defs.at(owner);
	const auto sys_id = next_id<ecsact_system_id>();
	const auto decl_id = ecsact_id_cast<ecsact_decl_id>(sys_id);
	const auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(sys_id);
	pkg_def.systems.push_back(sys_id);
	pkg_def.top_level_systems.push_back(sys_like_id);
	set_package_owner(sys_id, owner);
	auto& def = sys_defs[sys_id];
	def.name = std::string_view(system_name, system_name_len);
	if(def.name.empty()) {
		full_names[decl_id] = "";
	} else {
		full_names[decl_id] = pkg_def.name + "." + def.name;
	}

	return sys_id;
}

ecsact_action_id ecsact_create_action
	( ecsact_package_id  owner
	, const char*        action_name
	, int32_t            action_name_len
	)
{
	auto& pkg_def = package_defs.at(owner);
	const auto act_id = next_id<ecsact_action_id>();
	const auto decl_id = ecsact_id_cast<ecsact_decl_id>(act_id);
	const auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(act_id);
	pkg_def.actions.push_back(act_id);
	pkg_def.top_level_systems.push_back(sys_like_id);
	set_package_owner(act_id, owner);
	auto& def = act_defs[act_id];
	def.name = std::string_view(action_name, action_name_len);
	full_names[decl_id] = pkg_def.name + "." + def.name;

	return act_id;
}

ecsact_enum_id ecsact_create_enum
	( ecsact_package_id  owner
	, const char*        enum_name
	, int32_t            enum_name_len
	)
{
	auto& pkg_def = package_defs.at(owner);
	auto enum_id = next_id<ecsact_enum_id>();
	auto& def = enum_defs[enum_id];
	def.name = std::string_view(enum_name, enum_name_len);

	return enum_id;
}

ecsact_enum_value_id ecsact_add_enum_value
	( ecsact_enum_id  enum_id
	, int32_t         value
	, const char*     value_name
	, int32_t         value_name_len
	)
{
	auto& def = enum_defs.at(enum_id);
	auto enum_value_id = def.next_enum_value_id();
	auto& enum_value = def.enum_values[enum_value_id];
	enum_value.name = std::string(value_name, value_name_len);
	enum_value.value = value;
	return enum_value_id;
}

int32_t ecsact_meta_count_systems
	(	ecsact_package_id package_id
	)
{
	auto& pkg_def = package_defs.at(package_id);
	return static_cast<int32_t>(pkg_def.systems.size());
}

void ecsact_meta_get_system_ids
	( ecsact_package_id  package_id
	, int32_t            max_system_count
	, ecsact_system_id*  out_system_ids
	, int32_t*           out_system_count
	)
{
	auto& pkg_def = package_defs.at(package_id);
	auto itr = pkg_def.systems.begin();
	for(int32_t i=0; max_system_count > i && itr != pkg_def.systems.end(); ++i) {
		out_system_ids[i] = *itr;
		++itr;
	}

	if(out_system_count != nullptr) {
		*out_system_count = static_cast<int32_t>(pkg_def.systems.size());
	}
}

int32_t ecsact_meta_count_actions
	(	ecsact_package_id package_id
	)
{
	auto& pkg_def = package_defs.at(package_id);
	return static_cast<int32_t>(pkg_def.actions.size());
}

void ecsact_meta_get_action_ids
	( ecsact_package_id  package_id
	, int32_t            max_action_count
	, ecsact_action_id*  out_action_ids
	, int32_t*           out_action_count
	)
{
	auto& pkg_def = package_defs.at(package_id);
	auto itr = pkg_def.actions.begin();
	for(int32_t i=0; max_action_count > i && itr != pkg_def.actions.end(); ++i) {
		out_action_ids[i] = *itr;
		++itr;
	}

	if(out_action_count != nullptr) {
		*out_action_count = static_cast<int32_t>(pkg_def.actions.size());
	}
}

int32_t ecsact_meta_count_components
	( ecsact_package_id package_id
	)
{
	auto& pkg_def = package_defs.at(package_id);
	return static_cast<int32_t>(pkg_def.components.size());
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

const char* ecsact_meta_action_name
	( ecsact_action_id act_id
	)
{
	return act_defs.at(act_id).name.c_str();
}

int32_t ecsact_meta_count_enums
	( ecsact_package_id package_id
	)
{
	auto& pkg_def = package_defs.at(package_id);
	return static_cast<int32_t>(pkg_def.enums.size());
}

void ecsact_meta_get_enum_ids
	( ecsact_package_id  package_id
	, int32_t            max_enum_count
	, ecsact_enum_id*    out_enum_ids
	, int32_t*           out_enum_count
	)
{
	auto& pkg_def = package_defs.at(package_id);

	auto itr = pkg_def.enums.begin();
	for(int i=0; max_enum_count > i && itr != pkg_def.enums.end(); ++i) {
		out_enum_ids[i] = *itr;
		++itr;
	}

	if(out_enum_count) {
		*out_enum_count = static_cast<int32_t>(pkg_def.enums.size());
	}
}

ecsact_builtin_type ecsact_meta_enum_storage_type
	( ecsact_enum_id enum_id
	)
{
	using std::numeric_limits;

	auto& def = enum_defs.at(enum_id);
	int32_t min_value = numeric_limits<int32_t>::max();
	int32_t max_value = numeric_limits<int32_t>::min();
	for(auto& [_, enum_value] : def.enum_values) {
		if(enum_value.value > max_value) {
			max_value = enum_value.value;
		}

		if(enum_value.value < min_value) {
			min_value = enum_value.value;
		}
	}

	if(min_value < 0) {
		if(max_value > numeric_limits<int16_t>::max()) return ECSACT_I32;
		if(max_value > numeric_limits<int8_t>::max()) return ECSACT_I16;
		return ECSACT_I8;
	} else {
		if(max_value > numeric_limits<uint16_t>::max()) return ECSACT_U32;
		if(max_value > numeric_limits<uint8_t>::max()) return ECSACT_U16;
		return ECSACT_U8;
	}
}

int32_t ecsact_meta_count_enum_values
	( ecsact_enum_id enum_id
	)
{
	auto& def = enum_defs.at(enum_id);
	return static_cast<int32_t>(def.enum_values.size());
}

void ecsact_meta_get_enum_value_ids
	( ecsact_enum_id         enum_id
	, int32_t                max_enum_value_ids
	, ecsact_enum_value_id*  out_enum_value_ids
	, int32_t*               out_enum_values_count
	)
{
	auto& def = enum_defs.at(enum_id);
	
	auto itr = def.enum_values.begin();
	for(int i=0; max_enum_value_ids > i && itr != def.enum_values.end(); ++i) {
		out_enum_value_ids[i] = itr->first;
		++itr;
	}

	if(out_enum_values_count) {
		*out_enum_values_count = static_cast<int32_t>(def.enum_values.size());
	}
}

const char* ecsact_meta_enum_value_name
	( ecsact_enum_id        enum_id
	, ecsact_enum_value_id  enum_value_id
	)
{
	auto& def = enum_defs.at(enum_id);
	return def.enum_values.at(enum_value_id).name.c_str();
}

int32_t ecsact_meta_enum_value
	( ecsact_enum_id        enum_id
	, ecsact_enum_value_id  enum_value_id
	)
{
	auto& def = enum_defs.at(enum_id);
	return def.enum_values.at(enum_value_id).value;
}

void ecsact_meta_get_component_ids
	( ecsact_package_id     package_id
	, int32_t               max_component_count
	, ecsact_component_id*  out_component_ids
	, int32_t*              out_component_count
	)
{
	auto& pkg_def = package_defs.at(package_id);
	auto itr = pkg_def.components.begin();
	for(int32_t i=0; max_component_count > i && itr != pkg_def.components.end(); ++i) {
		out_component_ids[i] = *itr;
		++itr;
	}

	if(out_component_count != nullptr) {
		*out_component_count = static_cast<int32_t>(pkg_def.components.size());
	}
}

ecsact_field_id ecsact_add_field
	( ecsact_composite_id  composite_id
	, ecsact_field_type    field_type
	, const char*          field_name
	, int32_t              field_name_len
	)
{
	auto& def = get_composite(composite_id);
	auto field_id = def.next_field_id();

	def.fields[field_id] = {
		.name = std::string(field_name, field_name_len),
		.type = field_type,
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

	return field.type;
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

	if(!child_def.name.empty()) {
		auto& pkg_def = package_defs.at(owner_package_id(child));
		auto child_decl_id = ecsact_id_cast<ecsact_decl_id>(child);
		full_names.at(child_decl_id) = pkg_def.name + "." + child_def.name;
	}
}

void ecsact_add_child_system
	( ecsact_system_like_id  parent
	, ecsact_system_id       child
	)
{
	auto& child_def = sys_defs.at(child);
	auto& parent_def = get_system_like(parent);
	auto& pkg_def = package_defs.at(owner_package_id(parent));

	if((int32_t)child_def.parent_system_id != -1) {
		auto& prev_parent_def = get_system_like(child_def.parent_system_id);
		ecsact_remove_child_system(parent, child);
	}
	
	child_def.parent_system_id = parent;
	parent_def.nested_systems.push_back(child);

	if(!child_def.name.empty()) {
		auto child_decl_id = ecsact_id_cast<ecsact_decl_id>(child);
		auto parent_decl_id = ecsact_id_cast<ecsact_decl_id>(parent);

		auto& child_full_name = full_names.at(child_decl_id);
		if(!full_names.at(parent_decl_id).empty()) {
			child_full_name = full_names.at(parent_decl_id);
		} else {
			child_full_name = pkg_def.name;
		}

		child_full_name += "." + child_def.name;
	}
}

void ecsact_set_package_source_file_path
	( ecsact_package_id  package_id
	, const char*        source_file_path
	, int32_t            source_file_path_len
	)
{
	assert(source_file_path_len > 0);
	auto& def = package_defs.at(package_id);
	
	def.source_file_path = std::string(
		source_file_path,
		source_file_path_len
	);
}

const char* ecsact_meta_package_file_path
	( ecsact_package_id package_id
	)
{
	auto& def = package_defs.at(package_id);
	return def.source_file_path.c_str();
}

const char* ecsact_meta_enum_name
	( ecsact_enum_id enum_id
	)
{
	auto& def = enum_defs.at(enum_id);
	return def.name.c_str();
}

const char* ecsact_meta_registry_name
	( ecsact_registry_id
	)
{
	// No registries in parse resolver tuneim
	return "";
}

int32_t ecsact_meta_count_dependencies
	( ecsact_package_id package_id
	)
{
	auto& pkg_def = package_defs.at(package_id);	
	return static_cast<int32_t>(pkg_def.dependencies.size());
}

void ecsact_meta_get_dependencies
	( ecsact_package_id   package_id
	, int32_t             max_dependency_count
	, ecsact_package_id*  out_package_ids
	, int32_t*            out_dependencies_count
	)
{
	auto& pkg_def = package_defs.at(package_id);
	auto itr = pkg_def.dependencies.begin();
	for(int i=0; max_dependency_count > i; ++i, ++itr) {
		if(itr == pkg_def.dependencies.end()) {
			break;
		}
		out_package_ids[i] = *itr;
	}

	if(out_dependencies_count) {
		*out_dependencies_count = static_cast<int32_t>(pkg_def.dependencies.size());
	}
}

const char* ecsact_meta_decl_full_name
	( ecsact_decl_id id
	)
{
	return full_names.at(id).c_str();
}

int32_t ecsact_meta_count_child_systems
	( ecsact_system_like_id system_id
	)
{
	auto& def = get_system_like(system_id);
	return static_cast<int32_t>(def.nested_systems.size());
}

void ecsact_meta_get_child_system_ids
	( ecsact_system_like_id  system_id
	, int32_t                max_child_system_ids_count
	, ecsact_system_id*      out_child_system_ids
	, int32_t*               out_child_system_count
	)
{
	auto& def = get_system_like(system_id);

	auto itr = def.nested_systems.begin();
	for(int i=0; max_child_system_ids_count > i; ++i, ++itr) {
		if(itr == def.nested_systems.end()) {
			break;
		}
		out_child_system_ids[i] = *itr;
	}

	if(out_child_system_count) {
		*out_child_system_count = static_cast<int32_t>(def.nested_systems.size());
	}
}

ecsact_system_like_id ecsact_meta_get_parent_system_id
	( ecsact_system_id  child_system_id
	)
{
	auto& def = get_system_like(
		ecsact_id_cast<ecsact_system_like_id>(child_system_id)
	);
	return def.parent_system_id;
}

int32_t ecsact_meta_count_top_level_systems
	( ecsact_package_id package_id
	)
{
	auto& pkg_def = package_defs.at(package_id);
	return static_cast<int32_t>(pkg_def.top_level_systems.size());
}

void ecsact_meta_get_top_level_systems
	( ecsact_package_id       package_id
	, int32_t                 max_systems_count
	, ecsact_system_like_id*  out_systems
	, int32_t*                out_systems_count
	)
{
	auto& pkg_def = package_defs.at(package_id);

	auto itr = pkg_def.top_level_systems.begin();
	for(int i=0; max_systems_count > i; ++i, ++itr) {
		if(itr == pkg_def.top_level_systems.end()) {
			break;
		}
		out_systems[i] = *itr;
	}

	if(out_systems_count != nullptr) {
		*out_systems_count = static_cast<int32_t>(pkg_def.top_level_systems.size());
	}
}
