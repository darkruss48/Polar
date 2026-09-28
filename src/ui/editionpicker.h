#pragma once
#include <QWidget>
#include <QVariantAnimation>
#include <functional>

class EditionPickerWidget : public QWidget {
public:
    explicit EditionPickerWidget(QWidget *parent = nullptr);
    void setOnChanged(std::function<void(int)> callback);
    void setEditions(const QVector<int> &list, int selectedEdition, int latestEdition = 0);
    QSize sizeHint() const override { return {220,54}; }
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void wheelEvent(QWheelEvent *) override;
private:
    QVector<int> editions{0};
    int index = 0;
    int pressX = 0;
    int wheelDelta = 0;
    qreal offset = 0;
    bool pressed = false;
    QVariantAnimation animation;
    std::function<void(int)> onChanged;
    void select(int next);
    void settle();
};
