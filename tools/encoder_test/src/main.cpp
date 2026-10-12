// Encoder bench test for the V2 pod.
//
// Counts both tracking-wheel encoders exactly as the pod does (PCNT, full
// quadrature, 1us glitch filter) and prints one line with live counts and the
// raw A/B pin levels, so a dead wire and a wrong pin can be told apart.
//
// Pins are copied from include/pod_config.hpp. Keep them in step.

#include <cstdio>

#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr int kVertA = 40, kVertB = 42;
constexpr int kHorizA = 39, kHorizB = 41;

constexpr int kCountsPerRev = 8192;   // AMT102-V at 2048 PPR, x4
constexpr int kHigh = 32000, kLow = -32000;

struct Enc {
    const char* name;
    int pinA, pinB;
    pcnt_unit_handle_t unit = nullptr;
    long total = 0;       // accumulated, unwrapped
    int last = 0;
    int transitionsA = 0, transitionsB = 0;
    int prevA = -1, prevB = -1;
};

Enc g_enc[2] = {{"VERT ", kVertA, kVertB}, {"HORIZ", kHorizA, kHorizB}};

bool begin(Enc& e) {
    pcnt_unit_config_t u = {};
    u.low_limit = kLow;
    u.high_limit = kHigh;
    u.flags.accum_count = 1;
    if (pcnt_new_unit(&u, &e.unit) != ESP_OK) return false;

    pcnt_glitch_filter_config_t f = {};
    f.max_glitch_ns = 1000;
    if (pcnt_unit_set_glitch_filter(e.unit, &f) != ESP_OK) return false;
    pcnt_unit_add_watch_point(e.unit, kHigh);
    pcnt_unit_add_watch_point(e.unit, kLow);

    pcnt_channel_handle_t a = nullptr, b = nullptr;
    pcnt_chan_config_t ca = {};
    ca.edge_gpio_num = e.pinA;
    ca.level_gpio_num = e.pinB;
    pcnt_chan_config_t cb = {};
    cb.edge_gpio_num = e.pinB;
    cb.level_gpio_num = e.pinA;
    if (pcnt_new_channel(e.unit, &ca, &a) != ESP_OK) return false;
    if (pcnt_new_channel(e.unit, &cb, &b) != ESP_OK) return false;

    pcnt_channel_set_edge_action(a, PCNT_CHANNEL_EDGE_ACTION_DECREASE,
                                 PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    pcnt_channel_set_level_action(a, PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                  PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    pcnt_channel_set_edge_action(b, PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                                 PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    pcnt_channel_set_level_action(b, PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                  PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

    pcnt_unit_enable(e.unit);
    pcnt_unit_clear_count(e.unit);
    pcnt_unit_start(e.unit);
    return true;
}

}  // namespace

extern "C" void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(500));
    printf("\n\nGaelforce encoder test\n");
    printf("VERT  A=GPIO%d B=GPIO%d   HORIZ A=GPIO%d B=GPIO%d\n", kVertA, kVertB, kHorizA,
           kHorizB);
    printf("Spin each wheel. Counts should change; one full turn = %d.\n", kCountsPerRev);
    printf("A/B show the raw pin level (0/1). 'edges' counts level changes seen.\n\n");

    for (auto& e : g_enc) {
        if (!begin(e)) printf("%s: PCNT setup FAILED\n", e.name);
    }

    int64_t lastPrint = 0;
    for (;;) {
        for (auto& e : g_enc) {
            if (!e.unit) continue;
            int raw = 0;
            pcnt_unit_get_count(e.unit, &raw);
            // accum_count: raw wraps at the limits, so unwrap on a jump
            int d = raw - e.last;
            if (d > kHigh) d -= (kHigh - kLow);
            if (d < kLow) d += (kHigh - kLow);
            e.total += d;
            e.last = raw;

            const int a = gpio_get_level(static_cast<gpio_num_t>(e.pinA));
            const int b = gpio_get_level(static_cast<gpio_num_t>(e.pinB));
            if (e.prevA >= 0 && a != e.prevA) ++e.transitionsA;
            if (e.prevB >= 0 && b != e.prevB) ++e.transitionsB;
            e.prevA = a;
            e.prevB = b;
        }

        if (esp_timer_get_time() - lastPrint > 250000) {
            lastPrint = esp_timer_get_time();
            for (int i = 0; i < 2; ++i) {
                const Enc& e = g_enc[i];
                printf("%s %9ld cnt (%+7.2f rev)  A=%d B=%d edges A%-5d B%-5d%s",
                       e.name, e.total, static_cast<double>(e.total) / kCountsPerRev,
                       e.prevA, e.prevB, e.transitionsA, e.transitionsB,
                       i == 0 ? "   |   " : "\n");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
