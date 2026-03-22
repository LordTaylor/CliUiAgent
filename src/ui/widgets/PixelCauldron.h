/**
 * @file PixelCauldron.h
 * @brief Procedural pixel-art cauldron animation widget.
 */
#pragma once
#include <QWidget>
#include <QTimer>
#include <QVector>
#include <QPointF>
#include <QColor>

namespace CodeHex {

/**
 * @class PixelCauldron
 * @brief A widget that displays a pixel-art cauldron with procedural animation.
 */
class PixelCauldron : public QWidget {
    Q_OBJECT
public:
    enum class State {
        Idle,
        Thinking,
        Error
    };

    explicit PixelCauldron(QWidget* parent = nullptr);

public slots:
    /** @brief Sets the cauldron state (Idle, Thinking, Error). */
    void setState(State state);
    
    /** @brief Convenince slot for string-based status updates. */
    void setStatus(const QString& status);

protected:
    void paintEvent(QPaintEvent* event) override;
    void timerEvent(QTimerEvent* event) override;

private:
    State m_state = State::Idle;
    int m_animationTimerId = -1;
    float m_phase = 0.0f;
    float m_hue = 0.0f;
    float m_glowPhase = 0.0f;

    struct Particle {
        QPointF pos;
        float speed;
        float life;
        QColor color;
    };
    QVector<Particle> m_particles;

    void updateParticles();
    void drawPixel(class QPainter& p, int x, int y, int size, const QColor& color);
};

} // namespace CodeHex
