#pragma once

#include <entt/entt.hpp>
#include <initializer_list>
#include <vector>
#include <tuple>

struct EntityRef
{
	entt::entity value;
};

struct Int
{
	int value;
};

struct Float
{
	float value;
};

using comp_entity = entt::entity;
using instance_entity = entt::entity;
using type_entity = entt::entity;

enum class EComponentMember
{
	None = 0,
	EntityRef,
	Int,	
	Float,
	Count
};

struct ComponentMemberDefinition
{
	std::string name;
	EComponentMember kind;
};

struct ComponentMember
{
	EComponentMember kind;

	union
	{
		EntityRef e;
		Int i;
		Float f;
	} data;
};

struct ComponentType
{
	static inline constexpr const std::size_t MaxMembers = 10;

	std::string name;
	std::vector<ComponentMemberDefinition> members;
	entt::sparse_set adorned_entities;
};

struct Instance
{
	std::unordered_map<type_entity, comp_entity> registered;
};

struct Component
{
	entt::entity key_id;
	entt::entity type_id;
	std::unordered_map<std::string, std::uint8_t> member_index;
	std::vector<ComponentMember> members;
};

/* ecs */

struct ECS
{
	entt::registry registry;
	entt::sparse_set created_entities;
	std::unordered_map<std::string, type_entity> types;
};

const ComponentType& ecs_get_type(ECS& ecs, std::string name)
{
	return ecs.registry.get<ComponentType>(ecs.types[name]);
}

entt::entity ecs_create_type(ECS& ecs, std::string name, std::vector<std::tuple<std::string, EComponentMember>> members)
{
	assert(members.size() < ComponentType::MaxMembers);

	auto entity = ecs.registry.create();

	auto& type_def = ecs.registry.emplace<ComponentType>(entity);
	type_def.name = name;

	for (const auto& mem : members)
	{
		auto& member_name = std::get<0>(mem);
		auto& member_kind = std::get<1>(mem);

		ComponentMemberDefinition def;
		def.name = member_name;
		def.kind = member_kind;
		type_def.members.push_back(def);
	}

	ecs.types.insert({ name, entity });

	return entity;
}

instance_entity ecs_create_instance(ECS& ecs)
{
	auto entity = ecs.registry.create();
	ecs.registry.emplace<Instance>(entity);
	return entity;
}

comp_entity ecs_adorn_instance(ECS& ecs, instance_entity key, std::string type_name)
{
	assert(ecs.types.count(type_name) > 0);

	const auto& type = ecs.types[type_name];

	auto entity = ecs.registry.create();

	auto& instance = ecs.registry.emplace<Component>(entity);
	instance.key_id = key;
	instance.type_id = type;

	auto& type_def = ecs.registry.get<ComponentType>(type);
	type_def.adorned_entities.emplace(key);

	std::uint8_t index = 0;
	for (const auto& mem : type_def.members)
	{
		ComponentMember member{};

		instance.member_index.insert({ mem.name, index++ });
		member.kind = mem.kind;
		switch (member.kind)
		{
		case EComponentMember::EntityRef:
			member.data.e.value = entt::null;
			break;
		case EComponentMember::Int:
			member.data.i.value = 0;
			break;
		case EComponentMember::Float:
			member.data.f.value = 0.0f;
			break;
		case EComponentMember::Count:
		case EComponentMember::None:
			assert(member.kind != EComponentMember::Count && member.kind != EComponentMember::None);
			break;
		}

		instance.members.push_back(member);
	}

	auto& instance_reg = ecs.registry.get<Instance>(key);
	instance_reg.registered.insert({ type, entity });

	return entity;
}

Component& ecs_get_component_by_instance(ECS& ecs, instance_entity instance_id, std::string type_name)
{
	auto& instance_reg = ecs.registry.get<Instance>(instance_id);
	auto type_id = ecs.types.find(type_name);
	assert(type_id != ecs.types.end());
	return ecs.registry.get<Component>(instance_reg.registered[type_id->second]);
}

ComponentMember& ecs_get_member_in_component(ECS& ecs, Component& comp, std::string member_name)
{
	auto member_index = comp.member_index.find(member_name)->second;
	return comp.members[member_index];
}

template<typename V>
void ecs_set_member_in_component(Component& comp, std::string member_name, V value)
{}

template<>
void ecs_set_member_in_component(Component& comp, std::string member_name, EntityRef e)
{
	auto member_index = comp.member_index.find(member_name)->second;
	assert(comp.members[member_index].kind == EComponentMember::EntityRef);
	comp.members[member_index].data.e = e;
}

template<>
void ecs_set_member_in_component(Component& comp, std::string member_name, Int i)
{
	auto member_index = comp.member_index.find(member_name)->second;
	assert(comp.members[member_index].kind == EComponentMember::Int);
	comp.members[member_index].data.i = i;
}

template<>
void ecs_set_member_in_component(Component& comp, std::string member_name, Float i)
{
	auto member_index = comp.member_index.find(member_name)->second;
	assert(comp.members[member_index].kind == EComponentMember::Float);
	comp.members[member_index].data.f = i;
}

entt::sparse_set ecs_query(ECS& ecs, std::vector<std::string> positive, std::vector<std::string> negative = {})
{
	entt::sparse_set result;
	for (auto type_name : positive)
	{	
		auto type_id = ecs.types[type_name];
		auto& comp_type = ecs.registry.get<ComponentType>(type_id);

		if (result.empty())
		{
			result.insert(comp_type.adorned_entities.begin(), comp_type.adorned_entities.end());
		}
		else
		{
			for (auto entity : result)
			{
				if (!comp_type.adorned_entities.contains(entity))
				{
					result.remove(entity);
				}
			}
		}
	}

	for (auto type_name : negative)
	{
		auto type_id = ecs.types[type_name];
		auto& comp_type = ecs.registry.get<ComponentType>(type_id);
		
		for (auto entity : result)
		{
			if (comp_type.adorned_entities.contains(entity))
			{
				result.remove(entity);
			}
		}
	}

	return result;
}
