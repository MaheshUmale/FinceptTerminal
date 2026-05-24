#pragma once
#include "screens/dashboard/widgets/BaseWidget.h"
#include "trading/TradingTypes.h"

#include <QHash>
#include <QLabel>

namespace fincept::screens::widgets {

/// Consolidated wealth view across all connected brokers (Upstox, Dhan, Zerodha).
/// Subscribes to `broker:*:*:balance` and `broker:*:*:positions`.
class MasterPortfolioWidget : public BaseWidget {
    Q_OBJECT
  public:
    explicit MasterPortfolioWidget(QWidget* parent = nullptr);

  protected:
    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;

  private:
    void update_ui();
    void apply_styles();
    void hub_resubscribe();
    void hub_unsubscribe_all();

    struct BrokerWealth {
        double balance = 0;
        double unrealized_pnl = 0;
        double day_pnl = 0;
    };

    QHash<QString, BrokerWealth> wealth_by_account_;
    bool hub_active_ = false;

    QLabel* total_wealth_label_ = nullptr;
    QLabel* total_pnl_label_ = nullptr;
    QLabel* day_pnl_label_ = nullptr;

    ui::DataTable* table_ = nullptr;
};

} // namespace fincept::screens::widgets
