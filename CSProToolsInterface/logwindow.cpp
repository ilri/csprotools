#include "logwindow.h"
#include "ui_logwindow.h"

logWindow::logWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::logWindow)
{
    ui->setupUi(this);
}

logWindow::~logWindow()
{
    delete ui;
}
