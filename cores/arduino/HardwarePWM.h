/**
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _HARDWARE_PWM_H_
#define _HARDWARE_PWM_H_

#include <pinmapping.h>

// 32bit bitset used to track whether a pin is activly using hardware pwm
extern uint32_t active_pwm_pins;

void pwm_hardware_init(const uint32_t prescale, const uint32_t period);

// return the bits to attach the PWM hardware depending on port using a lookup table
[[nodiscard]] constexpr int8_t pin_feature_pwm(const pin_t pin) noexcept {
  constexpr std::array<int8_t, 5> lookup {-1, 2, 1, 3, -1};
  return lookup[LPC1768_PIN_PORT(pin)];
}

// return a reference to a PWM timer register using a lookup table as they are not contiguous
[[nodiscard]] constexpr uint32_t pwm_match_lookup(const pin_t pin) noexcept {
  constexpr uint32_t MR0_OFFSET = 24, MR4_OFFSET = 64;
  return LPC_PWM1_BASE + (LPC1768_PIN_PWM(pin) > 3 ? MR4_OFFSET : MR0_OFFSET ) + (sizeof(uint32_t) * LPC1768_PIN_PWM(pin)) ;
}

// return a reference to a PWM timer register using a lookup table as they are not contiguous
[[nodiscard]] constexpr auto& pin_pwm_match(const pin_t pin) noexcept {
   return util::memory_ref<uint32_t>(pwm_match_lookup(pin));
}

// generate a unique bit for each hardware PWM capable pin
[[nodiscard]] constexpr uint8_t pwm_pin_id(const pin_t pin) noexcept {
  return (LPC1768_PIN_PORT(pin) * 6) + (LPC1768_PIN_PWM(pin) - 1);
}

// return true if a pwm channel is already attached to a pin
[[nodiscard]] constexpr bool pwm_channel_active(const pin_t pin) noexcept {
  const uint32_t channel = LPC1768_PIN_PWM(pin) - 1;
  return LPC1768_PIN_PWM(pin) && util::bitset_mask(active_pwm_pins, util::bitset_value(6 + channel, 2 * 6 + channel, 3 * 6 + channel));
}

// return true if a pin is already attached to PWM hardware
[[nodiscard]] constexpr bool pwm_pin_active(const pin_t pin) noexcept {
  return LPC1768_PIN_PWM(pin) && util::bit_test(active_pwm_pins, pwm_pin_id(pin));
}

[[gnu::always_inline]] inline void pwm_set_period(const uint32_t period) {
  LPC_PWM1->MR0 = period - 1;               // TC resets every period cycles
  util::bit_set(LPC_PWM1->LER, 0);
}

// update the bitset an activate hardware pwm channel for output
[[gnu::always_inline]] inline void pwm_activate_channel(const pin_t pin) {
  util::bit_set(active_pwm_pins, pwm_pin_id(pin));         // mark the pin as active
  util::bit_set(LPC_PWM1->PCR, 8 + LPC1768_PIN_PWM(pin));  // turn on the pins PWM output (8 offset + PWM channel)
}

// update the bitset and deactivate the hardware pwm channel
[[gnu::always_inline]] inline void pwm_deactivate_channel(const pin_t pin) {
  util::bit_clear(active_pwm_pins, pwm_pin_id(pin));      // mark pin as inactive
  if(!pwm_channel_active(pin)) util::bit_clear(LPC_PWM1->PCR, 8 + LPC1768_PIN_PWM(pin)); // turn off the PWM output
}

// update the match register for a channel and set the latch to update on next period
[[gnu::always_inline]] inline void pwm_set_match(const pin_t pin, const uint32_t value) {
  pin_pwm_match(pin) = value;
  util::bit_set(LPC_PWM1->LER, LPC1768_PIN_PWM(pin));
}

[[gnu::always_inline]] inline void pwm_hardware_attach(pin_t pin, uint32_t value) {
  pwm_set_match(pin, value);
  pwm_activate_channel(pin);
  pin_enable_feature(pin, pin_feature_pwm(pin));
}

#endif // _HARDWARE_PWM_H_
