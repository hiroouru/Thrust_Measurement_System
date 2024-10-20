import serial
import time
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from threading import Thread

# Configure the serial port
serial_port = 'COM7'  # Replace with real serial port
baud_rate = 115200

# Initialize global variables
data_values = []
plotting_started = False

# Function to read data from the serial port
def read_serial_data():
    global data_values, plotting_started
    try:
        with serial.Serial(serial_port, baud_rate, timeout=1) as ser:
            while True:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    try:
                        value = float(line)
                        data_values.append(value)
                        if not plotting_started:
                            plotting_started = True
                    except ValueError:
                        pass  # Ignore invalid data
    except serial.SerialException as e:
        print(f"Serial error: {e}")

# Function to update the plot
def update_plot(frame):
    if plotting_started:
        plt.cla()
        plt.plot(data_values, label='Load Cell Data')
        plt.xlabel('Time')
        plt.ylabel('Load Cell Data')
        plt.title('Real-time Load Cell Data')
        plt.legend(loc='upper right')

# Main function
if __name__ == "__main__":
    # Set up the plot
    fig = plt.figure()
    ani = FuncAnimation(fig, update_plot, interval=1000)

    # Start reading serial data
    serial_thread = Thread(target=read_serial_data)
    serial_thread.start()

    # Display the plot
    plt.show()

    # Join the thread after the plot is closed
    serial_thread.join()
