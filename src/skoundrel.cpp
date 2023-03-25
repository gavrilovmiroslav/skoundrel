#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  600

#include <SDL2/SDL.h>
#include <entt/entt.hpp>
#include <libtcod.hpp>
#include <iostream>

#undef main 

#pragma once
#include <cstdio>
#include "ecs.h"
#include "parse.h"

using namespace std::chrono;

int main(int argc, char* argv[])
{
	ECS ecs;

	Context ctx;
	ctx.ecs = &ecs;

	auto statements = parse(
		"define Position(x: int, y: int);"
		"define Mass(kg: int);"
		"define Hit(ok: bool);"
		"define Player;"
		"define Collision;"

		"create player-character1 with Position(x: 1 + 1, y: 2 + 2), Player, Hit(ok: false);"
		"detach Player from player-character1;"
		"destroy player-character1;"

		"create player-character1 with Position(x: 1 + 1, y: 2 + 2), Player, Hit(ok: false);"		
		"create player-character2 with Position(x: 12, y: 32), Player;"
		"attach Mass(kg: 1) to player-character1;"
		"attach Mass(kg: 33) to player-character1;"
		"detach Mass from player-character1;"

		"system Product[] {"
		"  foreach player1 with Position(x1, y1), Player, Hit(ok) {"
		"    print();"
		"  }"
		"}"

		"system Collisions[] {"
		"  foreach e1 with Position(x1, y1), Collision {"
		"    create collision with Position(x: x1 * 2, y: 2 * y1), Collision;"
		"    print();"
		"    destroy e1;"
		"  }"
		"}"
	);

	if (ctx.is_parse_okay())
	{
		for (auto stat : statements)
			stat->execute(ctx);

		printf("\n----------------\n");
		ctx.update();
		printf("\n----------------\n");
		ctx.update();
		printf("\n----------------\n");
		ctx.update();
	}

	return 0;
}
