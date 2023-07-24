#include <cdraw.h>

/* DO NOT USE */
int main (int argc, char *argv[]) {
    color_t b = {0, 0, 0};
    surface *page, *row;
    FILE* book = fopen(argv[1], "r");
    char* buffer;
    int bufptr = 0;
    int bufSize = 64;
    int c = 0;

    renderState_t *rs;
    parserState_t *ps;

    int currentRowWidth = 0;

    buffer = malloc(bufSize * sizeof(char));

    row = DL_empty(0, 0);
    page = DL_empty(0, 0);

    rs = createRenderState(640, "Times New Roman", b, b, (color_t){255, 255, 255}, 1);
    ps = createParserState();

    while((c = fgetc(book)) != EOF) {
        parserHTMLAddChar(ps, rs, c);
    }
    buffer[bufptr] = 0;
    addWordToRow(rs, buffer);

    cairo_surface_write_to_png (finishRender(rs), "page.png");
}

