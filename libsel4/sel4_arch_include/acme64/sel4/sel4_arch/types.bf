--
-- Copyright 2022, Axel Heider <axelheider@gmx.de>
-- Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
--
-- SPDX-License-Identifier: BSD-2-Clause
--

#include <autoconf.h>

base 64

---- Arch-independent object types

block VMFault {
    padding   512
    field     IP                64
    field     Addr              64
    field     PrefetchFault     64
    padding                     32
    field     FSR               5
    padding                     8
    padding                     15
    field     seL4_FaultType    4
}

-- VM attributes
block vm_attributes {
    padding                     32
    padding                     31
    field     acmeExecuteNever   1
}

block NullFault {
   padding                    704
   padding                     60
   field      seL4_FaultType    4
}

block CapFault {
   padding                          256
   field      IP                     64
   field      Addr                   64
   field      InRecvPhase            64
   field      LookupFailureType      64
   -- these vary according to LookupFailureType
   field      MR4                    64
   field      MR5                    64
   field      MR6                    64
   padding                           60
   field      seL4_FaultType          4
}

block UnknownSyscall {
   field      FaultIP                64
   field      SP                     64
   field      LR                     64
   field      R0                     64
   field      R1                     64
   field      R2                     64
   field      R3                     64
   field      Syscall                64
   padding                           60
   field      seL4_FaultType          4
}

block UserException {
   padding                          448
   field      FaultIP                64
   field      SP                     64
   field      Number                 64
   field      Code                   64
   padding                           60
   field      seL4_FaultType          4
}

#ifdef CONFIG_KERNEL_MCS
block Timeout {
    padding                        576
    field     data                  64
    field     consumed              64
    padding                         60
    field     seL4_FaultType         4
}
#endif

#include <sel4/arch/shared_types.bf>
