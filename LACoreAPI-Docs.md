# LACoreAPI Developer Documentation
This documentation is for those who want to write c-programs targeting the LACore. __NOTE__: Make sure you have followed the [Installation Guide](Readme.md) and have a working version of the riscv64 cross-compiler with LACore support, and the LACoreAPI tests are passing when run on spike. 

## LACore Architecture Quick Overview
The LACore is an extension to a RISC-V scalar CPU, and it adds 56 instructions to the base ISA in order to accelerate linear algebra applications. In addition to the typical scalar CPU components (Decoder, Register File, PC...) there are 4 hardware components of the LACore that the developer should be aware of:

1) __Execution Unit__: The LACore's Execution Unit is a systolic array that is composed of a 2 sections: a parallel vector unit (takes vector inputs and produces a vector output), and a vector reduction unit (takes vector inputs and produces scalar outputs). The vector unit takes 3 input data streams (whose backends could be scalars, vectors or sparse matrices) and produces 1 output stream. The Vector Unit can perform any of the 8 following arithmetic operations (A,B,C are inputs, D is output):

        (A+B)*C = D
        (A-B)*C = D
        (A+B)/C = D
        (A-B)/C = D
        (A*B)+C = D
        (A/B)+C = D
        (A*B)-C = D
        (A/B)-C = D
    
    The Reduction Unit takes the 1 output stream of the Vector Unit as it's input and performs any of the following reduction operations on it, to produce a scalar output (A0,A1... are elements of input vector A, while B is the output scalar):
    
        sum(A0, A1, A2 ...) = B
        min(A0, A1, A2 ...) = B
        max(A0, A1, A2 ...) = B
        
    Therefore, when combining the Vector Unit and Reduction Unit, there are 24 possible arithmetic operations you can perform on the 3 input streams when producing a scalar-output or multi-stream-output, and only 8 possible arithmetic operations you can perform for a vector-output.
    
    _Vector, Scalar and Multi-Stream Output Modes_
    
    There are 3 output modes, as discussed. The vector output mode takes the output vector directly from the Vector Unit, and it skips the Reduction Unit altogether. So for vector-output operations, if you specify input vectors of length 100, the output vector will have length 100 as well. For scalar outputs, there is only 1 output element, regardless of how many elements are in the input vectors. This is because the Reduction Unit _reduces_ the vector output into a scalar. The Multi-stream output can be thought of as "many small dot-products within a larger vector output". So instead of the Reduction Unit outputting a single scalar, it will actually output a _stream_ of scalars that are the result of dotproducts of subsections of the input vectors. For example, if the input vectors are 100 elements long, and you perform dot-products on subsections of length 20, then the output vector will be 5 elements long, with each element being the result of a 20-element vector dot-product
    
    _Dual Precision_
    
    The Execution Unit can operate on both single-precision and double-precision elements. That means you can use `double` and `float` in your C-code. You do not configure the Execution Unit for the vector precisions directly though. Instead, you will configure the LACfg registers through configuration instructions, and in turn, _they_ will program the Execution Unit for the correct precisions.
    
2) __LAMemUnits__: These are configurable data-streaming memory units. They read data-streams from the scratchpad and memory and feed them to the Execution Unit, and then take the output of the Execution Unit and write them back to scratchpad or memory. The underlying data-format, precision and location are configurations that you store in the LACfgs. The LACfgs are used to configure the LAMemUnits during and execution or data movement instruction. The LAMemUnit can read or write any of the following data formats:

    1) __scalar__: a single 32-bit or 64-bit floating point element
    2) __vector__: a vector of floating point elements. Can have `stride`, `count`, `skip` configuration fields
    3) __sparse matrix__: a compressed sparse matrix, similar to the Compressed Column Storage (CCS) format. Can have `data_ptr`, `idx_ptr`, `jdx_ptr`, `idx_cnt`, `jdx_cnt` and `data_skip` fields

    In addition to the 3 formats of data-streams, you can specify the precision of each stream (single or double), and the location (scratchpad or memory address). The LAMemUnits have a complex FSM that will take the data-source configuration and turn it into an abstract data-stream that it feeds into the Execution Unit. The Execution Unit, therefore, does not deal at all with the underlying data-sources or formats, and only sees abstracted input and output streams through FIFOs connected to the LAMemUnits.
    
3) __LACfgs__: As mentioned, the LACfgs provide the configuration for the data streams. The LACore developer will typically have to configure a few LACfgs using configuration instructions before each call to an execution instruction.

4) __Scratchpad__: The scratchpad is a 64 kB (can be larger or smaller) temporary memory for intermediate computation results, or to store data that is accessed frequently. It has a __separate address space__ starting at `0x0`. This means you can store a vector at address `0x0` inside the scratchpad, and it won't segfault, because it's not part of the main virtual address space.

## The LACore ISA
The LACore's ISA is an extension to the RISC-V ISA, which can be found on the [RISC-V website](https://riscv.org/specifications/). The `custom-0` opcode space is being used by the LACore. The LACore extension ISA has 68 instructions that are broken into 3 categories:

- __Execution Instructions__: 56 instructions that have 3 input streams and 1 output stream and perform some arithmetic operation.
- __Data Movement Instruction__: 3 instructions that moves 1 input stream to 1 output stream. Similar to a hardware-accelerated `memcpy()` but way more configurable and powerful
- __Configuration Instructions__: 9 instructions that configure LACfgs to hold a scalar, vector or sparse matrix data-source in the scratchpad, memory or the LACfg itself.

Please become familiar with the LACore architecture and terminology before trying to understand the LACore instructions. In the following sections, `lrd`, `lra`, `lrb`, and `lrc` can take the values `lr0`, `lr1`, `lr2`, `lr3`, `lr4`, `lr5`, `lr6`, or `lr7`, since there are 8 LACfgs (if your design has more or less LACfgs, you can have more of these). This is the syntax the assembler understands. `lrxa`, `lrxb` and `lrxc` can be any of the RISC-V general purpose integer registers, such as `x1`, `x2`, etc. Finally, `lrfa` can be any of the RISC-V general purpose floating-point registers, such as `f1`, `f2`, etc.

### Execution Instructions

There are 3 output-modes for execution instructions: vector-output, scalar-output and multi-stream output, as described in the Architecture Overview above. As mentioned, there are 8 vector-output, 24 scalar-output and 24 multi-stream-output instructions:

__Vector Output__
Vector Output instructions have the format `ldv{m|d}{a|s}{b|t}`. The switches configure only the Vector Unit within the datapath, and stand for:
    
- `{m|d}` is `m` for multiply and `d` for divide
- `{a|s}` is `a` for add and `s` for subtract
- `{b|t}` is `b` for `(A*/B)+-C` and `t` for `(A+-B)*/C`

There are 5 arguments to the Vector Output instructions: `lrd` is the index of the LACfg that is configured for the output stream, while `lra`, `lrb` and `lrc` are the LACfgs that are configured for the input streams. `lrxa` is the RISC-V general-purpose integer register that specifies how many input elements to operate on:

    ldvmab     lrd lra lrb lrc lrxa
    ldvmat     lrd lra lrb lrc lrxa
    ldvmsb     lrd lra lrb lrc lrxa
    ldvmst     lrd lra lrb lrc lrxa
    ldvdab     lrd lra lrb lrc lrxa
    ldvdat     lrd lra lrb lrc lrxa
    ldvdsb     lrd lra lrb lrc lrxa
    ldvdst     lrd lra lrb lrc lrxa

__Scalar Output__
There are 24 scalar output instructions, and they have the form: `ldr{m|d}{a|s}{b|t}{n|x|s}`. The switches configure both the Vector Unit and Reduction Unit within the datapath and stand for:

- `{m|d}` is `m` for multiply and `d` for divide
- `{a|s}` is `a` for add and `s` for subtract
- `{b|t}` is `b` for `(A*/B)+-C` and `t` for `(A+-B)*/C`
- `{n|x|s}` is `n` to reduce to using `min()`, `x` to reduce using `max()` and `s` to reduce using `sum()`

The 5 arguments are the same as those specified in the Vector Output section:

    ldrmabn    lrd lra lrb lrc lrxa
    ldrmabx    lrd lra lrb lrc lrxa
    ldrmabs    lrd lra lrb lrc lrxa
    ldrmatn    lrd lra lrb lrc lrxa
    ldrmatx    lrd lra lrb lrc lrxa
    ldrmats    lrd lra lrb lrc lrxa
    ldrmsbn    lrd lra lrb lrc lrxa
    ldrmsbx    lrd lra lrb lrc lrxa
    ldrmsbs    lrd lra lrb lrc lrxa
    ldrmstn    lrd lra lrb lrc lrxa
    ldrmstx    lrd lra lrb lrc lrxa
    ldrmsts    lrd lra lrb lrc lrxa
    ldrdabn    lrd lra lrb lrc lrxa
    ldrdabx    lrd lra lrb lrc lrxa
    ldrdabs    lrd lra lrb lrc lrxa
    ldrdatn    lrd lra lrb lrc lrxa
    ldrdatx    lrd lra lrb lrc lrxa
    ldrdats    lrd lra lrb lrc lrxa
    ldrdsbn    lrd lra lrb lrc lrxa
    ldrdsbx    lrd lra lrb lrc lrxa
    ldrdsbs    lrd lra lrb lrc lrxa
    ldrdstn    lrd lra lrb lrc lrxa
    ldrdstx    lrd lra lrb lrc lrxa
    ldrdsts    lrd lra lrb lrc lrxa
 
__Multi-Stream Output__   
There are 24 scalar output instructions, and they have the form: `ldrm{m|d}{a|s}{b|t}{n|x|s}`. The switches configure both the Vector Unit and Reduction Unit within the datapath and stand for:  

- `{m|d}` is `m` for multiply and `d` for divide
- `{a|s}` is `a` for add and `s` for subtract
- `{b|t}` is `b` for `(A*/B)+-C` and `t` for `(A+-B)*/C`
- `{n|x|s}` is `n` to reduce to using `min()`, `x` to reduce using `max()` and `s` to reduce using `sum()`

The 5 arguments are the same as those specified in the Vector Output section:
    
    ldrmmabn   lrd lra lrb lrc lrxa
    ldrmmabx   lrd lra lrb lrc lrxa
    ldrmmabs   lrd lra lrb lrc lrxa
    ldrmmatn   lrd lra lrb lrc lrxa
    ldrmmatx   lrd lra lrb lrc lrxa
    ldrmmats   lrd lra lrb lrc lrxa
    ldrmmsbn   lrd lra lrb lrc lrxa
    ldrmmsbx   lrd lra lrb lrc lrxa
    ldrmmsbs   lrd lra lrb lrc lrxa
    ldrmmstn   lrd lra lrb lrc lrxa
    ldrmmstx   lrd lra lrb lrc lrxa
    ldrmmsts   lrd lra lrb lrc lrxa
    ldrmdabn   lrd lra lrb lrc lrxa
    ldrmdabx   lrd lra lrb lrc lrxa
    ldrmdabs   lrd lra lrb lrc lrxa
    ldrmdatn   lrd lra lrb lrc lrxa
    ldrmdatx   lrd lra lrb lrc lrxa
    ldrmdats   lrd lra lrb lrc lrxa
    ldrmdsbn   lrd lra lrb lrc lrxa
    ldrmdsbx   lrd lra lrb lrc lrxa
    ldrmdsbs   lrd lra lrb lrc lrxa
    ldrmdstn   lrd lra lrb lrc lrxa
    ldrmdstx   lrd lra lrb lrc lrxa
    ldrmdsts   lrd lra lrb lrc lrxa

### Data Movement Instructions
There are 3 data movement instructions. The first is `lxferdata` and has the following format:

    lxferdata      lrd lra lrxa 
    
Here, `lrd` is the destination LACfg, `lra` is the source LACfg and `lrxa` holds the number of elements to trasfer from source to destination. The next instructions are `lxfercsrget` and `lxfercsrclear`, which just move data from the LACsrReg to an integer register, and clear the LACsrReg, respectively. They have the following formats:

    lxfercsrget            lrxa 
    lxfercsrclear               

The LACsrReg is 64-bits and just holds error conditions due to floating-point exceptions, poorly configured LACfgs, or any other LACore-related exception.

### Configuration Instructions

Finally, there are 9 configuration instruction. These are important to get right, and will probably be the source of many of the bugs developers encounter, because they are so configurable and powerful. The instructions allow you to configure the LACfg to hold a scalar, a vector or a sparse matrix. The scalar can be configured in 1 instruction (either `lcfgsx{s|d}` or `lcfgsf{s|d}`), while the vector and sparse matrix configurations require 2 instructions each (one of `lcfgvadrv` and `lcfgvadr{s|t}`, and one of `lcfgvlay{s|d}`). Additionally, you can specify the location of the data-source through a special sytnax: curly braces around `lrd` means it is in the scratchpad, smooth parenthesis around `lrd` means its in main memory, and no braces around `lrd` means the value is stored in the LACfg itself.

__scalar configuration__
A scalar can be configured using either `lcfgsx{s|d}` or `lcfgsf{s|d}`). Here `s` is used for single-precision and `d` is used for double-precision. The instructions take the following forms:

    lcfgsx{s|d} lrd lrxa
    lcfgsf{s|d} lrd lrfa
    
In the `lcfgsx{s|d}` instructions, you store the address of the scalar in the LACfg, while the in the `lcfgsf{s|d}` instructions, you store the actual floating-point value in the LACfg itself (so no scratchpad or memory accesses are needed during execution). Since `lcfgsx{s|d}` specifies a memory or scratchpad location, you have to use `()` or `{}` around `lrd`, respectively. For example:

    lcfgsxs (lr3) x5
    lcfgsxd {lr2} x8
    
The first one above will configure LACfg 3 to read the single-precision scalar located at the address in memory specified by the contents of integer register x5. The second instruction configures LACfg 2 to read the double-precision scalar located at the address in scratchpad specified by the contents of integer register x8.

__vector configuration__
Vector configuration in an LACfg takes 2 instructions: 1 to configure the address and the other to configure the layout in memory or scratchpad. The address configuration is done by the `lcfgvadrv` instruction and takes the form:

    lcfgvadrv  lrd lrxa
    
Where `lrxa` holds the address of the first element of the vector in memory or scratchpad. After you specify the address, you need to specify the layout of the vector, meaning the `stride`, `count`, and `skip`. You can refer to the images in the [RSVP processor](http://dl.acm.org/citation.cfm?id=956540) for a good idea of how these fields work. Its essentially just a gather-scatter interface. An important note is that `stride`, `count` and `skip` represent _elements_ not bytes. So a skip of 3 for a double-precision vector means a skip of 24 bytes. The syntax for these instructions are:

    lcfgvlay{s|d} lrd lrxa lrxb lrxc

Here, `lrxa` holds the `stride` value, `lrxb` holds the `count` value and `lrxc` holds the `skip` value. Note that `stride` and `skip` can be negative. Just like scalar address configuration, you need to specify whether the vector is in memory or scratchpad using special syntax around `lrd`: `{}` means scratchpad and `()` means memory. For example:

    lcfgvlays (lr0) x3 x4 x5
    
This above instruction configures LACfg 0 to hold the stride/count/skip of a vector in memory specified by the contents of integer registers `x3`/`x4`/`x5`.

__sparse configuration__
Sparse Matrix configuration in an LACfg takes 2 instructions: 1 to configure the pointers to the vectors of the compressed matrix, and the other to configure the layout in memory or scratchpad. The address configuration is done by the `lcfgvadr{s|t}` instructions and takes the form:

    lcfgvadr{s|t} lrd lrxa lrxb lrxc

Here, `s` means the LAMemUnits should read the sparse matrix normally, while `t` means the LAMemUnit should read the sparse matrix transposed (which is more complicated and slower). `lrxa` holds the pointer to the `data` vector, `lrxb` holds the pointer to the `idx` vector and `lrxc` holds the pointer to the `jdx` vector. `idx` corresponds to the major index and `jdx` corresponds to the minor index. So for a CCS matrix, `idx` holds the accumulated number of elements for each column, while in a CRS matrix, `idx` holds the accumulated number of elements for each row. After configuring the address pointers, you need to then issue the `lcfgvlay{s|d}` instruction to configure the extra layout information for the sparse matrix: the `idx_count`, `jdx_count` and `data_skip` fields. The instruction format is:

    lcfgvlay{s|d} lrd lrxa lrxb lrxc
    
Here, `lrxa` holds `idx_count` (major dimension of sparse matrix), `lrxb` holds `jdx_count` (minor dimension of sparse matrix), and `lrxc` holds `data_skip` (number of logical elements to skip when producing the data-stream). Note that this is the same instruction used in the vector configuration above. This is because it modifies the exact same contents of the LACfg register in the same way for both vectors and sparse matrices! It is a multi-purpose instruction. The same rules apply for scrachpad and memory location specification by using `{}` or `()` around `lrd`. 

## The LACoreAPI
Now you should be familiar with the LACore architecture and the instruction set. Additionally, you should have followed the [installation guide](Readme.md) and have the full riscv64 toolchain and spike simulator working in order to test any LACore programs you write. Assuming you have done all this, you have two options to generate LACore instructions:
    
1) write inline assembly in C whenever you want to perform an LACore instruction
2) use the LACoreAPI, which does 1) for you

It is recommended you follow approach 2), since the legwork has been done for you. The LACoreAPI is a header-only C-library (so no linking needed) that targets the LACore. Unfortunately, it takes a fairly long time to compile when using the library (10-20 seconds) due to the way it was developed. The LACoreAPI can be found in `linalg-benchmarks/la_core_api/include/la_core`. When using the LACoreAPI in your applications, you can just add `linalg-benchmarks/la_core_api/include` to your include path, and include the following in your `*.c` or `*.h` files:

    #include "la_core/la_core.h"
    #include "la_core/la_set_scalar.h"
    #include "la_core/la_set_vec.h"
    #include "la_core/la_set_spv.h"
    #include "la_core/la_copy.h"
    #include "la_core/la_data_op.h"
    
Each of the files above performs a different function.

### la_core.h
`la_core.h` contains the major type definitions used by all the other LACoreAPI files. You should always include this

### la_set_scalar.h
`la_set_scalar.h` contains the API to configure scalars in the LACfgs. It exposes the following functions:

    la_set_scalar_dp_reg(reg, val);
    la_set_scalar_sp_reg(reg, val);
    la_set_scalar_dp_mem(reg, addr);
    la_set_scalar_sp_mem(reg, addr);
    la_set_scalar_dp_sch(reg, addr);
    la_set_scalar_sp_sch(reg, addr);
    
Here, `reg` can be a number between 0-7 inclusive (since there are 8 LACfgs in the LACore). The first two instructions, you actually set `val` to the floating-point value to store in the LACfg, while the last 4 instructions, you set `addr` to the address of the scalar in memory or scratchpad.

### la_set_vec.h
`la_set_vec.h` contains the API to configure vectors in the LACfgs. It exposes the following functions:

    la_set_vec_dp_mem(reg, addr, stride, count, skip);
    la_set_vec_dp_sch(reg, addr, stride, count, skip);
    la_set_vec_sp_mem(reg, addr, stride, count, skip);
    la_set_vec_sp_sch(reg, addr, stride, count, skip);
    
    la_set_vec_adrcnt_dp_mem(reg, addr, count);
    la_set_vec_adrcnt_dp_sch(reg, addr, count);
    la_set_vec_adrcnt_sp_mem(reg, addr, count);
    la_set_vec_adrcnt_sp_sch(reg, addr, count);
    
    la_set_vec_adr_dp_mem(reg, addr);
    la_set_vec_adr_dp_sch(reg, addr);
    la_set_vec_adr_sp_mem(reg, addr);
    la_set_vec_adr_sp_sch(reg, addr);
    
The first four functions are self explanatory. The last 8 functions are just convenience wrappers around the first 4 functions, and default the `stride`, `count` and `skip` fields to sane values of 1, 1, and 0.

### la_set_spv.h
`la_set_spv.h` contains the API to configure sparse matrices in the LACfgs. It exposes the following functions:

    la_set_spv_nrm_dp_mem(data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, data_skip);
    la_set_spv_nrm_dp_sch(data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, data_skip);
    la_set_spv_nrm_sp_mem(data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, data_skip);
    la_set_spv_nrm_sp_sch(data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, data_skip);
    
    la_set_spv_tns_dp_mem(data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, data_skip);
    la_set_spv_tns_dp_sch(data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, data_skip);
    la_set_spv_tns_sp_mem(data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, data_skip);
    la_set_spv_tns_sp_sch(data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, data_skip);
    
The first four instructions configure normal sparse matrix data-streams, while the last 4 instructions configure transposed matrix data-streams. The fields are self-explanatory (read the ISA section for sparse configuration if you don't know what any of the 6 fields mean).

### la_copy.h
`la_copy.h` contains the API to transfer data from a source specified by an LACfg to a destination specified by another LACfg. `la_copy.h` exposes the following API:

    la_copy(dst, src, count);
    
This is pretty much a sophisticated, hardware-accelerated `memcpy()`. count specifies the number of elements to transfer. `dst` and `src` are integers in range 0-7 inclusive, both pointing to previously-configured LACfgs. They can hold values that are either 32-bit or 64-bit floating point, and they don't have to be the same precision as each other. Also, `dst` and `src` don't have to be the same data format. For example, `dst` can be a vector, while `src` is a scalar, or a sparse matrix. 

### la_data_op.h
`la_data_op.h` contains the API to perform an arithmetic operation using 3 input data-sources and 1 output data-source. 

It exposes the following vector-output functions:

    AaddBmulC(D_reg, A_reg, B_reg, C_reg, count);
    AmulBaddC(D_reg, A_reg, B_reg, C_reg, count);
    AsubBmulC(D_reg, A_reg, B_reg, C_reg, count);
    AmulBsubC(D_reg, A_reg, B_reg, C_reg, count);
    AaddBdivC(D_reg, A_reg, B_reg, C_reg, count);
    AdivBaddC(D_reg, A_reg, B_reg, C_reg, count);
    AsubBdivC(D_reg, A_reg, B_reg, C_reg, count);
    AdivBsubC(D_reg, A_reg, B_reg, C_reg, count);

It exposes the following scalar-output functions:

    AaddBmulC_min(D_reg, A_reg, B_reg, C_reg, count);
    AmulBaddC_min(D_reg, A_reg, B_reg, C_reg, count);
    AsubBmulC_min(D_reg, A_reg, B_reg, C_reg, count);
    AmulBsubC_min(D_reg, A_reg, B_reg, C_reg, count);
    AaddBdivC_min(D_reg, A_reg, B_reg, C_reg, count);
    AdivBaddC_min(D_reg, A_reg, B_reg, C_reg, count);
    AsubBdivC_min(D_reg, A_reg, B_reg, C_reg, count);
    AdivBsubC_min(D_reg, A_reg, B_reg, C_reg, count);
    AaddBmulC_max(D_reg, A_reg, B_reg, C_reg, count);
    AmulBaddC_max(D_reg, A_reg, B_reg, C_reg, count);
    AsubBmulC_max(D_reg, A_reg, B_reg, C_reg, count);
    AmulBsubC_max(D_reg, A_reg, B_reg, C_reg, count);
    AaddBdivC_max(D_reg, A_reg, B_reg, C_reg, count);
    AdivBaddC_max(D_reg, A_reg, B_reg, C_reg, count);
    AsubBdivC_max(D_reg, A_reg, B_reg, C_reg, count);
    AdivBsubC_max(D_reg, A_reg, B_reg, C_reg, count);
    AaddBmulC_sum(D_reg, A_reg, B_reg, C_reg, count);
    AmulBaddC_sum(D_reg, A_reg, B_reg, C_reg, count);
    AsubBmulC_sum(D_reg, A_reg, B_reg, C_reg, count);
    AmulBsubC_sum(D_reg, A_reg, B_reg, C_reg, count);
    AaddBdivC_sum(D_reg, A_reg, B_reg, C_reg, count);
    AdivBaddC_sum(D_reg, A_reg, B_reg, C_reg, count);
    AsubBdivC_sum(D_reg, A_reg, B_reg, C_reg, count);
    AdivBsubC_sum(D_reg, A_reg, B_reg, C_reg, count);

It exposes the following multi-stream output functions:

    AaddBmulC_min_multi(D_reg, A_reg, B_reg, C_reg, count);
    AmulBaddC_min_multi(D_reg, A_reg, B_reg, C_reg, count);
    AsubBmulC_min_multi(D_reg, A_reg, B_reg, C_reg, count);
    AmulBsubC_min_multi(D_reg, A_reg, B_reg, C_reg, count);
    AaddBdivC_min_multi(D_reg, A_reg, B_reg, C_reg, count);
    AdivBaddC_min_multi(D_reg, A_reg, B_reg, C_reg, count);
    AsubBdivC_min_multi(D_reg, A_reg, B_reg, C_reg, count);
    AdivBsubC_min_multi(D_reg, A_reg, B_reg, C_reg, count);
    AaddBmulC_max_multi(D_reg, A_reg, B_reg, C_reg, count);
    AmulBaddC_max_multi(D_reg, A_reg, B_reg, C_reg, count);
    AsubBmulC_max_multi(D_reg, A_reg, B_reg, C_reg, count);
    AmulBsubC_max_multi(D_reg, A_reg, B_reg, C_reg, count);
    AaddBdivC_max_multi(D_reg, A_reg, B_reg, C_reg, count);
    AdivBaddC_max_multi(D_reg, A_reg, B_reg, C_reg, count);
    AsubBdivC_max_multi(D_reg, A_reg, B_reg, C_reg, count);
    AdivBsubC_max_multi(D_reg, A_reg, B_reg, C_reg, count);
    AaddBmulC_sum_multi(D_reg, A_reg, B_reg, C_reg, count);
    AmulBaddC_sum_multi(D_reg, A_reg, B_reg, C_reg, count);
    AsubBmulC_sum_multi(D_reg, A_reg, B_reg, C_reg, count);
    AmulBsubC_sum_multi(D_reg, A_reg, B_reg, C_reg, count);
    AaddBdivC_sum_multi(D_reg, A_reg, B_reg, C_reg, count);
    AdivBaddC_sum_multi(D_reg, A_reg, B_reg, C_reg, count);
    AsubBdivC_sum_multi(D_reg, A_reg, B_reg, C_reg, count);
    AdivBsubC_sum_multi(D_reg, A_reg, B_reg, C_reg, count);
    
In all the above functions, `D_reg` is an integer from 0-7 inclusive and points to the destination LACfg. `A_reg`, `B_reg` and `C_reg` are integers from 0-7 inclusive and point to the A, B, and C source registers. `count` is the number of input elements to operate on (not necessarily the number of output elements in the case of scalar output and multi-stream output). Also, the names of the functions describe how the datapath is configured, and they correspond to the 8 different Vector Unit configurations and 3 different Reduction Unit configurations described in the Architecture section above. 

### Examples of LACoreAPI usage
The LACoreAPI is used heavily in all the HPCC benchmarks found in the `linalg-benchmarks/benchmarks` folder. Also, you can find examples of sparse-matrix usage in the `linalg-benchmarks/sparse_benchmarks` folder. 