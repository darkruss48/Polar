#include "wtapi.h"
#include "wtdata.h"
#include "performanceanalysis.h"
#include "leaderboard.h"
#include "functb.h"
#include "qdialog.h"
#include "qjsonarray.h"
#include "qjsonobject.h"
#include <QEvent>
#include <QListWidgetItem>
#include <iostream>
#include <string>
#include "render.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QRegularExpression>
#include <QSizePolicy>
#include <QMap>
#include <QMenu>
#include <QAction>
#include <QSet>
#include <cmath> // NEW: for std::llabs
#include <QGraphicsDropShadowEffect>  // NEW
#include <QPropertyAnimation>         // NEW
#include <QPainter>                   // NEW
#include <QPainterPath>               // NEW
#include <QTimer>                     // NEW
#include <QRandomGenerator>           // NEW
#include "appsettings.h" // NEW: for selectedEdition

static int g_EmbersParticleBudget = 400; // + c'est grand, + de particules

// NEW: keep the shine overlay sized with its label
class LabelResizeFilter : public QObject {
public:
    LabelResizeFilter(QLabel* label, QWidget* shine, QObject* parent = nullptr)
        : QObject(parent), m_label(label), m_shine(shine) {
        if (m_label) m_label->installEventFilter(this);
    }
protected:
    bool eventFilter(QObject* obj, QEvent* ev) override {
        if (obj == m_label && ev->type() == QEvent::Resize) {
            if (m_shine && m_label) {
                m_shine->resize(18, m_label->height());
            }
        }
        return QObject::eventFilter(obj, ev);
    }
private:
    QLabel*  m_label = nullptr;
    QWidget* m_shine = nullptr;
};

// REMOVE: old flame particle/tongue overlay
// class FlameParticlesWidget : public QWidget { ... }  // removed

// NEW: Gradient-painted metallic label for ranks #1/#2/#3 (text-only, with shine)
class MetallicLabel : public QLabel {
public:
    enum class Type { Gold, Silver, Bronze };
    explicit MetallicLabel(const QString& text, Type t, QWidget* parent = nullptr)
        : QLabel(text, parent), m_type(t)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, [this](){
            m_shine += 0.03;
            if (m_shine > 1.5) m_shine = -0.5;
            update();
        });
        m_timer->start(50); // ~20 FPS
    }
protected:
    void showEvent(QShowEvent *event) override { QWidget::showEvent(event); m_timer->start(50); }
    void hideEvent(QHideEvent *event) override { m_timer->stop(); QWidget::hideEvent(event); }

    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);

        const QString txt = text();
        if (txt.isEmpty()) return;

        const QFontMetrics fm(font());
        const int w = width();
        const int h = height();
        // Position: left + vertically centered (like default)
        const int textW = fm.horizontalAdvance(txt);
        const int x = 0; // align left
        const int baseline = (h + fm.ascent() - fm.descent()) / 2;

        // Build glyph path
        QPainterPath path;
        path.addText(x, baseline, font(), txt);

        // Metallic vertical gradient
        QLinearGradient g(0, baseline - fm.ascent(), 0, baseline + fm.descent());
        switch (m_type) {
            case Type::Gold:
                g.setColorAt(0.00, QColor("#B8860B"));
                g.setColorAt(0.45, QColor("#FFD700"));
                g.setColorAt(0.55, QColor("#FFF3A6"));
                g.setColorAt(1.00, QColor("#B8860B"));
                break;
            case Type::Silver:
                g.setColorAt(0.00, QColor("#6E7B8B"));
                g.setColorAt(0.45, QColor("#C0C0C0"));
                g.setColorAt(0.55, QColor("#F0F0F0"));
                g.setColorAt(1.00, QColor("#6E7B8B"));
                break;
            case Type::Bronze:
                g.setColorAt(0.00, QColor("#6B3E1F"));
                g.setColorAt(0.45, QColor("#CD7F32"));
                g.setColorAt(0.55, QColor("#E6B07A"));
                g.setColorAt(1.00, QColor("#6B3E1F"));
                break;
        }

        p.setPen(Qt::NoPen);
        p.setBrush(g);
        p.drawPath(path);

        // Soft glow (draw again slightly larger with low alpha)
        QPen outline(QColor(255,255,255,40), 1.0);
        p.setPen(outline);
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);

        // Shine band: thin diagonal sweep, clipped to text
        p.save();
        p.setClipPath(path);
        const qreal bandW = qMax(10, textW/10);
        const qreal cx = (x + textW * m_shine);
        QLinearGradient shineGrad(cx - bandW, 0, cx + bandW, 0);
        shineGrad.setColorAt(0.0, QColor(255,255,255,0));
        shineGrad.setColorAt(0.5, QColor(255,255,255,120));
        shineGrad.setColorAt(1.0, QColor(255,255,255,0));
        p.fillRect(QRectF(0, 0, w, h), shineGrad);
        p.restore();
    }
private:
    Type m_type;
    QTimer* m_timer = nullptr;
    qreal m_shine = -0.5; // sweep position (relative [~ -0.5..1.5])
};

// NEW: flaming text label (orange base + smooth moving white highlights on glyphs)
class FlameTextLabel : public QLabel {
public:
    explicit FlameTextLabel(const QString& text, QWidget* parent = nullptr)
        : QLabel(text, parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, [this](){
            updateFlickers();
            update();
        });
        m_timer->start(33); // ~30 FPS: smooth enough, low CPU
        initFlickers();
    }
protected:
    void showEvent(QShowEvent *event) override { QWidget::showEvent(event); m_timer->start(33); }
    void hideEvent(QHideEvent *event) override { m_timer->stop(); QWidget::hideEvent(event); }

    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);

        const QString txt = text();
        if (txt.isEmpty()) return;

        const QFontMetrics fm(font());
        const int h = height();
        const int baseline = (h + fm.ascent() - fm.descent()) / 2;
        const int x = 0; // align left

        QPainterPath path;
        path.addText(x, baseline, font(), txt);

        // Base warm orange gradient
        const QRectF bb = path.boundingRect();
        QLinearGradient baseGrad(bb.center().x(), bb.top(), bb.center().x(), bb.bottom());
        baseGrad.setColorAt(0.00, QColor(255,128,32));
        baseGrad.setColorAt(0.50, QColor(255,160,64));
        baseGrad.setColorAt(1.00, QColor(240,110,28));

        p.setPen(Qt::NoPen);
        p.setBrush(baseGrad);
        p.drawPath(path);

        // Smooth highlights: narrow bands with sinusoidal alpha, blended softly
        p.save();
        p.setClipPath(path);
        p.setCompositionMode(QPainter::CompositionMode_Screen); // soft additive
        for (const Flicker& f : m_flickers) {
            const qreal alpha = qBound<qreal>(0.0, (f.alphaBase/255.0) * (0.4 + 0.6 * (0.5 * (1.0 + std::sin(f.phase)))), 1.0);
            if (alpha <= 0.002) continue;

            const qreal w = qMax<qreal>(1.0, f.half * 2.0);
            const qreal left = f.c - f.half;
            const QRectF stripe(left, bb.top(), w, bb.height());

            // Horizontal gradient concentrated at center
            QLinearGradient g(left, 0, left + w, 0);
            g.setColorAt(0.00, QColor(255,255,255, 0));
            g.setColorAt(0.50, QColor(255,255,255, int(alpha * 90))); // reduced intensity
            g.setColorAt(1.00, QColor(255,255,255, 0));

            p.fillRect(stripe, g);
        }
        p.restore();
    }
private:
    struct Flicker { qreal c; qreal half; qreal vx; qreal phase; qreal dphase; qreal alphaBase; };
    QVector<Flicker> m_flickers;
    QTimer* m_timer = nullptr;

    static qreal rnd01() { return QRandomGenerator::global()->generateDouble(); }

    void initFlickers() {
        m_flickers.clear();
        const int w = qMax(1, width());
        const int count = 2 + (w > 220 ? 1 : 0); // 2..3 bands max
        for (int i=0; i<count; ++i) {
            Flicker f;
            f.c = rnd01() * w;
            f.half = 3.0 + rnd01() * 6.0;           // 3..9 px half-width
            f.vx = (rnd01() * 0.6 - 0.3);           // -0.3..+0.3 px/tick
            f.phase = rnd01() * 6.28318;            // 0..2π
            f.dphase = 0.04 + rnd01() * 0.05;       // 0.04..0.09 rad/tick
            f.alphaBase = 0.25 + rnd01() * 0.15;    // 0.25..0.40 in [0..1] scale later
            m_flickers.push_back(f);
        }
    }

    void updateFlickers() {
        const qreal w = qMax(1, width());
        if (m_flickers.isEmpty()) { initFlickers(); return; }
        for (auto& f : m_flickers) {
            f.c += f.vx;
            if (f.c < 0.0)  { f.c = 0.0;  f.vx = -f.vx; }
            if (f.c > w)    { f.c = w;    f.vx = -f.vx; }
            f.phase += f.dphase;
            // very light, slow jitter of width to avoid static look
            f.half += (rnd01() * 0.2 - 0.1); // -0.1..+0.1 px
            f.half = qBound<qreal>(2.5, f.half, 10.0);
        }
    }
};

// NEW: embers overlay, clipped to the text path of a target label (parent label) - softened
class EmbersOverlay : public QWidget {
public:
    explicit EmbersOverlay(QLabel* target, QWidget* parent = nullptr)
        : QWidget(parent), m_target(target)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setVisible(true);
        if (m_target) m_target->installEventFilter(this);
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &EmbersOverlay::tick);
        m_timer->start(40); // ~25 FPS
        m_pts.reserve(qMax(64, g_EmbersParticleBudget));
    }
protected:
    void showEvent(QShowEvent *event) override { QWidget::showEvent(event); if(m_timer) m_timer->start(40); }
    void hideEvent(QHideEvent *event) override { if(m_timer) m_timer->stop(); QWidget::hideEvent(event); }

    bool eventFilter(QObject* obj, QEvent* ev) override {
        if (obj == m_target && ev->type() == QEvent::Resize) {
            setGeometry(m_target->rect());
        }
        return QWidget::eventFilter(obj, ev);
    }
    void paintEvent(QPaintEvent*) override {
        if (!m_target) return;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setClipPath(textPath());
        p.setCompositionMode(QPainter::CompositionMode_Plus); // warmer additive glow

        for (const auto& e : m_pts) {
            const qreal t = e.life / e.maxLife;
            const int alpha = int(120 * (1.0 - t)); // softened
            QColor c(255, 180 - int(70*t), 40, alpha); // orange -> dim
            p.setBrush(c);
            p.setPen(Qt::NoPen);
            p.drawEllipse(e.pos, e.size, e.size);
        }
    }
private:
    struct Ember { QPointF pos; QPointF vel; qreal life=0, maxLife=1.0, size=1.0; };
    QVector<Ember> m_pts;
    QTimer* m_timer = nullptr;
    QLabel* m_target = nullptr;

    static qreal rnd01() { return QRandomGenerator::global()->generateDouble(); }

    QPainterPath textPath() const {
        QPainterPath path;
        if (!m_target) return path;
        const QString txt = m_target->text();
        QFont f = m_target->font();
        const QFontMetrics fm(f);
        const int h = height();
        const int baseline = (h + fm.ascent() - fm.descent()) / 2;
        path.addText(0, baseline, f, txt);
        return path;
    }
    void spawn(int n) {
        if (!m_target) return;
        const QFontMetrics fm(m_target->font());
        const int h = height();
        const int w = width();
        const int baseline = (h + fm.ascent() - fm.descent()) / 2;

        for (int i=0; i<n; ++i) {
            Ember e;
            // Spawn within the letter area (spread across ascender zone)
            const qreal u = rnd01() * 0.9; // 0..0.9 of ascender
            e.pos = QPointF(rnd01() * w, baseline - u * fm.ascent());
            // Gentle upward drift
            const qreal vx = (rnd01() * 0.24 - 0.12);           // -0.12..+0.12
            const qreal vy = -(rnd01() * (0.90 - 0.45) + 0.45); // -(0.45..0.90)
            e.vel = QPointF(vx, vy);
            e.maxLife = 24.0 + QRandomGenerator::global()->bounded(18); // 24..41 ticks
            e.life = 0.0;
            e.size = 0.6 + rnd01() * (1.3 - 0.6); // 0.6..1.3
            if (m_pts.size() < g_EmbersParticleBudget) m_pts.push_back(e);
        }
    }
    void tick() {
        for (int i = m_pts.size()-1; i >= 0; --i) {
            auto& e = m_pts[i];
            e.pos += e.vel;
            e.life += 1.0;
            // much smaller size jitter to avoid twinkling
            e.size *= (1.0 + (rnd01() * 0.012 - 0.006)); // -0.006..+0.006
            if (e.life >= e.maxLife) m_pts.remove(i);
        }
        // Spawn scaled by budget, without exceeding it
        const int baseSpawn = qMax(1, g_EmbersParticleBudget / 80); // 80->1, 160->2, 240->3, ...
        const int room = qMax(0, g_EmbersParticleBudget - m_pts.size());
        const int toSpawn = qMin(room, baseSpawn);
        if (toSpawn > 0) spawn(toSpawn);
        update();
    }
};

// pointeurs
QGraphicsView *Leaderboard::graphPlaceholder = nullptr;
QLabel *Leaderboard::dataPlaceholder = nullptr;
QLabel *Leaderboard::avgPlaceholder = nullptr; // NEW
QLabel *Leaderboard::gapPlaceholder = nullptr; // NEW
// NEW
QListWidget* Leaderboard::playerListPtr = nullptr; // NEW
// NEW: overlay state
QString Leaderboard::baseSeriesName = QString();
QSet<QString> Leaderboard::overlayNames;
// NEW: snapshot
QVector<Leaderboard::SimpleRow> Leaderboard::snapshotRows;
// NEW: current selection
QString Leaderboard::currentSelectedName = QString(); // NEW

// NEW: two-column placeholders (define statics)
QLabel *Leaderboard::dataLeftPlaceholder = nullptr;
QLabel *Leaderboard::dataRightPlaceholder = nullptr;
QLabel *Leaderboard::avgLeftPlaceholder = nullptr;
QLabel *Leaderboard::avgRightPlaceholder = nullptr;
QLabel *Leaderboard::gapLeftPlaceholder = nullptr;
QLabel *Leaderboard::gapRightPlaceholder = nullptr;

Leaderboard::Leaderboard(QWidget *parent) : QWidget(parent)
{
    // mainLayout = new QVBoxLayout(this);
    // contentLayout = new QHBoxLayout();

    // playerList = new QListWidget(this);
    // refreshButton = new QPushButton(this);
    // // graphPlaceholder = new QLabel(this);
    // dataPlaceholder = new QLabel(this);

    // // Partie gauche : liste de joueurs
    // connect(playerList, &QListWidget::itemDoubleClicked, this, &Leaderboard::onPlayerDoubleClicked);

    // // Partie droite : zone graphique + données
    // QVBoxLayout *rightLayout = new QVBoxLayout();
    // // rightLayout->addWidget(graphPlaceholder);
    // rightLayout->addWidget(dataPlaceholder);

    // // contentLayout->addWidget(playerList);
    // // contentLayout->addLayout(rightLayout);
    // // mainLayout->addLayout(contentLayout);
    // // mainLayout->addWidget(refreshButton);

    // connect(refreshButton, &QPushButton::clicked, this, &Leaderboard::onRefreshClicked);
    // connect(refreshButton, &QPushButton::clicked, this, [this]() {
    //     Leaderboard::onRefreshClicked(this);
    // });
    
    std::cout << "LEADERBOARDc : " << std::endl;
    retranslateUi();
    std::cout << "LEADERBOARDu : " << std::endl;

}

Leaderboard::~Leaderboard() {}

void Leaderboard::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void Leaderboard::retranslateUi()
{
    if (refreshButton) refreshButton->setText(tr("Refreshe"));
    if (dataPlaceholder) dataPlaceholder->setText(tr("Data placeholder here"));
    // setWindowTitle(tr("Leaderboard"));
}

// Helpers (formatted numbers)
static QString formatThousands(qint64 v) {
    QString s = QString::number(v);
    return s.replace(QRegularExpression("(\\d)(?=(\\d{3})+(?!\\d))"), "\\1,");
}

// Points -> "X.Y M"
static QString formatMillions(qint64 v) {
    double m = static_cast<double>(v) / 1'000'000.0;
    return QString::number(m, 'f', m >= 10.0 ? 1 : 2) + " M";
}

// Format M with sign (e.g. +1.2 M / -0.7 M)
static QString formatMillionsSigned(qint64 v) {
    const qint64 av = std::llabs(v);
    const double m = static_cast<double>(av) / 1'000'000.0;
    const QString sign = (v > 0 ? "+" : (v < 0 ? "-" : ""));
    return QString("%1%2 M").arg(sign).arg(QString::number(m, 'f', m >= 10.0 ? 1 : 2));
}

// NEW: compact signed M for tight cell display (e.g. +1.2M)
static QString formatMillionsSignedCompact(qint64 v) {
    const qint64 av = std::llabs(v);
    const double m = static_cast<double>(av) / 1'000'000.0;
    const QString sign = (v > 0 ? "+" : (v < 0 ? "-" : ""));
    return QString("%1%2M").arg(sign).arg(QString::number(m, 'f', m >= 10.0 ? 1 : 2));
}

// NEW: ETA formatter from hours -> "~XhYmin" or "~Zmin"
static QString formatEtaFromHours(double hours)
{
    if (hours <= 0.0) return QString("~0min");
    const int totalMin = static_cast<int>(std::round(hours * 60.0));
    const int h = totalMin / 60;
    const int m = totalMin % 60;
    if (h == 0) return QString("~%1min").arg(m);
    if (m == 0) return QString("~%1h").arg(h);
    return QString("~%1h%2min").arg(h).arg(m);
}

// NEW: Points/hour on recent window (last K steps, 15min per step)
static double computePointsPerHour(const QJsonObject &player) {
    return Performance::summarize(Performance::samples(player),2).recentRate;
}
static double lastHour(const QJsonObject &player) {
    const auto samples=Performance::samples(player);
    return samples.isEmpty()?Performance::unavailable:samples.last().hour;
}

// Helpers: convert step-count (15min each) to "XhYmin" string
static QString formatDurationFromSteps(int steps)
{
    if (steps <= 0) return QString("0h");
    int totalMin = steps * 15;
    int h = totalMin / 60;
    int m = totalMin % 60;
    if (m == 0) return QString::number(h) + "h";
    if (h == 0) return QString::number(m) + "min";
    return QString("%1h%2min").arg(h).arg(m);
}

// trailing stable points count -> steps AFK (15 min per step)
static int computeAfkStepsTrailing(const QStringList& pointsList) {
    if (pointsList.size() < 2) return 0;
    const QString last = pointsList.last().trimmed();
    int cnt = 0;
    for (int i = pointsList.size() - 2; i >= 0; --i) {
        if (pointsList.at(i).trimmed() == last) cnt++;
        else break;
    }
    return cnt;
}

// trailing strictly increasing points -> steps of continuous farm
static int computeFarmStepsTrailing(const QStringList& pointsList) {
    if (pointsList.size() < 2) return 0;
    int cnt = 0;
    for (int i = pointsList.size() - 1; i > 0; --i) {
        qint64 cur = pointsList.at(i).trimmed().toLongLong();
        qint64 prev = pointsList.at(i-1).trimmed().toLongLong();
        if (cur > prev) cnt++;
        else break;
    }
    return cnt;
}

// Build a compact row widget
static QWidget* createPlayerRowWidget(int rank, const QString& name, qint64 points,
                                      const QString& gapBelowLabel,
                                      bool isAfk, const QString& statusText,
                                      const QString& gapDeltaLabel, const QString& gapDeltaColor,
                                      bool highlightFlames /* NEW */)
{
    auto row = new QWidget();
    auto root = new QHBoxLayout(row);
    // NEW: tighter layout
    root->setContentsMargins(8, 6, 16, 6);
    root->setSpacing(8);

    // Left: Rank
    QLabel* lblRank = nullptr;
    if (rank == 1 || rank == 2 || rank == 3) {
        MetallicLabel::Type t = (rank == 1) ? MetallicLabel::Type::Gold
                                : (rank == 2 ? MetallicLabel::Type::Silver
                                             : MetallicLabel::Type::Bronze);
        lblRank = new MetallicLabel(QString("#%1").arg(rank), t, row);
    } else {
        lblRank = new QLabel(QString("#%1").arg(rank), row);
    }
    QFont fRank = lblRank->font();
    fRank.setBold(true);
    fRank.setPointSize(fRank.pointSize() + 3);
    lblRank->setFont(fRank);
    lblRank->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    lblRank->setMinimumWidth(44);

    root->addWidget(lblRank, 0);

    // Middle: Name + Status
    auto mid = new QVBoxLayout();
    mid->setSpacing(2);
    QLabel* lblName = highlightFlames ? static_cast<QLabel*>(new FlameTextLabel(name, row))
                                      : static_cast<QLabel*>(new QLabel(name, row));
    QFont fName = lblName->font(); fName.setBold(true); fName.setPointSize(fName.pointSize() + 1);
    lblName->setFont(fName);
    auto lblStatus = new QLabel(statusText);
    // NEW: status en plus petit et discret
    lblStatus->setStyleSheet(QString("color:%1; font-size:11px;").arg(isAfk ? "#E74C3C" : "#2ECC71"));
    mid->addWidget(lblName);
    mid->addWidget(lblStatus);
    root->addLayout(mid, 1);

    // NEW: add embers directly inside the glyphs (only when flaming)
    if (highlightFlames) {
        auto embers = new EmbersOverlay(lblName, lblName);
        embers->setGeometry(lblName->rect());
        embers->show();
    }

    // Right: Points + Gap (+ colored delta)
    auto right = new QVBoxLayout();
    right->setSpacing(2);
    right->setContentsMargins(0, 0, 5, 0);
    auto lblPts = new QLabel(QString("%1 pts").arg(formatMillions(points)));
    lblPts->setStyleSheet("color:#CFCFCF;");
    lblPts->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // bottom row with colored delta then grey gap
    auto gapRow = new QHBoxLayout();
    gapRow->setContentsMargins(0,0,0,0);
    gapRow->setSpacing(6);

    auto lblDelta = new QLabel(gapDeltaLabel);
    lblDelta->setStyleSheet(QString("color:%1;").arg(gapDeltaColor));
    lblDelta->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lblDelta->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    lblDelta->setFixedWidth(62);

    auto lblGap = new QLabel(gapBelowLabel);
    lblGap->setStyleSheet("color:#A0A0A0;");
    lblGap->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lblGap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    gapRow->addWidget(lblDelta, 0);
    gapRow->addWidget(lblGap, 1);
    gapRow->setStretch(0, 0);
    gapRow->setStretch(1, 1);

    right->addWidget(lblPts);
    right->addLayout(gapRow);
    root->addLayout(right, 0);

    row->setMinimumHeight(68);
    row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    row->setStyleSheet("QWidget { background: transparent; }");
    return row;
}

// NEW: style all info labels (left+right)
static void ensureInfoLabelsStyled()
{
    auto apply = [](QLabel* lbl, bool isLeft){
        if (!lbl) return;
        if (lbl->property("twoColStyled").toBool()) return; // apply once

        lbl->setTextFormat(Qt::PlainText);
        lbl->setWordWrap(false);
        lbl->setAlignment(isLeft ? (Qt::AlignLeft | Qt::AlignTop) : (Qt::AlignRight | Qt::AlignTop));

        // Fixed size (slightly bigger) to avoid cumulative growth
        lbl->setStyleSheet(QString(
            "QLabel {"
            "  font-size:13px;"
            "  padding:2px 4px;"
            "%1"
            "}"
        ).arg(isLeft ? "color:#A8A8A8;" : ""));

        lbl->setMinimumSize(0, 0);
        lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // mark as styled
        lbl->setProperty("twoColStyled", true);
    };
    apply(Leaderboard::dataLeftPlaceholder,  true);
    apply(Leaderboard::dataRightPlaceholder, false);
    apply(Leaderboard::avgLeftPlaceholder,   true);
    apply(Leaderboard::avgRightPlaceholder,  false);
    apply(Leaderboard::gapLeftPlaceholder,   true);
    apply(Leaderboard::gapRightPlaceholder,  false);
}

// NEW: helper to set two columns with same number of lines
static void setTwoColumnText(QLabel* leftLbl, QLabel* rightLbl,
                             QStringList leftLines, QStringList rightLines)
{
    if (!leftLbl || !rightLbl) return;
    const int n = qMax(leftLines.size(), rightLines.size());
    leftLines.reserve(n);
    rightLines.reserve(n);
    while (leftLines.size() < n)  leftLines << "";
    while (rightLines.size() < n) rightLines << "";
    leftLbl->setTextFormat(Qt::PlainText);rightLbl->setTextFormat(Qt::PlainText);
    leftLbl->setText(leftLines.join("\n"));
    rightLbl->setText(rightLines.join("\n"));
}

// Séparateur avec espace plus généreux
static QString hr() { return "<div style='border-top:1px solid rgba(255,255,255,0.14); margin:12px 0;'></div>"; }

// 2-col table helper (tuned spacing + largeur col. label fixe)
static QString kvTable(const QList<QPair<QString,QString>>& rows) {
    QString html = "<table style='width:100%; border-collapse:separate; border-spacing:0 6px;'>";
    for (const auto& r : rows) {
        html += QString(
            "<tr>"
              "<td style='color:#A8A8A8; width:52%; padding:2px 8px 2px 0; white-space:nowrap;'>%1</td>"
              "<td style='text-align:right; padding:2px 0 2px 8px;'>%2</td>"
            "</tr>"
        ).arg(r.first.toHtmlEscaped(), r.second.toHtmlEscaped());
    }
    html += "</table>";
    return html;
}

// Indentation utilitaire (décaler à droite un bloc HTML)
static QString indentBlock(const QString& inner, int px = 14) {
    return QString("<div style='margin-left:%1px;'>%2</div>").arg(px).arg(inner);
}

// NEW: builders without headings (titles now provided by QGroupBox)
static QString buildInfoHtml(const QString& name, const QString& rank, const QString& wins, const QString& totalPts, const QString& afk)
{
    return kvTable({
        { QObject::tr("Nom"), name },
        { QObject::tr("Rank"), rank },
        { QObject::tr("Wins"), wins },
        { QObject::tr("Points totaux"), totalPts },
        { QObject::tr("Heures AFK"), afk }
    });
}

// Dernier paramètre: paceHtml déjà formaté (table)
static QString buildAvgHtml(const QString& activeStr, const QString& afkStr, double avgWinsPerHour, const QString& paceHtml)
{
    const QString tbl = kvTable({
        { QObject::tr("Non-AFK"), activeStr },
        { QObject::tr("AFK"), afkStr },
        { QObject::tr("Wins/h (actif)"), QString::number(avgWinsPerHour, 'f', 2) }
    });
    const QString paceBlock = paceHtml.isEmpty()
        ? QString()
        : QString("<div style='margin-top:8px;'>%1</div>").arg(paceHtml);
    return tbl + paceBlock;
}

void Leaderboard::onRefreshClicked(MainWindow *this_, QListWidget *playerList)
{
    if(!this_ || !playerList) return;
    static quint64 refreshGeneration=0;
    const auto generation=++refreshGeneration;
    const auto region=AppSettings::region;
    const int edition=AppSettings::selectedEdition;
    WtApi::instance().get(WtApi::endpoint(edition,"get-top100",region),playerList,
        [this_,playerList,region,edition,generation](const QByteArray &bytes,const QString &error) {
    if(generation!=refreshGeneration || region!=AppSettings::region || edition!=AppSettings::selectedEdition) return;
    auto ladder=WtData::normalize(QJsonDocument::fromJson(bytes),"top");
    if(!error.isEmpty() || !ladder.value("top").isArray()) {
        playerList->setToolTip(QObject::tr("Refresh failed; previous snapshot retained. %1").arg(error));return;
    }
    if(AppSettings::hideNegativeTimes) WtData::filterNegativeHours(ladder);
    QString selectedId;
    if(auto *selected=playerList->currentItem()) selectedId=selected->data(Qt::UserRole).toJsonObject().value("id").toVariant().toString();
    const QString baseName=Leaderboard::currentSelectedName;
    const auto overlays=Leaderboard::overlayNames;
    playerList->setUpdatesEnabled(false);
    playerList->clear();
    playerList->setToolTip(QObject::tr("Snapshot received %1").arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
    playerList->setUniformItemSizes(true);
    playerList->setSpacing(3);
    playerList->setSelectionMode(QAbstractItemView::SingleSelection);

    if (ladder.contains("top") && ladder["top"].isArray()) {
        QJsonArray jsonArray = ladder["top"].toArray();

        struct RowData { int rank; QString name; qint64 lastPoints; QStringList pointsList; QJsonObject obj; };
        QVector<RowData> rows; rows.reserve(jsonArray.size());

        for (const QJsonValue &value : jsonArray) {
            QJsonObject obj = value.toObject();

            const QString ranksStr = obj["ranks"].toString().remove("[").remove("]");
            const QStringList ranksVals = ranksStr.split(",", Qt::SkipEmptyParts);
            int rank = ranksVals.isEmpty() ? 0 : ranksVals.last().trimmed().toInt();

            const QString name = obj["name"].toString();

            const QString pointsStr = obj["points"].toString().remove("[").remove("]");
            const QStringList pointsList = pointsStr.split(",", Qt::SkipEmptyParts);

            qint64 lastPoints = 0;
            if (!pointsList.isEmpty()) {
                bool ok = false;
                lastPoints = pointsList.last().trimmed().toLongLong(&ok);
                if (!ok) lastPoints = 0;
            }

            rows.push_back({rank, name, lastPoints, pointsList, obj});
        }

        std::sort(rows.begin(), rows.end(), [](const RowData& a, const RowData& b){ return a.rank < b.rank; });

        // NEW: fill snapshot
        snapshotRows.clear();
        snapshotRows.reserve(rows.size());

        for (int i = 0; i < rows.size(); ++i) {
            const auto& r = rows[i];

            const auto samples=Performance::samples(r.obj);
            const auto activity=Performance::summarize(samples);
            const bool isAfk=activity.trailingIdleHours>0;
            double farmHours=0;
            for(int j=samples.size()-1;j>0;--j) {
                const double dt=samples[j].hour-samples[j-1].hour;
                if(dt<=0 || dt>0.75 || samples[j].points<=samples[j-1].points) break;
                farmHours+=dt;
            }
            const QString statusText=activity.valid
                ? (isAfk?QString("AFK %1").arg(formatEtaFromHours(activity.trailingIdleHours)):QString("Farm %1").arg(formatEtaFromHours(farmHours)))
                : QObject::tr("Unknown");

            // Gap with below only (current - below)
            QString gapBelowLabel;
            qint64 gapBelow = 0;
            if (i + 1 < rows.size()) {
                gapBelow = r.lastPoints - rows[i+1].lastPoints;
                if (gapBelow > 0) gapBelowLabel = QString("+%1").arg(formatMillions(gapBelow));
            }

            // NEW: delta gap (last step) with compact display and color
            QString gapDeltaLabel = "0";
            QString gapDeltaColor = "#A0A0A0";
            if (i + 1 < rows.size()) {
                const auto& b = rows[i+1];
                auto prevVal = [&](const QStringList& lst)->qint64 {
                    if (lst.size() >= 2) return lst.at(lst.size()-2).trimmed().toLongLong();
                    return lst.isEmpty() ? 0 : lst.last().trimmed().toLongLong();
                };
                const qint64 prevA = prevVal(r.pointsList);
                const qint64 prevB = prevVal(b.pointsList);
                const qint64 prevGap = prevA - prevB;
                const qint64 deltaGap = gapBelow - prevGap;
                gapDeltaLabel = formatMillionsSignedCompact(deltaGap); // NEW
                if (deltaGap > 0) gapDeltaColor = "#2ECC71"; // green = on creuse
                else if (deltaGap < 0) gapDeltaColor = "#E74C3C"; // red = on perd
            }

            // NEW: trigger flames for current farm >= 24h (96 steps of 15min)
            const bool heavyFarm = farmHours >= 24;

            QWidget* widget = createPlayerRowWidget(
                r.rank, r.name, r.lastPoints, gapBelowLabel, isAfk, statusText,
                gapDeltaLabel, gapDeltaColor,
                heavyFarm
            );

            auto* item = new QListWidgetItem(playerList);
            item->setData(Qt::UserRole, r.obj);
            item->setSizeHint(QSize(playerList->viewport()->width(), 80));
            playerList->addItem(item);
            playerList->setItemWidget(item, widget);

            // snapshot item
            snapshotRows.push_back({r.rank, r.name, r.lastPoints, r.pointsList, r.obj});
        }

        // Fix: disconnect only what we rewire (avoid breaking other handlers)
        QObject::disconnect(playerList, &QListWidget::itemClicked, nullptr, nullptr);
        QObject::disconnect(playerList, &QListWidget::itemDoubleClicked, nullptr, nullptr);
        QObject::disconnect(playerList, &QListWidget::customContextMenuRequested, nullptr, nullptr);

        QObject::connect(playerList, &QListWidget::itemDoubleClicked, playerList, [this_, playerList](QListWidgetItem* item){
            if (!item) return;
            QJsonObject user = item->data(Qt::UserRole).toJsonObject();
            Leaderboard::affichergraphiqueettexte(this_, user);
        });

        // NEW: also react on single click to show infos immediately
        QObject::connect(playerList, &QListWidget::itemClicked, playerList, [this_, playerList](QListWidgetItem* item){
            if (!item) return;
            QJsonObject user = item->data(Qt::UserRole).toJsonObject();
            Leaderboard::affichergraphiqueettexte(this_, user);
        });

        // Right-click context menu on the list to add overlay series
        playerList->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(playerList, &QListWidget::customContextMenuRequested, playerList,
                         [this_, playerList](const QPoint& pos){
            QListWidgetItem* item = playerList->itemAt(pos);
            if (!item) return;
            QJsonObject user = item->data(Qt::UserRole).toJsonObject();
            const QString name = user["name"].toString();
            const QString hoursStr = user["hour"].toString();
            const QString paceStr  = user["wins_pace"].toString();

            QMenu menu(playerList);
            QAction* actAdd = menu.addAction(QString(tr("Rajouter %1 au graphique").arg(name)));

            const bool alreadyAdded = Leaderboard::overlayNames.contains(name);
            const bool isBase = (!Leaderboard::baseSeriesName.isEmpty() && Leaderboard::baseSeriesName == name);
            if ((Leaderboard::overlayNames.size() >= 4 && !alreadyAdded) || isBase || name.isEmpty()) {
                actAdd->setEnabled(false);
            }

            QObject::connect(actAdd, &QAction::triggered, playerList, [this_, name, hoursStr, paceStr, user](){
                // If no chart yet, make this selection the base
                QChart* chart = Render::chartFromView(Leaderboard::graphPlaceholder);
                // Parse rank from 'user'
                int r = 0;
                const QString rs = user["ranks"].toString().remove("[").remove("]");
                const QStringList rv = rs.split(",", Qt::SkipEmptyParts);
                if (!rv.isEmpty()) r = rv.last().trimmed().toInt();

                if (!chart) {
                    QJsonObject u; u["name"] = name; u["hour"] = hoursStr; u["wins_pace"] = paceStr; u["ranks"] = user["ranks"];
                    Leaderboard::affichergraphiqueettexte(this_, u);
                    return;
                }
                // Otherwise, add as overlay (with rank)
                if (Render::addSeriesToExistingChart(Leaderboard::graphPlaceholder, hoursStr, paceStr, name, r)) {
                    Leaderboard::overlayNames.insert(name);
                }
            });

            menu.exec(playerList->mapToGlobal(pos));
        });

        // Chart right-click to remove overlays
        if (Leaderboard::graphPlaceholder) {
            QObject::disconnect(Leaderboard::graphPlaceholder, &QWidget::customContextMenuRequested, nullptr, nullptr);
            Leaderboard::graphPlaceholder->setContextMenuPolicy(Qt::CustomContextMenu);
            QObject::connect(Leaderboard::graphPlaceholder, &QWidget::customContextMenuRequested,
                             Leaderboard::graphPlaceholder, [this_](const QPoint& pos){
                if (!Leaderboard::graphPlaceholder) return;
                QMenu menu(Leaderboard::graphPlaceholder);
                if (Leaderboard::overlayNames.isEmpty()) {
                    QAction* none = menu.addAction(tr("Aucun joueur ajouté"));
                    none->setEnabled(false);
                } else {
                    for (const QString& name : std::as_const(Leaderboard::overlayNames)) {
                        QAction* act = menu.addAction(tr("Supprimer %1").arg(name));
                        QObject::connect(act, &QAction::triggered, Leaderboard::graphPlaceholder, [name](){
                            if (Render::removeSeriesByName(Leaderboard::graphPlaceholder, name)) {
                                Leaderboard::overlayNames.remove(name);
                            }
                        });
                    }
                    menu.addSeparator();
                    QAction* actClear = menu.addAction(tr("Supprimer tout le monde"));
                    QObject::connect(actClear, &QAction::triggered, Leaderboard::graphPlaceholder, [](){
                        Render::clearAllOverlaySeries(Leaderboard::graphPlaceholder, Leaderboard::baseSeriesName);
                        Leaderboard::overlayNames.clear();
                    });
                }
                QPoint globalPos = Leaderboard::graphPlaceholder->viewport()->mapToGlobal(pos);
                menu.exec(globalPos);
            });
        }
    }


    playerList->setUpdatesEnabled(true);
    QJsonObject selected;
    for(int i=0;i<playerList->count();++i) {
        auto *item=playerList->item(i);const auto user=item->data(Qt::UserRole).toJsonObject();
        if(!selectedId.isEmpty() && user.value("id").toVariant().toString()==selectedId) {
            playerList->setCurrentItem(item);selected=user;break;
        }
    }
    if(!selected.isEmpty()) {
        Leaderboard::affichergraphiqueettexte(this_,selected);
        for(const auto &name:overlays) {
            QJsonObject match;int count=0;
            for(int i=0;i<playerList->count();++i) {
                const auto user=playerList->item(i)->data(Qt::UserRole).toJsonObject();
                if(user.value("name").toString()==name) {match=user;++count;}
            }
            // Legacy overlays use names: ambiguous matches are deliberately not restored.
            if(count==1 && name!=selected.value("name").toString() && Render::addSeriesToExistingChart(
                Leaderboard::graphPlaceholder,match.value("hour").toString(),match.value("wins_pace").toString(),name))
                Leaderboard::overlayNames.insert(name);
        }
    } else if(!baseName.isEmpty()) {
        if(Leaderboard::graphPlaceholder && Leaderboard::graphPlaceholder->scene()) Leaderboard::graphPlaceholder->scene()->clear();
        Leaderboard::currentSelectedName.clear();Leaderboard::overlayNames.clear();
    }
    },0); // Explicit/automatic refresh always bypasses cache, while in-flight requests coalesce.
}

void Leaderboard::affichergraphiqueettexte(MainWindow * this_, QJsonObject user, bool preserveOverlays)
{
    // Afficher les données du joueur sélectionné
    QString ex = user["ranks"].toString().remove("[").remove("]");
    QStringList values = ex.split(",", Qt::SkipEmptyParts);
    QString last_ranks = values.isEmpty() ? QStringLiteral("0") : values.last().trimmed();
    int baseRank = last_ranks.toInt();
    const QString name = user["name"].toString();
    Leaderboard::currentSelectedName = name; // pour auto-refresh
    std::cout << "Utilisateur sélectionné : " << name.toStdString() << std::endl;

    QString ydata = "wins_pace";

    // Récup données pour le graphe
    QString hours = user["hour"].toString();
    QString points = user[ydata].toString();

    // Base series + overlays (préserver si demandé)
    Leaderboard::baseSeriesName = name;
    if (!preserveOverlays) {
        Leaderboard::overlayNames.clear();
    }
    Render::render_leaderboard(this_, Leaderboard::graphPlaceholder, hours, points, ydata, name, baseRank);

    // Dernières valeurs formatées
    QString last_points = user["points"].toString().remove("[").remove("]").split(",").last().trimmed();
    last_points = QString::number(last_points.toLongLong()).replace(QRegularExpression("(\\d)(?=(\\d{3})+(?!\\d))"), "\\1,");
    QString last_wins = user["wins"].toString().remove("[").remove("]").split(",").last().trimmed();
    QString last_hours = user["hour"].toString().remove("[").remove("]").split(",").last().trimmed();

    const auto activity=Performance::summarize(Performance::samples(user));
    const QStringList pointsSteps = user["points"].toString().remove("[").remove("]").split(",", Qt::SkipEmptyParts);
    // Listes pour stats
    const QStringList winsList = user["wins"].toString().remove("[").remove("]").split(",", Qt::SkipEmptyParts);
    const QStringList paceList = user["wins_pace"].toString().remove("[").remove("]").split(",", Qt::SkipEmptyParts);

    const auto sampleHours=WtData::numbers(user.value("hour"));
    // Compte des pas actifs, AFK et wins gagnées uniquement sur pas actifs
    int afkSteps = 0, activeSteps = 0;
    qint64 activeWinsGained = 0;
    QMap<int,int> paceCounts; // pace entier -> occurrences sur pas actifs

    for (int i = 1; i < pointsSteps.size(); ++i) {
        qint64 curP  = pointsSteps.at(i).trimmed().toLongLong();
        qint64 prevP = pointsSteps.at(i-1).trimmed().toLongLong();
        if (curP == prevP) {
            afkSteps++;
        } else if (curP > prevP) {
            activeSteps++;
            if (i < winsList.size() && i < sampleHours.size() && sampleHours[i]-sampleHours[i-1]>0 && sampleHours[i]-sampleHours[i-1]<=0.75) {
                qint64 curW  = winsList.at(i).trimmed().toLongLong();
                qint64 prevW = winsList.at(i-1).trimmed().toLongLong();
                activeWinsGained += qMax<qint64>(0, curW - prevW);
            }
            if (i < paceList.size()) {
                bool ok = false;
                double paceVal = paceList.at(i).trimmed().toDouble(&ok);
                if (ok) {
                    int paceInt = static_cast<int>(paceVal + 1e-9);
                    paceCounts[paceInt] += 1;
                }
            }
        }
    }

    const QString afkStr = formatEtaFromHours(activity.idleHours);
    const QString activeStr = formatEtaFromHours(activity.activeHours);
    const double activeHours = activity.activeHours;
    const double avgWinsPerHour = (activeHours > 0.0) ? (static_cast<double>(activeWinsGained) / activeHours) : 0.0;

    // Meilleures paces: top pace et pace-1
    int topPace = 0;
    for (auto it = paceCounts.constBegin(); it != paceCounts.constEnd(); ++it) {
        if (it.key() > topPace) topPace = it.key();
    }
    const int secondPace = (topPace > 0) ? (topPace - 1) : 0;
    const int countTop = paceCounts.value(topPace, 0);
    const int countSecond = paceCounts.value(secondPace, 0);
    const int totalActiveSteps = qMax(1, activeSteps); // éviter /0
    const double pctTop = (countTop * 100.0) / totalActiveSteps;
    const double pctSecond = (countSecond * 100.0) / totalActiveSteps;

    // Styliser et remplir les 3 blocs
    ensureInfoLabelsStyled();

    // INFOS
    if (Leaderboard::dataLeftPlaceholder && Leaderboard::dataRightPlaceholder) {
        QStringList left = { tr("Nom"), tr("Rank"), tr("Wins"), tr("Points totaux"), tr("Heures AFK") };
        QStringList right = { name, last_ranks, last_wins, last_points, afkStr };
        setTwoColumnText(Leaderboard::dataLeftPlaceholder, Leaderboard::dataRightPlaceholder, left, right);
    }

    // INFOS MOYENNE (+ deux meilleures paces)
    if (Leaderboard::avgLeftPlaceholder && Leaderboard::avgRightPlaceholder) {
        QStringList left = { tr("Non-AFK"), tr("AFK"), tr("Wins/h (actif)") };
        QStringList right = { activeStr, afkStr, QString::number(avgWinsPerHour, 'f', 2) };
        if (topPace > 0) {
            left  << QString("Pace %1").arg(topPace)    << QString("Pace %1").arg(secondPace);
            right << QString("%1% (%2/%3)").arg(QString::number(pctTop, 'f', 1)).arg(countTop).arg(totalActiveSteps)
                  << QString("%1% (%2/%3)").arg(QString::number(pctSecond, 'f', 1)).arg(countSecond).arg(totalActiveSteps);
        }
        setTwoColumnText(Leaderboard::avgLeftPlaceholder, Leaderboard::avgRightPlaceholder, left, right);
    }

    // GAP: Au-dessus et En-dessous (nom + gap + Δ + ETA)
    if (Leaderboard::gapLeftPlaceholder && Leaderboard::gapRightPlaceholder && !snapshotRows.isEmpty()) {
        // Rank courant
        const int rank = [&]{
            QString ex = user["ranks"].toString().remove("[").remove("]");
            const QStringList vals = ex.split(",", Qt::SkipEmptyParts);
            return vals.isEmpty() ? 0 : vals.last().trimmed().toInt();
        }();

        // Position dans le snapshot
        int idx = -1;
        for (int i = 0; i < snapshotRows.size(); ++i) {
            if (snapshotRows[i].rank == rank) { idx = i; break; }
        }

        auto prevVal = [&](const QStringList& lst)->qint64 {
            if (lst.size() >= 2) return lst.at(lst.size()-2).trimmed().toLongLong();
            return lst.isEmpty() ? 0 : lst.last().trimmed().toLongLong();
        };

        QStringList left, right;
        if (idx >= 0) {
            const auto& me = snapshotRows[idx];

            // Au-dessus
            if (idx - 1 >= 0) {
                const auto& up = snapshotRows[idx-1];
                const qint64 gapUp = up.lastPoints - me.lastPoints;
                const qint64 prevGapUp = prevVal(up.pointsList) - prevVal(me.pointsList);
                const qint64 dGapUp = gapUp - prevGapUp;
                const double dvUp = computePointsPerHour(me.player) - computePointsPerHour(up.player);
                const QString etaUp = (std::abs(lastHour(me.player)-lastHour(up.player))<=0.05 && dvUp > 1e-6 && gapUp > 0 && gapUp/dvUp <= this_->remainingTournamentHours()) ? formatEtaFromHours(gapUp / dvUp) : QString("—");

                left  << tr("Au-dessus") << tr("Gap") << tr("Rattraper");
                right << QString("#%1 %2").arg(up.rank).arg(up.name)
                      << QString("%1 (%2)").arg(formatMillions(gapUp)).arg(formatMillionsSigned(dGapUp))
                      << etaUp;
            } else {
                left  << tr("Au-dessus");
                right << tr("Aucun");
            }

            // Ligne vide séparatrice
            left  << "";
            right << "";

            // En-dessous
            if (idx + 1 < snapshotRows.size()) {
                const auto& down = snapshotRows[idx+1];
                const qint64 gapDown = me.lastPoints - down.lastPoints;
                const qint64 prevGapDown = prevVal(me.pointsList) - prevVal(down.pointsList);
                const qint64 dGapDown = gapDown - prevGapDown;
                const double dvDown = computePointsPerHour(down.player) - computePointsPerHour(me.player);
                const QString etaDown = (std::abs(lastHour(me.player)-lastHour(down.player))<=0.05 && dvDown > 1e-6 && gapDown > 0 && gapDown/dvDown <= this_->remainingTournamentHours()) ? formatEtaFromHours(gapDown / dvDown) : QString("—");

                left  << tr("En-dessous") << tr("Gap") << tr("Se faire rattraper");
                right << QString("#%1 %2").arg(down.rank).arg(down.name)
                      << QString("%1 (%2)").arg(formatMillions(gapDown)).arg(formatMillionsSigned(dGapDown))
                      << etaDown;
            } else {
                left  << tr("En-dessous");
                right << tr("Aucun");
            }
        }

        setTwoColumnText(Leaderboard::gapLeftPlaceholder, Leaderboard::gapRightPlaceholder, left, right);
    }

    std::cout << "Utilisateur sélectionné : " << name.toStdString() << std::endl;
}

// NEW: auto-refresh that preserves overlays and updates list + chart + infos
void Leaderboard::autoRefresh(MainWindow *this_)
{
    onRefreshClicked(this_,Leaderboard::playerListPtr);
}

void Leaderboard::onPlayerDoubleClicked(QListWidgetItem *item)
{
    // Afficher plus de détails du joueur sélectionné
}