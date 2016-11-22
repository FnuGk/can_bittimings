#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>


#define IS_ODD(x)   ((x & 1) == 1)

#define MAX13050_INTERFACE_DELAY_NS   160
#define BUS_PROP_DELAY_NS             5
#define INTERFACE_PROP_DELAY_NS       MAX13050_INTERFACE_DELAY_NS

#define MAX_NBT 25
#define MIN_NBT 8
#define PRESCALER_MAX   (1 << 5)
#define PRESCALER_MIN   (1 << 0)

#define PROP_SEG_MAX    8

#define BAUDRATE_MAX        (1000 * 1000) // 1000kbit/s or 1mbit/s
#define BAUDRATE_VARIANCE   (5 * 1000) // 5kbit/s

#define SYNC_SEG    1 // always 1 TQ
#define SJW_MAX     4

struct bit_timings {
    int prescaler;
    int nbt; // Nominal Bit Time (Total number of TQ)
    int prop_seg;
    int phase_seg1;
    int phase_seg2;
    int sjw;
    float osc_tol;
    float sample_point;
    int32_t baudrate;
};

static void print_bt(struct bit_timings *bt) {
    printf("---------------------------------------\n");
    printf("prescaler: %d\n", bt->prescaler);
    printf("NBT: %d\n", bt->nbt);
    printf("prop_seg: %d\n", bt->prop_seg);
    printf("phase_seg1: %d\n", bt->phase_seg1);
    printf("phase_seg2: %d\n", bt->phase_seg2);
    printf("sjw: %d\n", bt->sjw);
    printf("Oscilator tolrence: %f%%\n", bt->osc_tol);
    printf("sample point: %f%%\n", bt->sample_point);
    printf("baudrate: %f (kbit/s)\n", bt->baudrate/1000.0);
    printf("---------------------------------------\n");
}

int calc_bit_timings(struct bit_timings *bt, int can_clk, int bus_len, int32_t desired_baudrate) {
    bt->osc_tol = 0;
    int n_found = 0;
    for (int prescaler = PRESCALER_MIN; prescaler <= PRESCALER_MAX; prescaler++) {
        if (can_clk % prescaler != 0) continue; // must be interger devisible

        for (int nbt = MIN_NBT; nbt <= MAX_NBT; nbt++) {
            const int32_t baudrate = can_clk / prescaler / nbt ;/// 1000.0; // in kbit/s
            if (baudrate > BAUDRATE_MAX) continue;

            if (abs(baudrate - desired_baudrate) > BAUDRATE_VARIANCE) continue;
            // if (baudrate != desired_baudrate) continue;

            const int bit_time =  (1.0/nbt)*1000; // 1TQ in nanoseconds

            const int bus_delay  = bus_len * BUS_PROP_DELAY_NS;
            const int t_prop_seg = 2 * (bus_delay + INTERFACE_PROP_DELAY_NS);
            const int prop_seg   = ceil(t_prop_seg / (float)bit_time);
            if (prop_seg > PROP_SEG_MAX) continue;

            const int phase_segments = nbt - prop_seg - SYNC_SEG;
            int phase_seg1;
            int phase_seg2;
            if (phase_segments < 3) {
                continue;
            } else if (phase_segments == 3) {
                phase_seg1 = 1;
                phase_seg2 = 2;
            } else {
                if (IS_ODD(phase_segments)) {
                    phase_seg1 = phase_segments/2; // integer division floors the result
                    phase_seg2 = phase_seg1 + 1;
                } else {
                    phase_seg2 = phase_seg1 = phase_segments/2;
                }
            }

            const int sjw = fmin(SJW_MAX, phase_seg1);

            const float osc_tol1 = sjw / (float)(10 * nbt);
            const float osc_tol2 = fmin(phase_seg1, phase_seg2) / (float)(2 * (13 * nbt - phase_seg2));
            const float osc_tol  = fmin(osc_tol1, osc_tol2) * 100;

            const float sample_point = (nbt - phase_seg2 ) / (float)nbt * 100;

            if (osc_tol > bt->osc_tol) {
                bt->prescaler    = prescaler;
                bt->nbt          = nbt;
                bt->prop_seg     = prop_seg;
                bt->phase_seg1   = phase_seg1;
                bt->phase_seg2   = phase_seg2;
                bt->sjw          = sjw;
                bt->osc_tol      = osc_tol;
                bt->sample_point = sample_point;
                bt->baudrate     = baudrate;
            }
            n_found++;
        }
    }

    return n_found;
}

int main(void) {
    const int bus_len = 5; // in meters
    const int can_clk = 48e6;//11059200;//48e6;//11059200;//48e6; // in hz

    struct bit_timings bt;
    const int n = calc_bit_timings(&bt, can_clk, bus_len, BAUDRATE_MAX);
    print_bt(&bt);
    printf("Found %d possible bit timings\n", n);

	return 0;
}
