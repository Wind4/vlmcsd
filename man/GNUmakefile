################################################################################

.PHONY: clean


PDFDOCS = vlmcs.1.pdf vlmcsd.7.pdf vlmcsd.8.pdf vlmcsdmulti.1.pdf vlmcsd.ini.5.pdf vlmcsd-floppy.7.pdf
HTMLDOCS = $(PDFDOCS:.pdf=.html)
UNIXDOCS = $(PDFDOCS:.pdf=.unix.txt)
DOSDOCS = $(PDFDOCS:.pdf=.dos.txt)

%.pdf : %
  ifeq ($(shell uname), Darwin)
	groff -Tps -mandoc -c $< | pstopdf -i -o $@
  else
	groff -Tpdf -mandoc -c $< > $@
  endif

%.html : %
	groff -Thtml -mandoc -c $< > $@ 

%.unix.txt : %
	groff -P -c -Tascii -mandoc -c $< | col -bx > $@

%.dos.txt : %.unix.txt
#	unix2dos -n $< $@
#	sed -e 's/$$/\r/' $< > $@
	awk 'sub("$$", "\r")' $< > $@

alldocs : $(UNIXDOCS) $(HTMLDOCS) $(PDFDOCS) $(DOSDOCS)

pdfdocs : $(PDFDOCS)

dosdocs : $(DOSDOCS)

unixdocs : $(UNIXDOCS)

htmldocs : $(HTMLDOCS)

clean:
	rm -f $(PDFDOCS) $(DOSDOCS) $(UNIXDOCS) $(HTMLDOCS)

help:
	@echo "Help is available by typing 'make help' in directory $(shell realpath `pwd`/..). Use 'cd ..' to get there."

