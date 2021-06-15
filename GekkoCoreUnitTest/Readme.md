# GekkoCoreUnitTest

Project for testing GekkoCore.

## GekkoIsaTests

The Gekko ISA tests are a hybrid approach, to test the correctness of Gekko emulation in all modes (interpreter, recompiler).

First, the correctness of the interpreter is checked, by comparing Gekko registers values after an instruction execution with expected results.

After that, a single instruction is recompiled as a pseudo-segment from a single instruction and the execution in recompilation mode is compared with expected results.

Only the correctness of instructions (register values) is checked during testing. Special modes of operation or exceptions are tested in other tests, so as not to complicate the subject area of testing.
