# Makefile para compilar el informe IEEE
MAIN = main
BIB = referencias

# Local compiler search: ../bin/tectonic, /tmp/tectonic, or system tectonic
TECTONIC ?= $(shell for b in ../bin/tectonic /tmp/tectonic $$(which tectonic 2>/dev/null); do if [ -x "$$b" ]; then echo "$$b"; break; fi; done)

all: $(MAIN).pdf

$(MAIN).pdf: $(MAIN).tex $(BIB).bib secciones/*.tex
	@if [ -n "$(TECTONIC)" ] && [ -x "$(TECTONIC)" ]; then \
		echo "Compilando con Tectonic ($(TECTONIC))..."; \
		$(TECTONIC) --keep-logs $(MAIN).tex; \
	elif which pdflatex > /dev/null 2>&1; then \
		echo "Compilando con pdflatex..."; \
		pdflatex -interaction=nonstopmode $(MAIN); \
		bibtex $(MAIN); \
		pdflatex -interaction=nonstopmode $(MAIN); \
		pdflatex -interaction=nonstopmode $(MAIN); \
	else \
		echo "Error: Ni tectonic ni pdflatex fueron encontrados."; \
		exit 1; \
	fi

clean:
	rm -f *.aux *.bbl *.blg *.log *.out *.toc *.synctex.gz secciones/*.aux

.PHONY: all clean
