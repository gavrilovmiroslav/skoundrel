#pragma once

#pragma once

#include <string>
#include <vector>
#include <deque>
#include <unordered_set>
#include <iostream>
#include <algorithm>
#include <cassert>

#include "ecs.h"

enum class EToken
{
	Keyword,
	Number,
	OpenBracket,
	ClosedBracket,
	OpenParen,
	ClosedParen,
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
	Query,
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
	Entity,
	Int,
	Float,
};

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

struct Expr;

struct Context
{
	ECS* ecs = nullptr;
	std::unordered_map<std::string, std::shared_ptr<Expr>> env;
};

struct Expr
{
	virtual TypedValue eval(Context& ctx) = 0;
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
};

struct EntityExpr : public Expr
{
	EntityRef r;

	TypedValue eval(Context& ctx) override
	{
		TypedValue v;
		v.type = EType::Entity;
		v.data.entity_value = r.value;
		return v;
	}
};

struct VarExpr : public Expr
{
	std::string name;

	VarExpr(std::string s) : name(s)
	{}

	TypedValue eval(Context& ctx) override
	{
		return ctx.env[name]->eval(ctx);
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
};

struct CompCtor
{
	std::string comp_name;
	std::vector<std::tuple<std::string, std::shared_ptr<Expr>>> fields;
};

struct Statement
{
	virtual void execute(Context& ctx) = 0;
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
					assert(typed_val.type == EType::Int);
					ecs_set_member_in_component(comp, member_name, Int{ typed_val.data.int_value });
					break;
				case EComponentMember::Float:
					assert(typed_val.type == EType::Float);
					ecs_set_member_in_component(comp, member_name, Float{ typed_val.data.float_value });
					break;
				case EComponentMember::EntityRef:
					assert(typed_val.type == EType::Entity);
					ecs_set_member_in_component(comp, member_name, EntityRef{ typed_val.data.entity_value });
					break;
				}
				i++;
			}
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
			
			if (c == '(' || c == ')' || c == ',' || c == ';' || c == ':' || c == '[' || c == ']' || c == '_' || c == '@')
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
	static std::unordered_set<std::string> keywords{ "create", "entity", "with", "query", "define" };
	static std::unordered_set<std::string> symbols{ "(", ")", ",", ";", ":", "[", "]", "_", "@", "+", "-", "*", "/" };

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
			else if (tok == "query")
				token.keyword = EKeyword::Query;
			else if (tok == "define")
				token.keyword = EKeyword::Define;
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
	assert(tok.type == type);
	tokens.pop_front();
}

void maybe_digest(std::deque<Token>& tokens, EToken type)
{
	auto& tok = tokens.front();
	if (tok.type == type)
	{
		tokens.pop_front();
	}
}

void digest_keyword(std::deque<Token>& tokens, EKeyword keyword)
{
	auto& tok = tokens.front();
	assert(tok.type == EToken::Keyword);
	assert(tok.keyword == keyword);
	tokens.pop_front();
}

void expect(std::deque<Token>& tokens, EToken type)
{
	auto& tok = tokens.front();
	assert(tok.type == type);
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
	assert(type_name == "int" || type_name == "ref" || type_name == "float");

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

std::vector<std::shared_ptr<Statement>> parse(std::string input)
{
	std::vector<std::shared_ptr<Statement>> statements;

	auto& tokens = tokenize(split(input));
	
	while (!tokens.empty() && tokens.front().type == EToken::Keyword)
	{
		auto tok = tokens.front();
		if (tok.keyword == EKeyword::Define)
		{
			statements.push_back(parse_comp_define(tokens));
		}
		else if (tok.keyword == EKeyword::Create)
		{
			statements.push_back(parse_create_entity(tokens));
		}
	}
	
	return statements;
}
