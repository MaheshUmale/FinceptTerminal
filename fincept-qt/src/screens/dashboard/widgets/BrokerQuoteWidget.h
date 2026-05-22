#pragma once
#include "screens/dashboard/widgets/BaseWidget.h"
#include "trading/TradingTypes.h"

#include <QGridLayout>
#include <QLabel>

namespace fincept::screens::widgets {

/// Real-time broker quote card — fetches data from connected Indian/Global brokers.
/// Shows live LTP, bid/ask, and depth from brokers like Upstox, Dhan, or Zerodha.
///
/// Subscribes to `broker:<id>:<account>:quote:<symbol>` on the DataHub.
class BrokerQuoteWidget : public BaseWidget {
    Q_OBJECT
  public:
    explicit BrokerQuoteWidget(const QJsonObject& config = {}, QWidget* parent = nullptr);

    QJsonObject config() const override;
    void apply_config(const QJsonObject& cfg) override;

  protected:
    void on_theme_changed() override;
    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;

  private:
    void apply_styles();
    void refresh_data();
    void populate(const trading::BrokerQuote& quote);
    void hub_resubscribe();
    void hub_unsubscribe_all();

    QString broker_id_;
    QString account_id_;
    QString symbol_;

    QLabel* price_label_ = nullptr;
    QLabel* change_label_ = nullptr;
    QLabel* arrow_label_ = nullptr;
    QLabel* ticker_label_ = nullptr;
    QFrame* sep_ = nullptr;
    QLabel* bid_val_ = nullptr;
    QLabel* ask_val_ = nullptr;
    QLabel* high_val_ = nullptr;
    QLabel* low_val_ = nullptr;
    QLabel* volume_val_ = nullptr;

    QVector<QWidget*> stat_cells_;
    QVector<QLabel*> stat_labels_;
    QVector<QLabel*> stat_values_;

    bool hub_active_ = false;
};

} // namespace fincept::screens::widgets
