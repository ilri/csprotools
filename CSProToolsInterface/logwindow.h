#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QDialog>

namespace Ui {
class logWindow;
}

class logWindow : public QDialog
{
    Q_OBJECT

public:
    explicit logWindow(QWidget *parent = 0);
    ~logWindow();

private:
    Ui::logWindow *ui;
};

#endif // LOGWINDOW_H
