#include "dtm.h"
#include "encoding.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#define RV_X(x, s, n) \
  (((x) >> (s)) & ((1 << (n)) - 1))
#define ENCODE_ITYPE_IMM(x) \
  (RV_X(x, 0, 12) << 20)
#define ENCODE_STYPE_IMM(x) \
  ((RV_X(x, 0, 5) << 7) | (RV_X(x, 5, 7) << 25))
#define ENCODE_SBTYPE_IMM(x) \
  ((RV_X(x, 1, 4) << 8) | (RV_X(x, 5, 6) << 25) | (RV_X(x, 11, 1) << 7) | (RV_X(x, 12, 1) << 31))
#define ENCODE_UTYPE_IMM(x) \
  (RV_X(x, 12, 20) << 12)
#define ENCODE_UJTYPE_IMM(x) \
  ((RV_X(x, 1, 10) << 21) | (RV_X(x, 11, 1) << 20) | (RV_X(x, 12, 8) << 12) | (RV_X(x, 20, 1) << 31))

#define LOAD(xlen, dst, base, imm) \
  (((xlen) == 64 ? 0x00003003 : 0x00002003) \
   | ((dst) << 7) | ((base) << 15) | (uint32_t)ENCODE_ITYPE_IMM(imm))
#define STORE(xlen, src, base, imm) \
  (((xlen) == 64 ? 0x00003023 : 0x00002023) \
   | ((src) << 20) | ((base) << 15) | (uint32_t)ENCODE_STYPE_IMM(imm))
#define JUMP(there, here) (0x6f | (uint32_t)ENCODE_UJTYPE_IMM((there) - (here)))
#define BNE(r1, r2, there, here) (0x1063 | ((r1) << 15) | ((r2) << 20) | (uint32_t)ENCODE_SBTYPE_IMM((there) - (here)))
#define ADDI(dst, src, imm) (0x13 | ((dst) << 7) | ((src) << 15) | (uint32_t)ENCODE_ITYPE_IMM(imm))
#define SRL(dst, src, sh) (0x5033 | ((dst) << 7) | ((src) << 15) | ((sh) << 20))
#define FENCE_I 0x100f
#define X0 0
#define S0 8
#define S1 9

#define WRITE 1
#define SET 2
#define CLEAR 3
#define CSRRx(type, dst, csr, src) (0x73 | ((type) << 12) | ((dst) << 7) | ((src) << 15) | (uint32_t)((csr) << 20))

uint64_t dtm_t::do_command(dtm_t::req r)
{
  req_buf = r;
  target->switch_to();
  assert(resp_buf.resp == 0);
  return resp_buf.data;
}

uint64_t dtm_t::read(uint32_t addr)
{
  return do_command((req){addr, 1, 0});
}

uint64_t dtm_t::write(uint32_t addr, uint64_t data)
{
  return do_command((req){addr, 2, data});
}

void dtm_t::nop()
{
  do_command((req){0, 0, 0});
}

uint32_t dtm_t::run_program(const uint32_t program[], size_t n, size_t result)
{
  assert(n <= ram_words());
  assert(result < ram_words());

  uint64_t interrupt_bit = 0x200000000U;
  for (size_t i = 0; i < n; i++)
    write(i, program[i] | (i == n-1 ? interrupt_bit : 0));

  while (true) {
    uint64_t rdata = read(result);
    if (!(rdata & interrupt_bit))
      return (uint32_t)rdata;
  }
}

size_t dtm_t::chunk_align()
{
  return xlen / 8;
}

void dtm_t::read_chunk(uint64_t taddr, size_t len, void* dst)
{
  uint32_t prog[ram_words()];
  uint32_t res[ram_words()];
  int result_word = 2 + (len / (xlen/8)) * 2;
  int addr_word = result_word;
  int prog_words = addr_word + (xlen/32);

  prog[0] = LOAD(xlen, S0, X0, ram_base() + addr_word * 4);
  for (size_t i = 0; i < len/(xlen/8); i++) {
    prog[2*i+1] = LOAD(xlen, S1, S0, i * (xlen/8));
    prog[2*i+2] = STORE(xlen, S1, X0, ram_base() + (result_word * 4) + (i * (xlen/8)));
  }
  prog[result_word - 1] = JUMP(rom_ret(), ram_base() + (result_word - 1) * 4);
  prog[addr_word] = (uint32_t)taddr;
  prog[addr_word + 1] = (uint32_t)(taddr >> 32);

  res[0] = run_program(prog, prog_words, result_word);
  for (size_t i = 1; i < len/4; i++)
    res[i] = read(result_word + i);
  memcpy(dst, res, len);
}

void dtm_t::write_chunk(uint64_t taddr, size_t len, const void* src)
{
  uint32_t prog[ram_words()];
  int data_word = 2 + (len / (xlen/8)) * 2;
  int addr_word = data_word + len/4;
  int prog_words = addr_word + (xlen/32);

  prog[0] = LOAD(xlen, S0, X0, ram_base() + addr_word * 4);
  for (size_t i = 0; i < len/(xlen/8); i++) {
    prog[2*i+1] = LOAD(xlen, S1, X0, ram_base() + (data_word * 4) + (i * (xlen/8)));
    prog[2*i+2] = STORE(xlen, S1, S0, i * (xlen/8));
  }
  prog[data_word - 1] = JUMP(rom_ret(), ram_base() + (data_word - 1)*4);
  memcpy(prog + data_word, src, len);
  prog[addr_word] = (uint32_t)taddr;
  prog[addr_word + 1] = (uint32_t)(taddr >> 32);

  run_program(prog, prog_words, data_word);
}

void dtm_t::clear_chunk(uint64_t taddr, size_t len)
{
  uint32_t prog[ram_words()];
  int addr1_word = 6;
  int addr2_word = addr1_word + (xlen/32);
  int prog_words = addr2_word + (xlen/32);

  if (prog_words + (xlen/32) > ram_words())
    return htif_t::clear_chunk(taddr, len);

  prog[0] = LOAD(xlen, S0, X0, ram_base() + addr1_word * 4);
  prog[1] = LOAD(xlen, S1, X0, ram_base() + addr2_word * 4);
  prog[2] = STORE(xlen, X0, S0, 0);
  prog[3] = ADDI(S0, S0, xlen/8);
  prog[4] = BNE(S0, S1, 2*4, 4*4);
  prog[5] = JUMP(rom_ret(), ram_base() + 5*4);
  prog[addr1_word] = (uint32_t)taddr;
  prog[addr1_word + 1] = (uint32_t)(taddr >> 32);
  prog[addr2_word] = (uint32_t)(taddr + len);
  prog[addr2_word + 1] = (uint32_t)((taddr + len) >> 32);

  run_program(prog, prog_words, 0);
}

uint64_t dtm_t::write_csr(unsigned which, uint64_t data)
{
  return modify_csr(which, data, WRITE);
}

uint64_t dtm_t::set_csr(unsigned which, uint64_t data)
{
  return modify_csr(which, data, SET);
}

uint64_t dtm_t::clear_csr(unsigned which, uint64_t data)
{
  return modify_csr(which, data, CLEAR);
}

uint64_t dtm_t::read_csr(unsigned which)
{
  return set_csr(which, 0);
}

uint64_t dtm_t::modify_csr(unsigned which, uint64_t data, uint32_t type)
{
  int data_word = 4;
  int prog_words = data_word + xlen/32;
  uint32_t prog[] = {
    LOAD(xlen, S0, X0, ram_base() + data_word * 4),
    CSRRx(type, S0, which, S0),
    STORE(xlen, S0, X0, ram_base() + data_word * 4),
    JUMP(rom_ret(), ram_base() + 12),
    (uint32_t)data,
    (uint32_t)(data >> 32)
  };

  uint64_t res = run_program(prog, prog_words, data_word);
  if (xlen == 64)
    res |= read(data_word + 1) << 32;
  return res;
}

size_t dtm_t::chunk_max_size()
{
  if (xlen == 32)
    return 4 * ((ram_words() - 4) / 3);
  else
    return 8 * ((ram_words() - 6) / 4);
}

uint32_t dtm_t::get_xlen()
{
  const uint32_t prog[] = {
    CSRRx(SET, S0, CSR_MISA, X0),
    ADDI(S1, X0, 62),
    SRL(S0, S0, S1),
    STORE(32, S0, X0, ram_base()),
    JUMP(rom_ret(), ram_base() + 16)
  };

  uint32_t result = run_program(prog, sizeof(prog)/sizeof(*prog), 0);
  if (result < 2)
    return 32;
  else if (result == 2)
    return 64;
  abort();
}

void dtm_t::fence_i()
{
  const uint32_t prog[] = {
    FENCE_I,
    JUMP(rom_ret(), ram_base() + 4)
  };

  run_program(prog, sizeof(prog)/sizeof(*prog), 0);
}

void host_thread_main(void* arg)
{
  ((dtm_t*)arg)->producer_thread();
}

void dtm_t::reset()
{
  fence_i();
  // set pc and un-halt
  write_csr(0x7b1, 0x80000000U);
  clear_csr(0x7b0, 8);
}

void dtm_t::idle()
{
  for (int idle_cycles = 0; idle_cycles < max_idle_cycles; idle_cycles++)
    nop();
}

void dtm_t::producer_thread()
{
  // halt hart
  dminfo = -1U;
  xlen = 32;
  set_csr(0x7b0, 8);

  // query hart
  dminfo = read(0x11);
  xlen = get_xlen();

  htif_t::run();

  while (true)
    nop();
}

void dtm_t::start_host_thread()
{
  req_wait = false;
  resp_wait = false;

  target = context_t::current();
  host.init(host_thread_main, this);
  host.switch_to();
}

dtm_t::dtm_t(const std::vector<std::string>& args)
  : htif_t(args)
{
  start_host_thread();
}

dtm_t::~dtm_t()
{
}

void dtm_t::tick(
  bool      req_ready,
  bool      resp_valid,
  resp      resp_bits)
{
  if (!resp_wait) {
    if (!req_wait) {
      req_wait = true;
    } else if (req_ready) {
      req_wait = false;
      resp_wait = true;
    }
  }

  if (resp_valid) {
    assert(resp_wait);
    resp_wait = false;

    resp_buf = resp_bits;
    host.switch_to();
  }
}

void dtm_t::return_resp(resp resp_bits){
  resp_buf = resp_bits;
  host.switch_to();
}
