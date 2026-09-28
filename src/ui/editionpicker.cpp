#include "editionpicker.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <cmath>

EditionPickerWidget::EditionPickerWidget(QWidget *parent) : QWidget(parent) {
    setMinimumSize(120,36);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::OpenHandCursor);
    setAccessibleName(tr("WT edition"));
    setToolTip(tr("Drag, scroll or use arrow keys. Current always requests tournament 0."));
    animation.setDuration(160);
    animation.setEasingCurve(QEasingCurve::OutCubic);
    connect(&animation,&QVariantAnimation::valueChanged,this,[this](const QVariant &v){offset=v.toReal();update();});
}
void EditionPickerWidget::setOnChanged(std::function<void(int)> callback) { onChanged=std::move(callback); }
void EditionPickerWidget::setEditions(const QVector<int> &list,int selectedEdition,int) {
    animation.stop(); offset=0;
    editions=list;
    if (!editions.contains(0)) editions.prepend(0);
    index=editions.indexOf(selectedEdition);
    if(index<0) index=0;
    update();
}
void EditionPickerWidget::paintEvent(QPaintEvent *) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(hasFocus()?palette().highlight().color():QColor(255,255,255,35),1));
    p.setBrush(QColor(30,30,30,180));
    p.drawRoundedRect(rect().adjusted(1,1,-1,-1),8,8);
    p.setClipRect(rect().adjusted(2,2,-2,-2));
    const qreal step=width()/3.0;
    for(int i=qMax(0,index-2);i<=qMin(int(editions.size())-1,index+2);++i) {
        const qreal x=width()/2.0+(i-index)*step+offset;
        const qreal focus=qMax(0.0,1.0-std::abs(x-width()/2.0)/step);
        QFont f=font(); f.setPixelSize(qMax(10,int(height()*(0.28+0.10*focus)))); f.setBold(focus>0.6); p.setFont(f);
        p.setPen(QColor(255,255,255,int(110+145*focus)));
        p.drawText(QRectF(x-step/2,0,step,height()),Qt::AlignCenter,editions[i]==0?tr("Current"):QString::number(editions[i]));
    }
}
void EditionPickerWidget::settle() {
    animation.stop(); animation.setStartValue(offset); animation.setEndValue(0.0); animation.start();
}
void EditionPickerWidget::select(int next) {
    next=qBound(0,next,int(editions.size())-1);
    const int previous=index;
    index=next;
    offset+=(index-previous)*(width()/3.0);
    settle();
    if(index!=previous && onChanged) onChanged(editions[index]);
}
void EditionPickerWidget::mousePressEvent(QMouseEvent *e) {
    if(e->button()!=Qt::LeftButton) return;
    animation.stop(); offset=0; pressed=true; pressX=e->position().x(); setCursor(Qt::ClosedHandCursor);
}
void EditionPickerWidget::mouseMoveEvent(QMouseEvent *e) {
    if(!pressed) return;
    offset=qBound(-width()/3.0,e->position().x()-pressX,width()/3.0); update();
}
void EditionPickerWidget::mouseReleaseEvent(QMouseEvent *e) {
    if(!pressed || e->button()!=Qt::LeftButton) return;
    pressed=false; setCursor(Qt::OpenHandCursor);
    const qreal delta=e->position().x()-pressX;
    if(std::abs(delta)>width()/8.0) select(index+(delta<0?1:-1));
    else if(std::abs(delta)<6 && e->position().x()<width()/3.0) select(index-1);
    else if(std::abs(delta)<6 && e->position().x()>width()*2/3.0) select(index+1);
    else settle();
}
void EditionPickerWidget::keyPressEvent(QKeyEvent *e) {
    if(e->key()==Qt::Key_Left) select(index-1);
    else if(e->key()==Qt::Key_Right) select(index+1);
    else if(e->key()==Qt::Key_Home) select(0);
    else QWidget::keyPressEvent(e);
}
void EditionPickerWidget::wheelEvent(QWheelEvent *e) {
    wheelDelta+=e->angleDelta().y();
    if(std::abs(wheelDelta)>=120) {select(index+(wheelDelta<0?1:-1));wheelDelta=0;}
    e->accept();
}
