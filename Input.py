import serial
import time
import math

# --- CONFIGURATION ---
COM_PORT = 'COM9'
BAUD_RATE = 115200

try:
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.1)
    print(f"Connected to {COM_PORT}. Starting Race Simulation...")
except Exception as e:
    print(f"Failed to connect: {e}")
    exit()

# Race variables
time_elapsed = 0
gear = 1
rpm = 4000
oil_temp = 85.0
boost = 0.0
hybrid_temp = 25.0
soc = 80.0 

try:
    while True:
        
        rpm_cycle = math.sin(time_elapsed) 
        rpm = int(4500 + (rpm_cycle + 1) * 1350)
        
        
        if rpm > 7000 and gear < 6:
            gear += 1
        elif rpm < 5000 and gear > 1:
            gear -= 1

        boost = max(0.0, (rpm - 3000) / 2500.0)
        if boost > 1.8: boost = 1.8

        oil_temp += 0.01
        
        hybrid_temp += (0.005 + (boost * 0.01)) 
        
        soc -= 0.05
        if soc < 0: soc = 0

        commands = [
            f"RPM{rpm}\n",
            f"G{gear}\n",
            f"BP{boost:.2f}\n",
            f"OT{oil_temp:.1f}\n",
            f"HT{hybrid_temp:.1f}\n",
            f"SoC{int(soc)}\n"
        ]
        
        for cmd in commands:
            ser.write(cmd.encode('utf-8'))
            
        print(f"Gear: {gear} | RPM: {rpm} | Boost: {boost:.2f} | OilT: {oil_temp:.1f} | HT: {hybrid_temp:.1f} | SoC: {int(soc)}%")
        
        time.sleep(0.1)
        time_elapsed += 0.1

except KeyboardInterrupt:
    print("\nSimulation Stopped.")
    ser.close()