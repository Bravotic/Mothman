#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <cdraw.h>
#include <stdio.h>

#include "tagparser.h"

typedef struct renderState_t {
    surface* page;
    surface* row;
    int pageWidth;
    int currentRowWidth;

    const char* font;
    color_t textColor;
    color_t linkColor;
    color_t backgroundColor;

    int scaleFactor;


    unsigned char lastRowBlank;
    unsigned char currentRowNotSpace;

} renderState_t;

typedef struct parserState_t {
    char *buffer;
    int bufSize;
    int bufPtr;
    unsigned char inTag;

    unsigned char bold;
    unsigned char italics;
    unsigned char huge;
    unsigned char large;
    unsigned char small;
    unsigned char tiny;
    unsigned char hidden;
    unsigned char center;
    unsigned char link;

    char *href;

    char *skipUntilFound;
    int skipPtr;

    unsigned int currentTag;

    struct HTMLElement_t *root;
    struct HTMLElement_t *current;
} parserState_t;

enum type {
    HTML_TYPE_NONE,
    HTML_TYPE_TEXT,
    HTML_TYPE_NEWLINE,
    HTML_TYPE_BLANKLINE,
    HTML_TYPE_HR,
    HTML_TYPE_IMAGE,
    HTML_TYPE_BULLETPOINT,
    HTML_TYPE_TAB
};

enum tags {
    HTML_NONE = 0,
    HTML_B,
    HTML_I,
    HTML_HR,
    HTML_H1,
    HTML_H2,
    HTML_H3,
    HTML_H4,
    HTML_H5,
    HTML_H6,
    HTML_TITLE,
    HTML_STYLE,
    HTML_SCRIPT,
    HTML_P,
    HTML_ASIDE,
    HTML_DIV,
    HTML_BR,
    HTML_CENTER,
    HTML_UL,
    HTML_LI,
    HTML_A,
    HTML_DT,
    HTML_DD,
    HTML_ADDRESS,
    HTML_DL,
    HTML_TR,
    HTML_COMMENT
};

#define HTML_FLAG_NONE 0
#define HTML_FLAG_BOLD 1
#define HTML_FLAG_ITALICS 2
#define HTML_FLAG_HUGE 4
#define HTML_FLAG_LARGE 8
#define HTML_FLAG_SMALL 16
#define HTML_FLAG_TINY 32
#define HTML_FLAG_CENTER 64
#define HTML_FLAG_LINK 128

#define flagSet(x, flag) x |= flag
#define flagUnset(x, flag) x &= ~flag
#define flagGet(x, flag) x & flag


typedef struct HTMLElement_t {
    unsigned char type;
    char *text;
    char *href;
    char *alt;

    unsigned short flags;

    struct HTMLElement_t *next;
    struct HTMLElement_t *parent;
} HTMLElement_t;

HTMLElement_t *createHTMLElement(HTMLElement_t *parent) {
    HTMLElement_t *e;
    e = (HTMLElement_t*)malloc(sizeof(HTMLElement_t));
    e->type = HTML_TYPE_NONE;
    e->text = NULL;
    e->href = NULL;
    e->alt = NULL;

    e->flags = HTML_FLAG_NONE;

    e->next = NULL;
    e->parent = parent;

    if (parent != NULL) {
        parent->next = e;
    }

    return e;
}

typedef struct RenderSettings_t {
    color_t backgroundColor;
    color_t textColor;
    color_t linkColor;
    int fontSize;
    const char *defaultFont;
    const char *monoFont;
    double scale;
} RenderSettings_t;

RenderSettings_t *createRenderSettings() {
    RenderSettings_t *rs;
    rs = malloc(sizeof(RenderSettings_t));
    return rs;
}

/* TODO: Split most of the settings off into their now RenderSettings class for ease of use */

/* Creates our render state, basically everything we need to render the page.
 *
 * Arguments:
 *  pw - page width in pixles. Used to fit the content to the page. No guarantees that the content will take up this width exactly however
 *  font - name of the font which is to be used for text on the page.
 *  col - color of the text on the page.
 *  linkColor - color of the links on the page
 *  backgroundColor - backgroundColor of the page.
 *  scaleFactor - scale factor of the page, used for High DPI displays.
 */
renderState_t* createRenderState(int pageWidth, const char* font, color_t col, color_t linkColor, color_t backgroundColor, int scaleFactor) {
    renderState_t *ret;
    ret = malloc(sizeof(renderState_t));

    ret->row = DL_empty(0, 0);
    ret->currentRowWidth = 0;

    ret->font = font;
    ret->textColor = col;
    ret->linkColor = linkColor;
    ret->backgroundColor = backgroundColor;
    ret->scaleFactor = scaleFactor;

    ret->currentRowNotSpace = 0;

    ret->lastRowBlank = 0;

    ret->pageWidth = pageWidth * ret->scaleFactor;
    ret->page = DL_empty(ret->pageWidth, 0);
    return ret;
}

void freeRenderState(renderState_t *rs) {
    DL_free_surface(rs->page);
    DL_free_surface(rs->row);
    free(rs);
}

parserState_t* createParserState() {
    parserState_t *ret;
    ret = malloc(sizeof(parserState_t));
    ret->bufSize = 64;
    ret->bufPtr = 0;
    ret->buffer = malloc(ret->bufSize * sizeof(char));
    ret->buffer[ret->bufPtr] = 0;
    ret->inTag = 0;

    ret->bold = 0;
    ret->italics = 0;
    ret->huge = 0;
    ret->large = 0;
    ret->small = 0;
    ret->tiny = 0;
    ret->hidden = 0;
    ret->center = 0;
    ret->link = 0;

    ret->skipPtr = 0;
    ret->skipUntilFound = NULL;

    ret->root = createHTMLElement(NULL);
    ret->current = ret->root;

    return ret;
}

void freeParserState(parserState_t *t) {
    free(t->buffer);
    free(t);
}

void addNewLine(parserState_t *ps) {
    HTMLElement_t *e;
    e = createHTMLElement(ps->current);
    e->type = HTML_TYPE_NEWLINE;
    ps->current = e;
}

void addBlankLine(parserState_t *ps) {
    if(ps->current->type == HTML_TYPE_NEWLINE) {
        if(ps->current->parent->type != HTML_TYPE_NEWLINE) {
            addNewLine(ps);
        }
    }
    if(ps->current->type != HTML_TYPE_BLANKLINE) {
        HTMLElement_t *e;
        e = createHTMLElement(ps->current);
        e->type = HTML_TYPE_BLANKLINE;
        ps->current = e;
    }
}

void addHRLine(parserState_t *ps) {
    HTMLElement_t *e;
    e = createHTMLElement(ps->current);
    e->type = HTML_TYPE_HR;
    ps->current = e;
}

void addBulletPoint(parserState_t *ps) {
    HTMLElement_t *e;
    e = createHTMLElement(ps->current);
    e->type = HTML_TYPE_BULLETPOINT;
    ps->current = e;
}

void addTab(parserState_t *ps) {
    HTMLElement_t *e;
    e = createHTMLElement(ps->current);
    e->type = HTML_TYPE_TAB;
    ps->current = e;
}

void parserHTMLAddChar(parserState_t *ps, char c) {

    /* Nasty hack to ignore things in comments */
    if (ps->inTag && strncmp(ps->buffer, "!--", 4) == 0) {
        printf("Found comment\n");
        ps->skipPtr = 0;
        ps->skipUntilFound = "-->";
        ps->bufPtr = 0;
        ps->buffer[0] = 0;
    }

    /* We are skipping over text until we find something to tell us to stop */
    if (ps->skipUntilFound != NULL) {
        if (tolower(c) == ps->skipUntilFound[ps->skipPtr]) {
            ps->skipPtr++;
            if (ps->skipUntilFound[ps->skipPtr] == 0) {
                ps->skipUntilFound = NULL;
                ps->skipPtr = 0;
            }
        }
        else {
            ps->skipPtr = 0;
        }
    }

    /* We are in a tag */
    else if (ps->inTag) {
        if (c == '>') {
            int i;
            char closing;
            char *tagname;
            tagname = malloc((ps->bufPtr + 1) * sizeof(char));

            ps->buffer[ps->bufPtr] = 0;

            for(i = 0; ps->buffer[i] != 0 && ps->buffer[i] > ' '; i++) {
                tagname[i] = tolower(ps->buffer[i]);
            }

            tagname[i] = 0;

            if(tagname[0] == '/') {
                unsigned int parserRes = parseTagname(&tagname[1]);

                switch(parserRes) {
                    case HTML_B:
                        ps->bold -= 1;
                    break;

                    case HTML_ADDRESS:
                    case HTML_I:
                        ps->italics -= 1;
                    break;

                    case HTML_H1:
                        ps->bold -= 1;
                        ps->huge -= 1;
                        addBlankLine(ps);

                    break;

                    case HTML_H2:
                        ps->bold -= 1;
                        ps->large -= 1;
                        addBlankLine(ps);

                    break;

                    case HTML_H3:
                        ps->bold -= 1;
                        addBlankLine(ps);

                    break;

                    case HTML_H4:
                        ps->bold -= 1;
                        ps->small -= 1;
                        addBlankLine(ps);
                    break;

                    case HTML_H5:
                        ps->bold -= 1;
                        ps->tiny -= 1;
                        addBlankLine(ps);
                    break;

                    case HTML_H6:
                        ps->bold -= 1;
                        ps->tiny -= 1;
                        addBlankLine(ps);
                    break;

                    case HTML_BR:
                        addNewLine(ps);
                    break;

                    case HTML_CENTER:
                        addBlankLine(ps);
                        ps->center -= 1;
                    break;

                    case HTML_TITLE:
                        ps->hidden -= 1;
                    break;

                    case HTML_LI:
                        addNewLine(ps);
                    break;

                    case HTML_A:
                        ps->link -= 1;
                    break;

                    case HTML_TR:
                        addBlankLine(ps);
                    break;

                    default:
                    break;
                }
                ps->bufPtr = 0;
            }
            else {
                unsigned int parserRes = parseTagname(tagname);
                switch(parserRes) {
                    case HTML_B:
                        ps->bold += 1;
                    break;

                    case HTML_ADDRESS:
                    case HTML_I:
                        ps->italics += 1;
                    break;

                    case HTML_HR:
                        addBlankLine(ps);
                        addHRLine(ps);
                        addBlankLine(ps);
                    break;

                    case HTML_H1:
                        addBlankLine(ps);
                        ps->bold += 1;
                        ps->huge += 1;
                    break;

                    case HTML_H2:
                        addBlankLine(ps);
                        ps->bold += 1;
                        ps->large += 1;
                    break;

                    case HTML_H3:
                        addBlankLine(ps);
                        ps->bold += 1;
                    break;

                    case HTML_H4:
                        addBlankLine(ps);
                        ps->bold += 1;
                        ps->small += 1;
                    break;

                    case HTML_H5:
                        addBlankLine(ps);
                        ps->bold += 1;
                        ps->tiny += 1;
                    break;

                    case HTML_H6:
                        addBlankLine(ps);
                        ps->bold += 1;
                        ps->tiny += 1;
                    break;

                    case HTML_P:
                        addBlankLine(ps);
                    break;

                    case HTML_BR:
                        addNewLine(ps);
                    break;

                    case HTML_CENTER:
                        addBlankLine(ps);
                        ps->center += 1;
                    break;

                    case HTML_TITLE:
                        ps->hidden += 1;
                    break;

                    case HTML_A:
                        ps->link += 1;
                    break;

                    case HTML_DT:
                        addNewLine(ps);
                    break;

                    case HTML_DD:
                        addNewLine(ps);
                        addTab(ps);
                    break;

                    case HTML_ASIDE:
                    case HTML_UL:
                    case HTML_DL:
                        addBlankLine(ps);
                    break;

                    case HTML_LI:
                        addNewLine(ps);
                        addTab(ps);
                        addBulletPoint(ps);
                    break;

                    case HTML_STYLE:
                        ps->skipPtr = 0;
                        ps->skipUntilFound = "</style>";
                    break;

                    case HTML_SCRIPT:
                        ps->skipPtr = 0;
                        ps->skipUntilFound = "</script>";
                    break;


                default:
                break;

                }
            }

            ps->bufSize = 64;
            ps->bufPtr = 0;
            ps->buffer = malloc(64 * sizeof(char));
            ps->buffer[ps->bufPtr] = 0;
            ps->inTag -= 1;

        }
        else {
            ps->buffer[ps->bufPtr++] = (char) c;

            if(ps->bufPtr >= ps->bufSize) {
                ps->bufSize += 64;
                ps->buffer = realloc(ps->buffer, ps->bufSize * sizeof(char));
            }
        }
    }
    else {
        /* We have encountered the start of a tag, so add our buffer as an element */
        if (c == '<' || c == 0) {

            /* If our buffer is zero long, we don't really want to add it */
            if (ps->bufPtr > 0) {
                HTMLElement_t *e;

                e = createHTMLElement(ps->current);
                ps->current->next = e;

                e->type = HTML_TYPE_TEXT;

                int x = 0;
                flagSet(x, HTML_FLAG_HUGE);

                /* Set the flags */
                if (ps->bold) {
                    flagSet(e->flags, HTML_FLAG_BOLD);
                }

                if (ps->italics) {
                    flagSet(e->flags, HTML_FLAG_ITALICS);
                }

                if (ps->huge) {
                    flagSet(e->flags, HTML_FLAG_HUGE);
                }

                if(ps->large) {
                    flagSet(e->flags, HTML_FLAG_LARGE);
                }

                if (ps->small) {
                    flagSet(e->flags, HTML_FLAG_SMALL);
                }

                if (ps->tiny) {
                    flagSet(e->flags, HTML_FLAG_TINY);
                }

                if(ps->center) {
                    flagSet(e->flags, HTML_FLAG_CENTER);
                }

                if(ps->link) {
                    flagSet(e->flags, HTML_FLAG_LINK);
                    /* If we are in a link, we share the same href as the a tag we previously parsed */
                    e->href = ps->href;
                }

                ps->buffer[ps->bufPtr] = 0;

                e->text = ps->buffer;

                ps->current = e;

                ps->bufSize = 64;
                ps->bufPtr = 0;
                ps->buffer = malloc(64 * sizeof(char));
                ps->buffer[ps->bufPtr] = 0;
            }
            ps->inTag += 1;
        }
        else {
            /* HTML wants only one space between words, and also treats newlines as spaces... and ALSO wants lines trimmed, so deal with that here */
            if (c <= ' ' && ps->hidden == 0) {
                /* Our last character was a space, so don't add a new one */
                if (ps->bufPtr > 0 && ps->buffer[ps->bufPtr - 1] == ' ') {

                }
                /* Our last thing added to our page was a new line, and there is nothing in our buffer, aka we have a completely blank line */
                else if((ps->current->type == HTML_TYPE_NEWLINE || ps->current->type == HTML_TYPE_BLANKLINE || ps->current == ps->root) && ps->bufPtr == 0) {

                }
                /* If it is none of those, we can just add it to our buffer */
                else {
                    ps->buffer[ps->bufPtr++] = ' ';

                    if(ps->bufPtr >= ps->bufSize) {
                        ps->bufSize += 64;
                        ps->buffer = realloc(ps->buffer, ps->bufSize * sizeof(char));
                    }
                }
            }
            else if (ps->hidden == 0) {
                ps->buffer[ps->bufPtr++] = (char) c;

                if(ps->bufPtr >= ps->bufSize) {
                    ps->bufSize += 64;
                    ps->buffer = realloc(ps->buffer, ps->bufSize * sizeof(char));
                }
            }
        }
    }
}

#include <stdio.h>

void recursivePrintHtmlElement(HTMLElement_t *root) {
    if (root->type == HTML_TYPE_TEXT) {
        printf("[%s]", root->text);
    }
    else if(root->type == HTML_TYPE_NEWLINE) {
        printf("[NL]\n");
    }
    else if(root->type == HTML_TYPE_BLANKLINE) {
        printf("\n[BL]\n");
    }
    else if(root->type == HTML_TYPE_HR) {
        printf("===");
    }
    else {
        printf("");
    }

    if(root->next != NULL) {
        recursivePrintHtmlElement(root->next);
    }
}

surface *addRowToPage(surface *row, surface *page, int *centered) {
    if((*centered) > 0) {
        (*centered) = 0;
        return DL_above(page, row);
    }
    else {
        return DL_above_align(page, row, LEFT);
    }
}
#include <math.h>

surface *renderPage(HTMLElement_t *root, RenderSettings_t *settings, int pw) {
    surface *row, *page, *newPage, *background;
    char *start;
    HTMLElement_t *current;
    int rowWidth, centered, pageWidth;

    pageWidth = pw * settings->scale;

    current = root;
    row = DL_empty(0, 0);
    rowWidth = 0;
    page = DL_empty(pageWidth, 1);

    centered = 0;

    /* I would have done this recursively, but it might produce a stack overflow on some pages :( */
    do {

        if(flagGet(current->flags, HTML_FLAG_CENTER)) {
            centered = 1;
        }

        if (current->type == HTML_TYPE_TEXT) {
            surface *word;
            char *wordBuffer;
            int size, i, running, wordBufferPtr, wordBufferCap;
            color_t color;

            start = current->text;

            wordBufferPtr = 0;
            wordBufferCap = 64;
            wordBuffer = malloc(wordBufferCap * sizeof(char));

            /* This is pretty ugly and weird to follow, but it should be pretty efficient */
            /* Might end up making this more straight forward in the future */
            for(i = 0, running = 1; running; i++) {
                switch (current->text[i]) {
                    case 0:
                        running = 0;
                        /* fall through */
                    case ' ':
                    {
                        int wordWidth;

                        /* This should stop all Qt Painter errors */
                        if(wordBufferPtr == 0 && !running) {

                            break;
                        }


                        /* Edge case where if we are really unlucky, and
                         * wordBufferPtr is right on the edge, we could
                         * accidentally write to memory we should be able to,
                         * Fix me. */
                        wordBuffer[wordBufferPtr++] = current->text[i];
                        wordBuffer[wordBufferPtr] = 0;

                        if (flagGet(current->flags, HTML_FLAG_HUGE)) {
                           size = (int)round((double)settings->fontSize * settings->scale * 2);
                        }
                        else if(flagGet(current->flags, HTML_FLAG_LARGE)) {
                            size = (int)round((double)settings->fontSize * 1.5 * settings->scale);
                        }
                        else if(flagGet(current->flags, HTML_FLAG_SMALL)) {
                            size = (int)round((double)settings->fontSize * 0.75 * settings->scale);
                        }
                        else if(flagGet(current->flags, HTML_FLAG_TINY)) {
                            size = (int)round((double)settings->fontSize * 0.5 * settings->scale);
                        }
                        else {
                            size = (int)round((double)settings->fontSize * settings->scale);
                        }

                        if (flagGet(current->flags, HTML_FLAG_LINK)) {
                            color = settings->linkColor;
                        }
                        else {
                            color = settings->textColor;
                        }

                        word = DL_text(wordBuffer, size, color, settings->defaultFont, flagGet(current->flags, HTML_FLAG_BOLD), flagGet(current->flags, HTML_FLAG_ITALICS));
                        wordWidth = DL_get_width(word);

                        if (rowWidth + wordWidth >= pageWidth) {
                            surface *newPage;

                            newPage = addRowToPage(row, page, &centered);
                            DL_free_surface(row);
                            DL_free_surface(page);
                            page = newPage;
                            row = word;
                            rowWidth = wordWidth;
                        }
                        else {
                            surface *newRow;

                            newRow = DL_beside_align(row, word, BOTTOM);
                            DL_free_surface(row);
                            DL_free_surface(word);
                            row = newRow;
                            rowWidth += wordWidth;
                        }

                        wordBufferPtr = 0;
                    }
                    break;
                    default:
                        wordBuffer[wordBufferPtr++] = current->text[i];
                        if (wordBufferPtr >= wordBufferCap) {
                            wordBufferCap += 64;
                            wordBuffer = realloc(wordBuffer, wordBufferCap);
                        }
                    break;
                }
            }

        }
        else if(current->type == HTML_TYPE_NEWLINE) {
            surface *newPage;

            newPage = addRowToPage(row, page, &centered);
            DL_free_surface(row);
            DL_free_surface(page);
            page = newPage;
            row = DL_empty(0, 0);
            rowWidth = 0;
        }
        else if(current->type == HTML_TYPE_BLANKLINE) {


            newPage = addRowToPage(row, page, &centered);
            DL_free_surface(row);
            DL_free_surface(page);
            page = newPage;

            row = DL_text(" ", 16, settings->textColor, settings->defaultFont, 0, 0);
            newPage = addRowToPage(row, page, &centered);

            DL_free_surface(row);
            DL_free_surface(page);

            page = newPage;
            row = DL_empty(0, 0);
            rowWidth = 0;
        }
        else if(current->type == HTML_TYPE_HR) {
            surface *newPage;

            row = DL_rectangle(pageWidth, 1, settings->textColor);
            rowWidth = pageWidth;
        }
        else if(current->type == HTML_TYPE_TAB) {
            row = DL_text("        ", 16, settings->textColor, settings->defaultFont, 0, 0);
            rowWidth = DL_get_width(row);
        }
        else if(current->type == HTML_TYPE_BULLETPOINT) {
            surface *newPage, *newrow, *point, *spacer;

            spacer = DL_text(" ", 16, settings->textColor, settings->defaultFont, 0, 0);

            /* Add point */
            point = DL_square(5, settings->textColor);
            newrow = DL_beside(row, point);
            DL_free_surface(row);
            DL_free_surface(point);

            /* Add right spacer */
            row = DL_beside(newrow, spacer);
            DL_free_surface(newrow);
            DL_free_surface(spacer);

            rowWidth = DL_get_width(row);
        }
        else {

        }

        current = current->next;
    } while(current != NULL);

    newPage = addRowToPage(row, page, &centered);
    DL_free_surface(row);
    DL_free_surface(page);
    background = DL_rectangle(DL_get_width(newPage), DL_get_height(newPage), settings->backgroundColor);
    page = DL_overlay(background, newPage);
    DL_free_surface(newPage);
    DL_free_surface(background);
    return page;
}
/*
#include <cairo/cairo.h>

int main (int argc, char *argv[]) {
    color_t b = {0, 0, 0};
    FILE* book = fopen(argv[1], "r");
    char* buffer;
    int bufptr = 0;
    int bufSize = 64;
    int c = 0;

    renderState_t *rs;
    parserState_t *ps;

    int currentRowWidth = 0;

    buffer = malloc(bufSize * sizeof(char));

    ps = createParserState();

    raise(SIGINT);

    while((c = fgetc(book)) != EOF) {
        parserHTMLAddChar(ps, c);
    }

    printf("Recursive print\n");
    recursivePrintHtmlElement(ps->root);
    printf("Render page");
    RenderSettings_t *set;
    set = createRenderSettings();

    set->backgroundColor = (color_t){255, 255, 255};
    set->textColor = (color_t){0, 0, 0};
    set->linkColor = (color_t){0, 0, 255};
    set->defaultFont = "Times New Roman";
    set->fontSize = 16;
    set->scale = 1;

    surface *test = renderPage(ps->root, set, 640);

    cairo_surface_write_to_png (test, "page.png");

    buffer[bufptr] = 0;
}*/
