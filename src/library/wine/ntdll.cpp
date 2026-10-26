/*
    Copyright 2015-2026 Clément Gallet <clement.gallet@ens-lyon.org>

    This file is part of libTAS.

    libTAS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libTAS.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Hooks of the time syscalls of wine unix `ntdll.so`. We only replace the
 * entries of the syscall table, so that wine itself keeps using the real
 * time, which it needs to compute timeouts. */

#include "ntdll.h"
#include "winehook.h"

#include "logging.h"
#include "DeterministicTimer.h"
#include "general/sleepwrappers.h"
#include "../shared/SharedConfig.h"

#include <cstdint>
#include <cinttypes>
#include <cstring>
#include <ctime>
#include <sys/mman.h>
#include <unistd.h>

namespace libtas {

/* NTSTATUS values */
static const uint32_t STATUS_SUCCESS = 0;
static const uint32_t STATUS_ACCESS_VIOLATION = 0xC0000005;

/* Windows time unit is 100ns */
static const int64_t TICKSPERSEC = 10000000;

/* Seconds between 1601-01-01 and 1970-01-01 */
static const int64_t SECS_1601_TO_1970 = 11644473600LL;

/* Fixed address of the KUSER_SHARED_DATA structure */
static unsigned char* const KUSD_ADDRESS = reinterpret_cast<unsigned char*>(0x7ffe0000);

/* Offsets of time fields in KUSER_SHARED_DATA */
static const int KUSD_TICKCOUNTLOW = 0x000;
static const int KUSD_TICKCOUNTMULTIPLIER = 0x004;
static const int KUSD_INTERRUPTTIME = 0x008;
static const int KUSD_SYSTEMTIME = 0x014;
static const int KUSD_TICKCOUNT = 0x320;

static bool kusd_taken = false;

namespace orig {

static uint32_t (*NtQueryPerformanceCounter)(int64_t *counter, int64_t *frequency);
static uint32_t (*NtQuerySystemTime)(int64_t *time);
static uint32_t (*NtDelayExecution)(unsigned char alertable, const int64_t *timeout);

}

static int64_t toWindowsTicks(const struct timespec& ts)
{
    return ts.tv_sec * TICKSPERSEC + ts.tv_nsec / 100;
}

/* Write a KSYSTEM_TIME, in the order expected by readers: High2Time,
 * LowPart then High1Time */
static void writeSystemTime(int offset, int64_t value)
{
    volatile int32_t* time = reinterpret_cast<volatile int32_t*>(KUSD_ADDRESS + offset);
    time[2] = static_cast<int32_t>(value >> 32);
    time[0] = static_cast<int32_t>(value);
    time[1] = static_cast<int32_t>(value >> 32);
}

void takeover_wine_shared_data()
{
    if (kusd_taken)
        return;

    size_t page_size = sysconf(_SC_PAGESIZE);

    /* Check that wine mapped the page */
    unsigned char vec;
    if (mincore(KUSD_ADDRESS, page_size, &vec) != 0)
        return;

    /* The page is shared with wineserver, which keeps updating the time
     * values. Replace it with a private copy that we update ourselves. */
    unsigned char* copy = new unsigned char[page_size];
    memcpy(copy, KUSD_ADDRESS, page_size);

    void* addr = mmap(KUSD_ADDRESS, page_size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (addr == MAP_FAILED) {
        LOG(LL_ERROR, LCF_WINE, "Could not replace the shared user data page");
        delete[] copy;
        return;
    }

    memcpy(KUSD_ADDRESS, copy, page_size);
    delete[] copy;

    LOG(LL_DEBUG, LCF_WINE, "Replaced the shared user data page");
    kusd_taken = true;
    update_wine_shared_data();
}

void update_wine_shared_data()
{
    if (!kusd_taken)
        return;

    DeterministicTimer& detTimer = DeterministicTimer::get();
    struct timespec mono = detTimer.getTicks(SharedConfig::TIMETYPE_UNTRACKED_MONOTONIC);
    struct timespec real = detTimer.getTicks(SharedConfig::TIMETYPE_UNTRACKED_REALTIME);

    int64_t msec = mono.tv_sec * 1000 + mono.tv_nsec / 1000000;
    uint32_t multiplier = *reinterpret_cast<uint32_t*>(KUSD_ADDRESS + KUSD_TICKCOUNTMULTIPLIER);
    if (multiplier == 0)
        multiplier = 1 << 24;

    *reinterpret_cast<volatile uint32_t*>(KUSD_ADDRESS + KUSD_TICKCOUNTLOW) = static_cast<uint32_t>(msec);
    writeSystemTime(KUSD_TICKCOUNT, (msec << 24) / multiplier);
    writeSystemTime(KUSD_INTERRUPTTIME, toWindowsTicks(mono));
    writeSystemTime(KUSD_SYSTEMTIME, toWindowsTicks(real) + SECS_1601_TO_1970 * TICKSPERSEC);
}

static uint32_t NtQueryPerformanceCounter(int64_t *counter, int64_t *frequency)
{
    LOGTRACE_SIMPLE(LCF_WINE | LCF_TIMEGET);
    takeover_wine_shared_data();
    if (!counter)
        return STATUS_ACCESS_VIOLATION;

    struct timespec ts = DeterministicTimer::get().getTicks(SharedConfig::TIMETYPE_QUERYPERFORMANCECOUNTER);
    *counter = toWindowsTicks(ts);
    if (frequency)
        *frequency = TICKSPERSEC;
    LOG(LL_DEBUG, LCF_WINE | LCF_TIMEGET, "  returning %" PRId64, *counter);
    return STATUS_SUCCESS;
}

static uint32_t NtQuerySystemTime(int64_t *time)
{
    LOGTRACE_SIMPLE(LCF_WINE | LCF_TIMEGET);
    takeover_wine_shared_data();
    if (!time)
        return STATUS_ACCESS_VIOLATION;

    struct timespec ts = DeterministicTimer::get().getTicks(SharedConfig::TIMETYPE_CLOCKGETTIME_REALTIME);
    *time = toWindowsTicks(ts) + SECS_1601_TO_1970 * TICKSPERSEC;
    LOG(LL_DEBUG, LCF_WINE | LCF_TIMEGET, "  returning %" PRId64, *time);
    return STATUS_SUCCESS;
}

static uint32_t NtDelayExecution(unsigned char alertable, const int64_t *timeout)
{
    takeover_wine_shared_data();

    /* GetTickCount() reads the shared user data page without any call that
     * we can count for time-tracking. Busy loops on it usually yield with
     * Sleep(0), so count this as a GetTickCount() call instead. */
    if (timeout && (*timeout == 0))
        DeterministicTimer::get().getTicks(SharedConfig::TIMETYPE_GETTICKCOUNT);

    /* Infinite sleep, yield, and alertable sleeps which can be interrupted
     * by APCs are left to wine */
    if (!timeout || (*timeout == 0) || alertable) {
        LOGTRACE(LCF_WINE | LCF_SLEEP, "%s call, left to wine", __func__);
        return orig::NtDelayExecution(alertable, timeout);
    }

    /* Negative timeouts are relative, positive ones are absolute system time */
    int64_t ticks = -*timeout;
    if (*timeout > 0) {
        int64_t now;
        NtQuerySystemTime(&now);
        ticks = *timeout - now;
    }
    if (ticks < 0)
        ticks = 0;

    struct timespec ts;
    ts.tv_sec = ticks / TICKSPERSEC;
    ts.tv_nsec = (ticks % TICKSPERSEC) * 100;

    LOGTRACE(LCF_WINE | LCF_SLEEP, "%s call - sleep for %d.%09d sec", __func__, ts.tv_sec, ts.tv_nsec);
    transfer_sleep(ts, nullptr);
    return STATUS_SUCCESS;
}

#define HOOK_WINE_SYSCALL(FUNC) hook_wine_syscall(path, #FUNC, reinterpret_cast<void**>(&orig::FUNC), reinterpret_cast<void*>(FUNC))

void hook_ntdll(const char* path)
{
    static bool hooked = false;
    if (hooked)
        return;
    hooked = true;

    register_wine_ntdll(path);

    HOOK_WINE_SYSCALL(NtQueryPerformanceCounter);
    HOOK_WINE_SYSCALL(NtQuerySystemTime);
    HOOK_WINE_SYSCALL(NtDelayExecution);
}

}
