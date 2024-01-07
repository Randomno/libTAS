/*
    Copyright 2015-2023 Clément Gallet <clement.gallet@ens-lyon.org>

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

#include "randomwrappers.h"

#include "logging.h"
#include "hook.h"

namespace libtas {

DEFINE_ORIG_POINTER(random)
DEFINE_ORIG_POINTER(rand)

/* Override */ long int random (void) __THROW
{
    static int count = 0;
    LINK_NAMESPACE_GLOBAL(random);
    long int ret = orig::random();
    debuglogstdio(LCF_RANDOM, "%s call %d, returning %ld", __func__, count++, ret);
    return ret;
}

/* Override */ void srandom (unsigned int seed) __THROW
{
    debuglogstdio(LCF_RANDOM, "%s call with seed %u", __func__, seed);
    RETURN_NATIVE(srandom, (seed), nullptr);
}

/* Override */ char *initstate (unsigned int seed, char *statebuf,
            size_t statelen) __THROW
{
    debuglogstdio(LCF_RANDOM, "%s call with seed %u", __func__, seed);
    RETURN_NATIVE(initstate, (seed, statebuf, statelen), nullptr);
}

#ifdef __unix__
/* Override */ char *setstate (char *statebuf) __THROW
#elif defined(__APPLE__) && defined(__MACH__)
/* Override */ char *setstate (const char *statebuf)
#endif
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(setstate, (statebuf), nullptr);
}

/* Override */ int random_r (struct random_data *buf,
             int32_t *result) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(random_r, (buf, result), nullptr);
}

/* Override */ int srandom_r (unsigned int seed, struct random_data *buf) __THROW
{
    debuglogstdio(LCF_RANDOM, "%s call with seed %u", __func__, seed);
    RETURN_NATIVE(srandom_r, (seed, buf), nullptr);
}

/* Override */ int initstate_r (unsigned int seed, char *statebuf, size_t statelen,
            struct random_data *buf) __THROW
{
    debuglogstdio(LCF_RANDOM, "%s call with seed %u", __func__, seed);
    RETURN_NATIVE(initstate_r, (seed, statebuf, statelen, buf), nullptr);
}

/* Override */ int setstate_r (char *statebuf, struct random_data *buf) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(setstate_r, (statebuf, buf), nullptr);
}

/* Override */ int rand (void) __THROW
{
    static int count = 0;
    LINK_NAMESPACE_GLOBAL(rand);
    int ret = orig::rand();
    debuglogstdio(LCF_RANDOM, "%s call %d, returning %ld", __func__, count++, ret);
    return ret;
}

/* Override */ void srand (unsigned int seed) __THROW
{
    debuglogstdio(LCF_RANDOM, "%s call with seed %u", __func__, seed);
    RETURN_NATIVE(srand, (seed), nullptr);
}

/* Override */ int rand_r (unsigned int *seed) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(rand_r, (seed), nullptr);
}

/* Override */ double drand48 (void) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(drand48, (), nullptr);
}

/* Override */ double erand48 (unsigned short int xsubi[3]) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(erand48, (xsubi), nullptr);
}

/* Override */ long int lrand48 (void) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(lrand48, (), nullptr);
}

/* Override */ long int nrand48 (unsigned short int xsubi[3]) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(nrand48, (xsubi), nullptr);
}

/* Override */ long int mrand48 (void) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(mrand48, (), nullptr);
}

/* Override */ long int jrand48 (unsigned short int xsubi[3]) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(jrand48, (xsubi), nullptr);
}

/* Override */ void srand48 (long int seedval) __THROW
{
    debuglogstdio(LCF_RANDOM, "%s call with seed %ld", __func__, seedval);
    RETURN_NATIVE(srand48, (seedval), nullptr);
}

/* Override */ unsigned short int *seed48 (unsigned short int seed16v[3]) __THROW
{
    debuglogstdio(LCF_RANDOM, "%s call with seed %d %d %d", __func__, seed16v[0], seed16v[1], seed16v[2]);
    RETURN_NATIVE(seed48, (seed16v), nullptr);
}

/* Override */ void lcong48 (unsigned short int param[7]) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(lcong48, (param), nullptr);
}

/* Override */ int drand48_r (struct drand48_data *buffer, double *result) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(drand48_r, (buffer, result), nullptr);
}

/* Override */ int erand48_r (unsigned short int xsubi[3],
              struct drand48_data *buffer, double *result) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(erand48_r, (xsubi, buffer, result), nullptr);
}

/* Override */ int lrand48_r (struct drand48_data *buffer, long int *result) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(lrand48_r, (buffer, result), nullptr);
}

/* Override */ int nrand48_r (unsigned short int xsubi[3],
              struct drand48_data *buffer, long int *result) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(nrand48_r, (xsubi, buffer, result), nullptr);
}

/* Override */ int mrand48_r (struct drand48_data *buffer, long int *result) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(mrand48_r, (buffer, result), nullptr);
}

/* Override */ int jrand48_r (unsigned short int xsubi[3],
              struct drand48_data *buffer, long int *result) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(jrand48_r, (xsubi, buffer, result), nullptr);
}

/* Override */ int srand48_r (long int seedval, struct drand48_data *buffer) __THROW
{
    debuglogstdio(LCF_RANDOM, "%s call with seed %ld", __func__, seedval);
    RETURN_NATIVE(srand48_r, (seedval, buffer), nullptr);
}

/* Override */ int seed48_r (unsigned short int seed16v[3],
             struct drand48_data *buffer) __THROW
{
    debuglogstdio(LCF_RANDOM, "%s call with seed %d %d %d", __func__, seed16v[0], seed16v[1], seed16v[2]);
    RETURN_NATIVE(seed48_r, (seed16v, buffer), nullptr);
}

/* Override */ int lcong48_r (unsigned short int param[7],
              struct drand48_data *buffer) __THROW
{
    DEBUGLOGCALL(LCF_RANDOM);
    RETURN_NATIVE(lcong48_r, (param, buffer), nullptr);
}

}