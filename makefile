default:
	gcc -lm la_dedup.c -o la_dedup
	gcc -lm la_optimize.c -o la_optimize
	gcc -lm la_bounds.c -o la_bounds
	gcc -lm la_strip.c -o la_strip
