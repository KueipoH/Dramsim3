// this is cpu.cc
#include "cpu.h"
#include <iostream>
#include <fstream>
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
      start_cycle_(0), end_cycle_(0), addition_op_cycle_(addition_op_cycle), read_trace_mode_(false) {
        
        //std::string trace_file1 = "sorted_index_array.txt";
        //std::cout << "Attempting to open file: " << trace_file1 << std::endl;

        // Read trace file into memory
        std::string trace_file = "/home/bruce0909/DRAMsim3/src/sorted_index_array.txt";
        trace_data_ = ReadTrace(trace_file);
      }

void NMP_Core::ClockTick() {

    if (read_trace_mode_ == false){

    // put done transactions into input_SRAM
    std::pair<uint64_t, int> done_trans = memory_system_.ReturnDoneTrans(clk_);
    if (done_trans.first != static_cast<uint64_t>(-1)) {
        if (done_trans.second == 0) { // If the transaction is a read
            input_sram.push(done_trans);
        } else if (done_trans.second == 1) { // If the transaction is a write
            Embedding_sum_operation++;
        }
    }

    ProcessQueue(RW_queue_);
    /////////////
    PrintInputSramQueue(input_sram);
    /////////////
    ProcessInputSram();
    PrintOutputSramQueue(output_sram);

    MoveOutputSramToRWQueue();
    // nmp core addition operation
    std::cout << "RW_queue_.size(): " << RW_queue_.size() << std::endl;
    PrintQueue(RW_queue_);
    
    if (RW_queue_.size() < 1) {
        for (uint64_t i = 0; i < count_; ++i) {
            if(clk_ % 15 == 0){ //to control the request frequency
            uint64_t addressA = inputBase1_ + i * nodeDim_ + tid_;
            uint64_t addressB = inputBase2_ + i * nodeDim_ + tid_;

            uint64_t A = Read64B(addressA);
            uint64_t B = Read64B(addressB);}
        }
        
    }
    // memory clock tick!
    memory_system_.ClockTick();


    // update clock count
    std::cout << "Number of DRAM cycles: " << clk_ + 1 << " cycles" << std::endl;
    std::cout << "Embedding_sum_operation: " << Embedding_sum_operation << std::endl;
    //std::cout << "addition operation times : " << addition_op_cycle_  << " cycles" << std::endl;
    
    tid_++;
    clk_++;

    std::cout << "----------------------------Normal mode CURRENT TICK END-------------------------------------" << std::endl;
    }

    //-------------------------------------trace mode code-------------------------------------

    if (read_trace_mode_ == true){

    PutTraceIntoRWqueue();
    // PrintQueue(RW_queue_);
    ProcessQueue(RW_queue_);

    // put done transactions into input_SRAM 
    std::pair<uint64_t, int> done_trans = memory_system_.ReturnDoneTrans(clk_);
    if (done_trans.first != static_cast<uint64_t>(-1)) {
        if (done_trans.second == 0) { // If the transaction is a read
            input_sram.push(done_trans);
        } else if (done_trans.second == 1) { // If the transaction is a write
            Embedding_sum_operation++;
        }
    }

    PrintInputSramQueue(input_sram);
    ProcessTraceInputSram();


    memory_system_.ClockTick();
    clk_++;

    std::cout << "Embedding_sum_operation: " << Embedding_sum_operation << std::endl;
    std::cout << "----------------------------Trace mode CURRENT TICK END-------------------------------------" << std::endl;
    }

}

void NMP_Core::ProcessQueue(std::queue<std::pair<uint64_t, bool>>& transaction_queue) {
    auto PrintTransactionQueue = [](const std::queue<std::pair<uint64_t, bool>>& q) {
        std::queue<std::pair<uint64_t, bool>> copy = q;
        std::cout << "RW_queue: ";
        while (!copy.empty()) {
            const auto& transaction = copy.front();
            std::cout << "[" << transaction.first << ", " << (transaction.second ? "Write" : "Read") << "] ";
            copy.pop();
        }
        std::cout << std::endl;
    };

    //PrintTransactionQueue(transaction_queue);

    std::queue<std::pair<uint64_t, bool>> remaining_transactions;

    while (!transaction_queue.empty()) {
        const auto& transaction = transaction_queue.front();
        uint64_t address = transaction.first;
        bool ReadOrWrite = transaction.second;

        if (memory_system_.WillAcceptTransaction(address, ReadOrWrite)) {
            memory_system_.AddTransaction(address, ReadOrWrite);
        } else {
            //std::cout << "MEMORY CAN NOT TAKE MORE TRANSACTION FOR ADDRESS: " << address << std::endl;
            remaining_transactions.push(transaction);
        }

        transaction_queue.pop();
    }

    transaction_queue = remaining_transactions;

    PrintTransactionQueue(transaction_queue);
}

uint64_t NMP_Core::Read64B(uint64_t address) {
    RW_queue_.emplace(address, false);
    return 0;  
}

void NMP_Core::Write64B(uint64_t address, uint64_t data) {
    RW_queue_.emplace(address, true);
}

void NMP_Core::ElementWiseOperation(uint64_t A, uint64_t B) {
    addition_op_cycle_++;
}

void NMP_Core::PrintQueue(const std::queue<std::pair<uint64_t, bool>>& q) const {
    std::queue<std::pair<uint64_t, bool>> temp = q;
    std::cout << "RW_queue_: ";
    while (!temp.empty()) {
        const auto& transaction = temp.front();
        std::cout << "[" << transaction.first << ", " << (transaction.second ? "Write" : "Read") << "] ";
        temp.pop();
    }
    std::cout << std::endl;
}

void NMP_Core::PrintTransactionQueue(const std::queue<std::pair<uint64_t, bool>>& transaction_queue) const {
    std::queue<std::pair<uint64_t, bool>> temp_queue = transaction_queue; 
    std::cout << "Transaction Queue Contents:" << std::endl;
    while (!temp_queue.empty()) {
        const auto& transaction = temp_queue.front();
        uint64_t address = transaction.first;
        bool ReadOrWrite = transaction.second;
        std::cout << "Address: " << address << ", Type: " << (ReadOrWrite ? "WRITE" : "READ") << std::endl;
        temp_queue.pop();
    }
}

void NMP_Core::PrintInputSramQueue(const std::queue<std::pair<uint64_t, bool>>& q) {
    std::queue<std::pair<uint64_t, bool>> copy = q;
    std::cout << "InputSRAM: [";
    while (!copy.empty()) {
        std::cout << "(" << copy.front().first << ", " << (copy.front().second ? "Write" : "Read") << ")";
        copy.pop();
        if (!copy.empty()) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;
}

void NMP_Core::PrintOutputSramQueue(const std::queue<std::pair<uint64_t, bool>>& q) {
    std::queue<std::pair<uint64_t, bool>> copy = q;
    std::cout << "OutputSRAM: [";
    while (!copy.empty()) {
        std::cout << "(" << copy.front().first << ", " << (copy.front().second ? "Write" : "Read") << ")";
        copy.pop();
        if (!copy.empty()) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;
}


void NMP_Core::ProcessInputSram() {
    if (input_sram.size() >= 2) {
        // delete two datas
        auto data1 = input_sram.front();
        input_sram.pop();
        auto data2 = input_sram.front();
        input_sram.pop();

        // add write request data on output sram
        uint64_t write_addr = outputBase_ + tid_; // address
        output_sram.emplace(write_addr, true);

        std::cout << "Processed Input SRAM: (" << data1.first << ", " << (data1.second ? "Write" : "Read")
                  << "), (" << data2.first << ", " << (data2.second ? "Write" : "Read") << ") -> Output SRAM: ("
                  << write_addr << ", Write)" << std::endl;
    }
}

void NMP_Core::ProcessTraceInputSram() {
    // Check if RW_queue_ has any read request
    bool has_read_request = false;
    std::queue<std::pair<uint64_t, bool>> temp_queue = RW_queue_;

    while (!temp_queue.empty()) {
        if (temp_queue.front().second == false) {  // Check for read operation
            has_read_request = true;
            break;
        }
        temp_queue.pop();
    }

    if (!has_read_request) {
        // If RW_queue_ doesn't have any read requests, process input SRAM
        while (!input_sram.empty()) {
            auto data = input_sram.front();
            input_sram.pop();

            // Add write request data to output SRAM
            uint64_t write_addr = outputBase_ + tid_; // address
            output_sram.emplace(write_addr, true);

            // Add write request to RW_queue_
            RW_queue_.emplace(write_addr, true);  // write operation

            std::cout << "Processed Input SRAM: (" << data.first << ", " << (data.second ? "Write" : "Read")
                      << ") -> Output SRAM: (" << write_addr << ", Write)" << std::endl;
        }
    }
}


void NMP_Core::MoveOutputSramToRWQueue() {
    if (!output_sram.empty()) {
        auto transaction = output_sram.front();
        RW_queue_.push(transaction);
        output_sram.pop();
        std::cout << "Moved transaction to RW_queue_: [" << transaction.first << ", " << (transaction.second ? "Write" : "Read") << "]" << std::endl;
    }
}

void NMP_Core::PutTraceIntoRWqueue() {
    if (trace_data_.empty()) {
        std::cerr << "Trace data is empty" << std::endl;
        return;
    }

    // Check if RW_queue already has any read request
    bool has_read_request = false;
    std::queue<std::pair<uint64_t, bool>> temp_queue = RW_queue_;

    while (!temp_queue.empty()) {
        if (!temp_queue.front().second) {  // Check for read operation
            has_read_request = true;
            break;
        }
        temp_queue.pop();
    }

    if (has_read_request) {
        // If there are read requests in RW_queue, do not add new traces
        std::cerr << "RW_queue already has read requests, skipping adding trace" << std::endl;
        return;
    }

    // Add traces to RW_queue until a different destination is encountered
    uint64_t last_destination = trace_data_.front().second;
    for (const auto& trace : trace_data_) {
        uint64_t source = trace.first;
        uint64_t destination = trace.second;

        if (destination != last_destination) {
            break;
        }

        RW_queue_.emplace(source, false);  // read operation

        last_destination = destination;
    }

    // Remove processed traces
    trace_data_.erase(std::remove_if(trace_data_.begin(), trace_data_.end(),
        [last_destination](const std::pair<uint64_t, uint64_t>& trace) {
            return trace.second == last_destination;
        }), trace_data_.end());
}


std::vector<std::pair<uint64_t, uint64_t>> NMP_Core::ReadTrace(const std::string& filename) {
    std::vector<std::pair<uint64_t, uint64_t>> trace_data;
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return trace_data;
    }

    uint64_t source, destination;
    while (infile >> source >> destination) {
        trace_data.emplace_back(source, destination);
    }
    infile.close();

    return trace_data;
}

// std::cout << "1st Read Address: " << (inputBase1_ + i * nodeDim_ + tid_) << std::endl;
// std::cout << "2nd Read Address: " << (inputBase2_ + i * nodeDim_ + tid_) << std::endl;
// std::cout << "---Write Address: " << (outputBase_ + i * nodeDim_ + tid_) << std::endl;
// std::cout << "MEMORY CAN NOT TAKE MORE TRASACTION" << std::endl;
// std::cout << "poped" << std::endl;

}  // namespace dramsim3