/*
 * Copyright 2005 The Regents of The University of Michigan
 * Copyright 2007 MIPS Technologies, Inc.
 * Copyright 2016 The University of Virginia
 * Copyright 2020 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "arch/riscv/linux/se_workload.hh"

#include <sys/syscall.h>

#include "arch/riscv/process.hh"
#include "base/loader/object_file.hh"
#include "base/trace.hh"
#include "cpu/thread_context.hh"
#include "sim/syscall_emul.hh"

namespace gem5
{

namespace
{

class LinuxLoader : public Process::Loader
{
  public:
    Process *
    load(const ProcessParams &params, loader::ObjectFile *obj) override
    {
        auto arch = obj->getArch();
        auto opsys = obj->getOpSys();

        if (arch != loader::Riscv64 && arch != loader::Riscv32)
            return nullptr;

        if (opsys == loader::UnknownOpSys) {
            warn("Unknown operating system; assuming Linux.");
            opsys = loader::Linux;
        }

        if (opsys != loader::Linux)
            return nullptr;

        if (arch == loader::Riscv64)
            return new RiscvProcess64(params, obj);
        else
            return new RiscvProcess32(params, obj);
    }
};

LinuxLoader linuxLoader;

} // anonymous namespace

namespace RiscvISA
{

void
EmuLinux::syscall(ThreadContext *tc)
{
    Process *process = tc->getProcessPtr();
    // Call the syscall function in the base Process class to update stats.
    // This will move into the base SEWorkload function at some point.
    process->Process::syscall(tc);

    RegVal num = tc->getReg(RiscvISA::SyscallNumReg);
    if (dynamic_cast<RiscvProcess64 *>(process))
        syscallDescs64.get(num)->doSyscall(tc);
    else
        syscallDescs32.get(num)->doSyscall(tc);
}

/// Target uname() handler.
static SyscallReturn
unameFunc64(SyscallDesc *desc, ThreadContext *tc, VPtr<Linux::utsname> name)
{
    auto process = tc->getProcessPtr();

    strcpy(name->sysname, "Linux");
    strcpy(name->nodename,"sim.gem5.org");
    strcpy(name->release, process->release.c_str());
    strcpy(name->version, "#1 Mon Aug 18 11:32:15 EDT 2003");
    strcpy(name->machine, "riscv64");

    return 0;
}

/// Target uname() handler.
static SyscallReturn
unameFunc32(SyscallDesc *desc, ThreadContext *tc, VPtr<Linux::utsname> name)
{
    auto process = tc->getProcessPtr();

    strcpy(name->sysname, "Linux");
    strcpy(name->nodename,"sim.gem5.org");
    strcpy(name->release, process->release.c_str());
    strcpy(name->version, "#1 Mon Aug 18 11:32:15 EDT 2003");
    strcpy(name->machine, "riscv32");

    return 0;
}

SyscallDescTable<SEWorkload::SyscallABI64> EmuLinux::syscallDescs64 = {
    { 0,    "io_setup" },
    { 1,    "io_destroy" },
    { 2,    "io_submit" },
    { 3,    "io_cancel" },
    { 4,    "io_getevents" },
    { 5,    "setxattr" },
    { 6,    "lsetxattr" },
    { 7,    "fsetxattr" },
    { 8,    "getxattr" },
    { 9,    "lgetxattr" },
    { 10,   "fgetxattr" },
    { 11,   "listxattr" },
    { 12,   "llistxattr" },
    { 13,   "flistxattr" },
    { 14,   "removexattr" },
    { 15,   "lremovexattr" },
    { 16,   "fremovexattr" },
    { 17,   "getcwd", getcwdFunc },
    { 18,   "lookup_dcookie" },
    { 19,   "eventfd2" },
    { 20,   "epoll_create1" },
    { 21,   "epoll_ctl" },
    { 22,   "epoll_pwait" },
    { 23,   "dup", dupFunc },
    { 24,   "dup3" },
    { 25,   "fcntl", fcntl64Func },
    { 26,   "inotify_init1" },
    { 27,   "inotify_add_watch" },
    { 28,   "inotify_rm_watch" },
    { 29,   "ioctl", ioctlFunc<RiscvLinux64> },
    { 30,   "ioprio_get" },
    { 31,   "ioprio_set" },
    { 32,   "flock" },
    { 33,   "mknodat", mknodatFunc<RiscvLinux64> },
    { 34,   "mkdirat", mkdiratFunc<RiscvLinux64> },
    { 35,   "unlinkat", unlinkatFunc<RiscvLinux64> },
    { 36,   "symlinkat" },
    { 37,   "linkat" },
    { 38,   "renameat", renameatFunc<RiscvLinux64> },
    { 39,   "umount2" },
    { 40,   "mount" },
    { 41,   "pivot_root" },
    { 42,   "nfsservctl" },
    { 43,   "statfs", statfsFunc<RiscvLinux64> },
    { 44,   "fstatfs", fstatfsFunc<RiscvLinux64> },
    { 45,   "truncate", truncateFunc<RiscvLinux64> },
    { 46,   "ftruncate", ftruncate64Func },
    { 47,   "fallocate", fallocateFunc<RiscvLinux64> },
    { 48,   "faccessat", faccessatFunc<RiscvLinux64> },
    { 49,   "chdir", chdirFunc },
    { 50,   "fchdir" },
    { 51,   "chroot" },
    { 52,   "fchmod", fchmodFunc<RiscvLinux64> },
    { 53,   "fchmodat" },
    { 54,   "fchownat" },
    { 55,   "fchown", fchownFunc },
    { 56,   "openat", openatFunc<RiscvLinux64> },
    { 57,   "close", closeFunc },
    { 58,   "vhangup" },
    { 59,   "pipe2", pipe2Func },
    { 60,   "quotactl" },
#if defined(SYS_getdents64)
    { 61,   "getdents64", getdents64Func },
#else
    { 61,   "getdents64" },
#endif
    { 62,   "lseek", lseekFunc },
    { 63,   "read", readFunc<RiscvLinux64> },
    { 64,   "write", writeFunc<RiscvLinux64> },
    { 66,   "writev", writevFunc<RiscvLinux64> },
    { 67,   "pread64", pread64Func<RiscvLinux64> },
    { 68,   "pwrite64", pwrite64Func<RiscvLinux64> },
    { 69,   "preadv" },
    { 70,   "pwritev" },
    { 71,   "sendfile" },
    { 72,   "pselect6" },
    { 73,   "ppoll" },
    { 74,   "signalfd64" },
    { 75,   "vmsplice" },
    { 76,   "splice" },
    { 77,   "tee" },
    { 78,   "readlinkat", readlinkatFunc<RiscvLinux64> },
    { 79,   "fstatat", fstatat64Func<RiscvLinux64> },
    { 80,   "fstat", fstat64Func<RiscvLinux64> },
    { 81,   "sync" },
    { 82,   "fsync" },
    { 83,   "fdatasync" },
    { 84,   "sync_file_range2" },
    { 85,   "timerfd_create" },
    { 86,   "timerfd_settime" },
    { 87,   "timerfd_gettime" },
    { 88,   "utimensat" },
    { 89,   "acct" },
    { 90,   "capget" },
    { 91,   "capset" },
    { 92,   "personality" },
    { 93,   "exit", exitFunc },
    { 94,   "exit_group", exitGroupFunc },
    { 95,   "waitid" },
    { 96,   "set_tid_address", setTidAddressFunc },
    { 97,   "unshare" },
    { 98,   "futex", futexFunc<RiscvLinux64> },
    { 99,   "set_robust_list", ignoreWarnOnceFunc },
    { 100,  "get_robust_list", ignoreWarnOnceFunc },
    { 101,  "nanosleep", ignoreWarnOnceFunc },
    { 102,  "getitimer" },
    { 103,  "setitimer" },
    { 104,  "kexec_load" },
    { 105,  "init_module" },
    { 106,  "delete_module" },
    { 107,  "timer_create" },
    { 108,  "timer_gettime" },
    { 109,  "timer_getoverrun" },
    { 110,  "timer_settime" },
    { 111,  "timer_delete" },
    { 112,  "clock_settime" },
    { 113,  "clock_gettime", clock_gettimeFunc<RiscvLinux64> },
    { 114,  "clock_getres", clock_getresFunc<RiscvLinux64> },
    { 115,  "clock_nanosleep" },
    { 116,  "syslog" },
    { 117,  "ptrace" },
    { 118,  "sched_setparam" },
    { 119,  "sched_setscheduler" },
    { 120,  "sched_getscheduler" },
    { 121,  "sched_getparam" },
    { 122,  "sched_setaffinity" },
    { 123,  "sched_getaffinity", schedGetaffinityFunc<RiscvLinux64> },
    { 124,  "sched_yield", ignoreWarnOnceFunc },
    { 125,  "sched_get_priority_max" },
    { 126,  "sched_get_priority_min" },
    { 127,  "scheD_rr_get_interval" },
    { 128,  "restart_syscall" },
    { 129,  "kill" },
    { 130,  "tkill" },
    { 131,  "tgkill", tgkillFunc<RiscvLinux64> },
    { 132,  "sigaltstack" },
    { 133,  "rt_sigsuspend", ignoreWarnOnceFunc },
    { 134,  "rt_sigaction", ignoreWarnOnceFunc },
    { 135,  "rt_sigprocmask", ignoreWarnOnceFunc },
    { 136,  "rt_sigpending", ignoreWarnOnceFunc },
    { 137,  "rt_sigtimedwait", ignoreWarnOnceFunc },
    { 138,  "rt_sigqueueinfo", ignoreWarnOnceFunc },
    { 139,  "rt_sigreturn", ignoreWarnOnceFunc },
    { 140,  "setpriority" },
    { 141,  "getpriority" },
    { 142,  "reboot" },
    { 143,  "setregid" },
    { 144,  "setgid" },
    { 145,  "setreuid" },
    { 146,  "setuid", ignoreFunc },
    { 147,  "setresuid" },
    { 148,  "getresuid" },
    { 149,  "getresgid" },
    { 150,  "getresgid" },
    { 151,  "setfsuid" },
    { 152,  "setfsgid" },
    { 153,  "times", timesFunc<RiscvLinux64> },
    { 154,  "setpgid", setpgidFunc },
    { 155,  "getpgid" },
    { 156,  "getsid" },
    { 157,  "setsid" },
    { 158,  "getgroups" },
    { 159,  "setgroups" },
    { 160,  "uname", unameFunc64 },
    { 161,  "sethostname" },
    { 162,  "setdomainname" },
    { 163,  "getrlimit", getrlimitFunc<RiscvLinux64> },
    { 164,  "setrlimit", ignoreFunc },
    { 165,  "getrusage", getrusageFunc<RiscvLinux64> },
    { 166,  "umask", umaskFunc },
    { 167,  "prctl" },
    { 168,  "getcpu", getcpuFunc },
    { 169,  "gettimeofday", gettimeofdayFunc<RiscvLinux64> },
    { 170,  "settimeofday" },
    { 171,  "adjtimex" },
    { 172,  "getpid", getpidFunc },
    { 173,  "getppid", getppidFunc },
    { 174,  "getuid", getuidFunc },
    { 175,  "geteuid", geteuidFunc },
    { 176,  "getgid", getgidFunc },
    { 177,  "getegid", getegidFunc },
    { 178,  "gettid", gettidFunc },
    { 179,  "sysinfo", sysinfoFunc<RiscvLinux64> },
    { 180,  "mq_open" },
    { 181,  "mq_unlink" },
    { 182,  "mq_timedsend" },
    { 183,  "mq_timedrecieve" },
    { 184,  "mq_notify" },
    { 185,  "mq_getsetattr" },
    { 186,  "msgget" },
    { 187,  "msgctl" },
    { 188,  "msgrcv" },
    { 189,  "msgsnd" },
    { 190,  "semget" },
    { 191,  "semctl" },
    { 192,  "semtimedop" },
    { 193,  "semop" },
    { 194,  "shmget" },
    { 195,  "shmctl" },
    { 196,  "shmat" },
    { 197,  "shmdt" },
    { 198,  "socket", socketFunc<RiscvLinux64> },
    { 199,  "socketpair", socketpairFunc<RiscvLinux64> },
    { 200,  "bind", bindFunc },
    { 201,  "listen", listenFunc },
    { 202,  "accept", acceptFunc<RiscvLinux64> },
    { 203,  "connect", connectFunc },
    { 204,  "getsockname", getsocknameFunc },
    { 205,  "getpeername", getpeernameFunc },
    { 206,  "sendto", sendtoFunc<RiscvLinux64> },
    { 207,  "recvfrom", recvfromFunc<RiscvLinux64> },
    { 208,  "setsockopt", setsockoptFunc },
    { 209,  "getsockopt", getsockoptFunc },
    { 210,  "shutdown", shutdownFunc },
    { 211,  "sendmsg", sendmsgFunc },
    { 212,  "recvmsg", recvmsgFunc },
    { 213,  "readahead" },
    { 214,  "brk", brkFunc },
    { 215,  "munmap", munmapFunc<RiscvLinux64> },
    { 216,  "mremap", mremapFunc<RiscvLinux64> },
    { 217,  "add_key" },
    { 218,  "request_key" },
    { 219,  "keyctl" },
    { 220,  "clone", cloneBackwardsFunc<RiscvLinux64> },
    { 221,  "execve", execveFunc<RiscvLinux64> },
    { 222,  "mmap", mmapFunc<RiscvLinux64> },
    { 223,  "fadvise64" },
    { 224,  "swapon" },
    { 225,  "swapoff" },
    { 226,  "mprotect", ignoreFunc },
    { 227,  "msync", ignoreFunc },
    { 228,  "mlock", ignoreFunc },
    { 229,  "munlock", ignoreFunc },
    { 230,  "mlockall", ignoreFunc },
    { 231,  "munlockall", ignoreFunc },
    { 232,  "mincore", ignoreFunc },
    { 233,  "madvise", ignoreFunc },
    { 234,  "remap_file_pages" },
    { 235,  "mbind", ignoreFunc },
    { 236,  "get_mempolicy" },
    { 237,  "set_mempolicy" },
    { 238,  "migrate_pages" },
    { 239,  "move_pages" },
    { 240,  "tgsigqueueinfo" },
    { 241,  "perf_event_open" },
    { 242,  "accept4" },
    { 243,  "recvmmsg" },
    { 258,  "riscv_hwprobe", ignoreFunc },
    { 260,  "wait4", wait4Func<RiscvLinux64> },
    { 261,  "prlimit64", prlimitFunc<RiscvLinux64> },
    { 262,  "fanotify_init" },
    { 263,  "fanotify_mark" },
    { 264,  "name_to_handle_at" },
    { 265,  "open_by_handle_at" },
    { 266,  "clock_adjtime" },
    { 267,  "syncfs" },
    { 268,  "setns" },
    { 269,  "sendmmsg" },
    { 270,  "process_vm_ready" },
    { 271,  "process_vm_writev" },
    { 272,  "kcmp" },
    { 273,  "finit_module" },
    { 274,  "sched_setattr" },
    { 275,  "sched_getattr" },
    { 276,  "renameat2" },
    { 277,  "seccomp" },
    { 278,  "getrandom", getrandomFunc<RiscvLinux64> },
    { 279,  "memfd_create" },
    { 280,  "bpf" },
    { 281,  "execveat" },
    { 282,  "userfaultid" },
    { 283,  "membarrier" },
    { 284,  "mlock2" },
    { 285,  "copy_file_range" },
    { 286,  "preadv2" },
    { 287,  "pwritev2" },
    { 1024, "open", openFunc<RiscvLinux64> },
    { 1025, "link", linkFunc },
    { 1026, "unlink", unlinkFunc },
    { 1027, "mknod", mknodFunc },
    { 1028, "chmod", chmodFunc<RiscvLinux64> },
    { 1029, "chown", chownFunc },
    { 1030, "mkdir", mkdirFunc },
    { 1031, "rmdir", rmdirFunc },
    { 1032, "lchown" },
    { 1033, "access", accessFunc },
    { 1034, "rename", renameFunc },
    { 1035, "readlink", readlinkFunc<RiscvLinux64> },
    { 1036, "symlink", symlinkFunc },
    { 1037, "utimes", utimesFunc<RiscvLinux64> },
    { 1038, "stat", stat64Func<RiscvLinux64> },
    { 1039, "lstat", lstat64Func<RiscvLinux64> },
    { 1040, "pipe", pipeFunc },
    { 1041, "dup2", dup2Func },
    { 1042, "epoll_create" },
    { 1043, "inotifiy_init" },
    { 1044, "eventfd", eventfdFunc<RiscvLinux64> },
    { 1045, "signalfd" },
    { 1046, "sendfile" },
    { 1047, "ftruncate", ftruncate64Func },
    { 1048, "truncate", truncate64Func },
    { 1049, "stat", stat64Func<RiscvLinux64> },
    { 1050, "lstat", lstat64Func<RiscvLinux64> },
    { 1051, "fstat", fstat64Func<RiscvLinux64> },
    { 1052, "fcntl", fcntl64Func },
    { 1053, "fadvise64" },
    { 1054, "newfstatat", newfstatatFunc<RiscvLinux64> },
    { 1055, "fstatfs", fstatfsFunc<RiscvLinux64> },
    { 1056, "statfs", statfsFunc<RiscvLinux64> },
    { 1057, "lseek", lseekFunc },
    { 1058, "mmap", mmapFunc<RiscvLinux64> },
    { 1059, "alarm" },
    { 1060, "getpgrp", getpgrpFunc },
    { 1061, "pause" },
    { 1062, "time", timeFunc<RiscvLinux64> },
    { 1063, "utime" },
    { 1064, "creat" },
#if defined(SYS_getdents)
    { 1065, "getdents", getdentsFunc },
#else
    { 1065, "getdents" },
#endif
    { 1066, "futimesat" },
    { 1067, "select", selectFunc<RiscvLinux64> },
    { 1068, "poll", pollFunc<RiscvLinux64> },
    { 1069, "epoll_wait" },
    { 1070, "ustat" },
    { 1071, "vfork" },
    { 1072, "oldwait4" },
    { 1073, "recv" },
    { 1074, "send" },
    { 1075, "bdflush" },
    { 1076, "umount" },
    { 1077, "uselib" },
    { 1078, "sysctl" },
    { 1079, "fork" },
    { 2011, "getmainvars" }
};

SyscallDescTable<SEWorkload::SyscallABI32> EmuLinux::syscallDescs32 = {
    { 0,    "io_setup" },
    { 1,    "io_destroy" },
    { 2,    "io_submit" },
    { 3,    "io_cancel" },
    { 4,    "io_getevents" },
    { 5,    "setxattr" },
    { 6,    "lsetxattr" },
    { 7,    "fsetxattr" },
    { 8,    "getxattr" },
    { 9,    "lgetxattr" },
    { 10,   "fgetxattr" },
    { 11,   "listxattr" },
    { 12,   "llistxattr" },
    { 13,   "flistxattr" },
    { 14,   "removexattr" },
    { 15,   "lremovexattr" },
    { 16,   "fremovexattr" },
    { 17,   "getcwd", getcwdFunc },
    { 18,   "lookup_dcookie" },
    { 19,   "eventfd2" },
    { 20,   "epoll_create1" },
    { 21,   "epoll_ctl" },
    { 22,   "epoll_pwait" },
    { 23,   "dup", dupFunc },
    { 24,   "dup3" },
    { 25,   "fcntl", fcntlFunc },
    { 26,   "inotify_init1" },
    { 27,   "inotify_add_watch" },
    { 28,   "inotify_rm_watch" },
    { 29,   "ioctl", ioctlFunc<RiscvLinux32> },
    { 30,   "ioprio_get" },
    { 31,   "ioprio_set" },
    { 32,   "flock" },
    { 33,   "mknodat", mknodatFunc<RiscvLinux32> },
    { 34,   "mkdirat", mkdiratFunc<RiscvLinux32> },
    { 35,   "unlinkat", unlinkatFunc<RiscvLinux32> },
    { 36,   "symlinkat" },
    { 37,   "linkat" },
    { 38,   "renameat", renameatFunc<RiscvLinux32> },
    { 39,   "umount2" },
    { 40,   "mount" },
    { 41,   "pivot_root" },
    { 42,   "nfsservctl" },
    { 43,   "statfs", statfsFunc<RiscvLinux32> },
    { 44,   "fstatfs", fstatfsFunc<RiscvLinux32> },
    { 45,   "truncate", truncateFunc<RiscvLinux32> },
    { 46,   "ftruncate", ftruncateFunc<RiscvLinux32> },
    { 47,   "fallocate", fallocateFunc<RiscvLinux32> },
    { 48,   "faccessat", faccessatFunc<RiscvLinux32> },
    { 49,   "chdir", chdirFunc },
    { 50,   "fchdir" },
    { 51,   "chroot" },
    { 52,   "fchmod", fchmodFunc<RiscvLinux32> },
    { 53,   "fchmodat" },
    { 54,   "fchownat" },
    { 55,   "fchown", fchownFunc },
    { 56,   "openat", openatFunc<RiscvLinux32> },
    { 57,   "close", closeFunc },
    { 58,   "vhangup" },
    { 59,   "pipe2", pipe2Func },
    { 60,   "quotactl" },
#if defined(SYS_getdents64)
    { 61,   "getdents64", getdents64Func },
#else
    { 61,   "getdents64" },
#endif
    { 62,   "lseek", lseekFunc },
    { 63,   "read", readFunc<RiscvLinux32> },
    { 64,   "write", writeFunc<RiscvLinux32> },
    { 66,   "writev", writevFunc<RiscvLinux32> },
    { 67,   "pread64", pread64Func<RiscvLinux32> },
    { 68,   "pwrite64", pwrite64Func<RiscvLinux32> },
    { 69,   "preadv" },
    { 70,   "pwritev" },
    { 71,   "sendfile" },
    { 72,   "pselect6" },
    { 73,   "ppoll" },
    { 74,   "signalfd64" },
    { 75,   "vmsplice" },
    { 76,   "splice" },
    { 77,   "tee" },
    { 78,   "readlinkat", readlinkatFunc<RiscvLinux32> },
    { 79,   "fstatat" },
    { 80,   "fstat", fstatFunc<RiscvLinux32> },
    { 81,   "sync" },
    { 82,   "fsync" },
    { 83,   "fdatasync" },
    { 84,   "sync_file_range2" },
    { 85,   "timerfd_create" },
    { 86,   "timerfd_settime" },
    { 87,   "timerfd_gettime" },
    { 88,   "utimensat" },
    { 89,   "acct" },
    { 90,   "capget" },
    { 91,   "capset" },
    { 92,   "personality" },
    { 93,   "exit", exitFunc },
    { 94,   "exit_group", exitGroupFunc },
    { 95,   "waitid" },
    { 96,   "set_tid_address", setTidAddressFunc },
    { 97,   "unshare" },
    { 98,   "futex", futexFunc<RiscvLinux32> },
    { 99,   "set_robust_list", ignoreWarnOnceFunc },
    { 100,  "get_robust_list", ignoreWarnOnceFunc },
    { 101,  "nanosleep" },
    { 102,  "getitimer" },
    { 103,  "setitimer" },
    { 104,  "kexec_load" },
    { 105,  "init_module" },
    { 106,  "delete_module" },
    { 107,  "timer_create" },
    { 108,  "timer_gettime" },
    { 109,  "timer_getoverrun" },
    { 110,  "timer_settime" },
    { 111,  "timer_delete" },
    { 112,  "clock_settime" },
    { 113,  "clock_gettime", clock_gettimeFunc<RiscvLinux32> },
    { 114,  "clock_getres", clock_getresFunc<RiscvLinux32> },
    { 115,  "clock_nanosleep" },
    { 116,  "syslog" },
    { 117,  "ptrace" },
    { 118,  "sched_setparam" },
    { 119,  "sched_setscheduler" },
    { 120,  "sched_getscheduler" },
    { 121,  "sched_getparam" },
    { 122,  "sched_setaffinity" },
    { 123,  "sched_getaffinity", schedGetaffinityFunc<RiscvLinux32> },
    { 124,  "sched_yield", ignoreWarnOnceFunc },
    { 125,  "sched_get_priority_max" },
    { 126,  "sched_get_priority_min" },
    { 127,  "scheD_rr_get_interval" },
    { 128,  "restart_syscall" },
    { 129,  "kill" },
    { 130,  "tkill" },
    { 131,  "tgkill", tgkillFunc<RiscvLinux32> },
    { 132,  "sigaltstack" },
    { 133,  "rt_sigsuspend", ignoreWarnOnceFunc },
    { 134,  "rt_sigaction", ignoreWarnOnceFunc },
    { 135,  "rt_sigprocmask", ignoreWarnOnceFunc },
    { 136,  "rt_sigpending", ignoreWarnOnceFunc },
    { 137,  "rt_sigtimedwait", ignoreWarnOnceFunc },
    { 138,  "rt_sigqueueinfo", ignoreWarnOnceFunc },
    { 139,  "rt_sigreturn", ignoreWarnOnceFunc },
    { 140,  "setpriority" },
    { 141,  "getpriority" },
    { 142,  "reboot" },
    { 143,  "setregid" },
    { 144,  "setgid" },
    { 145,  "setreuid" },
    { 146,  "setuid", ignoreFunc },
    { 147,  "setresuid" },
    { 148,  "getresuid" },
    { 149,  "getresgid" },
    { 150,  "getresgid" },
    { 151,  "setfsuid" },
    { 152,  "setfsgid" },
    { 153,  "times", timesFunc<RiscvLinux32> },
    { 154,  "setpgid", setpgidFunc },
    { 155,  "getpgid" },
    { 156,  "getsid" },
    { 157,  "setsid" },
    { 158,  "getgroups" },
    { 159,  "setgroups" },
    { 160,  "uname", unameFunc32 },
    { 161,  "sethostname" },
    { 162,  "setdomainname" },
    { 163,  "getrlimit", getrlimitFunc<RiscvLinux32> },
    { 164,  "setrlimit", ignoreFunc },
    { 165,  "getrusage", getrusageFunc<RiscvLinux32> },
    { 166,  "umask", umaskFunc },
    { 167,  "prctl" },
    { 168,  "getcpu", getcpuFunc },
    { 169,  "gettimeofday", gettimeofdayFunc<RiscvLinux32> },
    { 170,  "settimeofday" },
    { 171,  "adjtimex" },
    { 172,  "getpid", getpidFunc },
    { 173,  "getppid", getppidFunc },
    { 174,  "getuid", getuidFunc },
    { 175,  "geteuid", geteuidFunc },
    { 176,  "getgid", getgidFunc },
    { 177,  "getegid", getegidFunc },
    { 178,  "gettid", gettidFunc },
    { 179,  "sysinfo", sysinfoFunc<RiscvLinux32> },
    { 180,  "mq_open" },
    { 181,  "mq_unlink" },
    { 182,  "mq_timedsend" },
    { 183,  "mq_timedrecieve" },
    { 184,  "mq_notify" },
    { 185,  "mq_getsetattr" },
    { 186,  "msgget" },
    { 187,  "msgctl" },
    { 188,  "msgrcv" },
    { 189,  "msgsnd" },
    { 190,  "semget" },
    { 191,  "semctl" },
    { 192,  "semtimedop" },
    { 193,  "semop" },
    { 194,  "shmget" },
    { 195,  "shmctl" },
    { 196,  "shmat" },
    { 197,  "shmdt" },
    { 198,  "socket", socketFunc<RiscvLinux32> },
    { 199,  "socketpair", socketpairFunc<RiscvLinux32> },
    { 200,  "bind", bindFunc },
    { 201,  "listen", listenFunc },
    { 202,  "accept", acceptFunc<RiscvLinux32> },
    { 203,  "connect", connectFunc },
    { 204,  "getsockname", getsocknameFunc },
    { 205,  "getpeername", getpeernameFunc },
    { 206,  "sendto", sendtoFunc<RiscvLinux32> },
    { 207,  "recvfrom", recvfromFunc<RiscvLinux32> },
    { 208,  "setsockopt", setsockoptFunc },
    { 209,  "getsockopt", getsockoptFunc },
    { 210,  "shutdown", shutdownFunc },
    { 211,  "sendmsg", sendmsgFunc },
    { 212,  "recvmsg", recvmsgFunc },
    { 213,  "readahead" },
    { 214,  "brk", brkFunc },
    { 215,  "munmap", munmapFunc<RiscvLinux32> },
    { 216,  "mremap", mremapFunc<RiscvLinux32> },
    { 217,  "add_key" },
    { 218,  "request_key" },
    { 219,  "keyctl" },
    { 220,  "clone", cloneBackwardsFunc<RiscvLinux32> },
    { 221,  "execve", execveFunc<RiscvLinux32> },
    { 222,  "mmap", mmapFunc<RiscvLinux32> },
    { 223,  "fadvise64" },
    { 224,  "swapon" },
    { 225,  "swapoff" },
    { 226,  "mprotect", ignoreFunc },
    { 227,  "msync", ignoreFunc },
    { 228,  "mlock", ignoreFunc },
    { 229,  "munlock", ignoreFunc },
    { 230,  "mlockall", ignoreFunc },
    { 231,  "munlockall", ignoreFunc },
    { 232,  "mincore", ignoreFunc },
    { 233,  "madvise", ignoreFunc },
    { 234,  "remap_file_pages" },
    { 235,  "mbind", ignoreFunc },
    { 236,  "get_mempolicy" },
    { 237,  "set_mempolicy" },
    { 238,  "migrate_pages" },
    { 239,  "move_pages" },
    { 240,  "tgsigqueueinfo" },
    { 241,  "perf_event_open" },
    { 242,  "accept4" },
    { 243,  "recvmmsg" },
    { 260,  "wait4", wait4Func<RiscvLinux32> },
    { 261,  "prlimit64", prlimitFunc<RiscvLinux32> },
    { 262,  "fanotify_init" },
    { 263,  "fanotify_mark" },
    { 264,  "name_to_handle_at" },
    { 265,  "open_by_handle_at" },
    { 266,  "clock_adjtime" },
    { 267,  "syncfs" },
    { 268,  "setns" },
    { 269,  "sendmmsg" },
    { 270,  "process_vm_ready" },
    { 271,  "process_vm_writev" },
    { 272,  "kcmp" },
    { 273,  "finit_module" },
    { 274,  "sched_setattr" },
    { 275,  "sched_getattr" },
    { 276,  "renameat2" },
    { 277,  "seccomp" },
    { 278,  "getrandom", getrandomFunc<RiscvLinux32> },
    { 279,  "memfd_create" },
    { 280,  "bpf" },
    { 281,  "execveat" },
    { 282,  "userfaultid" },
    { 283,  "membarrier" },
    { 284,  "mlock2" },
    { 285,  "copy_file_range" },
    { 286,  "preadv2" },
    { 287,  "pwritev2" },
    { 1024, "open", openFunc<RiscvLinux32> },
    { 1025, "link", linkFunc },
    { 1026, "unlink", unlinkFunc },
    { 1027, "mknod", mknodFunc },
    { 1028, "chmod", chmodFunc<RiscvLinux32> },
    { 1029, "chown", chownFunc },
    { 1030, "mkdir", mkdirFunc },
    { 1031, "rmdir", rmdirFunc },
    { 1032, "lchown" },
    { 1033, "access", accessFunc },
    { 1034, "rename", renameFunc },
    { 1035, "readlink", readlinkFunc<RiscvLinux32> },
    { 1036, "symlink", symlinkFunc },
    { 1037, "utimes", utimesFunc<RiscvLinux32> },
    { 1038, "stat", statFunc<RiscvLinux32> },
    { 1039, "lstat", lstatFunc<RiscvLinux32> },
    { 1040, "pipe", pipeFunc },
    { 1041, "dup2", dup2Func },
    { 1042, "epoll_create" },
    { 1043, "inotifiy_init" },
    { 1044, "eventfd", eventfdFunc<RiscvLinux32> },
    { 1045, "signalfd" },
    { 1046, "sendfile" },
    { 1047, "ftruncate", ftruncateFunc<RiscvLinux32> },
    { 1048, "truncate", truncateFunc<RiscvLinux32> },
    { 1049, "stat", statFunc<RiscvLinux32> },
    { 1050, "lstat", lstatFunc<RiscvLinux32> },
    { 1051, "fstat", fstatFunc<RiscvLinux32> },
    { 1052, "fcntl", fcntlFunc },
    { 1053, "fadvise64" },
    { 1054, "newfstatat", newfstatatFunc<RiscvLinux32> },
    { 1055, "fstatfs", fstatfsFunc<RiscvLinux32> },
    { 1056, "statfs", statfsFunc<RiscvLinux32> },
    { 1057, "lseek", lseekFunc },
    { 1058, "mmap", mmapFunc<RiscvLinux32> },
    { 1059, "alarm" },
    { 1060, "getpgrp", getpgrpFunc },
    { 1061, "pause" },
    { 1062, "time", timeFunc<RiscvLinux32> },
    { 1063, "utime" },
    { 1064, "creat" },
#if defined(SYS_getdents)
    { 1065, "getdents", getdentsFunc },
#else
    { 1065, "getdents" },
#endif
    { 1066, "futimesat" },
    { 1067, "select", selectFunc<RiscvLinux32> },
    { 1068, "poll", pollFunc<RiscvLinux32> },
    { 1069, "epoll_wait" },
    { 1070, "ustat" },
    { 1071, "vfork" },
    { 1072, "oldwait4" },
    { 1073, "recv" },
    { 1074, "send" },
    { 1075, "bdflush" },
    { 1076, "umount" },
    { 1077, "uselib" },
    { 1078, "sysctl" },
    { 1079, "fork" },
    { 2011, "getmainvars" }
};

} // namespace RiscvISA
} // namespace gem5
