import paho.mqtt.client as mqtt
import threading
import time
import random
import subprocess


class SensorEventProducer:
    def __init__(self, broker_address='localhost', topic='benchmark'):
        self.broker_address = broker_address
        self.topic = topic
        self.client = mqtt.Client()
        self.data_thread = None  # Store the thread object

    def connect(self):
        self.client.connect(self.broker_address)
        self.client.loop_start()

    def publish_data(self, id, value, payload, timestamp):
        message = f"{id},{value},{payload},{timestamp}"
        self.client.publish(self.topic, message)

    def generate_data(self, pub_freq: float):
        while True:
            id = 0
            payload = 1337
            value = random.uniform(1., 100.)
            timestamp = int(time.time()*1000.0)
            self.publish_data(id=id, value=value, payload=payload, timestamp=timestamp)
            time.sleep(pub_freq)

    def start_data_generation(self, pub_freq: float = .001):
        # Create a thread to run generate_data
        self.data_thread = threading.Thread(target=self.generate_data, args=(pub_freq,))
        self.data_thread.daemon = True  # Set as a daemon, so it terminates when the program ends
        self.data_thread.start()

    def stop_data_generation(self):
        if self.data_thread and self.data_thread.is_alive():
            self.data_thread.join()  # Wait for the thread to finish


class Broker:
    def __init__(self):
        self.process = None

    def start(self):
        self.process = subprocess.Popen(["mosquitto"])
        time.sleep(2)

    def stop(self):
        self.process.kill()
