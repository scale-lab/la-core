basic operations on vectors/scalars

   vector output (any/all of a, b or c is scalar)
      A*/(B+-C)
      (A+-B)*/C

   scalar output (any/all of a, b, or c is scalar)
      for each element in vector A,B,C:
         sum,min,max => A*/(B+-C)

   operation must specify
      if */
      if +-
      if A*/(B+-C) or (A+-B)*/C
      if sum,min or max for reduction
      total length of vectors

-------------------------------------------------------------------------------
basic configuration operations
   configure A operand
      is scalar or vector
      is register or memory if scalar
      what register if register and scalar
      offset, stride, count, skip if vector
      scratchpad or main memory if memory and scalar or vector

   configure B operand
      is scalar or vector
      is register or memory if scalar
      what register if register and scalar
      offset, stride, count, skip if vector
      scratchpad or main memory if memory and scalar or vector

   configure C operand
      is scalar or vector
      is register or memory if scalar
      what register if register and scalar
      offset, stride, count, skip if vector
      scratchpad or main memory if memory and scalar or vector

   configure D output
      is register or memory if scalar
      what register if register and scalar
      offset, stride, count, skip if vector
      scratchpad or main memory if memory and scalar or vector

-------------------------------------------------------------------------------
moving data from mutually exclusive locations

   move scalar from gp/fp reg to extension reg

   move scalar from extension reg to gp/fp reg

   move scalar from scratch/main memory to extension reg

   move scalar from extension reg to scratch/main memory

   move vector from scratchpad to main memory
      offset, stride, count, skip, total_count of input vector
      offset, stride, count, skip, total_count of output vector

   move vector from main memory to scratchpad memory
      offset, stride, count, skip, total_count of input vector
      offset, stride, count, skip, total_count of output vector

-------------------------------------------------------------------------------
basic memory organization for A, B, C
   A, B, C, D
      is scalar 
         in register(extension register, not x0-31)
         or in memory
            in scratchpad memory
            or in main memory

      or is vector, in memory
         in scratchpad memory
         or in main memory

         memory offset (+), stride (+-), count (+), skip (+-)

-------------------------------------------------------------------------------
since general purpose math can be done on gp and fp registers, we only provide
   interface to move items from gp/fp into ext regs. this prevents redundancy

-------------------------------------------------------------------------------
the extension only works on 'vectors' but it also accepts sparse matrix 
encodings. the 'sxcfgvadr' command then sets the offset into the compressed
matrix to start at, so the sxcfgvadr command is multipurpose