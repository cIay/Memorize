#ifndef DATAVIS_H
#define DATAVIS_H

#include <QWidget>

#include <QScrollArea>
#include <QScrollBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QImage>
#include <QBitmap>
#include <QMouseEvent>

class DataVis : public QWidget
{
    Q_OBJECT
public:
    explicit DataVis(QWidget *parent = 0,
                     char *buf = NULL,
                     long size = 0,
                     QImage::Format img_format = QImage::Format_RGB32);
    void loadData(char*, long, QImage::Format img_format = QImage::Format_RGB32);
    int calcPadding(long size) { return ((w*depth) - (size % (w*depth))); }
    QPoint getPoint(QMouseEvent* me) { return label->mapFromParent(me->pos()); }
    int getNBytes() { return n_bytes; }
    int getPixelDepth() { return depth; }
    int getWidth() { return w; }
    int getHeight() { return h; }
    int getOffset() { return offset; }
    float getScaling() { return cur_scaling; }
    QScrollBar* getScrollbar() { return scrollbar; }


private:
    virtual void mousePressEvent(QMouseEvent*);
    void padData();
    void maskPadding();
    void scaleImage(float);
    void loadNext(int, int);
    char *data;
    long n_bytes;
    int padding;
    int depth;
    QImage::Format format;
    QScrollArea *scrollarea;
    QScrollBar *scrollbar;
    QLabel *label;
    QPixmap *pix;
    int w, h;
    long offset;
    float cur_scaling;

private slots:
    void onScroll(int);

signals:

public slots:
};

#endif // DATAVIS_H
