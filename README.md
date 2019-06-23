# ZOFI: The Zero Overhead Fault Injector Project
![ZOFI logo](zofi_logo.gif)

ZOFI is a Zero Overhead Fault Injector.
It is a tool for very fast fault coverage analysis of transient faults on binaries.

---

# Quick Build/Install Instructions

#### Build Dependencies
ZOFI has been tested on relatively modern x86_64 Linux 4.4 or later systems with glibc 2.23 or later.
The main external build dependencies are:

- A C++14 compiler (<http://gcc.gnu.org/>)
- CMake 3.2 or later (<https://cmake.org/>)
- Capstone 4.0.1 or later (<https://www.capstone-engine.org/>)

#### Build Instructions
Skip this section if capstone 4.0.1 is already installed on your system:

```sh
    $ wget https://github.com/aquynh/capstone/archive/4.0.1.tar.gz
    $ tar -xvf 4.0.1.tar.gz
    $ cd capstone-4.0.1/ && ./make.sh && sudo make install
    $ sudo ldconfig
```

Download, build and install ZOFI:

```sh
    $ git clone https://github.com/vporpo/zofi
    $ cd zofi
    $ mkdir build && cd build
    $ cmake ../src/ && make -j && sudo make install
```
This will install zofi into `/usr/local/bin/`.
Please note that ZOFI can run straight from your build directory, so you could skip `&& sudo make install`.

Note: For a debug build you need to pass the `-DCMAKE_BUILD_TYPE=Debug` parameter to cmake:

```sh
    $ cmake -DCMAKE_BUILD_TYPE=Debug ../src/
```

#### Run
If all went well, you can now run ZOFI:

```sh
    $ zofi -help
```

Or if you did not install it on your system, you can run it straight from the build directory:

```sh
    $ ./zofi -help
```

# Getting Started (Quick Howto)
The main option that you need to specify is the path to the binary. This will run the binary once to check its original state, then it will run it once more injecting a fault in the process and will report the results.

```sh
    $ zofi -bin <path/to/binary> [Other options]
```

Here is an example:

```sh
    $ zofi -bin /usr/bin/seq -test-runs 50 -args -f '%g' 1 500000
```

This runs the `/usr/bin/seq` binary with arguments: `-f '%g' 1 500000` for `50` runs.

Here is what you will see on the terminal:

```
    $ zofi -bin /usr/bin/seq -test-runs 50 -args -f '%g' 1 500000
    ------ Options ------
    -args                  = -f %g 1 500000
    -bin                   = /usr/bin/seq
    -test-runs             = 50
    ---------------------
    -- Original (Timing) Run --
    4.56r/s        1/1       : 0%=========25%=========50%=========75%========100%
    Original Duration: 0.219s
    
    -- Test Runs --
    2.79r/s       50/50      : 0%=========25%=========50%=========75%========100%
    
    -------------------------------
    Outcome         : Cnt,     %
    -------------------------------
    Masked          :  10,  20.000%
    Exception       :  15,  30.000%
    InfExec         :   2,   4.000%
    Corrupted       :  23,  46.000%
```

To speed-up the execution on a multi-core system you can use `-j N` where `N` is the number of parallel jobs.
For example, on a quad-core system we would run 4 jobs like this:

```
    $ zofi -bin /usr/bin/seq -test-runs 50 -j 4 -args -f '%g' 1 500000
```

Please note that if you need to pass a `-args` option to ZOFI, it will have to be placed last in command line, after all other options. The reason is that everything that follows it is considered an argument of the binary being tested, not ZOFI's.

Also the environmental variables are transparent to ZOFI and get passed through to the binary.

You can list all options with `zofi -help`.

# Features

### Fault Injection Types
ZOFI supports various types of fault injections with the `-inject-to <TYPE STRING>` flag. Each character in the type string enables a specific class of injection. The default type string enables faults to explicitly (e) read(r) and written(w) registers: `"rwe"`. The types that are currently supported are:

1. Inject into registers read(`r`), or written(`w`) by the instruction. The default option enables both.

2. Inject to explicitly(`e`) or implicitly(`i`) accessed registers. A large number of x86 instructions will clobber registers that are not explicitly stated in the assembly code, such as the zero flag. The default option allows only the explicitly(`e`) accessed.

3. Inject errors to the instruction pointer if the current instruction is a control-flow(`c`) instruction, or other(`o`) type of instruction. ZOFI will not inject to either of them by default.

|Option | Inject to                       |
|-------|---------------------------------|
| `r`   | Registers read by instruction |
| `w`   | Registers written by instruction |
| `e`   | Explicitly accessed registers |
| `i`   | Implicitly accessed registers |
| `c`   | Instruction pointer if a control instruction|
| `o`   | Instruction pointer if not a control instruction|

### Multiple Test Runs in Parallel
ZOFI supports running multiple test runs in parallel to speed up the fault injection process.
The number of parallel jobs is controlled with the `-j` switch which defaults to 1.
Using multiple parallel jobs can speed up the evaluation significantly, as the test run throughput increases almost linearly with the number of jobs executed.

Please note that it is not advised to use higher jobs count than the number of threads supported by the target CPU, as this will mess with the timings of the injections.

### Support for Multi-Threaded Workloads (since v0.9.4)
ZOFI supports injecting faults to multi-threaded applications since version 0.9.4.
The process is very similar to single-threaded fault injection.
The main difference is that we now have to select a random running thread to inject the fault to.
Please note that the fault injection happens only to the selected thread, the rest of them are not even stopped.


### Custom Diff Command
When ZOFI checks the test run's output (stdout and stderr) against that from the original run, it uses a simple one to one comparison between them. If any discrepancy is found, it will be reported as "Corrupted" output.

This simplistic diff function, can sometimes falsely report "Corrupted" outputs even though no corruption took place.
This can happen when the output contains unique strings to each run, for example the current time, or the process id. In such cases a more suitable diff function would skip these patterns and would compare only the remaining output.

ZOFI provides the capability to do so, using a custom command with `-diff-cmd`.
This spawns a shell (/bin/bash by default but can be modified with `-diff-shell`) and executes the provided command within it.
If the executed commands exit normally with an exit code of 0, then the output is considered identical, otherwise it is considered "Corrupted".

For example, if we need to skip any pattern matching the regular expression `^Time:`, we would use the following:

```
    -diff-cmd '/usr/bin/diff <(/bin/grep -v "^Time" %ORIG_STDOUT) <(/bin/grep -v "^Time" %TEST_STDOUT)'
```

For more accurate checking, we should check both stdout and stderr, by connecting the two diffs with &&.

```
    -diff-cmd '/usr/bin/diff <(/bin/grep -v "^Time" %ORIG_STDOUT) <(/bin/grep -v "^Time" %TEST_STDOUT) && /usr/bin/diff %ORIG_STDOUT %TEST_STDERR'
```

Another example is the NAS Benchmarks, which need the following command to skip the "Time in seconds", "CPU Time", "Initialization time", "Mop/s total" , and "Compile date" lines.

```
    -diff-cmd '/usr/bin/diff <(/bin/grep -v "\([Tt]ime\)\|\(Mop/s total\)\|\(Compile date\)" %ORIG_STDOUT) <(/bin/grep -v "\([Tt]ime\)\|\(Mop/s total\)\|\(Compile date\)" %TEST_STDOUT)'
```


### Disabling fault injection to library code
By default ZOFI will inject faults to any instruction.
This can be problematic for fault-tolerance studies where the user's code has been protected by some fault-tolerance scheme, while the system's libraries have not.
ZOFI supports disabling fault injection to .so libraries that are dynamically linked to the executable, with the `-no-inject-to-libs` flag.



# Considerations

#### 1. Selecting the number of jobs -j N

##### i. Multi-Threding
Most modern machines implement Simultaneous Multi-Threading (SMT), which support more logical cores than the actual physical cores (usually twice as many).
It is advised to set the number of jobs no higher than the number of physical cores.
Otherwise, the timings may be affected considerably and that could affect the randomness of the experiments.

On single socket machines:

```
    $ cat /proc/cpuinfo | grep "cpu cores" | uniq
```

The number of sockets:

```
    $ cat /proc/cpuinfo | grep "physical id" | sort | uniq
```
 
The total number of physical cores is the number of cpu cores times the number of sockets.

##### ii. Shared Resources
Workloads that occupy a lot of shared resources, e.g., memory intensive workloads, need special consideration.
Running many of them in parallel can lead to unpredictable timings.
Therefore it is advised to run such workloads using fewer jobs.

##### iii. Variable Execution Time
Workloads with execution time that varies a from run to run.
The original timing run will most probably have a wrong value, therefore the user should probably override it with the `-bin-exec-time` flag, or use a very large overshoot value `-bin-exec-time-overshoot`.

##### iv. Frequency throttling
Most modern systems will throttle the processor frequency once the temperature of the processor becomes too high.
This can affect the timings too.
So please make sure to either use fewer jobs, or improve the cooling of your processor.


#### 2. Compiler's Built-in Error Detection using Hardening Techniques
Some compilers, like GCC, implement stack protection techniques to help protect from attacks.
Such techniques will report the error and the process will abort early, converting a potential exception to a corrupted output outcome.
If you are interested in measuring the application's raw fault coverage with no protections whatsoever, you may consider building your workload with such techniques disabled (for example `-fno-stack-protector`).




# Limitations
ZOFI has some well-known limitations:

#### 1. Limitations related to timing
Injecting faults on applications with either:
1. a very small run-time (e.g., less than 50ms), or
2. variable execution time,
is problematic.
ZOFI may end up trying to inject a fault when the application has already stopped.

ZOFI will try to tolerate such failures by retrying at some other random injection time, but if the failures build up to a large number, greater than `-max-injection-attempts`, then it will exit with an error.

Please note that the synchronization between the processes takes some time and the earliest a fault can be injected on a generic system is about half a millisecond.

#### 2. Workloads that misbehave when being stopped
If a workload uses signals as part of its normal operation, it may not expect to be signaled externally, and may stop working properly if so.
On such workloads ZOFI may report false results.

An example of this is /bin/sleep from GNU `coreutils`, which will return right after it receives a signal from ZOFI.


## Supported architectures
Even though the design of ZOFI is not tied to a specific architecture, it does have some minor glue code that is architecture-specific.
The current implementation only supports x86_64, but supporting other ISAs should require little effort.
For the most part ZOFI relies on the capstone-engine library (<https://www.capstone-engine.org/>) for the instruction semantics, which already supports several ISAs.



# Basic Testing: Zit test suite
ZOFI includes a test-suite in the `test/` directory.
You can run the whole suite by running `$ make check` from within the `build/` directory.

Tests are C/C++ source files with testing instructions prefixed by `// RUN:`.
All `// RUN:` lines are parsed by ZOFI's testing tool, ZIT (ZOFI Integrated Testing).
For details on how to use Zit, please see Zit's documentation in the sections that follow.


Zit is a testing script built to test source files with `// RUN:` test commands.
A typical test command includes three parts, usually connected with `&&` and pipes `|`.

1. The compilation CC/CXX part,
2. The ZOFI part,
3. The GREP part.

For example, the following test command first compiles the source file, then calls zofi, and finally uses grep to check for a specific pattern.

```
    // RUN: %CC %THIS_FILE -o %UNIQUE_FILE && PraiseBob=1 %ZOFI -bin %UNIQUE_FILE -test-runs 1 -v 1 -injection-time 1 -injections-per-run 0 -no-test-redirect 2>&1 | %GREP 'PraiseBob=1' 2>&1 > /dev/null
```

If the command returns 0, it passes. If not, it is reported as a failure.

#### Variable Substitution
Since we want to avoid hard-coded tool paths and filenames, Zit supports several automatically substituted variables, all prefixed by `%`.

| Macros         | Substituted by |
|----------------|----------------|
| `%CC`          | The C compiler specified in Zit and can be overriden with an argument.  |
| `%THIS_FILE`   | The file name of the zit test file (i.e., the source file).|
| `%UNIQUE_FILE` | A unique file path, usually in the form of `/tmp/tmp.xxx`.|
| `%ZOFI`        | The zofi tool binary. |
| `%GREP`        | The grep tool binary. |

#### Builtins
Zit also supports a range of builtin functions that can help with checking the output of ZOFI.

| Builtin        | Functionality |
|----------------|---------------|
|`%GREATER_THAN` | Takes one argument and checks whether the stdin is > the argument.     |
|`%LESS_THAN`    | Takes one argument and checks whether the stdin is < the argument.     |
| `%EQUALS`      | Takes one argument and checks whether the stdin is == to the argument. |
| `%GET_OUTCOME` `<Masked\|Exception\|InfExec\|Corrupted>` `<N\|%>` | Returns the fault coverage outcome. `N` returns the absolute count, while `%` returns the percentage.|


#### ZIT Command line arguments
This is the command line syntax of the Zit tool:

```
    Usage: zit <test.zit> [-zofi /path/to/zofi] [-cc /path/to/c/compiler] [-cxx /path/to/c++/compiler] [-help]
```

- `test.zit` Is one or more zit test files.
- `-s` Silent mode. Print only the absolute minimum.
- `-zofi /path/to/zofi` Override the zofi binary path.
- `-cc /path/to/cc` Override the C compiler path.
- `-cxx /path/to/cxx` Override the C++ compiler path.
- `-help` Prints a brief help message.

# Disclaimer

ZOFI is under development, so please expect it to fail.
You can help improve it by filing a bug report, or by contributing a fix.

# License

ZOFI is free software and is distributed under the GPL License version 2. <http://www.gnu.org/licenses/>

---
Contact: Vasileios Porpodas (v dot last name at gmail.com)
