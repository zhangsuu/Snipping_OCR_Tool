#ifndef SNIPPERWIDGET_H
#define SNIPPERWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QRect>

class SnipperWidget : public QWidget {
    Q_OBJECT

public:
    explicit SnipperWidget(QWidget *parent = nullptr);
    ~SnipperWidget() override = default;

    // 启动全屏截图
    void startSnipping();

signals:
    // 完成截图后发射该信号传递选区图片
    void imageCaptured(const QPixmap &pixmap);
    // 取消截图信号
    void snippingCancelled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QRect getNormalizedRect() const;

    QPixmap m_fullScreenPixmap;
    QPoint m_startPoint;
    QPoint m_endPoint;
    bool m_isSelecting = false;
    bool m_isCaptured = false;
};

#endif // SNIPPERWIDGET_H
