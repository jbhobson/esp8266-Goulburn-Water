#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "energy_statistics.h"

namespace esphome {
namespace energy_statistics {

static const char *const TAG = "energy_statistics";
static const char *const GAP = "  ";

void EnergyStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Energy statistics sensors");
  if (this->energy_today_) {
    LOG_SENSOR(GAP, "Energy Today", this->energy_today_);
  }
  if (this->energy_yesterday_) {
    LOG_SENSOR(GAP, "Energy Yesterday", this->energy_yesterday_);
  }
  if (this->energy_week_) {
    LOG_SENSOR(GAP, "Energy Week", this->energy_week_);
  }
  if (this->energy_month_) {
    LOG_SENSOR(GAP, "Energy Month", this->energy_month_);
  }
  if (this->energy_year_) {
    LOG_SENSOR(GAP, "Energy Year", this->energy_year_);
  }
}

void EnergyStatistics::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });

  this->pref_ = global_preferences->make_preference<energy_data_t>(fnv1_hash(TAG));

  energy_data_t loaded{};
  if (this->pref_.load(&loaded)) {
    this->energy_ = loaded;
  }

  const auto total = this->total_->get_state();
  if (!std::isnan(total)) {
    if (std::isnan(this->energy_.start_week)) {
      this->energy_.start_week = total;
    }
    if (std::isnan(this->energy_.start_month)) {
      this->energy_.start_month = total;
    }
    if (std::isnan(this->energy_.start_year)) {
      this->energy_.start_year = total;
    }
    this->process_(total);
  }
}


void EnergyStatistics::loop() {
  const auto t = this->time_->now();
  if (!t.is_valid()) {
    // time is not sync yet
    return;
  }

  const auto total = this->total_->get_state();
  if (std::isnan(total)) {
    // total is not published yet
    return;
  }

  // Only update if the day has changed (to avoid redundant calculations)
  if (t.day_of_year != this->energy_.current_day_of_year) {
    this->energy_.start_yesterday = this->energy_.start_today;
    this->energy_.start_today = total;

    // Handle new week start
    if (this->energy_.start_week == NAN || t.day_of_week == this->energy_week_start_day_) {
      this->energy_.start_week = total;
    }

    // Handle new month start
    if (this->energy_.start_month == NAN || t.day_of_month == 1) {
      this->energy_.start_month = total;
    }

    // Handle new year start
    if (this->energy_.start_year == NAN || t.day_of_year == 1) {
      this->energy_.start_year = total;
    }

    this->energy_.current_day_of_year = t.day_of_year;  // Update the current day of year
  }

  // Process the energy values
  this->process_(total);
}


void EnergyStatistics::process_(float total) {
  if (this->energy_today_) {
    float today = !std::isnan(this->energy_.start_today) ? (total - this->energy_.start_today) : 0;
    this->energy_today_->publish_state(today);
  }

  if (this->energy_yesterday_) {
    float yesterday = !std::isnan(this->energy_.start_yesterday) ? 
      (this->energy_.start_today - this->energy_.start_yesterday) : 0;
    this->energy_yesterday_->publish_state(yesterday);
  }

  if (this->energy_week_) {
    float week = !std::isnan(this->energy_.start_week) ? (total - this->energy_.start_week) : 0;
    this->energy_week_->publish_state(week);
  }

  if (this->energy_month_) {
    float month = !std::isnan(this->energy_.start_month) ? (total - this->energy_.start_month) : 0;
    this->energy_month_->publish_state(month);
  }

  if (this->energy_year_) {
    float year = !std::isnan(this->energy_.start_year) ? (total - this->energy_.start_year) : 0;
    this->energy_year_->publish_state(year);
  }

  this->save_();
}


void EnergyStatistics::save_() { this->pref_.save(&(this->energy_)); }  // Save to flash memory

}  // namespace energy_statistics
}  // namespace esphome
