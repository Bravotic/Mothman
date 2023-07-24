#include "mothmanwindow.h"

MothmanWindow::MothmanWindow(QWidget *parent)
    : QWidget(parent)
{
    this->resize(800, 600);
    vb = new QVBoxLayout(this);
    chrome = new QToolBar();

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open HTML Document"), "", tr("HTML Document (*.htm *.html)"));
    this->setWindowTitle(QString("Mothman | " + fileName));

    back = new QPushButton("<");
    forward = new QPushButton(">");
    menu = new QPushButton("Menu");
    url = new QLineEdit();
    url->setFrame(false);
    url->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    url->setText("file://" + fileName.toHtmlEscaped());

    chrome->addWidget(back);
    chrome->addWidget(forward);
    chrome->addWidget(url);
    chrome->addWidget(menu);
    chrome->setMovable(false);

    n = new MothmanView(fileName.toStdString().c_str());
    sa = new QScrollArea();

    sa->setWidget(n);
    sa->setWidgetResizable(true);

    vb->addWidget(chrome);
    vb->addWidget(sa);

    vb->setContentsMargins(0, 0, 0, 0);
    vb->setSpacing(0);
    this->setLayout(vb);
}

MothmanWindow::~MothmanWindow()
{
    destroy(n);
}

