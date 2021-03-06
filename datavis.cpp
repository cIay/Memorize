#include "datavis.h"

#include <QDebug>

DataVis::DataVis(QWidget *parent, char *buf, long size, QImage::Format img_format) :
    QWidget(parent),
    data(NULL),
    n_bytes(0),
    scrollarea(new QScrollArea(this)),
    scrollbar(scrollarea->verticalScrollBar()),
    label(new QLabel(scrollarea)),
    pix(NULL),
    w(28*8), h(1800),
    max_w(1024),
    offset(0),
    cur_scaling(1)
{
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    label->setScaledContents(true);

    scrollarea->setWidget(label);

    scrollarea->setMouseTracking(true);
    label->setMouseTracking(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(scrollarea);
    layout->setContentsMargins(0,0,0,0);
    setLayout(layout);

    loadData(buf, size, img_format);

    connect(scrollbar, SIGNAL(valueChanged(int)), this, SLOT(onScroll(int)));
}

void DataVis::clearData()
{
    data = NULL;
    n_bytes = 0;
    offset = 0;
    delete pix; pix = NULL;
    label->resize(0, 0);
    label->clear();
}

void DataVis::loadData(char *buf, long size, QImage::Format img_format)
{
    bool reset_pos = false;
    if (size != n_bytes) {
        reset_pos = true;
        offset = 0;
    }
    data = buf;
    n_bytes = size;
    format = img_format;
    depth = QImage::toPixelFormat(format).bitsPerPixel() / 8;
    padding = calcPadding(n_bytes);

    refresh();

    if (reset_pos)
        scrollbar->setValue(scrollbar->minimum());
}

void DataVis::refresh()
{
    if (data == NULL || n_bytes <= 0)
        return;

    int new_h = h;
    bool single_page = false;
    if (n_bytes < w*h*depth) {
        single_page = true;
        new_h = (n_bytes) / (w*depth);
        if ((n_bytes) % (w*depth) != 0)
            new_h++;
    }

    long nudge = 0;
    if (n_bytes+padding-offset < w*new_h*depth)
        nudge = (w*new_h*depth) - (n_bytes+padding-offset);

    if (nudge <= offset) {
        offset -= nudge;
    }
    else {
        qDebug() << "Error: Attempted negative offset";
        return;
    }

    QImage img((uchar*)(data+offset), w, new_h, format);

    delete pix;
    pix = new QPixmap(QPixmap::fromImage(img));

    if (single_page)
        maskPadding();

    scaleImage(1);
    scrollarea->setWidget(label);
}

void DataVis::loadNext(int step, int new_scroll)
{
    long end = n_bytes+padding - w*h*depth;
    if (step < 0) { // scrolling up
        if (offset == 0)
            return;
        else if (offset+step > 0)
            offset += step;
        else
            offset = 0;
    }
    else { // scrolling down
        if (offset == end || end < 0)
            return;
        else if (offset+step < end)
            offset +=step;
        else
            offset = end;
    }

    if (n_bytes+padding < w*h*depth)
        return;

    QImage img((uchar*)(data+offset), w, h, format);

    delete pix;
    pix = new QPixmap(QPixmap::fromImage(img));
    //pix->convertFromImage(img);

    if (offset == end)
        maskPadding();

    scaleImage(1);
    scrollarea->setWidget(label);
    scrollbar->setValue(new_scroll);
    scrollarea->ensureVisible(0,h*cur_scaling/2,0,0);
}

void DataVis::onScroll(int value)
{
    int step = w*h*depth/2;
    if (value == scrollbar->minimum()) { // scrolling up
        loadNext(-step, scrollbar->maximum()-1);
    }
    else if (value == scrollbar->maximum()) { // scrolling down
        loadNext(step, scrollbar->minimum()+1);
    }
}

int DataVis::calcPadding(long size) {
    int highest = 0;
    for (int i = 8; i <= max_w; i+=8) {
        int pad = (i*depth) - (size % (i*depth));
        if (pad > highest)
            highest = pad;
    }
    return highest;
}

void DataVis::maskPadding()
{
    uchar mask[w*h/8];
    for (int i = 0; i < w*h/8; i++) {
        if (i < (n_bytes-offset)/(depth*8)) {
            mask[i] = 0xFF;
        }
        else if (i == (n_bytes-offset)/(depth*8)) {
            mask[i] = 0xFF >> (8 - ((n_bytes-offset)/depth - i*8));
        }
        else {
            mask[i] = 0;
        }
    }
    pix->setMask(QBitmap::fromData(pix->size(), mask));
}

void DataVis::scaleImage(float scale_factor)
{
    if (cur_scaling * scale_factor < 1 || cur_scaling * scale_factor > 12)
        return;
    cur_scaling *= scale_factor;
    label->setPixmap(pix->scaled(pix->size() * cur_scaling, Qt::KeepAspectRatioByExpanding, Qt::FastTransformation));
    label->resize(label->pixmap()->size());

    scrollarea->verticalScrollBar()->setValue((scale_factor * scrollarea->verticalScrollBar()->value()) +
                                                ((scale_factor - 1) * scrollarea->verticalScrollBar()->pageStep()/2));
    scrollarea->horizontalScrollBar()->setValue((scale_factor * scrollarea->horizontalScrollBar()->value()) +
                                                ((scale_factor - 1) * scrollarea->horizontalScrollBar()->pageStep()/2));
}


void DataVis::mousePressEvent(QMouseEvent *event)
{
    if (pix != NULL) {
        if (event->button() == 1) { // left click
            scaleImage(1.25); // zoom in
        }
        else if (event->button() == 2) { // right click
            scaleImage(0.8); // zoom out
        }
    }
}
