#pragma once
#include <cstdint>
struct pwm_config {};
inline unsigned hostPwmLevel = 0;
constexpr unsigned GPIO_FUNC_PWM = 2;
constexpr unsigned PWM_IRQ_WRAP = 0;
inline unsigned pwm_gpio_to_slice_num(unsigned pin) { return pin / 2; }
inline unsigned pwm_gpio_to_channel(unsigned pin) { return pin & 1; }
inline pwm_config pwm_get_default_config() { return {}; }
inline void pwm_config_set_wrap(pwm_config *, unsigned) {}
inline void pwm_init(unsigned, pwm_config *, bool) {}
inline void pwm_set_chan_level(unsigned, unsigned, unsigned level) { hostPwmLevel = level; }
inline void gpio_set_function(unsigned, unsigned) {}
inline void irq_set_exclusive_handler(unsigned, void (*)()) {}
inline void pwm_clear_irq(unsigned) {}
inline void pwm_set_irq_enabled(unsigned, bool) {}
inline void irq_set_enabled(unsigned, bool) {}
inline void pwm_set_enabled(unsigned, bool) {}
