#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QInputDialog>

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

    dv = new DataVis(ui->containerWidget);
    dv->setMouseTracking(true);
    dv->installEventFilter(this);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(dv);
    layout->setContentsMargins(0,0,0,0);
    ui->containerWidget->setLayout(layout);

    connect(dv->getScrollbar(), SIGNAL(valueChanged(int)), this, SLOT(onScroll(int)));
}

MainWindow::~MainWindow()
{
    delete ui;
    delete fd;
    delete md;
}


void MainWindow::openFile() {
    QString file = QFileDialog::getOpenFileName(this, "OPEN A FILE");
    if (!file.isNull()) {
        sourcetype = NONE;
        delete fd;
        fd = new FileData(file.toStdString());
        fd->padBuffer(dv->calcPadding(fd->getSize()));
        dv->loadData(fd->getBuffer(), fd->getSize());
        sourcetype = FILE;
        setLabeltext(0);
    }
}

void MainWindow::selectProcess() {
    MemData::fillProcLists();
    QStringList processes;
    for (uint i = 0; i < MemData::getProcIDlist().size(); i++) {
        processes << QString::fromStdWString(MemData::getProcNamelist()[i]) + " (" + QString::number(MemData::getProcIDlist()[i]) + ")";
    }
    bool ok = false;
    QString item = QInputDialog::getItem(this, "CHOOSE A PROCESS", "Process:", processes, 0, false, &ok);
    if (ok) {
        QString pid = item.mid(item.lastIndexOf('(')+1, item.lastIndexOf(')') - item.lastIndexOf('(') - 1);
        sourcetype = NONE;
        delete md;
        md = new MemData(pid.toULong());
        md->padBuffer(dv->calcPadding(md->getSize()));
        dv->loadData((char*) md->getBuffer(), md->getSize());
        sourcetype = MEM;
        setLabeltext(0);
    }
}

void MainWindow::setLabeltext(int pos) {
    QString labeltext;
    unsigned long long addr;
    switch(sourcetype) {
        case FILE:
            addr = pos;
            labeltext = "0x" + QString::number(addr, 16).rightJustified(11, '0');
            break;
        case MEM:
            addr = (unsigned long long) md->findAddr(pos);
            labeltext = "0x" + QString::number(addr, 16).rightJustified(11, '0');
            break;
        default:
            labeltext = "";
    }
    ui->addrLabel->setText(labeltext);
}

void MainWindow::onScroll(int value) {
    if (value == dv->getScrollbar()->minimum() || value == dv->getScrollbar()->maximum()) {
        setLabeltext(dv->getOffset());
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
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
