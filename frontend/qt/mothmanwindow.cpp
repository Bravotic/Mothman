#include "mothmanwindow.h"

MothmanWindow::MothmanWindow(QWidget *parent)
    : QWidget(parent)
{
    this->resize(800, 600);
    vb = new QVBoxLayout(this);
    chrome = new QToolBar();

    //QString fileName = QFileDialog::getOpenFileName(this, tr("Open HTML Document"), "", tr("HTML Document (*.htm *.html)"));
    this->setWindowTitle(QString("Mothman"));

    back = new QPushButton("<");
    forward = new QPushButton(">");
    menu = new QPushButton("Go!");

    connect(menu, &QPushButton::released, this, &MothmanWindow::goToUrl);
    url = new QLineEdit();
    url->setFrame(false);
    url->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    url->setText("https://bravotic.com");

    chrome->addWidget(back);
    chrome->addWidget(forward);
    chrome->addWidget(url);
    chrome->addWidget(menu);
    chrome->setMovable(false);

    n = new MothmanView("https://bravotic.com");
    sa = new QScrollArea();

    sa->setWidget(n);
    sa->setWidgetResizable(true);

    vb->addWidget(chrome);
    vb->addWidget(sa);

    vb->setContentsMargins(0, 0, 0, 0);
    vb->setSpacing(0);
    this->setLayout(vb);
}

void MothmanWindow::goToUrl() {
    n->loadURL(url->text().toStdString().c_str());
}

MothmanWindow::~MothmanWindow()
{
    destroy(n);
}

