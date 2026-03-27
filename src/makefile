default:
	gcc -lm la_bounds.c -o la_bounds
	gcc -lm la_dedup.c -o la_dedup
	gcc -lm la_position.c -o la_position
	gcc -lm la_rotate.c -o la_rotate
	gcc -lm la_strip.c -o la_strip
	gcc -lm la_svg2g.c -o la_svg2g -lxml2 -I/usr/include/libxml2
	gcc -lm la_g2svg.c -o la_g2svg -lcairo -I/usr/include/cairo
	gcc -lm la_g2pdf.c -o la_g2pdf -lcairo -I/usr/include/cairo
	gcc -lm -O3 la_optimize.c -o la_optimize

install:
	install -p -s la_bounds /usr/bin
	install -p -s la_dedup /usr/bin
	install -p -s la_position /usr/bin
	install -p -s la_rotate /usr/bin
	install -p -s la_strip /usr/bin
	install -p -s la_svg2g /usr/bin
	install -p -s la_g2svg /usr/bin
	install -p -s la_g2pdf /usr/bin

uninstall:
	rm -f /usr/bin/la_bounds
	rm -f /usr/bin/la_dedup
	rm -f /usr/bin/la_position
	rm -f /usr/bin/la_rotate
	rm -f /usr/bin/la_strip
	rm -f /usr/bin/la_svg2g
	rm -f /usr/bin/la_g2svg
	rm -f /usr/bin/la_g2pdf

