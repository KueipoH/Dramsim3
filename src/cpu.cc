#include "cpu.h"
#include <iostream>
#include <cmath>
#include <unordered_map>

namespace dramsim3 {

void RandomCPU::ClockTick() {
    memory_system_.ClockTick();
    if (get_next_) {
        last_addr_ = gen();
        last_write_ = (gen() % 3 == 0);
    }
    get_next_ = memory_system_.WillAcceptTransaction(last_addr_, last_write_);
    if (get_next_) {
        memory_system_.AddTransaction(last_addr_, last_write_);
    }
    clk_++;
}

void StreamCPU::ClockTick() {
    memory_system_.ClockTick();
    if (offset_ >= array_size_ || clk_ == 0) {
        addr_a_ = gen();
        addr_b_ = gen();
        addr_c_ = gen();
        offset_ = 0;
    }

    if (!inserted_a_ && memory_system_.WillAcceptTransaction(addr_a_ + offset_, false)) {
        memory_system_.AddTransaction(addr_a_ + offset_, false);
        inserted_a_ = true;
    }
    if (!inserted_b_ && memory_system_.WillAcceptTransaction(addr_b_ + offset_, false)) {
        memory_system_.AddTransaction(addr_b_ + offset_, false);
        inserted_b_ = true;
    }
    if (!inserted_c_ && memory_system_.WillAcceptTransaction(addr_c_ + offset_, true)) {
        memory_system_.AddTransaction(addr_c_ + offset_, true);
        inserted_c_ = true;
    }
    if (inserted_a_ && inserted_b_ && inserted_c_) {
        offset_ += stride_;
        inserted_a_ = false;
        inserted_b_ = false;
        inserted_c_ = false;
    }
    clk_++;
}

TraceBasedCPU::TraceBasedCPU(const std::string& config_file, const std::string& output_dir, const std::string& trace_file)
    : CPU(config_file, output_dir), trace_file_(trace_file) {
    trace_file_.open(trace_file);
    if (trace_file_.fail()) {
        std::cerr << "Trace file does not exist" << std::endl;
        AbruptExit(__FILE__, __LINE__);
    }
}

void TraceBasedCPU::ClockTick() {
    memory_system_.ClockTick();
    if (!trace_file_.eof()) {
        if (get_next_) {
            get_next_ = false;
            trace_file_ >> trans_;
        }
        if (trans_.added_cycle <= clk_) {
            get_next_ = memory_system_.WillAcceptTransaction(trans_.addr, trans_.is_write);
            if (get_next_) {
                memory_system_.AddTransaction(trans_.addr, trans_.is_write);
            }
        }
    }
    clk_++;
}

NMP_Core::NMP_Core(const std::string& config_file, const std::string& output_dir,
                   uint64_t inputBase1, uint64_t inputBase2, uint64_t outputBase,
                   uint64_t nodeDim, uint64_t count, int addition_op_cycle)
    : CPU(config_file, output_dir), inputBase1_(inputBase1), inputBase2_(inputBase2),
      outputBase_(outputBase), nodeDim_(nodeDim), count_(count), tid_(0),
      start_cycle_(0), end_cycle_(0), addition_op_cycle_(addition_op_cycle) {}

void NMP_Core::ClockTick() {
    memory_system_.ClockTick();

    if (clk_ == 0) {
        start_cycle_ = clk_;
    }

    for (uint64_t i = 0; i < count_; ++i) {
        uint64_t A = Read64B(inputBase1_ + i * nodeDim_ + tid_);
        uint64_t B = Read64B(inputBase2_ + i * nodeDim_ + tid_);
        uint64_t C = ElementWiseOperation(A, B);
        Write64B(outputBase_ + i * nodeDim_ + tid_, C);
    }

    end_cycle_ = clk_;
    std::cout << "Operation time: " << (end_cycle_ - start_cycle_) << " cycles" << std::endl;
    std::cout << "addition operation times" << addition_op_cycle_  << "cycles" << std::endl;
    

    clk_++;
}

uint64_t NMP_Core::Read64B(uint64_t address) {
    if (memory_system_.WillAcceptTransaction(address, false)) {
        memory_system_.AddTransaction(address, false);
    }

    return 0;  // Placeholder return value
}

void NMP_Core::Write64B(uint64_t address, uint64_t data) {
    if (memory_system_.WillAcceptTransaction(address, true)) {
        memory_system_.AddTransaction(address, true);
    }
}

uint64_t NMP_Core::ElementWiseOperation(uint64_t A, uint64_t B) {
    addition_op_cycle_++;
    return A + B;
}

}  // namespace dramsim3
