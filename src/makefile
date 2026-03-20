default:
	gcc -lm la_bounds.c -o la_bounds
	gcc -lm la_dedup.c -o la_dedup
	gcc -lm la_position.c -o la_position
	gcc -lm la_rotate.c -o la_rotate
	gcc -lm la_strip.c -o la_strip

install:
	install -p -s la_bounds /usr/bin
	install -p -s la_dedup /usr/bin
	install -p -s la_position /usr/bin
	install -p -s la_rotate /usr/bin
	install -p -s la_strip /usr/bin

uninstall:
	rm -f /usr/bin/la_bounds
	rm -f /usr/bin/la_dedup
	rm -f /usr/bin/la_position
	rm -f /usr/bin/la_rotate
	rm -f /usr/bin/la_strip

