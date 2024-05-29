#ifndef __CPU_H
#define __CPU_H

#include <fstream>
#include <functional>
#include <random>
#include <vector>
#include <string>
#include "memory_system.h"

namespace dramsim3 {

class CPU {
   public:
    CPU(const std::string& config_file, const std::string& output_dir)
        : memory_system_(
              config_file, output_dir,
              std::bind(&CPU::ReadCallBack, this, std::placeholders::_1),
              std::bind(&CPU::WriteCallBack, this, std::placeholders::_1)),
          clk_(0) {}
    virtual void ClockTick() = 0;
    virtual void PrintStats() const { memory_system_.PrintStats(); }
    virtual void ReadCallBack(uint64_t addr) { return; }
    virtual void WriteCallBack(uint64_t addr) { return; }

   protected:
    MemorySystem memory_system_;
    uint64_t clk_;
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

class TensorDimm : public CPU {
public:
    TensorDimm(const std::string &config_file, const std::string &output_dir);
    void ClockTick() override;
    void PrintStats() const override;
private:
    void VectorAdd();
    std::vector<uint64_t> input_a_;
    std::vector<uint64_t> input_b_;
    std::vector<uint64_t> output_c_;
    uint64_t array_size_;
    uint64_t offset_;
    bool inserted_a_;
    bool inserted_b_;
    bool inserted_c_;
    uint64_t vector_add_cycles_;  // To track the cycles spent on vector addition
};

}  // namespace dramsim3
#endif
