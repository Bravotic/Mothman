#ifndef MOTHMANVIEW_H
#define MOTHMANVIEW_H

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <stdio.h>
#include <QPaintEvent>

extern "C"{
#if defined(CDRAW_PORT_CAIRO)
#include <cairo/cairo.h>
#endif
#include <mothman/mothman.h>
}

class MothmanView : public QWidget
{
public:
    MothmanView(const char *htmlPath, QWidget *parent = nullptr);

    QSize sizeHint() const;
    QSize size() const {
        return QSize(contentW, contentH);
    }

    QSize minimumSize() {
        return QSize(contentW, contentH);
    }

    void setPath(const char* path) {
        this->path = path;
    }

public slots:
    void screenChanged();

private:
    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);
    const char *path;
    int contentH;
    int contentW;
    int widH;
    int widW;
    int dpi;
    surface *content;
    bool needsToRerender;
    QImage *canvas;
    parserState_t *ps;
};

#endif // MOTHMANVIEW_H
