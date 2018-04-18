#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "datavis.h"
#include "filedata.h"
#include "memdata.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    Ui::MainWindow *ui;
    DataVis *dv;
    enum Datasource {NONE, FILE, MEM};
    Datasource sourcetype;
    FileData *fd;
    MemData *md;
    void setLabeltext(int);

private slots:
    void onScroll(int);
    void openFile();
    void selectProcess();
};

#endif // MAINWINDOW_H
