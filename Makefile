all : fb3-2

fb3-2 : fb3-2.l fb3-2.y fb3-2.h
		bison -d fb3-2.y
		flex -o fb3-2.lex.c fb3-2.l
		gcc -o $@ fb3-2.tab.c fb3-2.lex.c fb3-2.funcs.c

clean :
		for file in fb3-2.tab.c fb3-2.tab.h fb3-2.lex.c fb3-2 ; do \
			if [ -e $$file ]; then rm $$file; fi; \
		done

.SILENT : clean