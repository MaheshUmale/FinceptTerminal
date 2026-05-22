#include "screens/dashboard/widgets/MasterPortfolioWidget.h"
#include "ui/theme/Theme.h"
#include "datahub/DataHub.h"
#include "datahub/DataHubMetaTypes.h"
#include "trading/AccountManager.h"
#include "trading/BrokerRegistry.h"

#include <QVBoxLayout>

namespace fincept::screens::widgets {

MasterPortfolioWidget::MasterPortfolioWidget(QWidget* parent)
    : BaseWidget(tr("MASTER PORTFOLIO"), parent, ui::colors::AMBER) {

    auto* vl = content_layout();
    vl->setContentsMargins(12, 12, 12, 12);
    vl->setSpacing(10);

    // Summary header
    auto* summary = new QWidget;
    auto* sl = new QHBoxLayout(summary);
    sl->setContentsMargins(0, 0, 0, 0);

    auto make_summary_item = [&](const QString& label, QLabel*& val_out) {
        auto* col = new QVBoxLayout;
        auto* l = new QLabel(label);
        l->setStyleSheet(QString("color: %1; font-size: 9px; font-weight: bold;").arg(ui::colors::TEXT_TERTIARY()));
        val_out = new QLabel("₹0.00");
        val_out->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: bold;").arg(ui::colors::TEXT_PRIMARY()));
        col->addWidget(l);
        col->addWidget(val_out);
        sl->addLayout(col);
    };

    make_summary_item(tr("TOTAL WEALTH"), total_wealth_label_);
    sl->addSpacing(20);
    make_summary_item(tr("UNREALIZED P&L"), total_pnl_label_);
    sl->addSpacing(20);
    make_summary_item(tr("DAY P&L"), day_pnl_label_);
    sl->addStretch();

    vl->addWidget(summary);

    // Table
    table_ = new ui::DataTable;
    table_->set_headers({tr("ACCOUNT"), tr("BROKER"), tr("BALANCE"), tr("P&L")});
    table_->set_column_widths({120, 80, 100, 100});
    vl->addWidget(table_, 1);

    apply_styles();
}

void MasterPortfolioWidget::showEvent(QShowEvent* e) {
    BaseWidget::showEvent(e);
    if (!hub_active_)
        hub_resubscribe();
}

void MasterPortfolioWidget::hideEvent(QHideEvent* e) {
    BaseWidget::hideEvent(e);
    if (hub_active_)
        hub_unsubscribe_all();
}

void MasterPortfolioWidget::hub_resubscribe() {
    auto& hub = datahub::DataHub::instance();
    hub.unsubscribe(this);

    // Subscribe to balance for all brokers
    hub.subscribe_pattern(this, "broker:*:*:balance", [this](const QString& topic, const QVariant& v) {
        if (!v.canConvert<trading::BrokerFunds>()) return;
        const QStringList parts = topic.split(':');
        if (parts.size() < 3) return;
        const QString account_id = parts[2];
        wealth_by_account_[account_id].balance = v.value<trading::BrokerFunds>().total_balance;
        update_ui();
    });

    // Subscribe to positions for unrealized P&L
    hub.subscribe_pattern(this, "broker:*:*:positions", [this](const QString& topic, const QVariant& v) {
        if (!v.canConvert<QVector<trading::BrokerPosition>>()) return;
        const QStringList parts = topic.split(':');
        if (parts.size() < 3) return;
        const QString account_id = parts[2];

        double upnl = 0, dpnl = 0;
        for (const auto& pos : v.value<QVector<trading::BrokerPosition>>()) {
            upnl += pos.pnl;
            dpnl += pos.day_pnl;
        }
        wealth_by_account_[account_id].unrealized_pnl = upnl;
        wealth_by_account_[account_id].day_pnl = dpnl;
        update_ui();
    });

    hub_active_ = true;
    set_loading(false);
}

void MasterPortfolioWidget::hub_unsubscribe_all() {
    datahub::DataHub::instance().unsubscribe(this);
    hub_active_ = false;
}

void MasterPortfolioWidget::update_ui() {
    double total_wealth = 0, total_upnl = 0, total_dpnl = 0;
    table_->clear_data();

    auto accounts = trading::AccountManager::instance().list_accounts();
    for (const auto& account : accounts) {
        auto& w = wealth_by_account_[account.account_id];
        total_wealth += w.balance + w.unrealized_pnl;
        total_upnl += w.unrealized_pnl;
        total_dpnl += w.day_pnl;

        auto* broker = trading::BrokerRegistry::instance().get(account.broker_id);
        const QString bname = broker ? broker->name() : account.broker_id;

        table_->add_row({
            account.display_name,
            bname,
            QString("₹%1").arg(w.balance + w.unrealized_pnl, 0, 'f', 2),
            QString("₹%1").arg(w.unrealized_pnl, 0, 'f', 2)
        });
        int row = table_->rowCount() - 1;
        table_->set_cell_color(row, 3, ui::change_color(w.unrealized_pnl));
    }

    total_wealth_label_->setText(QString("₹%1").arg(total_wealth, 0, 'f', 2));
    total_pnl_label_->setText(QString("₹%1").arg(total_upnl, 0, 'f', 2));
    total_pnl_label_->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: bold;")
                                     .arg(ui::change_color(total_upnl)));

    day_pnl_label_->setText(QString("₹%1").arg(total_dpnl, 0, 'f', 2));
    day_pnl_label_->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: bold;")
                                     .arg(ui::change_color(total_dpnl)));
}

void MasterPortfolioWidget::apply_styles() {
    // Already set in constructor/update_ui
}

} // namespace fincept::screens::widgets
