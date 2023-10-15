from experiments.emulator.mqtt_generator import Broker, SensorEventProducer
from experiments.plotting.latency import load_latency_results, print_evt_ts_stats

if __name__ == "__main__":
    broker = Broker()
    broker.start()

    producer = SensorEventProducer()
    producer.connect()
    producer.start_data_generation()

    try:
        while True:
            pass
    except KeyboardInterrupt:
        # Handle Ctrl+C
        producer.stop_data_generation()
        broker.stop()
