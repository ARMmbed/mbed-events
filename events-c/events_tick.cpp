#include "events_tick.h"
#include <stdbool.h>
#include "mbed.h"


static class GlobalTicker {
public:
    GlobalTicker() {
        _tick = 0;
        _timer.start();
        _ticker.attach_us(this, &GlobalTicker::step, (1 << 16) * 1000);
    };

    void step() {
        _timer.reset();
        _tick += 1 << 16;
    }

    unsigned tick() {
        return _tick + (unsigned)_timer.read_ms();
    }

private:
    unsigned _tick;
    Timer _timer;
    Ticker _ticker;
} gticker;


unsigned events_tick() {
    return gticker.tick();
}
