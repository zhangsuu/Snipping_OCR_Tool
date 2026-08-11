#include "SnipperWidget.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>

SnipperWidget::SnipperWidget(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool) {
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

void SnipperWidget::startSnipping() {
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    // 获取主屏幕缩放比例与全部几何布局
    m_fullScreenPixmap = screen->grabWindow(0);

    // 设置全屏尺寸并置顶显示
    setGeometry(screen->geometry());
    m_isSelecting = false;
    m_isCaptured = false;
    m_startPoint = QPoint();
    m_endPoint = QPoint();

    showFullScreen();
    activateWindow();
    raise();
}

QRect SnipperWidget::getNormalizedRect() const {
    int x = qMin(m_startPoint.x(), m_endPoint.x());
    int y = qMin(m_startPoint.y(), m_endPoint.y());
    int w = qAbs(m_startPoint.x() - m_endPoint.x());
    int h = qAbs(m_startPoint.y() - m_endPoint.y());
    return QRect(x, y, w, h);
}

void SnipperWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. 绘制截取的原始屏幕画面
    painter.drawPixmap(rect(), m_fullScreenPixmap);

    // 2. 绘制全屏半透明遮罩
    painter.fillRect(rect(), QColor(0, 0, 0, 110));

    // 3. 绘制选区高亮与边框
    if (m_isSelecting || m_isCaptured) {
        QRect selRect = getNormalizedRect();
        if (selRect.width() > 0 && selRect.height() > 0) {
            qreal dpr = m_fullScreenPixmap.devicePixelRatio();
            QRectF srcRect(selRect.x() * dpr, selRect.y() * dpr, selRect.width() * dpr, selRect.height() * dpr);

            // 还原选区内的原本高清画面（无遮罩）
            painter.drawPixmap(selRect, m_fullScreenPixmap, srcRect);

            // 绘制鲜艳的选区外框
            QPen pen(QColor(0, 122, 255), 2);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(selRect);

            // 绘制尺寸指示气泡
            QString text = QString("%1 × %2 px").arg(selRect.width()).arg(selRect.height());
            QFont font("Segoe UI", 9, QFont::Bold);
            painter.setFont(font);
            QFontMetrics fm(font);
            int textWidth = fm.horizontalAdvance(text) + 16;
            int textHeight = fm.height() + 8;

            int tooltipX = selRect.x();
            int tooltipY = selRect.y() - textHeight - 6;
            if (tooltipY < 0) {
                tooltipY = selRect.y() + 6;
            }

            QRect tooltipRect(tooltipX, tooltipY, textWidth, textHeight);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(20, 20, 25, 210));
            painter.drawRoundedRect(tooltipRect, 4, 4);

            painter.setPen(QColor(240, 240, 240));
            painter.drawText(tooltipRect, Qt::AlignCenter, text);
        }
    }
}

void SnipperWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_startPoint = event->pos();
        m_endPoint = m_startPoint;
        m_isSelecting = true;
        m_isCaptured = false;
        update();
    }
}

void SnipperWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_isSelecting) {
        m_endPoint = event->pos();
        update();
    }
}

void SnipperWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_isSelecting) {
        m_endPoint = event->pos();
        m_isSelecting = false;

        QRect selRect = getNormalizedRect();
        hide();

        if (selRect.width() > 5 && selRect.height() > 5) {
            qreal dpr = m_fullScreenPixmap.devicePixelRatio();
            QRect physicalRect(
                qRound(selRect.x() * dpr),
                qRound(selRect.y() * dpr),
                qRound(selRect.width() * dpr),
                qRound(selRect.height() * dpr)
            );

            QPixmap cropped = m_fullScreenPixmap.copy(physicalRect);
            emit imageCaptured(cropped);
        } else {
            emit snippingCancelled();
        }
    }
}

void SnipperWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        hide();
        emit snippingCancelled();
    } else {
        QWidget::keyPressEvent(event);
    }
}
