#include <cstdio>
#include <iostream>

#include <entt/entt.hpp>

#include "ecs.h"
#include "parse.h"
 
int main(int argc, char* argv[])
{
	Context ctx;

	/*
		parse and interpret:

		>	define Position(x: int, y: int);
		>	define Mass(kg: int);
		>	define Foo(bar: int);

		>	create p1 with Position(x: 4, y: 5);
		>	create p2 with Mass(kg: 0), Foo(bar: 123);

		>	attach Mass(kg: 3) to p1;
		>	get Mass(x), Position(a, b) from p1;

		>	attach Mass(kg: x + a * b) to p2;
		>	get Mass(x2), Foo(bar) from p2;
		>	print();

		>	destroy p1;
		>	print();

		>	detach Foo from p2;
		>	print();
	*/

	parse_file(ctx, "test-coll.ska");

	// systems tick via `update`:
	ctx.update();

	return 0;
}
