#include "cpu.h"

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
    return;
}
//git test
//git test1
void StreamCPU::ClockTick() {
    memory_system_.ClockTick();
    if (offset_ >= array_size_ || clk_ == 0) {
        addr_a_ = gen();
        addr_b_ = gen();
        addr_c_ = gen();
        offset_ = 0;
    }

    if (!inserted_a_ &&
        memory_system_.WillAcceptTransaction(addr_a_ + offset_, false)) {
        memory_system_.AddTransaction(addr_a_ + offset_, false);
        inserted_a_ = true;
    }
    if (!inserted_b_ &&
        memory_system_.WillAcceptTransaction(addr_b_ + offset_, false)) {
        memory_system_.AddTransaction(addr_b_ + offset_, false);
        inserted_b_ = true;
    }
    if (!inserted_c_ &&
        memory_system_.WillAcceptTransaction(addr_c_ + offset_, true)) {
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
    return;
}

TraceBasedCPU::TraceBasedCPU(const std::string& config_file,
                             const std::string& output_dir,
                             const std::string& trace_file)
    : CPU(config_file, output_dir) {
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
            get_next_ = memory_system_.WillAcceptTransaction(trans_.addr,
                                                             trans_.is_write);
            if (get_next_) {
                memory_system_.AddTransaction(trans_.addr, trans_.is_write);
            }
        }
    }
    clk_++;
    return;
}

TensorDimm::TensorDimm(const std::string &config_file, const std::string &output_dir)
    : CPU(config_file, output_dir), array_size_(1024), offset_(0), 
      inserted_a_(false), inserted_b_(false), inserted_c_(false), vector_add_cycles_(0) {
    input_a_.resize(array_size_, 1);  // Initialize with some values
    input_b_.resize(array_size_, 2);  // Initialize with some values
    output_c_.resize(array_size_, 0);  // Initialize output
}

void TensorDimm::ClockTick() {
    if (offset_ < array_size_) {
        std::cout << "Performing Vector Add at cycle: " << clk_ << std::endl;
        VectorAdd();
        offset_++;
        vector_add_cycles_++;
    }
    memory_system_.ClockTick(); // 
    clk_++;
}


void TensorDimm::VectorAdd() {
    // 
    uint64_t addr_a = reinterpret_cast<uint64_t>(&input_a_[offset_]);
    uint64_t addr_b = reinterpret_cast<uint64_t>(&input_b_[offset_]);
    uint64_t addr_c = reinterpret_cast<uint64_t>(&output_c_[offset_]);

    if (memory_system_.WillAcceptTransaction(addr_a, false) &&
        memory_system_.WillAcceptTransaction(addr_b, false) &&
        memory_system_.WillAcceptTransaction(addr_c, true)) {
        memory_system_.AddTransaction(addr_a, false); // Read from input_a
        memory_system_.AddTransaction(addr_b, false); // Read from input_b
        memory_system_.AddTransaction(addr_c, true);  // Write to output_c

        // 
        output_c_[offset_] = input_a_[offset_] + input_b_[offset_];
        std::cout << "address start" << std::endl;
        std::cout << addr_a << std::endl;
        std::cout << addr_b << std::endl;
        std::cout << addr_c << std::endl;
        std::cout << "address end" << std::endl;
    } else {
        std::cerr << "Memory transaction not accepted for Vector Add at offset: " << offset_ << std::endl;
    }
}


void TensorDimm::PrintStats() const {
    CPU::PrintStats();
    std::cout << "Vector Add Cycles: " << vector_add_cycles_ << std::endl;
    std::ofstream out_file("tensor_dimm_stats.txt", std::ios::app);
    if (out_file.is_open()) {
        out_file << "Vector Add Cycles: " << vector_add_cycles_ << std::endl;
        out_file.close();
    } else {
        std::cerr << "Unable to open file for writing statistics" << std::endl;
    }
}

}  // namespace dramsim3
