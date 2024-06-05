// this is cpu.h
#ifndef DRAMSIM3_CPU_H
#define DRAMSIM3_CPU_H

#include <fstream>
#include <functional>
#include <random>
#include <string>
#include <queue>
#include "memory_system.h"

namespace dramsim3 {

class CPU {
   public:
    CPU(const std::string& config_file, const std::string& output_dir)
        : memory_system_(config_file, output_dir,
                         std::bind(&CPU::ReadCallBack, this, std::placeholders::_1),
                         std::bind(&CPU::WriteCallBack, this, std::placeholders::_1)),
          clk_(0) {}
    virtual void ClockTick() = 0;
    virtual void PrintStats() { memory_system_.PrintStats(); }

   protected:
    MemorySystem memory_system_;
    uint64_t clk_;
    void ReadCallBack(uint64_t addr) {}
    void WriteCallBack(uint64_t addr) {}
};

class RandomCPU : public CPU {
   public:
    using CPU::CPU;
    void ClockTick() override;

   private:
    uint64_t last_addr_;
    bool last_write_ = false;
    std::mt19937_64 gen;
    bool get_next_ = true;
};

class StreamCPU : public CPU {
   public:
    using CPU::CPU;
    void ClockTick() override;

   private:
    uint64_t addr_a_, addr_b_, addr_c_, offset_ = 0;
    std::mt19937_64 gen;
    bool inserted_a_ = false;
    bool inserted_b_ = false;
    bool inserted_c_ = false;
    const uint64_t array_size_ = 2 << 20;  // elements in array
    const int stride_ = 64;                // stride in bytes
};

class TraceBasedCPU : public CPU {
   public:
    TraceBasedCPU(const std::string& config_file, const std::string& output_dir,
                  const std::string& trace_file);
    ~TraceBasedCPU() { trace_file_.close(); }
    void ClockTick() override;

   private:
    std::ifstream trace_file_;
    Transaction trans_;
    bool get_next_ = true;
};

class NMP_Core : public CPU {
   public:
    NMP_Core(const std::string& config_file, const std::string& output_dir,
             uint64_t inputBase1, uint64_t inputBase2, uint64_t outputBase,
             uint64_t nodeDim, uint64_t count, int addition_op_cycle);
    void ClockTick() override;
    void PrintStats(std::ofstream& ofs);

   private:
    uint64_t inputBase1_, inputBase2_, outputBase_;
    uint64_t nodeDim_, count_;
    uint64_t tid_;
    uint64_t start_cycle_, end_cycle_;
    int addition_op_cycle_;

    uint64_t Read64B(uint64_t address);
    void Write64B(uint64_t address, uint64_t data);
    uint64_t ElementWiseOperation(uint64_t A, uint64_t B);
    void ProcessQueue(std::queue<std::pair<uint64_t, bool>>& transaction_queue);
    void PrintQueue(const std::queue<std::pair<uint64_t, bool>>& q) const;

    std::queue<std::pair<uint64_t, bool>> read_queue_;
    std::queue<std::pair<uint64_t, bool>> write_queue_;
};

}  // namespace dramsim3

#endif  // DRAMSIM3_CPU_H
