# Makefile para compilar el informe IEEE
MAIN = main
BIB = referencias

all: $(MAIN).pdf

$(MAIN).pdf: $(MAIN).tex $(BIB).bib secciones/*.tex
	@which pdflatex > /dev/null 2>&1 || (echo "pdflatex no encontrado en PATH. Puede editar en Neovim con VimTeX o subir a Overleaf." && exit 1)
	pdflatex -interaction=nonstopmode $(MAIN)
	bibtex $(MAIN)
	pdflatex -interaction=nonstopmode $(MAIN)
	pdflatex -interaction=nonstopmode $(MAIN)

clean:
	rm -f *.aux *.bbl *.blg *.log *.out *.toc *.synctex.gz secciones/*.aux

.PHONY: all clean
