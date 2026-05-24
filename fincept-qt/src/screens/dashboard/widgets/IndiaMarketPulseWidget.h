#pragma once
#include "screens/dashboard/widgets/BaseWidget.h"
#include "services/markets/MarketDataService.h"

#include <QLabel>
#include <QProgressBar>

namespace fincept::screens::widgets {

/// Indian Market Pulse widget for the dashboard.
/// Shows Advance/Decline ratio and top volume gainers for NSE.
class IndiaMarketPulseWidget : public BaseWidget {
    Q_OBJECT
  public:
    explicit IndiaMarketPulseWidget(QWidget* parent = nullptr);

  protected:
    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;

  private:
    void refresh_data();
    void hub_resubscribe();

    struct AdRatio {
        int advances = 0;
        int declines = 0;
        int unchanged = 0;
    };

    QLabel* nifty_val_ = nullptr;
    QLabel* sensex_val_ = nullptr;

    QProgressBar* ad_bar_ = nullptr;
    QLabel* ad_label_ = nullptr;

    ui::DataTable* volume_table_ = nullptr;

    bool hub_active_ = false;
};

} // namespace fincept::screens::widgets
