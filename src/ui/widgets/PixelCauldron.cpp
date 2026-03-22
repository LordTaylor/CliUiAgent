#include "PixelCauldron.h"

#include <QPainter>
#include <QRandomGenerator>
#include <QTimerEvent>
#include <cmath>

namespace CodeHex {

PixelCauldron::PixelCauldron(QWidget* parent) : QWidget(parent) {
    setFixedSize(128, 128);
    m_animationTimerId = startTimer(50);  // 20 FPS for smoother "Elite" animations
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

void PixelCauldron::setState(State state) {
    if (m_state == state)
        return;
    m_state = state;
    update();
}

void PixelCauldron::setStatus(const QString& status) {
    QString s = status.toLower();
    if (s.contains("error") || s.contains("bląd") || s.contains("fail")) {
        setState(State::Error);
    } else if (s.contains("thinking") || s.contains("analyzing") || s.contains("processing") ||
               s.contains("plan")) {
        setState(State::Thinking);
    } else {
        setState(State::Idle);
    }
}

void PixelCauldron::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true); // Enable for the glow effect

    int pixelSize = 8;
    int offsetX = 0;
    int offsetY = 0; // Shifted up to the top (prev 2, originally 4)

    // --- Magic Glow (Phase 50) ---
    if (m_state != State::Idle) {
        float pulse = (std::sin(m_glowPhase) + 1.0f) * 0.5f; // 0..1
        QRadialGradient glow(width()/2, height()/2 - 32, 50); // Moved glow up further
        QColor glowColor = (m_state == State::Thinking) ? QColor::fromHslF(m_hue, 0.8, 0.5) : QColor("#EF4444");
        glowColor.setAlpha(int(60 * pulse));
        glow.setColorAt(0, glowColor);
        glow.setColorAt(1, Qt::transparent);
        p.fillRect(rect(), glow);
    }
    p.setRenderHint(QPainter::Antialiasing, false); // Back to pixel-perfect

    // Draw Cauldron body
    QColor cauldronColor = QColor("#1F2937");
    QColor rimColor = QColor("#374151");

    if (m_state == State::Error) {
        cauldronColor = QColor("#7F1D1D");
        rimColor = QColor("#991B1B");
    }

    // Body: rounded shape (Extended to 8 units total including rim for 50% scale)
    for (int y = 8; y < 15; ++y) {
        for (int x = 4; x < 12; ++x) {
            bool isCorner = (y == 8 && (x == 4 || x == 11)) || (y == 14 && (x == 4 || x == 11));
            if (!isCorner) {
                drawPixel(p, x + offsetX, y + offsetY, pixelSize, cauldronColor);
            }
        }
    }

    // Rim: flat top
    for (int x = 3; x < 13; ++x) {
        drawPixel(p, x + offsetX, 7 + offsetY, pixelSize, rimColor);
    }

    // Boiling Surface / Liquid (Hue Shifting)
    if (m_state != State::Idle) {
        QColor liquidColor = (m_state == State::Thinking) ? QColor::fromHslF(m_hue, 0.8, 0.5) : QColor("#EF4444");
        for (int x = 4; x < 12; ++x) {
            float wave = std::sin(m_phase + x * 0.8f) * 1.5f;
            int y = 7 + (int)wave;
            drawPixel(p, x + offsetX, y + offsetY, pixelSize, liquidColor);
        }
    }

    // Draw Particles (Colorful Sparks)
    for (const auto& part : m_particles) {
        p.fillRect(QRectF(part.pos.x(), part.pos.y(), pixelSize, pixelSize), part.color);
    }
}

void PixelCauldron::timerEvent(QTimerEvent* event) {
    if (event->timerId() == m_animationTimerId) {
        m_phase += 0.4f;
        m_glowPhase += 0.15f;
        m_hue += 0.005f;
        if (m_hue > 1.0f) m_hue -= 1.0f;

        updateParticles();
        update();
    } else {
        QWidget::timerEvent(event);
    }
}

void PixelCauldron::updateParticles() {
    // Basic particle logic: spawn and rise
    if (m_state == State::Idle && m_particles.isEmpty())
        return;

    // Spawn new at the surface
    if (m_state != State::Idle && QRandomGenerator::global()->bounded(10) < 6) {
        Particle part;
        int pixelSize = 8;
        // Jitter for horizontal movement (scaled for pixelSize 8)
        // Y moved further up to match offsetY=0
        part.pos = QPointF(32 + QRandomGenerator::global()->bounded(64), 16);
        part.speed = 2.0f + QRandomGenerator::global()->generateDouble() * 2.0f;
        part.life = 1.0f;

        // Magical sparks color
        if (m_state == State::Thinking) {
            part.color = QColor::fromHslF(std::fmod(m_hue + QRandomGenerator::global()->generateDouble()*0.2, 1.0), 0.9, 0.6);
        } else {
            part.color = QColor("#F87171"); // Red for error
        }
        part.color.setAlpha(200);

        m_particles.append(part);
    }

    // Update existing: rise and fade
    for (int i = 0; i < m_particles.size(); ++i) {
        m_particles[i].pos.setY(m_particles[i].pos.y() - m_particles[i].speed);
        m_particles[i].life -= 0.08f;

        if (m_particles[i].life <= 0) {
            m_particles.removeAt(i);
            --i;
        } else {
            // Fade out
            int alpha = (int)(m_particles[i].life * 180);
            m_particles[i].color.setAlpha(alpha);
        }
    }
}

void PixelCauldron::drawPixel(QPainter& p, int x, int y, int size, const QColor& color) {
    p.fillRect(x * size, y * size, size, size, color);
}

}  // namespace CodeHex
