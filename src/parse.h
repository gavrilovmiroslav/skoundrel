#pragma once

#pragma once

#include <string>
#include <vector>
#include <deque>
#include <unordered_set>
#include <iostream>
#include <algorithm>
#include <optional>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <memory>

#include "ecs.h"

std::string string_format(const std::string fmt_str, ...) 
{
	int final_n, n = ((int)fmt_str.size()) * 2;
	std::unique_ptr<char[]> formatted;
	va_list ap;
	while (true) 
	{
		formatted.reset(new char[n]);
		strcpy(&formatted[0], fmt_str.c_str());
		va_start(ap, fmt_str);
		final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
		va_end(ap);
		if (final_n < 0 || final_n >= n)
			n += abs(final_n - n + 1);
		else
			break;
	}

	return std::string(formatted.get());
}

enum class EToken
{
	Keyword,
	Number,
	OpenBracket,
	ClosedBracket,
	OpenParen,
	ClosedParen,
	OpenBrace,
	ClosedBrace,
	Quote,
	Comma,
	Colon,
	Semicolon,
	Underscore,
	Monkey,
	Plus,
	Minus,
	Mult,
	Div,
	Mod,
	True,
	False,
	Lt, Le, Eq, Ne, Ge, Gt,
};

enum class EKeyword
{
	If,
	Else,
	Define,
	Create,
	Entity,
	With,
	Without,
	Query,
	Foreach,
	Print,
	System,
	Destroy,
	Attach,
	Detach,
	Get,
	To,
	From,
};

struct Token
{
	int line;
	int start;
	int end;
	EToken type;

	EKeyword keyword;
	std::string quote;
	int number;
	bool boolean;
};

struct ParseError
{
	std::string text;
	Token token;
};

enum class EType
{
	Null,
	Bool,
	Entity,
	Int,
	Float,
	String,
};

std::string stringify_type(EType type)
{
	switch (type)
	{
	case EType::Entity: return "Entity";
	case EType::Bool: return "Bool";
	case EType::Int: return "Int";
	case EType::Float: return "Float";
	case EType::String: return "String";
	case EType::Null: default: return "Null";
	}
}

std::string stringify_token(EToken token)
{
	switch (token)
	{
	case EToken::Keyword: return "keyword";
	case EToken::Number: return "number";
	case EToken::OpenBracket: return "open bracket";
	case EToken::ClosedBracket: return "closed bracket";
	case EToken::OpenParen: return "open parenthesis";
	case EToken::ClosedParen: return "closed parenthesis";
	case EToken::OpenBrace: return "open brace";
	case EToken::ClosedBrace: return "closed brace";
	case EToken::Quote: return "quote";
	case EToken::Comma: return "comma";
	case EToken::Colon: return "colon";
	case EToken::Semicolon: return "semicolon";
	case EToken::Underscore: return "underscore";
	case EToken::Monkey: return "at-sign";
	case EToken::Plus: return "plus";
	case EToken::Minus: return "minus";
	case EToken::Mult: return "mult";
	case EToken::Div: return "div";
	case EToken::Mod: return "mod";
	case EToken::True: return "true";	
	case EToken::Lt: return "<";
	case EToken::Le: return "<=";
	case EToken::Eq: return "==";
	case EToken::Ne: return "!=";
	case EToken::Ge: return ">=";
	case EToken::Gt: return ">";
	case EToken::False: default: return "false";
	}
}

std::string stringify_keyword(EKeyword keyword)
{
	switch (keyword)
	{
	case EKeyword::If: return "if";
	case EKeyword::Else: return "else";
	case EKeyword::Define: return "define";
	case EKeyword::Create: return "create";
	case EKeyword::Entity: return "entity";
	case EKeyword::With: return "with";
	case EKeyword::Without: return "without";
	case EKeyword::Query: return "query";
	case EKeyword::Foreach: return "foreach";
	case EKeyword::System: return "system";	
	case EKeyword::Attach: return "attach";
	case EKeyword::Detach: return "detach";
	case EKeyword::Get: return "get";
	case EKeyword::To: return "to";
	case EKeyword::From: return "from";
	case EKeyword::Print: default: return "print";
	}
}

struct {
	std::size_t index = 0;
	std::unordered_map<std::size_t, std::string> interned_strings_index;
	std::unordered_map<std::string, std::size_t> interned_strings_reindex;

	std::size_t add(std::string str)
	{
		if (interned_strings_reindex.count(str) > 0)
		{
			return interned_strings_reindex[str];
		}

		auto next = interned_strings_index.size();
		interned_strings_index[next] = str;
		interned_strings_reindex[str] = next;
		return next;
	}

	std::optional<std::size_t> get_index(std::string str)
	{
		if (interned_strings_reindex.count(str) > 0)
		{
			return std::make_optional(interned_strings_reindex[str]);
		}

		return std::nullopt;
	}

	std::optional<std::string> get_string(std::size_t index)
	{
		if (interned_strings_index.count(index) > 0)
		{
			return std::make_optional(interned_strings_index[index]);
		}

		return std::nullopt;
	}

} InternedStrings;

struct TypedValue
{
	EType type;

	union
	{
		entt::entity entity_value;
		int int_value;
		float float_value;
		bool bool_value;
		std::size_t intern_string_index;
	} data;
};

static const TypedValue operator+(const TypedValue& lhs, const TypedValue& rhs)
{
	if (lhs.type != rhs.type) return TypedValue{ EType::Null };
	if (lhs.type == EType::Null) return TypedValue{ EType::Null };
	if (lhs.type == EType::Entity) return TypedValue{ EType::Null };

	TypedValue res;
	switch (lhs.type)
	{	
	case EType::Bool:
		res.type = EType::Bool;
		res.data.bool_value = lhs.data.bool_value || rhs.data.bool_value;
		break;
	case EType::Int: 
		res.type = EType::Int;
		res.data.int_value = lhs.data.int_value + rhs.data.int_value;
		break;
	case EType::Float:
		res.type = EType::Float;
		res.data.float_value = lhs.data.float_value + rhs.data.float_value;
		break;
	case EType::String:
		res.type = EType::String;
		auto ls = InternedStrings.get_string(lhs.data.intern_string_index).value_or(std::string{ "" });
		auto rs = InternedStrings.get_string(rhs.data.intern_string_index).value_or(std::string{ "" });
		auto index = InternedStrings.add(ls + rs);
		res.data.intern_string_index = index;
		break;
	}

	return res;
}

static const TypedValue operator-(const TypedValue& lhs, const TypedValue& rhs)
{
	if (lhs.type != rhs.type) return TypedValue{ EType::Null };
	if (lhs.type == EType::Null) return TypedValue{ EType::Null };
	if (lhs.type == EType::Entity) return TypedValue{ EType::Null };
	if (lhs.type == EType::Bool) return TypedValue{ EType::Null };
	if (lhs.type == EType::String) return TypedValue{ EType::Null };

	TypedValue res;
	switch (lhs.type)
	{
	case EType::Int:
		res.type = EType::Int;
		res.data.int_value = lhs.data.int_value - rhs.data.int_value;
		break;
	case EType::Float:
		res.type = EType::Float;
		res.data.float_value = lhs.data.float_value - rhs.data.float_value;
		break;
	}

	return res;
}

static const TypedValue operator*(const TypedValue& lhs, const TypedValue& rhs)
{
	TypedValue res;

	if (lhs.type == EType::String && rhs.type == EType::Int)
	{
		res.type = EType::String;
		std::string val = InternedStrings.get_string(lhs.data.intern_string_index).value_or(std::string{ "" });
		std::string result = "";
		for (int i = 0; i < rhs.data.int_value; i++)
		{
			result += val;
		}
		res.data.intern_string_index = InternedStrings.add(result);
	}
	else
	{
		if (lhs.type != rhs.type) return TypedValue{ EType::Null };
		if (lhs.type == EType::Null) return TypedValue{ EType::Null };
		if (lhs.type == EType::Entity) return TypedValue{ EType::Null };

		switch (lhs.type)
		{
		case EType::Bool:
			res.type = EType::Bool;
			res.data.bool_value = lhs.data.bool_value && rhs.data.bool_value;
			break;
		case EType::Int:
			res.type = EType::Int;
			res.data.int_value = lhs.data.int_value * rhs.data.int_value;
			break;
		case EType::Float:
			res.type = EType::Float;
			res.data.float_value = lhs.data.float_value * rhs.data.float_value;
			break;
		}
	}

	return res;
}

static const TypedValue operator/(const TypedValue& lhs, const TypedValue& rhs)
{
	if (lhs.type != rhs.type) return TypedValue{ EType::Null };
	if (lhs.type == EType::Null) return TypedValue{ EType::Null };
	if (lhs.type == EType::Entity) return TypedValue{ EType::Null };
	if (lhs.type == EType::Bool) return TypedValue{ EType::Null };
	if (lhs.type == EType::String) return TypedValue{ EType::Null };

	TypedValue res;
	switch (lhs.type)
	{
	case EType::Int:
		res.type = EType::Int;
		res.data.int_value = lhs.data.int_value / rhs.data.int_value;
		break;
	case EType::Float:
		res.type = EType::Float;
		auto r = rhs.data.float_value;
		if (r == 0.0f) { r = 0.001f; } 
		res.data.float_value = lhs.data.float_value / r;
		break;
	}

	return res;
}

static const TypedValue operator%(const TypedValue& lhs, const TypedValue& rhs)
{
	if (lhs.type != rhs.type) return TypedValue{ EType::Null };
	if (lhs.type == EType::Null) return TypedValue{ EType::Null };
	if (lhs.type == EType::Entity) return TypedValue{ EType::Null };
	if (lhs.type == EType::Float) return TypedValue{ EType::Null };
	if (lhs.type == EType::Bool) return TypedValue{ EType::Null };
	if (lhs.type == EType::String) return TypedValue{ EType::Null };

	TypedValue res;
	switch (lhs.type)
	{
	case EType::Int:
		res.type = EType::Int;
		res.data.int_value = lhs.data.int_value % rhs.data.int_value;
		break;
	}

	return res;
}

static const TypedValue operator ==(const TypedValue& lhs, const TypedValue& rhs)
{
	if (lhs.type != rhs.type) return TypedValue{ EType::Null };
	if (lhs.type == EType::Null) return TypedValue{ EType::Null };

	TypedValue res;
	res.type = EType::Bool;

	switch (lhs.type)
	{
	case EType::Int:
		res.data.bool_value = lhs.data.int_value == rhs.data.int_value;
		break;
	case EType::Float:
		res.data.bool_value = lhs.data.float_value == rhs.data.float_value;
		break;
	case EType::Bool:
		res.data.bool_value = lhs.data.bool_value == rhs.data.bool_value;
		break;
	case EType::Entity:
		res.data.bool_value = lhs.data.entity_value == rhs.data.entity_value;
		break;
	case EType::String:
		res.data.bool_value = lhs.data.intern_string_index == rhs.data.intern_string_index;
		break;
	}
	return res;
}

static const TypedValue operator !=(const TypedValue& lhs, const TypedValue& rhs)
{
	if (lhs.type != rhs.type) return TypedValue{ EType::Null };
	if (lhs.type == EType::Null) return TypedValue{ EType::Null };

	TypedValue res;
	res.type = EType::Bool;

	switch (lhs.type)
	{
	case EType::Int:
		res.data.bool_value = lhs.data.int_value != rhs.data.int_value;
		break;
	case EType::Float:
		res.data.bool_value = lhs.data.float_value != rhs.data.float_value;
		break;
	case EType::Bool:
		res.data.bool_value = lhs.data.bool_value != rhs.data.bool_value;
		break;
	case EType::Entity:
		res.data.bool_value = lhs.data.entity_value != rhs.data.entity_value;
		break;
	case EType::String:
		res.data.bool_value = lhs.data.intern_string_index != rhs.data.intern_string_index;
		break;
	}
	return res;
}

#define LOGICAL_OP(_op_) \
static const TypedValue operator _op_(const TypedValue& lhs, const TypedValue& rhs) \
{ \
	if (lhs.type != rhs.type) return TypedValue{ EType::Null }; \
	if (lhs.type == EType::Null) return TypedValue{ EType::Null }; \
	if (lhs.type == EType::Entity) return TypedValue{ EType::Null }; \
	if (lhs.type == EType::Bool) return TypedValue{ EType::Null }; \
	TypedValue res; \
	res.type = EType::Bool; \
	switch (lhs.type) \
	{ \
	case EType::Int: \
		res.data.bool_value = lhs.data.int_value _op_ rhs.data.int_value; \
		break; \
	case EType::Float: \
		res.data.bool_value = lhs.data.float_value _op_ rhs.data.float_value; \
		break; \
	} \
	return res; \
} \

LOGICAL_OP(<);
LOGICAL_OP(<=);
LOGICAL_OP(>=);
LOGICAL_OP(>);

struct Scope;
struct Statement;

struct InterpretError
{
	std::string text;
	Statement* statement;
};

struct System
{
	std::string name;
	std::vector<std::shared_ptr<Statement>> block;

	System(std::string name, std::vector<std::shared_ptr<Statement>> block);
};

struct Context
{
	std::shared_ptr<ECS> ecs;
	
	Scope* scope;
	
	std::vector<std::string> source_lines;
	std::string source_text;
	std::vector<std::shared_ptr<Statement>> interpreted_statements;

	std::optional<ParseError> parse_error;
	std::optional<InterpretError> interpret_error;
	std::vector<System> systems;
	int depth = 0;

	Context();
	~Context();

	bool is_parse_okay();

	bool check_parse_error(std::optional<ParseError> error = std::nullopt);

	bool has_errors()
	{
		return parse_error.has_value() || interpret_error.has_value();
	}

	void execute();

	void update();
	
	void make_interpret_error(std::string err, Statement* statement)
	{
		InterpretError error;
		error.text = err;
		error.statement = statement;
		
		this->interpret_error = std::make_optional(error);
	}

	void die_with_error();
};

struct Expr
{
	virtual TypedValue eval(Context& ctx) = 0;
	virtual std::string to_string(Context& ctx) = 0;
};

struct BoolExpr : public Expr
{
	bool val;

	BoolExpr(bool b) : val(b)
	{}

	TypedValue eval(Context& ctx) override
	{
		TypedValue v;
		v.type = EType::Bool;
		v.data.bool_value = val;
		return v;
	}

	std::string to_string(Context& ctx) override
	{
		return val ? "true" : "false";
	}
};

struct IntExpr : public Expr
{
	int num;

	IntExpr(int n) : num(n)
	{}

	TypedValue eval(Context& ctx) override
	{
		TypedValue v;
		v.type = EType::Int;
		v.data.int_value = num;
		return v;
	}

	std::string to_string(Context& ctx) override
	{
		return std::to_string(num);
	}
};

struct FloatExpr : public Expr
{
	float num;

	FloatExpr(float n) : num(n)
	{}

	TypedValue eval(Context& ctx) override
	{
		TypedValue v;
		v.type = EType::Float;
		v.data.float_value = num;
		return v;
	}

	std::string to_string(Context& ctx) override
	{
		return std::to_string(num);
	}

};

struct EntityExpr : public Expr
{
	EntityRef r;

	EntityExpr(entt::entity e)
	{
		r.value = e;
	}

	TypedValue eval(Context& ctx) override
	{
		TypedValue v;
		v.type = EType::Entity;
		v.data.entity_value = r.value;
		return v;
	}

	std::string to_string(Context& ctx) override
	{
		return std::string("@") + std::to_string(entt::to_integral(r.value));
	}
};

struct VarExpr : public Expr
{
	std::string name;

	VarExpr(std::string s) : name(s)
	{}

	TypedValue eval(Context& ctx) override;

	std::string to_string(Context& ctx) override
	{
		return name;
	}
};

struct CompMemberRefExpr : public Expr
{
	std::string name;
	entt::entity entity;
	Component comp;
	int param_index;

	std::shared_ptr<Expr> value;

	CompMemberRefExpr(std::string s, entt::entity e, Component c, int p, std::shared_ptr<Expr> v)
		: name(s)
		, entity(e)
		, comp(c)
		, param_index(p)
		, value(v)
	{}

	TypedValue eval(Context& ctx) override
	{
		return value->eval(ctx);
	}

	std::string to_string(Context& ctx) override
	{
		auto& type = ctx.ecs->registry.get<ComponentType>(comp.type_id);
		return value->to_string(ctx) + " [@" + std::to_string(entt::to_integral((uint64_t)entity)) + "] " + type.name + "::" + name;
	}
};

enum class EArithmetic
{
	Add, Sub, Mult, Div, Mod,
};

enum class ELogical
{
	Lt, Le, Eq, Ne, Ge, Gt,
};

struct LogicalExpr : public Expr
{
	ELogical op;
	std::shared_ptr<Expr> lhs;
	std::shared_ptr<Expr> rhs;

	LogicalExpr(ELogical op, std::shared_ptr<Expr> l, std::shared_ptr<Expr> r)
		: op(op)
		, lhs(l)
		, rhs(r)
	{}

	TypedValue eval(Context& ctx) override
	{
		switch (op)
		{
		case ELogical::Lt:
			return lhs->eval(ctx) < rhs->eval(ctx);
		case ELogical::Le:
			return lhs->eval(ctx) <= rhs->eval(ctx);
		case ELogical::Eq:
			return lhs->eval(ctx) == rhs->eval(ctx);
		case ELogical::Ne:
			return lhs->eval(ctx) != rhs->eval(ctx);
		case ELogical::Ge:
			return lhs->eval(ctx) >= rhs->eval(ctx);
		case ELogical::Gt: default:
			return lhs->eval(ctx) > rhs->eval(ctx);
		}
	}

	std::string to_string(Context& ctx) override
	{
		switch (op)
		{
		case ELogical::Lt:
			return lhs->to_string(ctx) + " < " + rhs->to_string(ctx);
		case ELogical::Le:
			return lhs->to_string(ctx) + " <= " + rhs->to_string(ctx);
		case ELogical::Eq:
			return lhs->to_string(ctx) + " == " + rhs->to_string(ctx);
		case ELogical::Ne:
			return lhs->to_string(ctx) + " != " + rhs->to_string(ctx);
		case ELogical::Ge:
			return lhs->to_string(ctx) + " >= " + rhs->to_string(ctx);
		case ELogical::Gt: default:
			return lhs->to_string(ctx) + " > " + rhs->to_string(ctx);
		}
	}
};

struct ArithExpr : public Expr
{
	EArithmetic op;
	std::shared_ptr<Expr> lhs;
	std::shared_ptr<Expr> rhs;

	ArithExpr(EArithmetic op, std::shared_ptr<Expr> l, std::shared_ptr<Expr> r)
		: op(op)
		, lhs(l)
		, rhs(r)
	{}

	TypedValue eval(Context& ctx) override
	{
		switch (op)
		{
		case EArithmetic::Add:
			return lhs->eval(ctx) + rhs->eval(ctx);
		case EArithmetic::Sub:
			return lhs->eval(ctx) - rhs->eval(ctx);
		case EArithmetic::Mult:
			return lhs->eval(ctx) * rhs->eval(ctx);
		case EArithmetic::Div:
			return lhs->eval(ctx) / rhs->eval(ctx);
		case EArithmetic::Mod: default:
			return lhs->eval(ctx) % rhs->eval(ctx);
		}
	}

	std::string to_string(Context& ctx) override
	{
		switch (op)
		{
		case EArithmetic::Add:
			return lhs->to_string(ctx) + " + " + rhs->to_string(ctx);
		case EArithmetic::Sub:
			return lhs->to_string(ctx) + " - " + rhs->to_string(ctx);
		case EArithmetic::Mult:
			return lhs->to_string(ctx) + " * " + rhs->to_string(ctx);
		case EArithmetic::Div:
			return lhs->to_string(ctx) + " / " + rhs->to_string(ctx);
		case EArithmetic::Mod: default:
			return lhs->to_string(ctx) + " % " + rhs->to_string(ctx);
		}
	}
};

struct Scope
{
	Scope()
	{
		next.reset();
	}

	void add_binding(std::string name, std::shared_ptr<Expr> value)
	{
		if (next)
		{
			next->add_binding(name, value);
		}
		else
		{
			env.insert({ name, value });
		}
	}

	std::optional<entt::entity> internal_rec_delete_binding(std::string name)
	{
		if (next)
		{
			auto child_result = next->internal_rec_delete_binding(name);
			if (child_result == std::nullopt)
			{
				if (env.count(name) > 0)
				{
					auto e = env.find(name);
					auto val = e->second;
					env.erase(name);
					if (EntityExpr* ee = dynamic_cast<EntityExpr*>(val.get()))
					{
						return std::make_optional(ee->r.value);
					}
				}
				else
				{
					return std::nullopt;
				}
			}
			else
			{
				return child_result;
			}
		}
		else
		{
			if (env.count(name) > 0)
			{
				auto e = env.find(name);
				env.erase(name);
				if (EntityExpr* ee = dynamic_cast<EntityExpr*>(e->second.get()))
				{
					return std::make_optional(ee->r.value);
				}
			}
			else
			{
				return std::nullopt;
			}
		}
	}

	void internal_rec_delete_refs(entt::entity e)
	{
		std::vector<std::string> to_delete;
		for (auto& kv : env)
		{
			if (EntityExpr* ee = dynamic_cast<EntityExpr*>(kv.second.get()))
			{
				if (ee->r.value == e)
				{
					to_delete.push_back(kv.first);
				}
			}
			else if (CompMemberRefExpr* ref = dynamic_cast<CompMemberRefExpr*>(kv.second.get()))
			{
				if (ref->entity == e)
				{
					to_delete.push_back(kv.first);
				}
			}
		}

		for (auto key : to_delete)
		{
			env.erase(key);
		}

		if (next)
		{
			next->internal_rec_delete_refs(e);
		}
	}

	void internal_rec_delete_comp_ref(entt::entity e, entt::entity comp_type_id)
	{
		std::vector<std::string> to_delete;
		for (auto& kv : env)
		{
			if (CompMemberRefExpr* ref = dynamic_cast<CompMemberRefExpr*>(kv.second.get()))
			{
				if (ref->entity == e && ref->comp.type_id == comp_type_id)
				{
					to_delete.push_back(kv.first);
				}
			}
		}

		for (auto& key : to_delete)
		{
			env.erase(key);
		}

		if (next)
		{
			next->internal_rec_delete_comp_ref(e, comp_type_id);
		}
	}

	void delete_binding(std::string name)
	{
		auto result = internal_rec_delete_binding(name);
		if (result.has_value())
		{
			internal_rec_delete_refs(result.value());
		}
	}

	std::shared_ptr<Expr> get_local_binding(std::string name) const
	{
		if (env.count(name) > 0)
		{
			return env.find(name)->second;
		}
		else
		{
			return nullptr;
		}
	}

	std::shared_ptr<Expr> get_binding(std::string name) const
	{
		if (next)
		{
			auto bind = next->get_binding(name);
			if (bind != nullptr)
				return bind;
		}

		return get_local_binding(name);
	}

	void push_scope()
	{
		if (!next)
		{
			next = std::make_shared<Scope>();
		}
		else
		{
			next->push_scope();
		}
	}

	void pop_scope()
	{
		if (next && !next->next)
		{
			next = nullptr;
		}
		else if (next)
		{
			next->pop_scope();
		}
	}

	void print(Context& ctx)
	{
		auto ptr = next;
		print(ctx, 0);
	}

private:

	void print(Context& ctx, int indent)
	{
		for (auto& [name, value] : env)
		{
			auto space = std::string(indent, ' ');
			printf("%s%s: %s\n", space.c_str(), name.c_str(), value->to_string(ctx).c_str());
		}
		if (next)
		{
			next->print(ctx, indent + 1);
		}
	}

	std::unordered_map<std::string, std::shared_ptr<Expr>> env;	
	std::shared_ptr<Scope> next;
};

Context::Context()
{
	scope = new Scope();
	ecs = std::make_shared<ECS>();
}

Context::~Context()
{
	delete scope;
}

std::optional<ParseError> generic_parse_error = std::nullopt;

bool Context::is_parse_okay()
{
	this->parse_error = generic_parse_error;

	return !this->parse_error.has_value();
}

TypedValue VarExpr::eval(Context& ctx)
{
	std::shared_ptr<Expr> bind = ctx.scope->get_binding(name);
	if (bind)
	{
		return bind->eval(ctx);
	}
	else
	{
		return TypedValue{ EType::Null };
	}
}

struct CompCtor
{
	std::string comp_name;
	std::vector<std::tuple<std::string, std::shared_ptr<Expr>>> fields;
};

struct CompParamCtor
{
	std::string comp_name;
	std::vector<std::shared_ptr<Expr>> params;
};

using Range = std::tuple<Token, Token>;

struct Statement
{
	Token start, end;
	virtual void execute(Context& ctx) = 0;

	Statement(Token start, Token end)
		: start(start)
		, end(end)
	{}
};

void Context::update()
{
	for (auto& system : systems)
	{
		this->depth++;
		for (auto& statement : system.block)
		{
			statement->execute(*this);
		}
		this->depth--;
	}
}

void Context::execute()
{
	for (auto stat : interpreted_statements)
	{
		stat->execute(*this);
		if (has_errors())
		{
			die_with_error();
			return;
		}
	}
}

void Context::die_with_error()
{
	if (this->parse_error.has_value())
	{
		auto p = this->parse_error.value();
		printf("\n PARSER ERROR: \n\n");
		for (int i = -3; i < 0; i++)
		{
			int l = p.token.line + i;
			if (l >= 0)
				printf("  %s\n", this->source_lines[l].c_str());
		}
		auto sp = std::string(p.token.start, ' ') + std::string(p.token.end - p.token.start, '^');
		printf("\033[0;31m");
		printf("%s", sp.c_str());		
		printf(" -- (%d: %d) %s\n", p.token.line, p.token.start, p.text.c_str());
		printf("\033[0m");

		for (int i = 0; i < 3; i++)
		{
			std::size_t l = p.token.line + i + 1;
			if (l < source_lines.size())
				printf("  %s\n", this->source_lines[l].c_str());
		}
	}

	if (interpret_error.has_value())
	{
		auto i = interpret_error.value();
		printf("INTERPRETATION ERROR: %s\n", i.text.c_str());
		printf("  %s\n", this->source_lines[i.statement->start.line - 1].c_str());
	}
}

System::System(std::string name, std::vector<std::shared_ptr<Statement>> block)
	: name(name), block(block)
{}

struct IfStatement : public Statement
{
	std::shared_ptr<Expr> condition;
	std::vector<std::shared_ptr<Statement>> then_branch;
	std::vector<std::shared_ptr<Statement>> else_branch;

	IfStatement(Range range,
		std::shared_ptr<Expr> cond, 
		std::vector<std::shared_ptr<Statement>> then_block, 
		std::vector<std::shared_ptr<Statement>> else_block)
		: Statement(std::get<0>(range), std::get<1>(range))
		, condition(cond)
		, then_branch(then_block)
		, else_branch(else_block)
	{}

	void execute(Context& ctx) override
	{
		const auto& cond = condition->eval(ctx);
		if (cond.type != EType::Bool)
		{	
			ctx.make_interpret_error(string_format("Condition must be a boolean, %s found instead", stringify_type(cond.type)), this);
			return;
		}

		if (cond.data.bool_value)
		{
			ctx.depth++;
			for (auto statement : then_branch)
			{
				statement->execute(ctx);
			}
			ctx.depth--;
		}
		else
		{
			ctx.depth++;
			for (auto statement : else_branch)
			{
				statement->execute(ctx);
			}
			ctx.depth--;
		}
	}
};

struct DefineSystemStatement : public Statement
{
	std::string system_name;
	// TODO: add system constraints 
	std::vector<std::shared_ptr<Statement>> block;

	DefineSystemStatement(Range range, std::string name, std::vector<std::shared_ptr<Statement>> block)
		: Statement(std::get<0>(range), std::get<1>(range))
		, system_name(name), block(block)
	{}

	void execute(Context& ctx) override
	{
		if (ctx.has_errors()) return;
		if (ctx.depth > 0)
		{
			ctx.make_interpret_error(string_format("Cannot define component within system or query"), this);
			return;
		}

		ctx.systems.push_back(System(system_name, block));
	}
};

struct DefineComponentStatement : public Statement
{
	std::string comp_name;
	std::vector<std::tuple<std::string, EType>> members;

	DefineComponentStatement(Range range, std::string name, std::vector<std::tuple<std::string, EType>> mems)
		: Statement(std::get<0>(range), std::get<1>(range))
		, comp_name(name)
		, members(mems)
	{}

	void execute(Context& ctx) override
	{
		if (ctx.has_errors()) return;
		if (ctx.depth > 0) 
		{ 
			ctx.make_interpret_error(string_format("Cannot define component within system or query"), this);
			return;
		}

		std::vector<std::tuple<std::string, EComponentMember>> comp_members;
		for (auto& [k, v] : members)
		{
			switch (v)
			{
			case EType::Bool: comp_members.push_back({ k, EComponentMember::Bool }); break;
			case EType::Entity: comp_members.push_back({ k, EComponentMember::EntityRef }); break;
			case EType::Int: comp_members.push_back({ k, EComponentMember::Int }); break;
			case EType::Float: comp_members.push_back({ k, EComponentMember::Float }); break;
			}
		}
		ecs_create_type(*ctx.ecs, comp_name, comp_members);
	}
};

struct CreateEntityStatement : public Statement
{
	std::string entity_name;
	std::vector<CompCtor> components;

	CreateEntityStatement(Range range, std::string name, std::vector<CompCtor> flds)
		: Statement(std::get<0>(range), std::get<1>(range))
		, entity_name(name)
		, components(flds)
	{}

	void execute(Context& ctx) override
	{
		if (ctx.has_errors()) return;

		auto e = ecs_create_instance(*ctx.ecs);
		ctx.scope->add_binding(entity_name, std::shared_ptr<Expr>(new EntityExpr(e)));

		for (auto& ctor : components)
		{
			auto& type = ecs_get_type(*ctx.ecs, ctor.comp_name);
			ecs_adorn_instance(*ctx.ecs, e, ctor.comp_name);
			auto& comp = ecs_get_component_by_instance(*ctx.ecs, e, ctor.comp_name);
			
			int i = 0;
			for (auto& [member_name, value] : ctor.fields)
			{
				auto& typed_val = value->eval(ctx);

				switch (type.members[i].kind)
				{
				case EComponentMember::Bool:
					if (typed_val.type != EType::Bool)
					{
						ctx.make_interpret_error(string_format("Expected bool, got %s", stringify_type(typed_val.type).c_str()), this);
						return;
					}
					ecs_set_member_in_component(comp, member_name, Bool{ typed_val.data.bool_value });
					break;
				case EComponentMember::Int:
					if (typed_val.type != EType::Int)
					{
						ctx.make_interpret_error(string_format("Expected int, got %s", stringify_type(typed_val.type).c_str()), this);
						return;
					}
					ecs_set_member_in_component(comp, member_name, Int{ typed_val.data.int_value });
					break;
				case EComponentMember::Float:
					if (typed_val.type != EType::Float)
					{
						ctx.make_interpret_error(string_format("Expected float, got %s", stringify_type(typed_val.type).c_str()), this);
						return;
					}
					ecs_set_member_in_component(comp, member_name, Float{ typed_val.data.float_value });
					break;
				case EComponentMember::EntityRef:
					if (typed_val.type != EType::Entity)
					{
						ctx.make_interpret_error(string_format("Expected entity ref, got %s", stringify_type(typed_val.type).c_str()), this);
						return;
					}
					ecs_set_member_in_component(comp, member_name, EntityRef{ typed_val.data.entity_value });
					break;
				}
				i++;
			}
		}
	}
};

struct DestroyEntityStatement : public Statement
{
	std::string entity_name;

	DestroyEntityStatement(Range range, std::string entity_name)
		: Statement(std::get<0>(range), std::get<1>(range))
		, entity_name(entity_name)
	{}

	void execute(Context& ctx) override
	{
		if (ctx.has_errors()) return;

		auto entity = ctx.scope->get_binding(entity_name);
		if (!entity)
		{
			ctx.make_interpret_error(string_format("Variable '%s' not found", entity_name.c_str()), this);
			return;
		}

		auto val = entity->eval(ctx);
		if (val.type != EType::Entity)
		{
			ctx.make_interpret_error(string_format("Entity expected, but %s found instead", stringify_type(val.type).c_str()), this);
			return;
		}

		ecs_destroy_instance(*ctx.ecs, val.data.entity_value);
		ctx.scope->delete_binding(entity_name);
	}
};

struct PrintContextStatement : public Statement
{
	PrintContextStatement(Range range) 
		: Statement(std::get<0>(range), std::get<1>(range)) 
	{}

	void execute(Context& ctx) override
	{		
		if (ctx.has_errors()) return;

		ctx.scope->print(ctx);
		printf("\n");
	}
};


struct GetStatement : public Statement
{
	std::string entity_name;
	std::vector<CompParamCtor> components;

	GetStatement(Range range, std::string name, std::vector<CompParamCtor> comps)
		: Statement(std::get<0>(range), std::get<1>(range))
		, entity_name{ name }
		, components{ comps }
	{}

	void execute(Context& ctx) override
	{
		if (ctx.has_errors()) return;

		auto e = ctx.scope->get_binding(entity_name);
		if (!e)
		{
			ctx.make_interpret_error(string_format("Variable '%s' not found", entity_name.c_str()), this);
			return;
		}

		auto entity = dynamic_cast<EntityExpr*>(e.get());
		if (!entity)
		{
			ctx.make_interpret_error(string_format("Expected entity reference, got %s instead.", e->to_string(ctx).c_str()), this);
			return;
		}

		ctx.scope->push_scope();

		for (auto& comp_ctor : components)
		{
			const auto& comp = ecs_get_component_by_instance(*ctx.ecs, entity->r.value, comp_ctor.comp_name);
			int index = 0;
			for (auto& var_param : comp_ctor.params)
			{
				if (VarExpr* var = dynamic_cast<VarExpr*>(var_param.get()))
				{
					auto& name = var->name;
					auto& value = comp.members[index];

					std::shared_ptr<Expr> expr_value = nullptr;

					switch (value.kind)
					{
					case EComponentMember::Bool:
						expr_value.reset(new BoolExpr(value.data.b.value));
						break;
					case EComponentMember::Int:
						expr_value.reset(new IntExpr(value.data.i.value));
						break;
					case EComponentMember::Float:
						expr_value.reset(new FloatExpr(value.data.f.value));
						break;
					case EComponentMember::EntityRef:
						expr_value.reset(new EntityExpr(value.data.e.value));
						break;
					}

					std::shared_ptr<Expr> ref_expr(new CompMemberRefExpr(name, entity->r.value, comp, index, expr_value));
					ctx.scope->add_binding(name, ref_expr);
				}
				else
				{
					if (dynamic_cast<VarExpr*>(var_param.get()) != nullptr)
					{
						ctx.make_interpret_error(string_format("Expected variable name."), this);
						return;
					}
				}

				index++;
			}
		}
	}
};

struct AttachStatement : public Statement
{
	std::string entity_name;
	std::vector<CompCtor> components;

	AttachStatement(Range range, std::string name, std::vector<CompCtor> comps)
		: Statement(std::get<0>(range), std::get<1>(range))
		, entity_name{name}
		, components{comps}
	{}

	void execute(Context& ctx) override
	{
		if (ctx.has_errors()) return;

		auto e = ctx.scope->get_binding(entity_name);
		if (!e)
		{
			ctx.make_interpret_error(string_format("Variable '%s' not found", entity_name.c_str()), this);
			return;
		}

		auto entity = dynamic_cast<EntityExpr*>(e.get());
		if (!entity)
		{
			ctx.make_interpret_error(string_format("Expected entity reference, got %s instead.", e->to_string(ctx).c_str()), this);
			return;
		}

		for (auto& ctor : components)
		{
			auto& type = ecs_get_type(*ctx.ecs, ctor.comp_name);
			ecs_adorn_instance(*ctx.ecs, entity->r.value, ctor.comp_name);
			auto& comp = ecs_get_component_by_instance(*ctx.ecs, entity->r.value, ctor.comp_name);

			int i = 0;
			for (auto& [member_name, value] : ctor.fields)
			{
				auto& typed_val = value->eval(ctx);

				switch (type.members[i].kind)
				{
				case EComponentMember::Int:
					if (typed_val.type != EType::Int)
					{
						ctx.make_interpret_error(string_format("Expected int, got %s", stringify_type(typed_val.type).c_str()), this);
						return;
					}
					ecs_set_member_in_component(comp, member_name, Int{ typed_val.data.int_value });
					break;
				case EComponentMember::Float:
					if (typed_val.type != EType::Float)
					{
						ctx.make_interpret_error(string_format("Expected float, got %s", stringify_type(typed_val.type).c_str()), this);
						return;
					}
					ecs_set_member_in_component(comp, member_name, Float{ typed_val.data.float_value });
					break;
				case EComponentMember::EntityRef:
					if (typed_val.type != EType::Entity)
					{
						ctx.make_interpret_error(string_format("Expected entity ref, got %s", stringify_type(typed_val.type).c_str()), this);
						return;
					}
					ecs_set_member_in_component(comp, member_name, EntityRef{ typed_val.data.entity_value });
					break;
				}
				i++;
			}
		}
	}
};

struct DetachStatement : public Statement
{
	std::string entity_name;
	std::vector<std::string> components;

	DetachStatement(Range range, std::string name, std::vector<std::string> comps)
		: Statement(std::get<0>(range), std::get<1>(range))
		, entity_name{ name }
		, components{ comps }
	{}

	void execute(Context& ctx) override
	{
		if (ctx.has_errors()) return;

		auto e = ctx.scope->get_binding(entity_name);
		if (!e)
		{
			ctx.make_interpret_error(string_format("Variable '%s' not found", entity_name.c_str()), this);
			return;
		}

		auto entity = dynamic_cast<EntityExpr*>(e.get());
		if (!entity)
		{
			ctx.make_interpret_error(string_format("Expected entity reference, got %s instead.", e->to_string(ctx).c_str()), this);
			return;
		}

		for (auto& comp : components)
		{
			ecs_unadorn_instance(*ctx.ecs, entity->r.value, comp);
			ctx.scope->internal_rec_delete_comp_ref(entity->r.value, ecs_get_type_id(*ctx.ecs, comp));
		}
	}
};

struct QueryEntitiesStatement : public Statement
{
	std::string entity_name;
	std::vector<CompParamCtor> positive_components;
	std::vector<CompParamCtor> negative_components;

	std::vector<std::string> positive_names;
	std::vector<std::string> negative_names;
	std::vector<std::shared_ptr<Statement>> block;

	QueryEntitiesStatement(Range range, std::string entity_name, std::vector<CompParamCtor> positive, std::vector<CompParamCtor> negative, std::vector<std::shared_ptr<Statement>> block)
		: Statement(std::get<0>(range), std::get<1>(range))
		, entity_name(entity_name)
		, positive_components(positive)
		, negative_components(negative)
		, block(block)
	{
		for (auto& comp : positive_components)
		{
			positive_names.push_back(comp.comp_name);
		}

		for (auto& comp : negative_components)
		{
			negative_names.push_back(comp.comp_name);
		}
	}

	void execute(Context& ctx) override
	{
		if (ctx.has_errors()) return;

		auto& query = ecs_query(*ctx.ecs, positive_names, negative_names);
		for (auto entity : query)
		{
			if (!ctx.ecs->registry.valid(entity)) continue;

			ctx.scope->push_scope();
			ctx.scope->add_binding(entity_name, std::shared_ptr<EntityExpr>(new EntityExpr(entity)));

			for (auto& comp_ctor : positive_components)
			{
				const auto& comp = ecs_get_component_by_instance(*ctx.ecs, entity, comp_ctor.comp_name);
				int index = 0;
				for (auto& var_param : comp_ctor.params)
				{
					if (VarExpr* var = dynamic_cast<VarExpr*>(var_param.get()))
					{
						auto& name = var->name;
						auto& value = comp.members[index];
						
						std::shared_ptr<Expr> expr_value = nullptr;

						switch (value.kind)
						{
						case EComponentMember::Bool:
							expr_value.reset(new BoolExpr(value.data.b.value));
							break;
						case EComponentMember::Int:
							expr_value.reset(new IntExpr(value.data.i.value));
							break;
						case EComponentMember::Float: 
							expr_value.reset(new FloatExpr(value.data.f.value));
							break;
						case EComponentMember::EntityRef:
							expr_value.reset(new EntityExpr(value.data.e.value));
							break;
						}

						std::shared_ptr<Expr> ref_expr(new CompMemberRefExpr(name, entity, comp, index, expr_value));
						ctx.scope->add_binding(name, ref_expr);
					}
					else
					{
						if (dynamic_cast<VarExpr*>(var_param.get()) != nullptr)
						{
							ctx.make_interpret_error(string_format("Expected variable name."), this);
							return;
						}
					}

					index++;
				}
			}

			ctx.depth++;
			for (auto statement : block)
			{
				statement->execute(ctx);
			}
			ctx.depth--;
			ctx.scope->pop_scope();
		}
	}
};

const std::string WHITESPACE = " \n\r\t\f\v";

std::string ltrim(const std::string& s)
{
	size_t start = s.find_first_not_of(WHITESPACE);
	return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string& s)
{
	size_t end = s.find_last_not_of(WHITESPACE);
	return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string& s) {
	return rtrim(ltrim(s));
}

struct TokenPos
{
	std::string token;
	int line;
	int start;
	int end;
};

std::vector<TokenPos> split(std::string source_code)
{
	static std::unordered_set<char> symbols{ 
		'(', ')', ',', ';', ':', '[', ']', '_', 
		'@', '+', '-', '*', '/', '{', '}', '<', 
		'>', '=', '!'
	};

	std::vector<TokenPos> tokens;
	std::string current_token{};

	std::size_t i = 0;
	int x = 0;
	int y = 1;
	int start_x = 0;

	for (; i < source_code.size(); i++)
	{				
		const char c = source_code[i];
		
		if (c == '\n') 
		{
			y++; x = 0;
			start_x = 0;
		}

		x++;
		if (std::isalnum(c) || c == '-')
		{
			if (current_token == "")
			{
				start_x = x;
			}
			current_token += c;
		}
		else if (std::isspace(c))
		{
			if (current_token != "")
			{
				auto tok = trim(current_token);
				if (tok != "")
				{
					TokenPos pos;
					pos.token = current_token;
					pos.line = y;
					pos.start = start_x;
					pos.end = start_x + current_token.size();
					tokens.push_back(pos);
				}
				current_token = "";
			}
		}
		else
		{
			auto tok = trim(current_token);
			if (tok != "")
			{
				TokenPos pos;
				pos.token = tok;
				pos.line = y;
				pos.start = start_x;
				pos.end = start_x + tok.size();
				tokens.push_back(pos);
			}
			current_token = "";
			
			if (symbols.count(c) > 0)
			{
				if (current_token == "")
				{
					start_x = x;
				}
				current_token += c;
				if (current_token == "=" || current_token == "<" || current_token == ">")
				{
					auto back = tokens.back().token;
					if (back == "<" || back == ">" || back == "=" || back == "!")
					{
						current_token = back + c;
						tokens.pop_back();
					}
				}

				TokenPos pos;
				pos.token = current_token;
				pos.line = y;
				pos.start = start_x;
				pos.end = start_x + current_token.size();

				tokens.push_back(pos);
				current_token = "";
			}
		}
	}

	if (current_token.size() > 0)
	{
		TokenPos pos;
		pos.token = current_token;
		pos.line = y;
		pos.start = start_x;
		pos.end = start_x + current_token.size();
		tokens.push_back(pos);
	}

	return tokens;
}

std::deque<Token> tokenize(std::vector<TokenPos> token_pos)
{
	static std::unordered_set<std::string> keywords{ 
		"create", "entity", "with", "without", 
		"foreach", "query", "define", "print", 
		"system", "destroy", "attach", "detach",
		"get", "to", "from", "true", "false",
		"if", "else"
	};
	
	static std::unordered_set<std::string> symbols{ 
		"(", ")", ",", ";", ":", "[", "]", 
		"_", "@", "+", "-", "*", "/", "{", "}",
		"<", "<=", "==", "!=", ">=", ">"
	};

	std::deque<Token> tokens;
	for (auto current_token : token_pos)
	{
		auto tok = current_token.token;

		if (keywords.count(current_token.token) > 0)
		{
			Token token;
			token.line = current_token.line;
			token.start = current_token.start;
			token.end = current_token.end;

			token.type = EToken::Keyword;
			
			if (tok == "create")
				token.keyword = EKeyword::Create;
			else if (tok == "entity")
				token.keyword = EKeyword::Entity;
			else if (tok == "with")
				token.keyword = EKeyword::With;
			else if (tok == "without")
				token.keyword = EKeyword::Without;
			else if (tok == "foreach")
				token.keyword = EKeyword::Foreach;
			else if (tok == "query")
				token.keyword = EKeyword::Query;
			else if (tok == "define")
				token.keyword = EKeyword::Define;
			else if (tok == "print")
				token.keyword = EKeyword::Print;
			else if (tok == "system")
				token.keyword = EKeyword::System;
			else if (tok == "destroy")
				token.keyword = EKeyword::Destroy;
			else if (tok == "attach")
				token.keyword = EKeyword::Attach;
			else if (tok == "detach")
				token.keyword = EKeyword::Detach;
			else if (tok == "get")
				token.keyword = EKeyword::Get;
			else if (tok == "to")
				token.keyword = EKeyword::To;
			else if (tok == "from")
				token.keyword = EKeyword::From;
			else if (tok == "if")
				token.keyword = EKeyword::If;
			else if (tok == "else")
				token.keyword = EKeyword::Else;
			else if (tok == "false")
				token.type = EToken::False;
			else if (tok == "true") 
				token.type = EToken::True;

			tokens.push_back(token);
		}
		else if (symbols.count(tok) > 0)
		{
			Token token;
			token.line = current_token.line;
			token.start = current_token.start;
			token.end = current_token.end;

			if (tok == "(")
				token.type = EToken::OpenParen;
			else if (tok == ")")
				token.type = EToken::ClosedParen;
			else if (tok == ",")
				token.type = EToken::Comma;
			else if (tok == "[")
				token.type = EToken::OpenBracket;
			else if (tok == "]")
				token.type = EToken::ClosedBracket;
			else if (tok == ":")
				token.type = EToken::Colon;
			else if (tok == ";")
				token.type = EToken::Semicolon;
			else if (tok == "_")
				token.type = EToken::Underscore;
			else if (tok == "@")
				token.type = EToken::Monkey;
			else if (tok == "+")
				token.type = EToken::Plus;
			else if (tok == "-")
				token.type = EToken::Minus;
			else if (tok == "*")
				token.type = EToken::Mult;
			else if (tok == "/")
				token.type = EToken::Div;
			else if (tok == "{")
				token.type = EToken::OpenBrace;
			else if (tok == "}")
				token.type = EToken::ClosedBrace;
			else if (tok == "<")
				token.type = EToken::Lt;
			else if (tok == "<=")
				token.type = EToken::Le;
			else if (tok == "==")
				token.type = EToken::Eq;
			else if (tok == "!=")
				token.type = EToken::Ne;
			else if (tok == ">")
				token.type = EToken::Gt;
			else if (tok == ">=")
				token.type = EToken::Ge;
			tokens.push_back(token);
		}
		else if (std::isdigit(tok[0]))
		{
			Token token;
			
			token.line = current_token.line;
			token.start = current_token.start;
			token.end = current_token.end;

			token.type = EToken::Number;
			token.number = std::atoi(tok.c_str());
			tokens.push_back(token);
		}
		else
		{
			Token token;
			
			token.line = current_token.line;
			token.start = current_token.start;
			token.end = current_token.end;

			token.type = EToken::Quote;
			token.quote = tok;
			tokens.push_back(token);
		}
	}

	return tokens;
}

void advance(std::deque<Token>& tokens)
{
	tokens.pop_front();
}

void digest(std::deque<Token>& tokens, EToken type)
{
	auto& tok = tokens.front();
	if (tok.type != type)
	{
		ParseError p;
		p.text = string_format("Expected token type %s, but %s found instead.", stringify_token(type).c_str(), stringify_token(tok.type).c_str());
		p.token = tok;
		generic_parse_error = p;
		return;
	}
	tokens.pop_front();
}

bool maybe_digest(std::deque<Token>& tokens, EToken type)
{
	auto& tok = tokens.front();
	if (tok.type == type)
	{
		tokens.pop_front();
		return true;
	}
	else
	{
		return false;
	}
}

void digest_keyword(std::deque<Token>& tokens, EKeyword keyword)
{
	auto& tok = tokens.front();
	if (tok.type != EToken::Keyword)
	{
		ParseError p;
		p.text = string_format("Expected variable name, found %s instead", stringify_token(tok.type).c_str());
		p.token = tok;
		generic_parse_error = p;
		return;
	}

	if (tok.keyword != keyword)
	{
		ParseError p;
		p.text = string_format("Keyword %s expected, %s found.", stringify_keyword(keyword).c_str(), stringify_keyword(tok.keyword).c_str());
		p.token = tok;
		generic_parse_error = p;
		return;
	}
	tokens.pop_front();
}

void expect(std::deque<Token>& tokens, EToken type)
{
	auto& tok = tokens.front();
	if(tok.type != type)
	{
		ParseError p;
		p.text = string_format("Expected token type %s, but %s found instead.", stringify_token(type).c_str(), stringify_token(tok.type).c_str());
		p.token = tok;
		generic_parse_error = p;
		return;
	}
}

std::string digest_quote(std::deque<Token>& tokens)
{
	expect(tokens, EToken::Quote);
	auto quote = tokens.front().quote;
	tokens.pop_front();
	return quote;
}

EType parse_type_name(std::deque<Token>& tokens)
{
	auto tok = tokens.front();
	auto type_name = digest_quote(tokens);
	if (type_name != "int" && type_name != "ref" && type_name != "float" && type_name != "bool")
	{
		ParseError p;
		p.text = string_format("Expected either int, ref, float, or bool found %s instead.", type_name.c_str());
		p.token = tok;
		generic_parse_error = p;
		return EType::Null;
	}

	if (type_name == "bool")
	{
		return EType::Bool;
	}
	else if (type_name == "int")
	{
		return EType::Int;
	}
	else if (type_name == "ref")
	{
		return EType::Entity;
	}
	else if (type_name == "float")
	{
		return EType::Float;
	}

	return EType::Entity;
}

std::shared_ptr<Expr> parse_expr(std::deque<Token>& tokens);

std::shared_ptr<Expr> parse_atom(std::deque<Token>& tokens)
{
	auto tok = tokens.front();
	tokens.pop_front();

	if (tok.type == EToken::Number)
	{
		return std::shared_ptr<Expr>(new IntExpr(tok.number));
	}
	else if (tok.type == EToken::Quote)
	{
		return std::shared_ptr<Expr>(new VarExpr(tok.quote));
	}
	else if (tok.type == EToken::True)
	{
		return std::shared_ptr<Expr>(new BoolExpr(true));
	}
	else if (tok.type == EToken::False)
	{
		return std::shared_ptr<Expr>(new BoolExpr(false));
	}
	
	return nullptr;
}

std::shared_ptr<Expr> parse_arithmetic_factor(std::deque<Token>& tokens)
{
	if (tokens.front().type == EToken::OpenParen)
	{
		auto tok = parse_expr(tokens);
		digest(tokens, EToken::ClosedParen);

		return tok;
	}
	else
	{
		return parse_atom(tokens);
	}
}

std::shared_ptr<Expr> parse_arithmetic_operand(std::deque<Token>& tokens)
{
	std::unordered_map<EToken, EArithmetic> map{
		{ EToken::Mult, EArithmetic::Mult },
		{ EToken::Div, EArithmetic::Div },
		{ EToken::Mod, EArithmetic::Mod },
	};

	auto lhs = parse_arithmetic_factor(tokens);

	while (map.count(tokens.front().type) > 0)
	{
		auto op_tok = tokens.front();
		advance(tokens);
		auto rhs = parse_arithmetic_factor(tokens);
		if (rhs != nullptr)
			lhs = std::shared_ptr<ArithExpr>(new ArithExpr(map[op_tok.type], lhs, rhs));
	}

	return lhs;
}

std::shared_ptr<Expr> parse_logical_operand(std::deque<Token>& tokens)
{
	auto lhs = parse_arithmetic_operand(tokens);

	while (tokens.front().type == EToken::Plus || tokens.front().type == EToken::Minus)
	{
		auto op_tok = tokens.front();
		advance(tokens);
		auto rhs = parse_arithmetic_operand(tokens);
		if (rhs != nullptr)
			lhs = std::shared_ptr<ArithExpr>(new ArithExpr(op_tok.type == EToken::Plus ? EArithmetic::Add : EArithmetic::Sub, lhs, rhs));
	}

	return lhs;
}

std::shared_ptr<Expr> parse_expr(std::deque<Token>& tokens)
{
	static std::unordered_map<std::string, ELogical> log_ops{
	{ "<", ELogical::Lt },
	{ "<=", ELogical::Le },
	{ "==", ELogical::Eq },
	{ "!=", ELogical::Ne },
	{ ">=", ELogical::Ge },
	{ ">", ELogical::Gt }
	};

	auto lhs = parse_logical_operand(tokens);

	if (log_ops.count(stringify_token(tokens.front().type)) > 0)
	{
		auto op = log_ops[stringify_token(tokens.front().type)];
		auto op_tok = tokens.front();
		advance(tokens);
		auto rhs = parse_logical_operand(tokens);
		if (rhs != nullptr)
			lhs = std::shared_ptr<LogicalExpr>(new LogicalExpr(op, lhs, rhs));
	}

	return lhs;
}

CompCtor parse_comp_ctor(std::deque<Token>& tokens)
{
	auto comp_name = digest_quote(tokens);
	std::vector<std::tuple<std::string, std::shared_ptr<Expr>>> fields;

	if (tokens.front().type == EToken::OpenParen)
	{
		digest(tokens, EToken::OpenParen);

		while (tokens.front().type != EToken::ClosedParen)
		{
			auto member_name = digest_quote(tokens);
			digest(tokens, EToken::Colon);
			auto value = parse_expr(tokens);
			maybe_digest(tokens, EToken::Comma);
			fields.push_back({ member_name, value });
		}

		digest(tokens, EToken::ClosedParen);
	}
	return CompCtor{ comp_name, fields };
}

CompParamCtor parse_comp_params_ctor(std::deque<Token>& tokens)
{
	auto comp_name = digest_quote(tokens);
	std::vector<std::shared_ptr<Expr>> fields;

	if (tokens.front().type == EToken::OpenParen)
	{
		digest(tokens, EToken::OpenParen);

		while (tokens.front().type != EToken::ClosedParen)
		{			
			auto value = parse_expr(tokens);
			fields.push_back(value);
			maybe_digest(tokens, EToken::Comma);
		}

		digest(tokens, EToken::ClosedParen);
	}

	return CompParamCtor{ comp_name, fields };
}

// "define Position(x: int, y: int);"
std::shared_ptr<Statement> parse_comp_define(std::deque<Token>& tokens)
{
	auto start = tokens.front();
	digest_keyword(tokens, EKeyword::Define);
	auto comp_name = digest_quote(tokens);
	std::vector<std::tuple<std::string, EType>> members;

	if (tokens.front().type == EToken::OpenParen)
	{
		digest(tokens, EToken::OpenParen);
		while (tokens.front().type != EToken::ClosedParen)
		{
			auto member_name = digest_quote(tokens);
			digest(tokens, EToken::Colon);
			auto type_name = parse_type_name(tokens);
			maybe_digest(tokens, EToken::Comma);
			members.push_back({ member_name, type_name });
		}
		digest(tokens, EToken::ClosedParen);
	}
	auto end = tokens.front();
	digest(tokens, EToken::Semicolon);

	return std::make_shared<DefineComponentStatement>(std::tuple{ start, end }, comp_name, members);
}

//"create player-character with Position(x: 10, y: 10), Mass(kg: 1), Player();"
std::shared_ptr<Statement> parse_create_entity(std::deque<Token>& tokens)
{
	auto start = tokens.front();
	digest_keyword(tokens, EKeyword::Create);
	auto entity_name = digest_quote(tokens);
	std::vector<CompCtor> comps;

	if (tokens.front().keyword == EKeyword::With)
	{
		digest_keyword(tokens, EKeyword::With);
		while (tokens.front().type != EToken::Semicolon)
		{
			comps.push_back(parse_comp_ctor(tokens));
			maybe_digest(tokens, EToken::Comma);
		}
	}

	auto end = tokens.front();
	digest(tokens, EToken::Semicolon);

	return std::make_shared<CreateEntityStatement>(std::tuple{ start, end }, entity_name, comps);
}

std::vector<std::shared_ptr<Statement>> parse_block(std::deque<Token>& tokens);

//"if(something) { ... } else { ... }
std::shared_ptr<Statement> parse_if(std::deque<Token>& tokens)
{
	auto start_tok = tokens.front();	
	digest_keyword(tokens, EKeyword::If);
	digest(tokens, EToken::OpenParen);
	auto& expr = parse_expr(tokens);
	digest(tokens, EToken::ClosedParen);
	auto end_tok = tokens.front();
	digest(tokens, EToken::OpenBrace);
	auto& then_block = parse_block(tokens);
	std::vector<std::shared_ptr<Statement>> else_block;

	digest(tokens, EToken::ClosedBrace);
	if (tokens.front().keyword == EKeyword::Else)
	{
		digest_keyword(tokens, EKeyword::Else);
		digest(tokens, EToken::OpenBrace);
		else_block = parse_block(tokens);
		end_tok = tokens.front();
		digest(tokens, EToken::ClosedBrace);
	}
	
	return std::shared_ptr<Statement>(new IfStatement({ start_tok, end_tok }, expr, then_block, else_block));
}

//"destroy player-character;"
std::shared_ptr<Statement> parse_destroy_entity(std::deque<Token>& tokens)
{
	auto start = tokens.front();
	digest_keyword(tokens, EKeyword::Destroy);
	auto entity_name = digest_quote(tokens);
	auto end = tokens.front();
	digest(tokens, EToken::Semicolon);

	return std::make_shared<DestroyEntityStatement>(std::tuple{ start, end }, entity_name);
}

//"get Position(x, y) from e1;"
std::shared_ptr<Statement> parse_get_from_entity(std::deque<Token>& tokens)
{
	auto start = tokens.front();
	digest_keyword(tokens, EKeyword::Get);
	std::vector<CompParamCtor> comps;
	while (tokens.front().keyword != EKeyword::From)
	{
		comps.push_back(parse_comp_params_ctor(tokens));
		maybe_digest(tokens, EToken::Comma);
	}
	digest_keyword(tokens, EKeyword::From);
	auto entity_name = digest_quote(tokens);
	auto end = tokens.front();
	digest(tokens, EToken::Semicolon);

	return std::make_shared<GetStatement>(std::tuple{ start, end }, entity_name, comps);
}

//"attach Player(x: 2, y: 3) to player-character;"
std::shared_ptr<Statement> parse_attach_entity(std::deque<Token>& tokens)
{
	auto start = tokens.front();
	digest_keyword(tokens, EKeyword::Attach);
	std::vector<CompCtor> comps;
	while (tokens.front().keyword != EKeyword::To)
	{
		comps.push_back(parse_comp_ctor(tokens));
		maybe_digest(tokens, EToken::Comma);
	}
	digest_keyword(tokens, EKeyword::To);
	auto entity_name = digest_quote(tokens);
	auto end = tokens.front();
	digest(tokens, EToken::Semicolon);

	return std::make_shared<AttachStatement>(std::tuple{ start, end }, entity_name, comps);
}

//"detach Player from player-character;"
std::shared_ptr<Statement> parse_detach_entity(std::deque<Token>& tokens)
{
	auto start = tokens.front();
	digest_keyword(tokens, EKeyword::Detach);
	std::vector<std::string> comps;
	while (tokens.front().keyword != EKeyword::From)
	{
		comps.push_back(digest_quote(tokens));
		maybe_digest(tokens, EToken::Comma);
	}
	digest_keyword(tokens, EKeyword::From);
	auto entity_name = digest_quote(tokens);
	auto end = tokens.front();
	digest(tokens, EToken::Semicolon);

	return std::make_shared<DetachStatement>(std::tuple{ start, end }, entity_name, comps);
}

std::vector<std::shared_ptr<Statement>> parse_block(std::deque<Token>& tokens);

// "system Physics[] { }
std::shared_ptr<Statement> parse_system(std::deque<Token>& tokens)
{
	auto start = tokens.front();
	digest_keyword(tokens, EKeyword::System);
	auto system_name = digest_quote(tokens);
	if (maybe_digest(tokens, EToken::OpenBracket))
	{
		// TODO: constraints go here
		digest(tokens, EToken::ClosedBracket);
	}
	digest(tokens, EToken::OpenBrace);
	auto block = parse_block(tokens);
	auto end = tokens.front();
	digest(tokens, EToken::ClosedBrace);
	return std::shared_ptr<Statement>(new DefineSystemStatement({ start, end }, system_name, block));
}

// "foreach player with Position(x, y), Player without Mass { }"
std::shared_ptr<Statement> parse_foreach(std::deque<Token>& tokens)
{
	auto start = tokens.front();
	digest_keyword(tokens, EKeyword::Foreach);
	auto entity_name = digest_quote(tokens);
	std::vector<CompParamCtor> positive_comps;
	std::vector<CompParamCtor> negative_comps;

	auto tok = tokens.front();

	if (tok.type == EToken::Keyword && tok.keyword == EKeyword::With)
	{
		digest_keyword(tokens, EKeyword::With);
		while (true)
		{
			positive_comps.push_back(parse_comp_params_ctor(tokens));
			maybe_digest(tokens, EToken::Comma);

			tok = tokens.front();

			if (tok.type == EToken::OpenBrace) break;
			if (tok.type == EToken::Keyword && tok.keyword == EKeyword::Without) break;
		}
	}

	if (tok.type == EToken::Keyword && tok.keyword == EKeyword::Without)
	{
		digest_keyword(tokens, EKeyword::Without);
		while (tok.type != EToken::OpenBrace)
		{
			negative_comps.push_back(parse_comp_params_ctor(tokens));
			maybe_digest(tokens, EToken::Comma);

			tok = tokens.front();
		}
	}

	digest(tokens, EToken::OpenBrace);
	auto block = parse_block(tokens);
	auto end = tokens.front();
	digest(tokens, EToken::ClosedBrace);
	return std::shared_ptr<Statement>(new QueryEntitiesStatement({ start, end }, entity_name, positive_comps, negative_comps, block));
}

// "print();"
std::shared_ptr<Statement> parse_print(std::deque<Token>& tokens)
{
	auto start = tokens.front();
	digest_keyword(tokens, EKeyword::Print);
	digest(tokens, EToken::OpenParen);
	digest(tokens, EToken::ClosedParen);
	auto end = tokens.front();
	digest(tokens, EToken::Semicolon);
	return std::shared_ptr<Statement>(new PrintContextStatement({ start, end }));
}

std::vector<std::shared_ptr<Statement>> parse(std::string input)
{
	std::vector<std::shared_ptr<Statement>> statements;

	auto& tokens = tokenize(split(input));

	return parse_block(tokens);
}

std::vector<std::shared_ptr<Statement>> parse_block(std::deque<Token>& tokens)
{
	std::vector<std::shared_ptr<Statement>> statements;

	while (!tokens.empty())
	{
		if (tokens.front().type != EToken::Keyword)
		{
			if (tokens.front().type == EToken::Quote)
			{
				ParseError p;
				p.text = string_format("Keyword expected, unknown identifier '%s' found.", tokens.front().quote.c_str());
				p.token = tokens.front();
				generic_parse_error = p;
			}

			break;
		}

		if (generic_parse_error.has_value()) return statements;

		auto tok = tokens.front();
		if (tok.keyword == EKeyword::Define)
		{
			statements.push_back(parse_comp_define(tokens));
		}
		else if (tok.keyword == EKeyword::Create)
		{
			statements.push_back(parse_create_entity(tokens));
		}
		else if (tok.keyword == EKeyword::Destroy)
		{
			statements.push_back(parse_destroy_entity(tokens));
		}
		else if (tok.keyword == EKeyword::Attach)
		{
			statements.push_back(parse_attach_entity(tokens));
		}
		else if (tok.keyword == EKeyword::Detach)
		{
			statements.push_back(parse_detach_entity(tokens));
		}
		else if (tok.keyword == EKeyword::Get)
		{
			statements.push_back(parse_get_from_entity(tokens));
		}
		else if (tok.keyword == EKeyword::Foreach)
		{
			statements.push_back(parse_foreach(tokens));
		}
		else if (tok.keyword == EKeyword::Print)
		{
			statements.push_back(parse_print(tokens));
		}
		else if (tok.keyword == EKeyword::System)
		{
			statements.push_back(parse_system(tokens));
		}
		else if (tok.keyword == EKeyword::If)
		{
			statements.push_back(parse_if(tokens));
		}
	}

	return statements;
}

std::vector<std::shared_ptr<Statement>> parse_file(Context& ctx, std::string filename)
{
	std::ifstream file_stream(filename);
	if (file_stream.is_open())
	{
		std::string line;
		while (std::getline(file_stream, line))
		{
			ctx.source_lines.push_back(line);
		}

		std::stringstream joined;
		std::copy(ctx.source_lines.begin(), ctx.source_lines.end(),
			std::ostream_iterator<std::string>(joined, "\n"));

		ctx.source_text = joined.str();
		ctx.interpreted_statements = parse(ctx.source_text);

		if (ctx.is_parse_okay())
		{
			ctx.execute();
		}
		else
		{
			ctx.die_with_error();
		}

		return ctx.interpreted_statements;
	}
}