#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    sourcetype(NONE),
    fd(NULL),
    md(NULL)
{
    ui->setupUi(this);

    ui->mainToolBar->addAction(this->style()->standardIcon(QStyle::SP_DirOpenIcon), "Open File", this, SLOT(openFile()));
    ui->mainToolBar->addAction(this->style()->standardIcon(QStyle::SP_ComputerIcon), "Select Process", this, SLOT(selectProcess()));
    ui->mainToolBar->addSeparator();
    ui->mainToolBar->addAction(this->style()->standardIcon(QStyle::SP_BrowserReload), "Refresh", this, SLOT(reload()));

    QFont f("Consolas", 10);
    ui->addrLabel->setFont(f);

    dv = new DataVis(ui->containerWidget);
    dv->setMouseTracking(true);
    dv->installEventFilter(this);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(dv);
    layout->setContentsMargins(0,0,0,0);
    ui->containerWidget->setLayout(layout);

    ui->horizontalSlider->setMinimum(1);
    ui->horizontalSlider->setMaximum(dv->getMaxWidth()/8);
    ui->horizontalSlider->setSliderPosition(dv->getWidth()/8);

    connect(ui->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(onSlide(int)));
    connect(dv->getScrollbar(), SIGNAL(valueChanged(int)), this, SLOT(onScroll(int)));
}

MainWindow::~MainWindow()
{
    delete ui;
    delete fd;
    delete md;
}

void MainWindow::compare(bool diff)
{
    if (sourcetype == MEM) {
        QString pid = proc.mid(proc.lastIndexOf('(') + 1, proc.lastIndexOf(')') - proc.lastIndexOf('(') - 1);
        MemData *newmd = new MemData(pid.toULong());
        if (newmd->getBuffer() != NULL) {
            if (diff)
                md->cmpBytes(newmd, true);
            else
                md->cmpBytes(newmd, false);
        }
        if (md->getSize() > 0) {
            md->padBuffer(dv->calcPadding(md->getSize()));
            dv->loadData((char*) md->getBuffer(), md->getSize());
            QWidget::setWindowTitle("Memorize - [" + proc + "]");
            setLabeltext(dv->getOffset());
        }
        else {
            //sourcetype = NONE;
            //QWidget::setWindowTitle("Memorize");
            setLabeltext(0);
            dv->clearData();
        }
        delete newmd;
    }
}

void MainWindow::diff()
{
    compare(true);
}

void MainWindow::equal()
{
    compare(false);
}

void MainWindow::rmProcToolbarItems()
{
    QList<QAction*> act = ui->mainToolBar->actions();
    for (QList<QAction*>::iterator i = act.begin(); i < act.end(); i++) {
        if (((*i)->text().compare("Discard Unchanged Bytes") == 0) || ((*i)->text().compare("Keep Unchanged Bytes") == 0)) {
            ui->mainToolBar->removeAction(*i);
        }
    }
}

void MainWindow::loadFile()
{
    sourcetype = NONE;
    delete md; md = NULL;
    delete fd; fd = new FileData(file.toStdString());
    if (fd->getBuffer() != NULL) {
        fd->padBuffer(dv->calcPadding(fd->getSize()));
        dv->loadData(fd->getBuffer(), fd->getSize());
        sourcetype = FILE;
        QString fn = QString::fromStdString(fd->getFilename());
        QWidget::setWindowTitle("Memorize - [" + fn.right(fn.size() - fn.lastIndexOf('/') - 1) + "]");
        setLabeltext(dv->getOffset());
        rmProcToolbarItems();
    }
    else if (fd->getSize() > fd->getMaxsize()){
        QMessageBox::warning(this, "Memorize", "Chosen file is too large. Maximum size: " + QString::number(fd->getMaxsize()/1000000) + " MB");
    }
    else {
        QWidget::setWindowTitle("Memorize");
        setLabeltext(0);
        dv->clearData();
    }
}

void MainWindow::openFile()
{
    QString tmp = QFileDialog::getOpenFileName(this, "Open File");
    if (!tmp.isNull()) {
        file = tmp;
        loadFile();
    }
}

void MainWindow::loadProcess()
{
    sourcetype = NONE;
    QString pid = proc.mid(proc.lastIndexOf('(') + 1, proc.lastIndexOf(')') - proc.lastIndexOf('(') - 1);
    delete fd; fd = NULL;
    delete md; md = new MemData(pid.toULong());
    if (md->getBuffer() != NULL) {
        md->padBuffer(dv->calcPadding(md->getSize()));
        dv->loadData((char*) md->getBuffer(), md->getSize());
        sourcetype = MEM;
        QWidget::setWindowTitle("Memorize - [" + proc + "]");
        setLabeltext(dv->getOffset());
        QList<QAction*> act = ui->mainToolBar->actions();
        rmProcToolbarItems();
        ui->mainToolBar->addAction(this->style()->standardIcon(QStyle::SP_DialogOkButton), "Keep Unchanged Bytes", this, SLOT(equal()));
        ui->mainToolBar->addAction(this->style()->standardIcon(QStyle::SP_DialogDiscardButton), "Discard Unchanged Bytes", this, SLOT(diff()));
    }
    else {
        QWidget::setWindowTitle("Memorize");
        setLabeltext(0);
        dv->clearData();
    }
}

void MainWindow::selectProcess()
{
    MemData::fillProcLists();
    QStringList processes;
    for (uint i = 0; i < MemData::getProcIDlist().size(); i++) {
        processes << QString::fromStdWString(MemData::getProcNamelist()[i]) + " (" + QString::number(MemData::getProcIDlist()[i]) + ")";
    }
    bool ok = false;
    QString tmp = QInputDialog::getItem(this, "Choose Process", "Process:", processes, processes.size()-1, false, &ok);
    if (ok) {
        proc = tmp;
        loadProcess();
    }
}

void MainWindow::reload()
{
    switch (sourcetype) {
        case FILE:
            loadFile();
            break;
        case MEM:
            loadProcess();
            break;
        case NONE:
            break;
    }
}

void MainWindow::setLabeltext(long pos)
{
    QString labeltext;
    QString bytes = "";
    unsigned long long addr = 0;
    unsigned char *buffer = NULL;

    switch (sourcetype) {
        case FILE:
            addr = pos;
            buffer = (unsigned char*) fd->getBuffer();
            break;
        case MEM:
            addr = (unsigned long long) md->findAddr(pos);
            buffer = md->getBuffer();
            break;
        default:
            labeltext = "";
    }

    if (sourcetype == FILE || sourcetype == MEM) {
        if (buffer != NULL) {
            for (int i = 0; i < dv->getPixelDepth(); i++) {
                unsigned char c = buffer[pos+i];
                bytes.append(QString::number(c).rightJustified(3, ' '));
                /*
                QChar ch(c);
                bytes.append("(");
                // ignore the "Separator" and "Other" unicode classes
                // http://doc.qt.io/qt-5/qchar.html#Category-enum
                if (ch.category() > 5 && ch.category() < 14)
                    bytes.append("�");
                else
                    bytes.append(ch);
                bytes.append(")");
                */
                bytes.append(" ");
            }
        }
        labeltext = "0x" + QString::number(addr, 16).rightJustified(11, '0') + ": " + bytes;
    }

    ui->addrLabel->setText(labeltext);
}

void MainWindow::onSlide(int value)
{
    dv->setWidth(value*8);
    dv->refresh();
}

void MainWindow::onScroll(int value)
{
    if (value == dv->getScrollbar()->minimum() || value == dv->getScrollbar()->maximum()) {
        setLabeltext(dv->getOffset());
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == dv && sourcetype != NONE) {
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            int x = dv->getPoint(me).x() / dv->getScaling();
            int y = dv->getPoint(me).y() / dv->getScaling();
            if (x < dv->getWidth() && y < dv->getHeight()) {
                int i = y*dv->getWidth() + x + dv->getOffset()/dv->getPixelDepth();
                if (i < dv->getNBytes()/dv->getPixelDepth()) {
                    setLabeltext(i*dv->getPixelDepth());
                }
            }
        }
    }
    return false;
}
