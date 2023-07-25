#include "mothmanview.h"
#include "QtWidgets/qapplication.h"

#include <QFontDialog>
#include <iostream>
#include <QWindow>
#include <QFont>
#include <QScreen>

#include <curl/curl.h>

/* This is temporary and will be removed eventually */
static size_t curl_callback(void *data, size_t size, size_t nmemb, void *buffer) {

}

MothmanView::MothmanView(const char *url, QWidget *parent) : QWidget(parent)
{
    int c;

    path = strdup(url);
    this->content = DL_empty(0, 0);

    this->needsToRerender = false;

    // Load the page
    this->ps = createParserState();

    openURL(url, ps);

    this->needsToRerender = true;

}

void MothmanView::loadURL(const char* url) {
    freeParserState(this->ps);
    this->ps = createParserState();
    openURL(url, ps);
    this->needsToRerender = true;
    this->repaint();
}

QSize MothmanView::sizeHint() const {
    return QSize(contentW, contentH);
}

void MothmanView::screenChanged(){
    QScreen *screen = this->screen();
    this->dpi = screen->devicePixelRatio();
    this->needsToRerender = true;
    this->repaint();
}

extern "C" void MothmanView::paintEvent(QPaintEvent* qp) {
    int c;

    if(this->needsToRerender) {
        // Get our screen and our DPI
        QScreen *screen = this->screen();
        this->dpi = screen->devicePixelRatio();

        RenderSettings_t *rs;
        color_t text, linkcol, bgcol;

        // Get the default application colors for our renderer
        text.r = QApplication::palette().text().color().red();
        text.g = QApplication::palette().text().color().green();
        text.b = QApplication::palette().text().color().blue();

        linkcol.r = QApplication::palette().link().color().red();
        linkcol.g = QApplication::palette().link().color().green();
        linkcol.b = QApplication::palette().link().color().blue();

        bgcol.r = QApplication::palette().window().color().red();
        bgcol.g = QApplication::palette().window().color().green();
        bgcol.b = QApplication::palette().window().color().blue();

        // Create the render settings and parser state
        rs = createRenderSettings();
        rs->backgroundColor = bgcol;
        rs->linkColor = linkcol;
        rs->textColor = text;
        rs->scale = this->dpi;
        rs->defaultFont = "Times New Roman";
        rs->fontSize = 16;

        // Finish rendering our content
        this->content = renderPage(ps->root, rs, this->width());

        // Get the width and height for it
        contentH = DL_get_height(this->content);
        contentW = DL_get_width(this->content);

        // If our content height and weight arent the same as our widget height/width, we set it
        if((this->contentH / this->dpi) != this->height()) {
            this->setMinimumHeight(this->contentH / this->dpi);
        }

        if((this->contentW / this->dpi) != this->width()) {
            this->setMinimumWidth(this->contentW / this->dpi);
        }

        // We dont need to rerender
        this->needsToRerender = false;

        free(rs);
    }
#if defined(CDRAW_PORT_CAIRO)
    // If we are using cairo, we need to create a QImage we can draw to it
    this->canvas = new QImage((const unsigned char*)cairo_image_surface_get_data(this->content), this->contentW, this->contentH, QImage::Format_ARGB32);
    canvas->setDevicePixelRatio(this->dpi);
    QPainter p(this);
    p.drawImage(0, 0, *this->canvas);
#else
    // If we use Qt we just cast it to a pixmap and draw it
    ((QPixmap*)this->content)->setDevicePixelRatio(this->dpi);
    QPainter p(this);
    p.drawPixmap(0, 0, *(QPixmap*)this->content);
#endif


}


void MothmanView::resizeEvent(QResizeEvent* re) {

    // If our width changed, we need to rerender
    if(this->widW != re->size().width()){
        this->needsToRerender = true;
    }
    this->widW = re->size().width();
}
