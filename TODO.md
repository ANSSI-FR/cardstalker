# TODO list


## Buffer library

 - Adding tests on BUFF_Move()
 - Adding tests on BUFF_Copy()
 
## State machine
 - Implement timeouts on the ACKs
 - Implement LRC checks ...
 - Implement BUSY block emission when reception context is locked on byte reception.
 - README to explain how to use the SM lib.
 - Add a TIMEOUT response from the card.
 
Document pinout.

* Make EvoloveSendState to return SM_EMPTY to prevent a call even if not using interrupts.
* What happen when calling evolve state ... when no BlockReceive/Send ??
* Deadlock with the Do ... while in bridge 2 ??
* Remove enable Txe etc ... code from bridge 2 internals 
