import serial
import time
import random

# --- CONFIGURATION ---
COM_PORT = 'COM7' # Change this to your HUB's COM port!
BAUD_RATE = 115200

try:
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.1)
    print(f"Connected to {COM_PORT}. Starting Track Telemetry...")
except Exception as e:
    print(f"Failed to connect: {e}")
    exit()

# --- TRACK LAYOUT DEFINITION ---
# duration = seconds spent in this sector
# target_gear = the gear the car wants to reach by the end of the sector
track_sectors = [
    {"name": "Main Straight", "action": "accel", "duration": 8.0, "target_gear": 6},
    {"name": "Turn 1 (Heavy Brake)", "action": "brake", "duration": 2.0, "target_gear": 2},
    {"name": "Turns 2-3 (Tight Chicane)", "action": "modulate", "duration": 5.0, "target_gear": 3},
    {"name": "Short Chute", "action": "accel", "duration": 3.0, "target_gear": 4},
    {"name": "Turn 4 (Hairpin)", "action": "brake", "duration": 1.5, "target_gear": 2},
    {"name": "Back Straight", "action": "accel", "duration": 7.0, "target_gear": 6},
    {"name": "High Speed Sweeper", "action": "hold", "duration": 4.0, "target_gear": 5},
    {"name": "Final Corner", "action": "brake", "duration": 2.5, "target_gear": 3}
]

# --- BASE ENGINE STATES ---
rpm = 3700
gear = 2
oil_temp = 85.0
ewt = 88.0
iwt = 35.0
hybrid_temp = 25.0
soc = 80.0
lap_count = 1

def send_telemetry(action):
    global oil_temp, ewt, iwt, hybrid_temp, soc
    
    # 1. Physics Calculations
    # Boost builds over 3700 RPM, cuts to 0 on braking
    boost = max(0.0, (rpm - 3700) / 7300.0) if action in ['accel', 'hold', 'modulate'] else 0.0
    if boost > 1.8: boost = 1.8 + random.uniform(-0.02, 0.02) # Boost flutter
    
    # Oil Pressure directly tied to RPM (1.5 Bar at idle, 5.0 Bar at redline)
    oil_press = (rpm / 11000.0) * 3.5 + 1.5
    
    # Lambda: 0.82 (rich) on throttle, 1.10 (lean cut) on braking
    lam = 0.82 if boost > 0.5 else (1.10 if action == 'brake' else 0.95)
    
    # EGT rises with boost and RPM
    egt = 400 + (boost * 250) + ((rpm / 11000.0) * 200)
    
    # 2. Cumulative Wear & Tear
    oil_temp += 0.005
    ewt += 0.002
    iwt += 0.001
    hybrid_temp += (0.015 if boost > 1.0 else -0.002) # Heats up fast under boost
    
    soc -= 0.015
    if soc < 0: soc = 0
    hybrid_volts = 36.0 - ((80 - soc) * 0.1) # Voltage sags as battery dies
    
    # 3. Format and Send to ESP32 Hub
    commands = [
        f"RPM{int(rpm)}\n", f"G{gear}\n", f"BP{boost:.2f}\n",
        f"OT{oil_temp:.1f}\n", f"OP{oil_press:.1f}\n", f"EWT{ewt:.1f}\n",
        f"IWT{iwt:.1f}\n", f"L{lam:.2f}\n", f"IT{iwt+2.0:.1f}\n",
        f"EGT{egt:.0f}\n", f"BV{13.8 + random.uniform(-0.1, 0.1):.1f}\n",
        f"HT{hybrid_temp:.1f}\n", f"HV{hybrid_volts:.1f}\n", f"SoC{int(soc)}\n"
    ]
    
    for cmd in commands: 
        ser.write(cmd.encode('utf-8'))

# --- MAIN RACE LOOP ---
print("--- GREEN FLAG ---")
try:
    while True:
        print(f"\n--- STARTING LAP {lap_count} ---")
        
        for sector in track_sectors:
            print(f"Entering {sector['name']}...")
            duration = sector['duration']
            action = sector['action']
            target_gear = sector['target_gear']
            
            ticks = int(duration * 20) # Calculate how many 50ms updates fit in this sector
            
            for _ in range(ticks):
                # Simulated Driver Inputs
                if action == 'accel':
                    rpm += 160
                    if rpm > 10500 and gear < target_gear: # Shift Up
                        gear += 1
                        rpm = 5500
                    elif rpm > 11000: 
                        rpm = 11000 # Hitting the rev limiter!
                        
                elif action == 'brake':
                    rpm -= 250
                    if rpm < 5000 and gear > target_gear: # Shift Down (Rev Match)
                        gear -= 1
                        rpm = 7500 
                    elif rpm < 3700: 
                        rpm = 3700
                        
                elif action == 'modulate':
                    rpm += random.randint(-120, 150) # Throttle fluttering
                    if rpm > 10000 and gear < target_gear: 
                        gear += 1
                        rpm = 5500
                    elif rpm < 3700 and gear > target_gear: 
                        gear -= 1
                        rpm = 7000
                        
                elif action == 'hold':
                    rpm += random.randint(-40, 40) # Cruising through a long sweeper
                    
                # Blast the data and wait 50ms (20Hz)
                send_telemetry(action)
                time.sleep(0.05)
                
        lap_count += 1

except KeyboardInterrupt:
    print("\nCheckered Flag. Simulation Stopped.")
finally:
    ser.close()