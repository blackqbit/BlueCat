all: install.o
	cc -o install -static install.o

install.o: install.c
	cc -c install.c -DTARGET_CPU=$(BLUECAT_TARGET) \
			-DVERSION=$(BLUECAT_VERSION)

clean:
	-rm -f install install.exe install.o

