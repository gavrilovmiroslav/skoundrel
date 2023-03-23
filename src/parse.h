#pragma once

#pragma once

#include <string>
#include <vector>
#include <deque>
#include <unordered_set>
#include <iostream>
#include <algorithm>
#include <optional>

#include <stdarg.h>  // For va_start, etc.
#include <memory>    // For std::unique_ptr

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

#include "ecs.h"

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
};

enum class EKeyword
{
	Define,
	Create,
	Entity,
	With,
	Without,
	Query,
	Foreach,
	Print,
	System,
};

struct Token
{
	EToken type;

	EKeyword keyword;
	std::string quote;
	int number;
};

enum class EType
{
	Null,
	Entity,
	Int,
	Float,
};

std::string stringify_type(EType type)
{
	switch (type)
	{
	case EType::Entity: return "Entity";
	case EType::Int: return "Int";
	case EType::Float: return "Float";
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
	case EToken::Div: default: return "div";
	}
}

std::string stringify_keyword(EKeyword keyword)
{
	switch (keyword)
	{
	case EKeyword::Define: return "define";
	case EKeyword::Create: return "create";
	case EKeyword::Entity: return "entity";
	case EKeyword::With: return "with";
	case EKeyword::Without: return "without";
	case EKeyword::Query: return "query";
	case EKeyword::Foreach: return "foreach";
	case EKeyword::System: return "system";
	case EKeyword::Print: default: return "print";
	}
}

struct TypedValue
{
	EType type;

	union
	{
		entt::entity entity_value;
		int int_value;
		float float_value;
	} data;
};

struct Scope;
struct Statement;

struct System
{
	std::string name;
	std::vector<std::shared_ptr<Statement>> block;

	System(std::string name, std::vector<std::shared_ptr<Statement>> block);
};

struct Context
{
	ECS* ecs = nullptr;
	Scope* scope;
	std::optional<std::string> error;
	std::vector<System> systems;

	Context();
	~Context();

	bool is_parse_okay();

	bool check(std::optional<std::string> error);

	void update();
};

struct Expr
{
	virtual TypedValue eval(Context& ctx) = 0;
	virtual std::string to_string(Context& ctx) = 0;
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
		return std::string("#") + std::to_string((uint64_t)r.value);
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
	Component comp;
	int param_index;

	std::shared_ptr<Expr> value;

	CompMemberRefExpr(std::string s, Component c, int p, std::shared_ptr<Expr> v)
		: name(s)
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
		return std::string("&") + type.name + "::" + name + " (" + value->to_string(ctx) + ")";
	}
};

enum class EArithmetic
{
	Add, Sub, Mult, Div, Mod
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
		return lhs->eval(ctx);
		//switch (op)
		//{
		//case EArithmetic::Add:
		//	break;
		//case EArithmetic::Sub:
		//	break;
		//case EArithmetic::Mult:
		//	break;
		//case EArithmetic::Div:
		//	break;
		//case EArithmetic::Mod:
		//	break;
		//}
	}

	std::string to_string(Context& ctx) override
	{
		return "ARITHMETIC NOT DONE YET!";
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
		int i = 0;
		while (ptr)
		{
			i++;
			ptr = ptr->next;
		}
		print(ctx, i);
	}

private:

	void print(Context& ctx, int indent)
	{
		if (next)
		{
			next->print(ctx, indent - 1);
		}
		for (auto& [name, value] : env)
		{
			auto space = std::string(indent, ' ');
			printf("%s%s: %s\n", space.c_str(), name.c_str(), value->to_string(ctx).c_str());
		}
	}

	std::unordered_map<std::string, std::shared_ptr<Expr>> env;
	std::shared_ptr<Scope> next;
};

Context::Context()
{
	scope = new Scope();
}

Context::~Context()
{
	delete scope;
}

static std::optional<std::string> parse_error = std::nullopt;

bool Context::is_parse_okay()
{
	return check(parse_error);
}

bool Context::check(std::optional<std::string> error = std::nullopt)
{
	if (error.has_value())
	{
		this->error = error;
	}

	if (this->error.has_value())
	{
		printf("Error: %s\n", this->error.value().c_str());
		return false;
	}

	return true;
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

struct Statement
{
	virtual void execute(Context& ctx) = 0;
};

void Context::update()
{
	for (auto& system : systems)
	{
		for (auto& statement : system.block)
		{
			statement->execute(*this);
		}
	}
}

System::System(std::string name, std::vector<std::shared_ptr<Statement>> block)
	: name(name), block(block)
{}

struct DefineSystemStatement : public Statement
{
	std::string system_name;
	// TODO: add system constraints 
	std::vector<std::shared_ptr<Statement>> block;

	DefineSystemStatement(std::string name, std::vector<std::shared_ptr<Statement>> block)
		: system_name(name), block(block)
	{}

	void execute(Context& ctx) override
	{
		ctx.systems.push_back(System(system_name, block));
	}
};

struct DefineComponentStatement : public Statement
{
	std::string comp_name;
	std::vector<std::tuple<std::string, EType>> members;

	DefineComponentStatement(std::string name, std::vector<std::tuple<std::string, EType>> mems)
		: comp_name(name)
		, members(mems)
	{}

	void execute(Context& ctx) override
	{
		if (!ctx.check()) return;

		std::vector<std::tuple<std::string, EComponentMember>> comp_members;
		for (auto& [k, v] : members)
		{
			switch (v)
			{
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

	CreateEntityStatement(std::string name, std::vector<CompCtor> flds)
		: entity_name(name)
		, components(flds)
	{}

	void execute(Context& ctx) override
	{
		if (!ctx.check()) return;

		auto e = ecs_create_instance(*ctx.ecs);
		
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
				case EComponentMember::Int:		
					if (typed_val.type != EType::Int)
					{
						ctx.error = std::make_optional(string_format("Expected int, got %s", stringify_type(typed_val.type).c_str()));
						return;
					}
					ecs_set_member_in_component(comp, member_name, Int{ typed_val.data.int_value });
					break;
				case EComponentMember::Float:
					if (typed_val.type != EType::Float)
					{
						ctx.error = std::make_optional(string_format("Expected float, got %s", stringify_type(typed_val.type).c_str()));
						return;
					}
					ecs_set_member_in_component(comp, member_name, Float{ typed_val.data.float_value });
					break;
				case EComponentMember::EntityRef:
					if (typed_val.type != EType::Entity)
					{
						ctx.error = std::make_optional(string_format("Expected entity ref, got %s", stringify_type(typed_val.type).c_str()));
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

struct PrintContextStatement : public Statement
{
	PrintContextStatement() {}

	void execute(Context& ctx) override
	{		
		if (!ctx.check()) return;

		ctx.scope->print(ctx);
		printf("\n");
	}
};

struct QueryEntitiesStatement : public Statement
{
	std::vector<CompParamCtor> positive_components;
	std::vector<CompParamCtor> negative_components;

	std::vector<std::string> positive_names;
	std::vector<std::string> negative_names;
	std::vector<std::shared_ptr<Statement>> block;

	QueryEntitiesStatement(std::vector<CompParamCtor> positive, std::vector<CompParamCtor> negative, std::vector<std::shared_ptr<Statement>> block)
		: positive_components(positive)
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
		if (!ctx.check()) return;

		auto& query = ecs_query(*ctx.ecs, positive_names, negative_names);
		for (auto entity : query)
		{
			ctx.scope->push_scope();
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

						std::shared_ptr<Expr> ref_expr(new CompMemberRefExpr(name, comp, index, expr_value));
						ctx.scope->add_binding(name, ref_expr);
					}
					else
					{
						if (dynamic_cast<VarExpr*>(var_param.get()) != nullptr)
						{
							ctx.error = std::make_optional(string_format("Expected variable name."));
							return;
						}
					}

					index++;
				}
			}

			for (auto statement : block)
			{
				statement->execute(ctx);
			}
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

std::vector<std::string> split(std::string source_code)
{
	std::vector<std::string> tokens;
	std::string current_token{};

	int i = 0;
	for (; i < source_code.size(); i++)
	{
		const char c = source_code[i];
		if (std::isalnum(c) || c == '-')
		{
			current_token += c;
		}
		else if (std::isspace(c))
		{
			if (current_token != "")
			{
				auto tok = trim(current_token);
				if (tok != "")
				{
					tokens.push_back(current_token);
				}
				current_token = "";
			}
		}
		else
		{
			auto tok = trim(current_token);
			if (tok != "")
			{
				tokens.push_back(tok);
			}
			current_token = "";
			
			if (c == '(' || c == ')' || c == ',' || c == ';' || c == ':' || c == '[' || c == ']' || c == '_' || c == '@' || c == '{' || c == '}')
			{
				current_token += c;
				tokens.push_back(current_token);
				current_token = "";
			}
		}
	}

	if (current_token.size() > 0)
	{
		tokens.push_back(current_token);
	}

	return tokens;
}

std::deque<Token> tokenize(std::vector<std::string> text_tokens)
{
	static std::unordered_set<std::string> keywords{ "create", "entity", "with", "without", "foreach", "query", "define", "print", "system" };
	static std::unordered_set<std::string> symbols{ "(", ")", ",", ";", ":", "[", "]", "_", "@", "+", "-", "*", "/", "{", "}" };

	std::deque<Token> tokens;
	for (auto tok : text_tokens)
	{
		if (keywords.count(tok) > 0)
		{
			Token token;
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
			tokens.push_back(token);
		}
		else if (symbols.count(tok) > 0)
		{
			Token token;
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

			tokens.push_back(token);
		}
		else if (std::isdigit(tok[0]))
		{
			Token token;
			token.type = EToken::Number;
			token.number = std::atoi(tok.c_str());
			tokens.push_back(token);
		}
		else
		{
			Token token;
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
		parse_error = std::make_optional(string_format("Expected token type %s, but %s found instead.", stringify_token(type).c_str(), stringify_token(tok.type).c_str()));
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
		parse_error = std::make_optional(string_format("Expected variable name, found %s instead", stringify_token(tok.type).c_str()));
		return;
	}

	if (tok.keyword != keyword)
	{
		parse_error = std::make_optional(string_format("Keyword %s expected, %s found.", stringify_keyword(keyword).c_str(), stringify_keyword(tok.keyword).c_str()));
		return;
	}
	tokens.pop_front();
}

void expect(std::deque<Token>& tokens, EToken type)
{
	auto& tok = tokens.front();
	if(tok.type != type)
	{
		parse_error = std::make_optional(string_format("Expected token type %s, but %s found instead.", stringify_token(type).c_str(), stringify_token(tok.type).c_str()));
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
	auto type_name = digest_quote(tokens);
	if (type_name != "int" && type_name != "ref" && type_name != "float")
	{
		parse_error = std::make_optional(string_format("Expected either int, ref, or float, found %s instead.", type_name.c_str()));
		return EType::Null;
	}

	if (type_name == "int")
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
	auto lhs = parse_arithmetic_factor(tokens);

	while (tokens.front().type == EToken::Plus || tokens.front().type == EToken::Minus)
	{
		auto op_tok = tokens.front();
		advance(tokens);
		auto rhs = parse_arithmetic_factor(tokens);

		lhs = std::shared_ptr<ArithExpr>(new ArithExpr(op_tok.type == EToken::Plus ? EArithmetic::Add : EArithmetic::Sub, lhs, rhs));
	}

	return lhs;
}

std::shared_ptr<Expr> parse_expr(std::deque<Token>& tokens)
{
	auto lhs = parse_arithmetic_operand(tokens);

	while (tokens.front().type == EToken::Mult || tokens.front().type == EToken::Div)
	{
		auto op_tok = tokens.front();
		advance(tokens);
		auto rhs = parse_arithmetic_operand(tokens);

		lhs = std::shared_ptr<ArithExpr>(new ArithExpr(op_tok.type == EToken::Mult ? EArithmetic::Mult : EArithmetic::Div, lhs, rhs));
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

// 	"define Position(x: int, y: int);"
std::shared_ptr<Statement> parse_comp_define(std::deque<Token>& tokens)
{
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
	digest(tokens, EToken::Semicolon);

	return std::make_shared<DefineComponentStatement>(comp_name, members);
}

//	"create player-character with Position(x: 10, y: 10), Mass(kg: 1), Player();"
std::shared_ptr<Statement> parse_create_entity(std::deque<Token>& tokens)
{
	digest_keyword(tokens, EKeyword::Create);
	auto entity_name = digest_quote(tokens);
	digest_keyword(tokens, EKeyword::With);

	std::vector<CompCtor> comps;
	while (tokens.front().type != EToken::Semicolon)
	{
		comps.push_back(parse_comp_ctor(tokens));
		maybe_digest(tokens, EToken::Comma);
	}
	digest(tokens, EToken::Semicolon);

	return std::make_shared<CreateEntityStatement>(entity_name, comps);
}

std::vector<std::shared_ptr<Statement>> parse_block(std::deque<Token>& tokens);

// "system Physics[] { }
std::shared_ptr<Statement> parse_system(std::deque<Token>& tokens)
{
	digest_keyword(tokens, EKeyword::System);
	auto system_name = digest_quote(tokens);
	if (maybe_digest(tokens, EToken::OpenBracket))
	{
		// TODO: constraints go here
		digest(tokens, EToken::ClosedBracket);
	}
	digest(tokens, EToken::OpenBrace);
	auto block = parse_block(tokens);
	digest(tokens, EToken::ClosedBrace);

	return std::shared_ptr<Statement>(new DefineSystemStatement(system_name, block));
}
// "foreach player with Position(x, y), Player without Mass { }"
std::shared_ptr<Statement> parse_foreach(std::deque<Token>& tokens)
{
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
	digest(tokens, EToken::ClosedBrace);

	return std::shared_ptr<Statement>(new QueryEntitiesStatement(positive_comps, negative_comps, block));
}

// "print();"
std::shared_ptr<Statement> parse_print(std::deque<Token>& tokens)
{
	digest_keyword(tokens, EKeyword::Print);
	digest(tokens, EToken::OpenParen);
	digest(tokens, EToken::ClosedParen);
	digest(tokens, EToken::Semicolon);
	return std::shared_ptr<Statement>(new PrintContextStatement());
}

std::vector<std::shared_ptr<Statement>> parse(std::string input)
{
	std::vector<std::shared_ptr<Statement>> statements;

	auto& tokens = tokenize(split(input));
	
	while (!tokens.empty() && tokens.front().type == EToken::Keyword)
	{
		if (parse_error.has_value()) return statements;

		auto tok = tokens.front();
		if (tok.keyword == EKeyword::Define)
		{
			statements.push_back(parse_comp_define(tokens));
		}
		else if (tok.keyword == EKeyword::Create)
		{
			statements.push_back(parse_create_entity(tokens));
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
	}

	return statements;
}

std::vector<std::shared_ptr<Statement>> parse_block(std::deque<Token>& tokens)
{
	std::vector<std::shared_ptr<Statement>> statements;

	while (!tokens.empty() && tokens.front().type == EToken::Keyword)
	{
		if (parse_error.has_value()) return statements;

		auto tok = tokens.front();
		if (tok.keyword == EKeyword::Define)
		{
			parse_error = std::make_optional(string_format("Define not allowed in a block."));
			return std::vector<std::shared_ptr<Statement>>{};
		}
		else if (tok.keyword == EKeyword::Create)
		{
			statements.push_back(parse_create_entity(tokens));
		}
		else if (tok.keyword == EKeyword::Foreach)
		{
			statements.push_back(parse_foreach(tokens));
		}
		else if (tok.keyword == EKeyword::Print)
		{
			statements.push_back(parse_print(tokens));
		}
	}

	return statements;
}
