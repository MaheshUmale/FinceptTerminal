#pragma once
#include "screens/dashboard/widgets/BaseWidget.h"
#include "screens/equity_trading/EquityChart.h"
#include "trading/TradingTypes.h"

namespace fincept::screens::widgets {

/// Real-time broker chart widget for the dashboard.
/// Wraps the high-performance EquityChart and subscribes to broker candles.
class BrokerChartWidget : public BaseWidget {
    Q_OBJECT
  public:
    explicit BrokerChartWidget(const QJsonObject& config = {}, QWidget* parent = nullptr);

    QJsonObject config() const override;
    void apply_config(const QJsonObject& cfg) override;

  protected:
    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;

  private:
    void refresh_data();
    void hub_resubscribe();
    void hub_unsubscribe_all();

    QString broker_id_;
    QString account_id_;
    QString symbol_;
    QString timeframe_ = "15m";

    equity::EquityChart* chart_ = nullptr;
    bool hub_active_ = false;
};

} // namespace fincept::screens::widgets
