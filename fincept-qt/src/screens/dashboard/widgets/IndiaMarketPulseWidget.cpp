#include "screens/dashboard/widgets/IndiaMarketPulseWidget.h"
#include "ui/theme/Theme.h"
#include "datahub/DataHub.h"
#include "datahub/DataHubMetaTypes.h"

#include <QVBoxLayout>

namespace fincept::screens::widgets {

IndiaMarketPulseWidget::IndiaMarketPulseWidget(QWidget* parent)
    : BaseWidget(tr("INDIA MARKET PULSE"), parent, ui::colors::CYAN) {

    auto* vl = content_layout();
    vl->setContentsMargins(12, 12, 12, 12);
    vl->setSpacing(10);

    // Indices summary
    auto* idx_row = new QWidget;
    auto* il = new QHBoxLayout(idx_row);
    il->setContentsMargins(0, 0, 0, 0);

    auto make_idx = [&](const QString& name, QLabel*& val_out) {
        auto* col = new QVBoxLayout;
        auto* l = new QLabel(name);
        l->setStyleSheet(QString("color: %1; font-size: 9px; font-weight: bold;").arg(ui::colors::TEXT_TERTIARY()));
        val_out = new QLabel("--");
        val_out->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold;").arg(ui::colors::TEXT_PRIMARY()));
        col->addWidget(l);
        col->addWidget(val_out);
        il->addLayout(col);
    };

    make_idx("NIFTY 50", nifty_val_);
    il->addSpacing(20);
    make_idx("SENSEX", sensex_val_);
    il->addStretch();
    vl->addWidget(idx_row);

    // Advance/Decline Bar
    auto* ad_section = new QWidget;
    auto* al = new QVBoxLayout(ad_section);
    al->setContentsMargins(0, 5, 0, 5);
    al->setSpacing(4);

    auto* ad_hdr = new QHBoxLayout;
    auto* ad_title = new QLabel(tr("ADVANCE / DECLINE"));
    ad_title->setStyleSheet(QString("color: %1; font-size: 9px; font-weight: bold;").arg(ui::colors::TEXT_TERTIARY()));
    ad_hdr->addWidget(ad_title);
    ad_hdr->addStretch();
    ad_label_ = new QLabel("0 : 0");
    ad_label_->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: bold;").arg(ui::colors::TEXT_SECONDARY()));
    ad_hdr->addWidget(ad_label_);
    al->addLayout(ad_hdr);

    ad_bar_ = new QProgressBar;
    ad_bar_->setTextVisible(false);
    ad_bar_->setFixedHeight(8);
    ad_bar_->setRange(0, 100);
    ad_bar_->setValue(50);
    ad_bar_->setStyleSheet(QString(
        "QProgressBar { background: %1; border: none; border-radius: 4px; }"
        "QProgressBar::chunk { background: %2; border-radius: 4px; }")
        .arg(ui::colors::NEGATIVE(), ui::colors::POSITIVE()));
    al->addWidget(ad_bar_);
    vl->addWidget(ad_section);

    // Volume Table
    auto* vol_title = new QLabel(tr("TOP VOLUME GAINERS (NSE)"));
    vol_title->setStyleSheet(QString("color: %1; font-size: 9px; font-weight: bold; margin-top: 5px;")
                             .arg(ui::colors::TEXT_TERTIARY()));
    vl->addWidget(vol_title);

    volume_table_ = new ui::DataTable;
    volume_table_->set_headers({tr("SYMBOL"), tr("LTP"), tr("VOL CHG%")});
    volume_table_->set_column_widths({120, 80, 80});
    vl->addWidget(volume_table_, 1);

    connect(this, &BaseWidget::refresh_requested, this, &IndiaMarketPulseWidget::refresh_data);
    set_loading(true);
}

void IndiaMarketPulseWidget::showEvent(QShowEvent* e) {
    BaseWidget::showEvent(e);
    if (!hub_active_)
        hub_resubscribe();
}

void IndiaMarketPulseWidget::hideEvent(QHideEvent* e) {
    BaseWidget::hideEvent(e);
    // Keep active to maintain the pulse
}

void IndiaMarketPulseWidget::refresh_data() {
    QStringList topics = {"market:quote:^NSEI", "market:quote:^BSESN"};
    datahub::DataHub::instance().request(topics, true);
}

void IndiaMarketPulseWidget::hub_resubscribe() {
    auto& hub = datahub::DataHub::instance();

    hub.subscribe(this, "market:quote:^NSEI", [this](const QVariant& v) {
        if (!v.canConvert<services::QuoteData>()) return;
        auto q = v.value<services::QuoteData>();
        nifty_val_->setText(QString::number(q.price, 'f', 2));
        nifty_val_->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold;")
                                  .arg(ui::change_color(q.change_pct)));
        set_loading(false);
    });

    hub.subscribe(this, "market:quote:^BSESN", [this](const QVariant& v) {
        if (!v.canConvert<services::QuoteData>()) return;
        auto q = v.value<services::QuoteData>();
        sensex_val_->setText(QString::number(q.price, 'f', 2));
        sensex_val_->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold;")
                                   .arg(ui::change_color(q.change_pct)));
    });

    // Mock AD ratio and Volume Gainers for now as we don't have a specific producer for this aggregate yet
    // Real implementation would subscribe to a new topic like "market:india:pulse"
    ad_label_->setText("1624 : 842");
    ad_bar_->setValue(66);

    volume_table_->clear_data();
    volume_table_->add_row({"RELIANCE.NS", "2,942.50", "+412%"});
    volume_table_->add_row({"HDFCBANK.NS", "1,645.20", "+285%"});
    volume_table_->add_row({"TCS.NS", "4,120.00", "+195%"});
    volume_table_->set_cell_color(0, 2, ui::colors::POSITIVE());
    volume_table_->set_cell_color(1, 2, ui::colors::POSITIVE());
    volume_table_->set_cell_color(2, 2, ui::colors::POSITIVE());

    hub_active_ = true;
}

} // namespace fincept::screens::widgets
