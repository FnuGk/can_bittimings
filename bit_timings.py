import math

MAX_NBT = 25;
MIN_NBT = 8;
PRESCALAR_MAX = 1<<5#(1 << 10);
PRESCALAR_MIN = (1 << 0);

PROP_SEG_MAX = 8

BAUDRATE_MAX = 1000 # 1000kbit/s or 1mbit/s

SYNC_SEG = 1 # always 1 TQ

MAX_BAUDRATE_VARIANT = 5 # Filter baudrate +/- 5kbit/s

def is_odd(x):
    return (x % 2 == 0) == False

def find_bit_timings(can_clk, bus_len, desired_baudrate=None):
    for prescaler in range(PRESCALAR_MIN, PRESCALAR_MAX+1):
        clk_err = can_clk % prescaler
        if clk_err == 0:
        #if clk_err < 0.5:
            for nbt in range(MIN_NBT, MAX_NBT+1):
                    baudrate = can_clk / prescaler / nbt / 1000 # in kbit/s

                    if desired_baudrate is not None:
                        # filter baudrate +/- 5kbit/s
                        if abs(baudrate - desired_baudrate) > MAX_BAUDRATE_VARIANT: continue

                    if baudrate <= BAUDRATE_MAX:
                        bit_time = (1.0/nbt)*1000 # 1TQ in nanoseconds

                        bus_propagation_delay = 5; #* 10e-9 to get in seconds, otherwise it is nano seconds
                        interface_prop_delay = 160; # in nano secods. the physical interface transmitter/receiver prop delay

                        bus_delay = bus_len * bus_propagation_delay;

                        t_prop_seg = 2 * (bus_delay + interface_prop_delay);
                        prop_seg = math.ceil(t_prop_seg / bit_time)

                        if prop_seg > PROP_SEG_MAX:
                            continue

                        phase_segments = nbt - prop_seg - SYNC_SEG
                        if phase_segments < 3:
                            continue

                        if phase_segments == 3:
                            phase_seg1 = 1
                            phase_seg2 = 2
                        else:
                            if is_odd(phase_segments):
                                phase_seg1 = math.floor(phase_segments/2)
                                phase_seg2 = phase_seg1 + 1
                            else:
                                phase_seg2 = phase_seg1 = phase_segments/2

                        sjw = min(4, phase_seg1)

                        osc_tol1 = sjw / (10 * nbt)
                        osc_tol2 = min(phase_seg1, phase_seg2) / (2 * (13 * nbt - phase_seg2))
                        osc_tol = min(osc_tol1, osc_tol2) * 100

                        sample_point = (nbt - phase_seg2 ) / nbt * 100;

                        print("---------------------------------------")
                        print("Prescler: {}".format(prescaler))
                        print("NBT: {}".format(nbt))
                        print("PROP_SEG: {}".format(prop_seg))
                        print("phase_seg1: {}".format(phase_seg1))
                        print("phase_seg2: {}".format(phase_seg2))
                        print("sjw: {}".format(sjw))
                        print("Oscilator tolrence: {}%".format(osc_tol))
                        print("sample point: {}%".format(sample_point))
                        print("baudrate: {} (kbit/s)".format(baudrate))
                        print("---------------------------------------")

def main():
    can_clk = 48.0#11.059200#48.0
    bus_len = 5
    find_bit_timings(can_clk * (10**6), bus_len, 500)

if __name__ == '__main__':
    main()
