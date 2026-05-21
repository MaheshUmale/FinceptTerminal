#include "screens/dashboard/widgets/BrokerChartWidget.h"
#include "datahub/DataHub.h"
#include "datahub/DataHubMetaTypes.h"
#include "trading/BrokerTopic.h"
#include "trading/AccountManager.h"

#include <QVBoxLayout>

namespace fincept::screens::widgets {

BrokerChartWidget::BrokerChartWidget(const QJsonObject& config, QWidget* parent)
    : BaseWidget(tr("BROKER CHART"), parent, ui::colors::INFO) {

    auto accounts = trading::AccountManager::instance().list_accounts();
    if (config.contains("account_id")) {
        account_id_ = config.value("account_id").toString();
        broker_id_ = config.value("broker_id").toString();
    } else if (!accounts.isEmpty()) {
        account_id_ = accounts.first().account_id;
        broker_id_ = accounts.first().broker_id;
    }

    symbol_ = config.value("symbol").toString("RELIANCE");
    timeframe_ = config.value("timeframe").toString("15m");
    set_title(tr("CHART: %1").arg(symbol_.toUpper()));

    auto* vl = content_layout();
    vl->setContentsMargins(0, 0, 0, 0);

    chart_ = new equity::EquityChart(this);
    vl->addWidget(chart_);

    connect(chart_, &equity::EquityChart::timeframe_changed, this, [this](const QString& tf) {
        timeframe_ = tf;
        hub_resubscribe();
        refresh_data();
    });

    connect(this, &BaseWidget::refresh_requested, this, &BrokerChartWidget::refresh_data);
    set_loading(true);
}

void BrokerChartWidget::showEvent(QShowEvent* e) {
    BaseWidget::showEvent(e);
    if (!hub_active_)
        hub_resubscribe();
}

void BrokerChartWidget::hideEvent(QHideEvent* e) {
    BaseWidget::hideEvent(e);
    if (hub_active_)
        hub_unsubscribe_all();
}

void BrokerChartWidget::refresh_data() {
    if (account_id_.isEmpty()) return;
    const QString topic = trading::broker_topic(broker_id_, account_id_, QStringLiteral("candles"), symbol_ + ":" + timeframe_);
    datahub::DataHub::instance().request(topic, true);
}

void BrokerChartWidget::hub_resubscribe() {
    if (account_id_.isEmpty()) return;
    auto& hub = datahub::DataHub::instance();
    hub.unsubscribe(this);
    const QString topic = trading::broker_topic(broker_id_, account_id_, QStringLiteral("candles"), symbol_ + ":" + timeframe_);
    hub.subscribe(this, topic, [this](const QVariant& v) {
        if (!v.canConvert<QVector<trading::BrokerCandle>>())
            return;
        set_loading(false);
        chart_->set_candles(v.value<QVector<trading::BrokerCandle>>());
    });
    hub_active_ = true;
}

void BrokerChartWidget::hub_unsubscribe_all() {
    datahub::DataHub::instance().unsubscribe(this);
    hub_active_ = false;
}

} // namespace fincept::screens::widgets
