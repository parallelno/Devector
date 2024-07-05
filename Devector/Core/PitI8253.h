#include <iostream>
#include <cstdint>
#include <vector>
#include <functional>

namespace dev
{
    class Intel8253 {
    private:
        struct Counter {
            uint16_t count;
            uint16_t latch;
            uint8_t mode;
            bool bcd;
            bool waiting_for_count;
            bool latched_count;
            uint8_t rw_mode;
            bool output;
            bool gate;
            bool triggered;
            uint8_t phase;
            uint16_t read_latch;
            bool waiting_for_trigger;
            bool null_count;
        };

        std::vector<Counter> counters;
        std::function<void(int, bool)> output_callback;

    public:
        Intel8253(std::function<void(int, bool)> callback)
            : counters(3), output_callback(callback) {
            reset();
        }

        void reset() {
            for (int i = 0; i < 3; ++i) {
                counters[i] = {
                    0xFFFF, 0, 0, false, false, false, 0, true,
                    true, false, 0, 0, false, true
                };
            }
        }

        void write_control(uint8_t value) {
            uint8_t counter_select = (value >> 6) & 0x03;
            uint8_t rw_mode = (value >> 4) & 0x03;
            uint8_t mode = (value >> 1) & 0x07;
            bool bcd = value & 0x01;

            if (counter_select == 3) {
                // Read-back command
                for (int i = 0; i < 3; ++i) {
                    if (!(value & (1 << i))) {
                        if (!(value & 0x20)) {
                            counters[i].latched_count = true;
                            counters[i].read_latch = counters[i].count;
                        }
                        // Status latch not implemented in this simulation
                    }
                }
                return;
            }

            Counter& counter = counters[counter_select];
            counter.mode = (mode == 6 || mode == 7) ? mode - 4 : mode;
            counter.bcd = bcd;
            counter.rw_mode = rw_mode;
            counter.waiting_for_count = true;
            counter.null_count = true;

            if (counter.mode == 0) {
                set_output(counter_select, false);
            }
        }

        void write_counter(uint8_t counter_num, uint8_t value) {
            if (counter_num > 2) return;

            Counter& counter = counters[counter_num];

            if (counter.waiting_for_count) {
                if (counter.rw_mode == 1) {
                    counter.latch = value;
                    counter.waiting_for_count = false;
                    load_count(counter_num);
                }
                else if (counter.rw_mode == 2) {
                    counter.latch = (static_cast<uint16_t>(value) << 8);
                    counter.waiting_for_count = false;
                    load_count(counter_num);
                }
                else if (counter.rw_mode == 3) {
                    if (counter.waiting_for_count) {
                        counter.latch = value;
                        counter.waiting_for_count = false;
                    }
                    else {
                        counter.latch |= (static_cast<uint16_t>(value) << 8);
                        load_count(counter_num);
                    }
                }
            }
        }

        uint8_t read_counter(uint8_t counter_num) {
            if (counter_num > 2) return 0;

            Counter& counter = counters[counter_num];

            if (counter.latched_count) {
                uint8_t value = counter.read_latch & 0xFF;
                if (counter.rw_mode == 3) {
                    counter.read_latch >>= 8;
                    if (counter.read_latch == 0) {
                        counter.latched_count = false;
                    }
                }
                else {
                    counter.latched_count = false;
                }
                return value;
            }

            uint16_t value = counter.count;
            if (counter.rw_mode == 1) {
                return value & 0xFF;
            }
            else if (counter.rw_mode == 2) {
                return (value >> 8) & 0xFF;
            }
            else if (counter.rw_mode == 3) {
                if (counter.phase == 0) {
                    counter.phase = 1;
                    return value & 0xFF;
                }
                else {
                    counter.phase = 0;
                    return (value >> 8) & 0xFF;
                }
            }

            return 0;
        }

        void set_gate(uint8_t counter_num, bool state) {
            if (counter_num > 2) return;

            Counter& counter = counters[counter_num];
            bool old_state = counter.gate;
            counter.gate = state;

            if (counter.mode == 1 || counter.mode == 5) {
                if (!old_state && state) {
                    counter.triggered = true;
                }
            }
            else if (counter.mode == 2 || counter.mode == 3) {
                if (!state) {
                    set_output(counter_num, true);
                }
                else if (!old_state && state) {
                    load_count(counter_num);
                    set_output(counter_num, false);
                }
            }
        }

        void tick() {
            for (int i = 0; i < 3; ++i) {
                Counter& counter = counters[i];

                if (!counter.gate && (counter.mode != 0 && counter.mode != 4)) {
                    continue;
                }

                if (counter.waiting_for_trigger) {
                    if (counter.triggered) {
                        counter.waiting_for_trigger = false;
                        counter.triggered = false;
                        set_output(i, false);
                        if (counter.mode == 1) {
                            counter.count = counter.bcd ?
                                bcd_to_binary(counter.count) : counter.count;
                        }
                    }
                    else {
                        continue;
                    }
                }

                if (counter.count == 0 && counter.null_count) {
                    load_count(i);
                    counter.null_count = false;
                }

                if (counter.count > 0) {
                    counter.count--;
                }

                switch (counter.mode) {
                case 0:
                    if (counter.count == 0) {
                        set_output(i, true);
                    }
                    break;
                case 1:
                    if (counter.count == 0) {
                        set_output(i, true);
                        counter.waiting_for_trigger = true;
                    }
                    break;
                case 2:
                    if (counter.count == 1) {
                        set_output(i, false);
                    }
                    else if (counter.count == 0) {
                        set_output(i, true);
                        load_count(i);
                    }
                    break;
                case 3:
                    if (counter.count == 0) {
                        counter.output = !counter.output;
                        set_output(i, counter.output);
                        load_count(i);
                    }
                    else if (counter.count == counter.latch / 2) {
                        counter.output = !counter.output;
                        set_output(i, counter.output);
                    }
                    break;
                case 4:
                    if (counter.count == 0) {
                        set_output(i, true);
                        set_output(i, false);
                    }
                    break;
                case 5:
                    if (counter.count == 0) {
                        set_output(i, true);
                        set_output(i, false);
                        counter.waiting_for_trigger = true;
                    }
                    break;
                }
            }
        }

    private:
        void load_count(int counter_num) {
            Counter& counter = counters[counter_num];
            counter.count = counter.latch;
            if (counter.bcd) {
                counter.count = bcd_to_binary(counter.count);
            }
            if (counter.count == 0) {
                counter.count = 0x10000;
            }
        }

        void set_output(int counter_num, bool state) {
            counters[counter_num].output = state;
            output_callback(counter_num, state);
        }

        uint16_t bcd_to_binary(uint16_t bcd) {
            return (bcd & 0x0F) +
                ((bcd >> 4) & 0x0F) * 10 +
                ((bcd >> 8) & 0x0F) * 100 +
                ((bcd >> 12) & 0x0F) * 1000;
        }
    };
}