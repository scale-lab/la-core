#ifndef _ROCKET_DTM_H
#define _ROCKET_DTM_H

#include "htif.h"
#include "context.h"
#include <stdint.h>
#include <queue>
#include <semaphore.h>
#include <vector>
#include <string>
#include <stdlib.h>

// abstract debug transport module
class dtm_t : public htif_t
{
 public:
  dtm_t(const std::vector<std::string>& args);
  ~dtm_t();

  struct req {
    uint32_t addr;
    uint32_t op;
    uint64_t data;
  };

  struct resp {
    uint32_t resp;
    uint64_t data;
  };

  void tick(
    bool  req_ready,
    bool  resp_valid,
    resp  resp_bits
  );
  // Akin to tick, but the target thread returns a response on every invocation
  void return_resp(
    resp  resp_bits
  );

  bool req_valid() { return req_wait; }
  req req_bits() { return req_buf; }
  bool resp_ready() { return true; }

  uint64_t read(uint32_t addr);
  uint64_t write(uint32_t addr, uint64_t data);
  void nop();

  uint64_t read_csr(unsigned which);
  uint64_t write_csr(unsigned which, uint64_t data);
  uint64_t clear_csr(unsigned which, uint64_t data);
  uint64_t set_csr(unsigned which, uint64_t data);
  void fence_i();

  void producer_thread();

 private:
  context_t host;
  context_t* target;
  pthread_t producer;
  sem_t req_produce;
  sem_t req_consume;
  sem_t resp_produce;
  sem_t resp_consume;
  req req_buf;
  resp resp_buf;

  uint32_t run_program(const uint32_t program[], size_t n, size_t result);
  uint64_t modify_csr(unsigned which, uint64_t data, uint32_t type);

  void read_chunk(addr_t taddr, size_t len, void* dst) override;
  void write_chunk(addr_t taddr, size_t len, const void* src) override;
  void clear_chunk(addr_t taddr, size_t len) override;
  size_t chunk_align() override;
  size_t chunk_max_size() override;
  void reset() override;
  void idle() override;

  bool req_wait;
  bool resp_wait;
  uint32_t dminfo;
  uint32_t xlen;

  static const int max_idle_cycles = 10000;

  size_t ram_base() { return 0x400; }
  size_t rom_ret() { return 0x804; }
  size_t ram_words() { return ((dminfo >> 10) & 63) + 1; }
  uint32_t get_xlen();
  uint64_t do_command(dtm_t::req r);

  void parse_args(const std::vector<std::string>& args);
  void register_devices();
  void start_host_thread();

  friend class memif_t;
};

#endif
