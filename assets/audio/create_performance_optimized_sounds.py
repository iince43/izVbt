#!/usr/bin/env python3
"""
PERFORMANCE-OPTIMIZED VBT Audio System
PRIORITY: SPEED + VOLUME for instant feedback
FOCUS: Hedef altı = LOUDEST, FASTEST warning for more effort!
"""

import struct
import math

def create_wav_file(filename, samples, sample_rate=44100):
    """Create a WAV file from samples"""
    with open(filename, 'wb') as f:
        # WAV header
        f.write(b'RIFF')
        f.write(struct.pack('<I', 36 + len(samples) * 2))
        f.write(b'WAVE')
        f.write(b'fmt ')
        f.write(struct.pack('<I', 16))  # PCM format
        f.write(struct.pack('<H', 1))   # PCM
        f.write(struct.pack('<H', 1))   # Mono
        f.write(struct.pack('<I', sample_rate))
        f.write(struct.pack('<I', sample_rate * 2))
        f.write(struct.pack('<H', 2))
        f.write(struct.pack('<H', 16))
        f.write(b'data')
        f.write(struct.pack('<I', len(samples) * 2))
        
        # Audio data
        for sample in samples:
            f.write(struct.pack('<h', int(sample * 32767)))

def generate_ultra_fast_beep(frequency, duration, amplitude=0.7, sample_rate=44100):
    """Generate ULTRA FAST beep with minimal fade for instant response"""
    samples = []
    for i in range(int(sample_rate * duration)):
        t = i / sample_rate
        
        # MINIMAL fade (5ms) for maximum speed
        fade_samples = int(sample_rate * 0.005)  # 5ms only!
        fade_factor = 1.0
        
        if i < fade_samples:
            fade_factor = i / fade_samples
        elif i > len(range(int(sample_rate * duration))) - fade_samples:
            fade_factor = (len(range(int(sample_rate * duration))) - i) / fade_samples
            
        sample = amplitude * fade_factor * math.sin(2 * math.pi * frequency * t)
        samples.append(sample)
    return samples

def generate_urgent_double_beep(frequency, duration, pause, amplitude=0.8):
    """Generate URGENT double beep - FASTEST possible for critical warnings"""
    beep = generate_ultra_fast_beep(frequency, duration, amplitude)
    silence = [0.0] * int(44100 * pause)  # Minimal pause
    return beep + silence + beep

def main():
    print("⚡ Creating PERFORMANCE-OPTIMIZED VBT Audio System...")
    print("🎯 PRIORITY: SPEED + VOLUME + INSTANT FEEDBACK")
    print("🔥 FOCUS: Hedef altı = LOUDEST + FASTEST warning!")
    
    # 🟢 TARGET HIT - Quick success beep (800Hz, medium volume, fast)
    target_hit = generate_ultra_fast_beep(800, 0.08, 0.5)  # 80ms - FAST
    create_wav_file('vbt_target_hit.wav', target_hit)
    print("🟢 Created: vbt_target_hit.wav (800Hz, 80ms - QUICK SUCCESS)")
    
    # 💎 ABOVE TARGET - Pleasant double beep (1000Hz, good volume)
    above_target = generate_urgent_double_beep(1000, 0.06, 0.04, 0.6)  # 60ms + 40ms pause + 60ms
    create_wav_file('vbt_above_target.wav', above_target)
    print("💎 Created: vbt_above_target.wav (1000Hz double - ACHIEVEMENT)")
    
    # 🔴 BELOW TARGET - LOUDEST, MOST URGENT WARNING! (400Hz, MAX VOLUME)
    # This is THE MOST IMPORTANT sound - needs maximum impact!
    below_target = generate_urgent_double_beep(400, 0.12, 0.03, 1.0)  # MAX AMPLITUDE 1.0!
    create_wav_file('vbt_below_target.wav', below_target)
    print("🔴 Created: vbt_below_target.wav (400Hz, MAX VOLUME - URGENT EFFORT NEEDED!)")
    
    # 🔥 EXCEPTIONAL - Quick celebration (1200Hz, happy but not too long)
    exceptional = generate_ultra_fast_beep(1200, 0.15, 0.6)  # Longer but still fast
    create_wav_file('vbt_exceptional.wav', exceptional)
    print("🔥 Created: vbt_exceptional.wav (1200Hz celebration - EXCEPTIONAL!)")
    
    # 🏁 SET COMPLETE - Clear finish signal (600Hz, moderate length)
    set_complete = generate_ultra_fast_beep(600, 0.2, 0.5)  # Clear but not too long
    create_wav_file('vbt_set_complete.wav', set_complete)
    print("🏁 Created: vbt_set_complete.wav (600Hz finish signal)")
    
    # ⚠️ FATIGUE WARNING - Pulsing urgent beep (350Hz, high volume)
    fatigue_beeps = []
    for i in range(2):  # Only 2 pulses - faster than 3
        fatigue_beeps.extend(generate_ultra_fast_beep(350, 0.1, 0.7))
        if i < 1:  # Only one pause
            fatigue_beeps.extend([0.0] * int(44100 * 0.08))  # Short pause
    create_wav_file('vbt_fatigue_warning.wav', fatigue_beeps)
    print("⚠️ Created: vbt_fatigue_warning.wav (350Hz urgent pulse - FATIGUE!)")
    
    print("\n⚡ PERFORMANCE-OPTIMIZED VBT Audio System Created!")
    print("🎯 OPTIMIZATION FEATURES:")
    print("   🔴 Below Target = MAX VOLUME (1.0) + URGENT DOUBLE BEEP")
    print("   🟢 Target Hit = QUICK 80ms beep (instant feedback)")
    print("   💎 Above Target = Pleasant double beep (achievement)")
    print("   🔥 Exceptional = Fast celebration (don't waste time)")
    print("   🏁 Set Complete = Clear but quick finish")
    print("   ⚠️ Fatigue = 2-pulse warning (faster than 3)")
    print("\n🚀 PERFORMANCE FOCUS:")
    print("   ⚡ Minimal fade times (5ms)")
    print("   ⚡ Short durations (60-120ms)")
    print("   ⚡ Loud hedef altı warning")
    print("   ⚡ INSTANT athlete response!")

if __name__ == "__main__":
    main()