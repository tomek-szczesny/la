default:
	gcc -lm la_bounds.c -o la_bounds
	gcc -lm la_dedup.c -o la_dedup
	gcc -lm la_dup.c -o la_dup
	gcc -lm la_pos.c -o la_pos
	gcc -lm la_rot.c -o la_rot
	gcc -lm la_strip.c -o la_strip
	gcc -lm la_svg2g.c -o la_svg2g -lxml2 -I/usr/include/libxml2
	gcc -lm la_g2svg.c -o la_g2svg -lcairo -I/usr/include/cairo
	gcc -lm la_g2pdf.c -o la_g2pdf -lcairo -I/usr/include/cairo
	gcc -lm -O3 la_opt.c -o la_opt

install:
	install -p -s la_bounds /usr/bin
	install -p -s la_dedup /usr/bin
	install -p -s la_dup /usr/bin
	install -p -s la_pos /usr/bin
	install -p -s la_rot /usr/bin
	install -p -s la_strip /usr/bin
	install -p -s la_svg2g /usr/bin
	install -p -s la_g2svg /usr/bin
	install -p -s la_g2pdf /usr/bin
	install -p -s la_opt /usr/bin

uninstall:
	rm -f /usr/bin/la_bounds
	rm -f /usr/bin/la_dedup
	rm -f /usr/bin/la_dup
	rm -f /usr/bin/la_position
	rm -f /usr/bin/la_pos
	rm -f /usr/bin/la_rotate
	rm -f /usr/bin/la_rot
	rm -f /usr/bin/la_strip
	rm -f /usr/bin/la_svg2g
	rm -f /usr/bin/la_g2svg
	rm -f /usr/bin/la_g2pdf
	rm -f /usr/bin/la_optimize
	rm -f /usr/bin/la_opt

