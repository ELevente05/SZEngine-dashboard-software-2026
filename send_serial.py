import serial

port = "COM9"
baud = 115200

try:
    ser = serial.Serial(port, baud, timeout=1)
    print(f"Connected to {port} at {baud} baud")
    print("Type messages to send (type 'exit' to quit)\n")
    
    while True:
        message = input("Enter message: ")
        if message.lower() == "exit":
            print("Exiting...")
            break
        ser.write((message + '\n').encode())
        print(f"Sent: {message}\n")
        
except Exception as e:
    print(f"Error: {e}")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial port closed")
