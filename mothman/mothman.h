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
    HTML_TYPE_BULLETPOINT
};

enum tags {
    HTML_NONE = 0,
    HTML_B,
    HTML_I,
    HTML_HR,
    HTML_H1,
    HTML_H2,
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
    HTML_ADDRESS
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

HTMLElement_t *createHTMLElement(HTMLElement_t *parent);

typedef struct RenderSettings_t {
    color_t backgroundColor;
    color_t textColor;
    color_t linkColor;
    int fontSize;
    const char *defaultFont;
    const char *monoFont;
    double scale;
} RenderSettings_t;

RenderSettings_t *createRenderSettings();

parserState_t* createParserState();

void freeParserState(parserState_t *t);

void addBlankLine(parserState_t *ps);

void addHRLine(parserState_t *ps);

void addNewLine(parserState_t *ps);

void parserHTMLAddChar(parserState_t *ps, char c);

void recursivePrintHtmlElement(HTMLElement_t *root);

surface *addRowToPage(surface *row, surface *page, int *centered);

surface *renderPage(HTMLElement_t *root, RenderSettings_t *settings, int pageWidth);
