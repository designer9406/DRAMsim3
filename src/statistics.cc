#include "statistics.h"
#include "common.h"
#include "../ext/fmt/src/format.h"

using namespace std;
using namespace dramcore;

template<class T>
void PrintNameValue(std::ostream& where, std::string name, T value) {
    where << fmt::format("{:^40}{:^5}{:>12}", name, " = ", value) << endl;
    return;
}

template<class T>
void PrintNameValueDesc(std::ostream& where, std::string name, T value, 
                        std::string description) {
    // not making this a class method because we need to calculate 
    // power & bw later, which are not BaseStat members 
    where << fmt::format("{:<40}{:^5}{:>12}{:>8}{}", name, " = ", value, " # ", description) << endl;
    return;
}

CounterStat::CounterStat(std::string name, std::string desc):
    BaseStat(name, desc),
    count_(0),
    last_epoch_count_(0)
{}

void CounterStat::Print(std::ostream& where) const {
    PrintNameValueDesc(where, name_, count_, description_);
    return;
}

void CounterStat::UpdateEpoch() {
    last_epoch_count_ = count_;
    return;
}

void CounterStat::PrintEpoch(std::ostream& where) const {
    PrintNameValueDesc(where, name_, count_ - last_epoch_count_, description_);
    return;
}

void CounterStat::PrintCSVHeader(std::ostream& where) const {
    where << fmt::format("{},", name_);
    return;
}

void CounterStat::PrintCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", count_ );
    return;
}

void CounterStat::PrintEpochCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", count_ - last_epoch_count_);
    return;
}

DoubleStat::DoubleStat(double inc, std::string name, std::string desc):
    BaseStat(name, desc),
    value(0.0),
    last_epoch_value(0.0),
    inc_(inc)
{}
    

void DoubleStat::Print(std::ostream& where) const {
    PrintNameValueDesc(where, name_, value, description_);
    return;
}

void DoubleStat::UpdateEpoch() {
    last_epoch_value = value;
    return;
}

void DoubleStat::PrintEpoch(std::ostream& where) const {
    PrintNameValueDesc(where, name_, value - last_epoch_value, description_);
    return;
}

void DoubleStat::PrintCSVHeader(std::ostream& where) const {
    where << fmt::format("{},", name_);
    return;
}


void DoubleStat::PrintCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", value );
    return;
}

void DoubleStat::PrintEpochCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", value - last_epoch_value);
    return;
}

DoubleComputeStat::DoubleComputeStat(std::string name, std::string desc):
    BaseStat(name, desc)
{}

void DoubleComputeStat::Print(std::ostream& where) const {
    PrintNameValueDesc(where, name_, cumulative_value, description_);
    return;
}


void DoubleComputeStat::UpdateEpoch() {}

void DoubleComputeStat::PrintEpoch(std::ostream& where) const {
    PrintNameValueDesc(where, name_, epoch_value, description_);
    return;
}

void DoubleComputeStat::PrintCSVHeader(std::ostream& where) const {
    where << fmt::format("{},", name_);
    return;
}


void DoubleComputeStat::PrintCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", cumulative_value );
    return;
}

void DoubleComputeStat::PrintEpochCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", epoch_value);
    return;
}

HistogramStat::HistogramStat(int start, int end, uint32_t numb_bins, std::string name, std::string desc):
    BaseStat(name, desc),
    start_(start),
    end_(end),
    numb_bins_(numb_bins),
    neg_outlier_count_(0),
    pos_outlier_count_(0),
    last_epoch_neg_outlier_count_(0),
    last_epoch_pos_outlier_count_(0)
{
    if(start_ >= end_ || numb_bins <= 0) {
        cout << "Hisogram stat improperly specified" << endl;
        AbruptExit(__FILE__, __LINE__);
    }
    bin_count_.resize(numb_bins);
    last_epoch_bin_count_.resize(numb_bins);
}

void HistogramStat::AddValue(int val) {
    if( val >= start_ && val <= end_) {
        auto bin_index = (val*numb_bins_)/(end_ - start_); //TODO - Could be made better
        bin_count_[bin_index]++;
    }
    else if( val < start_) {
        neg_outlier_count_++;
    }
    else if ( val > end_) {
        pos_outlier_count_++;
    }
    return;
}

void HistogramStat::Print(std::ostream& where) const {
    auto bin_width = (end_ - start_)/numb_bins_;
    PrintNameValueDesc(where, name_, " ", description_);
    auto bin_str = fmt::format("[ < {} ]", start_);
    PrintNameValue(where, bin_str, neg_outlier_count_);
    for(auto i = 0; i < numb_bins_; i++) {
        auto bin_start = start_ + i*bin_width;
        auto bin_end = start_ + (i+1)*bin_width - 1;
        auto bin_str = fmt::format("[ {}-{} ]", bin_start, bin_end);
        PrintNameValue(where, bin_str, bin_count_[i]);
    }
    bin_str = fmt::format("[ > {} ]", end_);
    PrintNameValue(where, bin_str, pos_outlier_count_);
    return;
}

void HistogramStat::UpdateEpoch() {
    last_epoch_neg_outlier_count_ = neg_outlier_count_;
    last_epoch_pos_outlier_count_ = pos_outlier_count_;
    for(auto i = 0; i < numb_bins_; i++) {
        last_epoch_bin_count_[i] = bin_count_[i];
    }
    return;
}

void HistogramStat::PrintEpoch(std::ostream& where) const {
    //TODO - Think of ways to avoid code duplication - Currently CTRL+C,CTRL+V of Print with epoch subtraction
    auto bin_width = (end_ - start_)/numb_bins_;
    PrintNameValueDesc(where, name_, " ", description_);
    auto bin_str = fmt::format("[ < {} ]", start_);
    PrintNameValue(where, bin_str, neg_outlier_count_ - last_epoch_neg_outlier_count_);
    for(auto i = 0; i < numb_bins_; i++) {
        auto bin_start = start_ + i*bin_width;
        auto bin_end = start_ + (i+1)*bin_width - 1;
        auto bin_str = fmt::format("[ {}-{} ]", bin_start, bin_end);
        PrintNameValue(where, bin_str, bin_count_[i] - last_epoch_bin_count_[i]);
    }
    bin_str = fmt::format("[ > {} ]", end_);
    PrintNameValue(where, bin_str, pos_outlier_count_ - last_epoch_pos_outlier_count_);
    return;
}


void HistogramStat::PrintCSVHeader(std::ostream& where) const {
    auto bin_width = (end_ - start_)/numb_bins_;
    where << fmt::format("{}[<{}],", name_, start_);
    for(auto i = 0; i < numb_bins_; i++) {
        auto bin_start = start_ + i*bin_width;
        auto bin_end = start_ + (i+1)*bin_width - 1;
        where << fmt::format("{}[{}-{}],", name_, bin_start, bin_end);
    }
    where << fmt::format("{}[>{}],", name_, end_);
    return;
}

void HistogramStat::PrintCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", neg_outlier_count_);
    for(auto i = 0; i < numb_bins_; i++) {
        where << fmt::format("{},", bin_count_[i]);
    }
    where << fmt::format("{},", pos_outlier_count_);
    return;
}


void HistogramStat::PrintEpochCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", neg_outlier_count_ - last_epoch_neg_outlier_count_);
    for(auto i = 0; i < numb_bins_; i++) {
        where << fmt::format("{},", bin_count_[i] - last_epoch_bin_count_[i]);
    }
    where << fmt::format("{},", pos_outlier_count_ - last_epoch_pos_outlier_count_);
    return;
}

Statistics::Statistics(const Config& config):
    stats_list(),
    config_(config),
    last_clk_(0)
{
    //TODO - Should stats be global?
    numb_read_reqs_issued = CounterStat("numb_read_reqs_issued", "Number of read requests issued");
    numb_write_reqs_issued = CounterStat("numb_write_reqs_issued", "Number of write requests issued");
    hmc_reqs_done = CounterStat("hmc_reqs_done", "HMC Requests finished");
    numb_row_hits = CounterStat("numb_row_hits", "Number of row hits");
    numb_read_row_hits = CounterStat("numb_read_row_hits", "Number of read row hits");
    numb_write_row_hits = CounterStat("numb_write_row_hits", "Number of write row hits");
    numb_aggressive_precharges = CounterStat("numb_aggressive_precharges", "Number of aggressive precharges issued");
    numb_ondemand_precharges = CounterStat("numb_ondemand_precharges", "Number of on demand precharges issued");
    dramcycles = CounterStat("cycles", "Total number of DRAM execution cycles");
    access_latency = HistogramStat(0, 200, 10, "access_latency", "Histogram of access latencies");
    numb_buffered_requests = CounterStat("numb_buffered_requests", "Number of buffered requests because queues were full");
    hbm_dual_command_issue_cycles = CounterStat("hbm_dual_command_issue_cycles", "Number of cycles in which two commands were issued");
    hbm_dual_non_rw_cmd_attempt_cycles = CounterStat("hbm_dual_non_rw_cmd_attempt_cycles", "Number of cycles during which an opportunity to issue a read/write is possibly missed");
    numb_read_cmds_issued = CounterStat("numb_read_cmds_issued", "Number of read commands issued");
    numb_write_cmds_issued = CounterStat("numb_write_cmds_issued", "Number of write commands issued");
    numb_activate_cmds_issued = CounterStat("numb_activate_cmds_issued", "Number of activate commands issued");
    numb_precharge_cmds_issued = CounterStat("numb_precharge_cmds_issued", "Number of precharge commands issued");
    numb_refresh_cmds_issued = CounterStat("numb_refresh_cmds_issued", "Number of refresh commands issued");
    numb_refresh_bank_cmds_issued = CounterStat("numb_refresh_bank_cmds_issued", "Number of refresh bank commands issued");
    numb_self_refresh_enter_cmds_issued = CounterStat("numb_self_refresh_enter_cmds_issued", "Number of self-refresh mode enter commands issued");
    numb_self_refresh_exit_cmds_issued = CounterStat("numb_self_refresh_exit_cmds_issued", "Number of self-refresh mode exit commands issued");
    numb_rw_rowhits_pending_refresh = CounterStat("numb_rw_rowhits_pending_refresh", "Number of read/write row hits issued while a refresh was pending");
#ifdef DEBUG_HMC
    logic_clk CounterStat("hmc_logic_clk", "HMC logic clock");
    stats_list.push_back(&logic_clk);
#endif  // DEBUG_HMC
    sref_cycles = CounterStat("sref_cycles", "Cycles in self-refresh state");
    all_bank_idle_cycles = CounterStat("all_bank_idle_cycles", "Cycles of all banks are idle");
    active_cycles = CounterStat("rank_active_cycles", "Number of cycles the rank ramains active");
    // energy and power stats
    act_energy = DoubleComputeStat("act_energy", "ACT energy");
    read_energy = DoubleComputeStat("read_energy", "READ energy (not including IO)");
    write_energy = DoubleComputeStat("write_energy", "WRITE energy (not including IO)");
    ref_energy = DoubleComputeStat("ref_energy", "Refresh energy");
    refb_energy = DoubleComputeStat("refb_energy", "Bank-Refresh energy");
    act_stb_energy = DoubleComputeStat("act_stb_energy", "Active standby energy");
    pre_stb_energy = DoubleComputeStat("pre_stb_energy", "Precharge standby energy");
    pre_pd_energy = DoubleComputeStat("pre_pd_energy", "Precharge powerdown energy");
    sref_energy = DoubleComputeStat("sref_energy", "Self-refresh energy");
    total_energy = DoubleComputeStat("total_energy", "(pJ) Total energy consumed");
    average_power = DoubleComputeStat("average_power", "(mW) Average Power for all devices");
    average_bandwidth = DoubleComputeStat("average_bandwidth", "(GB/s) Average Aggregate Bandwidth");

    stats_list.push_back(&numb_read_reqs_issued);
    stats_list.push_back(&numb_write_reqs_issued);
    stats_list.push_back(&hmc_reqs_done);
    stats_list.push_back(&numb_row_hits);
    stats_list.push_back(&numb_read_row_hits);
    stats_list.push_back(&numb_write_row_hits);
    stats_list.push_back(&numb_aggressive_precharges);
    stats_list.push_back(&numb_ondemand_precharges);
    stats_list.push_back(&dramcycles);
    stats_list.push_back(&access_latency);
    stats_list.push_back(&numb_buffered_requests);
    stats_list.push_back(&hbm_dual_command_issue_cycles);
    stats_list.push_back(&hbm_dual_non_rw_cmd_attempt_cycles);
    stats_list.push_back(&numb_read_cmds_issued);
    stats_list.push_back(&numb_write_cmds_issued);
    stats_list.push_back(&numb_activate_cmds_issued);
    stats_list.push_back(&numb_precharge_cmds_issued);
    stats_list.push_back(&numb_refresh_cmds_issued);
    stats_list.push_back(&numb_refresh_bank_cmds_issued);
    stats_list.push_back(&numb_self_refresh_enter_cmds_issued);
    stats_list.push_back(&numb_self_refresh_exit_cmds_issued);
    stats_list.push_back(&numb_rw_rowhits_pending_refresh);
    stats_list.push_back(&all_bank_idle_cycles);
    stats_list.push_back(&active_cycles);
    stats_list.push_back(&act_energy);
    stats_list.push_back(&read_energy);
    stats_list.push_back(&write_energy);
    stats_list.push_back(&ref_energy);
    stats_list.push_back(&refb_energy);
    stats_list.push_back(&act_stb_energy);
    stats_list.push_back(&pre_stb_energy);
    stats_list.push_back(&pre_pd_energy);
    stats_list.push_back(&sref_energy);
    stats_list.push_back(&total_energy);
    stats_list.push_back(&average_power);
    stats_list.push_back(&average_bandwidth);
}


void Statistics::PreEpochCompute(uint64_t clk) {
    // because HMC requests != read commands,
    uint64_t reqs_issued_epoch, reqs_issued;
    if(!config_.IsHMC()) {
        reqs_issued_epoch = numb_read_reqs_issued.Count() - numb_read_reqs_issued.LastCount()
                            + numb_write_reqs_issued.Count() - numb_write_reqs_issued.LastCount();
        reqs_issued = numb_read_reqs_issued.Count() + numb_write_reqs_issued.Count();
    } else {
        reqs_issued_epoch = hmc_reqs_done.Count() - hmc_reqs_done.LastCount();
        reqs_issued = hmc_reqs_done.Count();
    }

    //Epoch level compute stats
    act_energy.epoch_value = (numb_activate_cmds_issued.Count() - numb_activate_cmds_issued.LastCount()) * config_.act_energy_inc;
    read_energy.epoch_value = (numb_read_cmds_issued.Count() - numb_read_cmds_issued.LastCount()) * config_.read_energy_inc;
    write_energy.epoch_value = (numb_write_cmds_issued.Count() - numb_write_cmds_issued.LastCount()) * config_.write_energy_inc;
    ref_energy.epoch_value = (numb_refresh_cmds_issued.Count() - numb_refresh_cmds_issued.LastCount()) * config_.ref_energy_inc;
    refb_energy.epoch_value = (numb_refresh_bank_cmds_issued.Count() - numb_refresh_bank_cmds_issued.LastCount()) * config_.refb_energy_inc;
    act_stb_energy.epoch_value = (active_cycles.Count() - active_cycles.LastCount()) * config_.act_stb_energy_inc;
    pre_stb_energy.epoch_value = (all_bank_idle_cycles.Count() - all_bank_idle_cycles.LastCount()) * config_.pre_stb_energy_inc;
    sref_energy.epoch_value = (sref_cycles.Count() - sref_cycles.LastCount()) * config_.sref_energy_inc;
    total_energy.epoch_value = act_energy.epoch_value + read_energy.epoch_value + write_energy.epoch_value
                               + ref_energy.epoch_value + refb_energy.epoch_value + act_stb_energy.epoch_value
                               + pre_stb_energy.epoch_value + pre_pd_energy.epoch_value + sref_energy.epoch_value;
    average_power.epoch_value = total_energy.epoch_value / (clk - last_clk_);
    average_bandwidth.epoch_value = (reqs_issued_epoch * config_.request_size_bytes) / ((clk - last_clk_) * config_.tCK);

    //cumulative compute stats
    act_energy.cumulative_value = numb_activate_cmds_issued.Count() * config_.act_energy_inc;
    read_energy.cumulative_value = numb_read_cmds_issued.Count() * config_.read_energy_inc;
    write_energy.cumulative_value = numb_write_cmds_issued.Count() * config_.write_energy_inc;
    ref_energy.cumulative_value = numb_refresh_cmds_issued.Count() * config_.ref_energy_inc;
    refb_energy.cumulative_value = numb_refresh_bank_cmds_issued.Count() * config_.refb_energy_inc;
    act_stb_energy.cumulative_value = active_cycles.Count() * config_.act_stb_energy_inc;
    pre_stb_energy.cumulative_value = all_bank_idle_cycles.Count() * config_.pre_stb_energy_inc;
    sref_energy.cumulative_value = sref_cycles.Count() * config_.sref_energy_inc;
    total_energy.cumulative_value = act_energy.cumulative_value + read_energy.cumulative_value + write_energy.cumulative_value
                                     + ref_energy.cumulative_value + refb_energy.cumulative_value + act_stb_energy.cumulative_value
                                     + pre_stb_energy.cumulative_value + sref_energy.cumulative_value;
    average_power.cumulative_value = total_energy.epoch_value / clk;
    average_bandwidth.cumulative_value = (reqs_issued * config_.request_size_bytes) / ((clk) * config_.tCK);
}

void Statistics::PrintStats(std::ostream &where) const {
    for(auto stat : stats_list) {
        stat->Print(where);
    }
}

void Statistics::UpdateEpoch(uint64_t clk) {
    // get clk information so that we can calculate power, bandwidth, etc.
    for(auto stat : stats_list) {
        stat->UpdateEpoch();
    }
    last_clk_ = clk;
    return;
}

void Statistics::PrintEpochStats(std::ostream& where) const {
    for(auto stat : stats_list) {
        stat->PrintEpoch(where);
    }
    return;
}


void Statistics::PrintStatsCSVHeader(std::ostream& where) const {
    for(auto stat : stats_list) {
        stat->PrintCSVHeader(where);
    }
    where << endl;
    return;
}

void Statistics::PrintStatsCSVFormat(std::ostream& where) const {
    for(auto stat : stats_list) {
        stat->PrintCSVFormat(where);
    }
    where << endl;
    return;
}

void Statistics::PrintEpochStatsCSVFormat(std::ostream& where) const {
    for(auto stat : stats_list) {
        stat->PrintEpochCSVFormat(where);
    }
    where << endl;
    return;
}

namespace dramcore {

ostream& operator<<(ostream& os, Statistics& stats) {
    stats.PrintStats(os);
    return os;
}

}