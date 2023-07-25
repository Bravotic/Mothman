#ifndef MOTHMANWINDOW_H
#define MOTHMANWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include "mothmanview.h"
#include <QScrollArea>
#include <QVBoxLayout>
#include <QToolBar>
#include <QPushButton>
#include <QLineEdit>

class MothmanWindow : public QWidget
{
    Q_OBJECT

public:
    MothmanWindow(QWidget *parent = nullptr);

    ~MothmanWindow();
private:
    void goToUrl();

    MothmanView *n;
    QScrollArea *sa;
    QVBoxLayout *vb;
    QToolBar *chrome;
    QPushButton *back;
    QPushButton *forward;
    QPushButton *menu;
    QLineEdit *url;
};
#endif // MOTHMANWINDOW_H
