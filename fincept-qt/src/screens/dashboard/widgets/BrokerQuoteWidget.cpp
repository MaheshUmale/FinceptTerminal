#include "screens/dashboard/widgets/BrokerQuoteWidget.h"
#include "ui/theme/Theme.h"
#include "datahub/DataHub.h"
#include "datahub/DataHubMetaTypes.h"
#include "trading/BrokerTopic.h"
#include "trading/AccountManager.h"

#include <QFrame>
#include <QJsonArray>

namespace fincept::screens::widgets {

BrokerQuoteWidget::BrokerQuoteWidget(const QJsonObject& config, QWidget* parent)
    : BaseWidget(tr("BROKER QUOTE"), parent, ui::colors::INFO) {

    // Default to the first active account if none provided
    auto accounts = trading::AccountManager::instance().list_accounts();
    if (config.contains("account_id")) {
        account_id_ = config.value("account_id").toString();
        broker_id_ = config.value("broker_id").toString();
    } else if (!accounts.isEmpty()) {
        account_id_ = accounts.first().account_id;
        broker_id_ = accounts.first().broker_id;
    }

    symbol_ = config.value("symbol").toString("RELIANCE");
    set_title(tr("%1: %2").arg(broker_id_.toUpper(), symbol_.toUpper()));

    auto* vl = content_layout();
    vl->setContentsMargins(12, 8, 12, 8);
    vl->setSpacing(8);

    // ── Price row ──
    auto* price_row = new QWidget(this);
    auto* prl = new QHBoxLayout(price_row);
    prl->setContentsMargins(0, 0, 0, 0);
    prl->setSpacing(8);

    price_label_ = new QLabel("--");
    prl->addWidget(price_label_);

    auto* change_col = new QWidget(this);
    auto* ccl = new QVBoxLayout(change_col);
    ccl->setContentsMargins(0, 4, 0, 0);
    ccl->setSpacing(0);

    arrow_label_ = new QLabel;
    ccl->addWidget(arrow_label_);

    change_label_ = new QLabel("--");
    ccl->addWidget(change_label_);

    prl->addWidget(change_col);
    prl->addStretch();

    ticker_label_ = new QLabel(symbol_);
    prl->addWidget(ticker_label_);

    vl->addWidget(price_row);

    // ── Separator ──
    sep_ = new QFrame;
    sep_->setFixedHeight(1);
    vl->addWidget(sep_);

    // ── Stats grid ──
    auto* stats = new QWidget(this);
    auto* gl = new QGridLayout(stats);
    gl->setContentsMargins(0, 0, 0, 0);
    gl->setSpacing(6);

    auto make_stat = [&](int row, int col, const QString& label, QLabel*& val_out) {
        auto* cell = new QWidget(this);
        stat_cells_.append(cell);
        auto* cl = new QVBoxLayout(cell);
        cl->setContentsMargins(8, 6, 8, 6);
        cl->setSpacing(2);

        auto* lbl = new QLabel(label);
        stat_labels_.append(lbl);
        cl->addWidget(lbl);

        val_out = new QLabel("--");
        stat_values_.append(val_out);
        cl->addWidget(val_out);

        gl->addWidget(cell, row, col);
    };

    make_stat(0, 0, tr("BID"), bid_val_);
    make_stat(0, 1, tr("ASK"), ask_val_);
    make_stat(1, 0, tr("HIGH"), high_val_);
    make_stat(1, 1, tr("LOW"), low_val_);
    make_stat(2, 0, tr("VOLUME"), volume_val_);

    vl->addWidget(stats);
    vl->addStretch();

    connect(this, &BaseWidget::refresh_requested, this, &BrokerQuoteWidget::refresh_data);

    apply_styles();
    set_loading(true);
}

void BrokerQuoteWidget::apply_styles() {
    price_label_->setStyleSheet(QString("color: %1; font-size: 28px; font-weight: bold; background: transparent;")
                                    .arg(ui::colors::TEXT_PRIMARY()));
    arrow_label_->setStyleSheet("font-size: 12px; background: transparent;");
    change_label_->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold; background: transparent;")
                                     .arg(ui::colors::TEXT_SECONDARY()));
    ticker_label_->setStyleSheet(
        QString("color: %1; font-size: 14px; font-weight: bold; background: transparent;").arg(ui::colors::AMBER()));
    sep_->setStyleSheet(QString("background: %1;").arg(ui::colors::BORDER_DIM()));

    for (auto* cell : stat_cells_)
        cell->setStyleSheet(QString("background: %1; border-radius: 2px;").arg(ui::colors::BG_RAISED()));
    for (auto* lbl : stat_labels_)
        lbl->setStyleSheet(
            QString("color: %1; font-size: 9px; background: transparent;").arg(ui::colors::TEXT_TERTIARY()));
    for (auto* val : stat_values_)
        val->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold; background: transparent;")
                               .arg(ui::colors::TEXT_PRIMARY()));
}

void BrokerQuoteWidget::on_theme_changed() {
    apply_styles();
}

void BrokerQuoteWidget::showEvent(QShowEvent* e) {
    BaseWidget::showEvent(e);
    if (!hub_active_)
        hub_resubscribe();
}

void BrokerQuoteWidget::hideEvent(QHideEvent* e) {
    BaseWidget::hideEvent(e);
    if (hub_active_)
        hub_unsubscribe_all();
}

void BrokerQuoteWidget::refresh_data() {
    if (account_id_.isEmpty()) return;
    const QString topic = trading::broker_topic(broker_id_, account_id_, QStringLiteral("quote"), symbol_);
    datahub::DataHub::instance().request(topic, true);
}

void BrokerQuoteWidget::hub_resubscribe() {
    if (account_id_.isEmpty()) return;
    auto& hub = datahub::DataHub::instance();
    hub.unsubscribe(this);
    const QString topic = trading::broker_topic(broker_id_, account_id_, QStringLiteral("quote"), symbol_);
    hub.subscribe(this, topic, [this](const QVariant& v) {
        if (!v.canConvert<trading::BrokerQuote>())
            return;
        set_loading(false);
        populate(v.value<trading::BrokerQuote>());
    });
    hub_active_ = true;
}

void BrokerQuoteWidget::hub_unsubscribe_all() {
    datahub::DataHub::instance().unsubscribe(this);
    hub_active_ = false;
}

void BrokerQuoteWidget::populate(const trading::BrokerQuote& q) {
    QString currency_sym = broker_id_ == "alpaca" ? "$" : "₹";
    price_label_->setText(QString("%1%2").arg(currency_sym).arg(q.ltp, 0, 'f', 2));

    bool positive = q.change_pct >= 0;
    QString color = positive ? ui::colors::POSITIVE() : ui::colors::NEGATIVE();

    arrow_label_->setText(positive ? QString(QChar(0x25B2)) : QString(QChar(0x25BC)));
    arrow_label_->setStyleSheet(QString("color: %1; font-size: 12px; background: transparent;").arg(color));

    change_label_->setText(QString("%1%2 (%3%4%)")
                               .arg(positive ? "+" : "")
                               .arg(q.change, 0, 'f', 2)
                               .arg(positive ? "+" : "")
                               .arg(q.change_pct, 0, 'f', 2));
    change_label_->setStyleSheet(
        QString("color: %1; font-size: 11px; font-weight: bold; background: transparent;").arg(color));

    price_label_->setStyleSheet(
        QString("color: %1; font-size: 28px; font-weight: bold; background: transparent;").arg(color));

    auto fmt = [&](double v) { return v > 0 ? QString("%1%2").arg(currency_sym).arg(v, 0, 'f', 2) : QString("--"); };
    bid_val_->setText(fmt(q.bid));
    ask_val_->setText(fmt(q.ask));
    high_val_->setText(fmt(q.high));
    low_val_->setText(fmt(q.low));

    // Format volume
    if (q.volume >= 1e9)
        volume_val_->setText(QString("%1B").arg(q.volume / 1e9, 0, 'f', 1));
    else if (q.volume >= 1e6)
        volume_val_->setText(QString("%1M").arg(q.volume / 1e6, 0, 'f', 1));
    else if (q.volume >= 1e3)
        volume_val_->setText(QString("%1K").arg(q.volume / 1e3, 0, 'f', 1));
    else
        volume_val_->setText(QString::number(static_cast<int>(q.volume)));
}

} // namespace fincept::screens::widgets
