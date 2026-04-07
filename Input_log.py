import serial
import time
import csv
import sys

# ==========================================
# --- 1. CONFIGURATION ---
# ==========================================
COM_PORT = 'COM9'
BAUD_RATE = 115200
CSV_FILENAME = 'FSAA_Endu_log_2023-08-26_levi_for_dash.csv'
UPDATE_RATE_HZ = 20

# ==========================================
# --- 2. CSV COLUMN MAPPING ---
# ==========================================
COLUMN_MAP = {
    "RPM": "RPM [61]",
    "G": "VSS Gear [90]",
    "BP": "MAP [20]",
    "OT": "Engine Oil Temp [805]",
    "OP": "Engine Oil Pressure [804]",
    "EWT": "Coolant temp [18]",
    "L": "Lambda [5]",
    "IT": "Intake air temp [17]",
    "EGT": "EGT 1  [128]",
    "BV": "Battery voltage [21]",
}

# ==========================================
# --- 3. MAIN SCRIPT ---
# ==========================================
def main():
    try:
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.1)
        print(f"[{COM_PORT}] Connected successfully.")
    except Exception as e:
        print(f"ERROR: Failed to connect to {COM_PORT}. Is the Serial Monitor closed?")
        sys.exit(1)

    try:
        with open(CSV_FILENAME, mode='r', encoding='utf-8-sig') as file:
            reader = csv.DictReader(file)
            print(f"Loaded CSV: {CSV_FILENAME}")
            print("Beginning Telemetry Playback...\n" + "-" * 50)
            
            row_count = 0
            start_time = time.time()

            for row in reader:
                commands_to_send = []
                print_output = []

                for prefix, csv_header in COLUMN_MAP.items():
                    if csv_header and csv_header in row:
                        val_str = row[csv_header].strip()
                        if val_str:  
                            try:
                                val_float = float(val_str)
                                
                                if prefix == "BP":
                                    val_float = val_float / 100.0
                                    
                                    # Optional: Display Gauge Boost (0 = atmosphere) 
                                    # instead of Absolute (1 = atmosphere), uncomment the line below:
                                    # val_float = val_float - 1.0 
                                    # if val_float < 0: val_float = 0.0
                                
                                if prefix in ["RPM", "G", "SoC"]:
                                    val_formatted = str(int(val_float))
                                else:
                                    val_formatted = f"{val_float:.2f}"

                                commands_to_send.append(f"{prefix}{val_formatted}\n")
                                
                                if prefix in ['RPM', 'G', 'BP', 'OT']:
                                    print_output.append(f"{prefix}: {val_formatted}")
                            
                            except ValueError:
                                pass

                if commands_to_send:
                    for cmd in commands_to_send:
                        ser.write(cmd.encode('utf-8'))
                    
                    print(f"Row {row_count:05d} | " + " | ".join(print_output))
                    time.sleep(1.0 / UPDATE_RATE_HZ)
                
                row_count += 1

            duration = time.time() - start_time
            print("-" * 50 + f"\nPlayback Complete! Sent {row_count} rows in {duration:.1f} seconds.")

    except FileNotFoundError:
        print(f"ERROR: Could not find '{CSV_FILENAME}'. Check the file name and folder.")
    except KeyboardInterrupt:
        print("\nPlayback Manually Stopped.")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == '__main__':
    main()