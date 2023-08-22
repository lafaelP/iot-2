import os
import plotly.graph_objects as go
import pandas as pd
from influxdb_client import InfluxDBClient
from nicegui import ui

# InfluxDB configuration
INFLUX_URL = "https://us-east-1-1.aws.cloud2.influxdata.com"
ORG = "a11b07c582b5ad36"
BUCKET = "iot-2"
TOKEN = "WsDF2EMLG0BX27qAxysiWA3t4VTPka2XGp2krlne5vbD5HPjpC4PSlGqDiZf3hF6WZ5zQIOys1bdC7H93q0OLw=="

# Create InfluxDB client
client = InfluxDBClient(url=INFLUX_URL, token=TOKEN, org=ORG)

# Initial time range in minutes
initial_time_range = 1

chart = go.Figure()

def display_measurement_data(measurement_choice, time_range_minutes, field):
    query = f'from(bucket:"{BUCKET}") |> range(start: -{time_range_minutes}m) |> filter(fn: (r) => r._measurement == "{measurement_choice}") |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")'
    result = client.query_api().query_data_frame(query)

    if field in result.columns:
        chart.data = []  
        chart.add_trace(go.Scatter(x=result["_time"], y=result[field], mode='lines', name=field))
        chart.update_layout(title=f"{field} over Time", xaxis_title="Time", yaxis_title=field)
        chart_placeholder.update()  
        data_for_csv = result[["_time", field]]
        csv_button.data = data_for_csv.to_csv(index=False)
    else:
        ui.warning(f"Field '{field}' not found in the data for the selected measurement.")

time_range_options = [1, 5, 10, 30, 60, 120, 180]
time_range_choices = [f"{time} minute{'s' if time > 1 else ''}" for time in time_range_options]
time_range_dropdown = ui.select(time_range_choices, value=f"{initial_time_range} minutes")

measurement_choice = ui.select(["BME measurements", "INA measurements"], value="BME measurements")

field_choice = ui.select(["Humidity", "Pressure", "Temperature", "busVoltage", "Current", "Power", "ShuntVoltage"], value="Humidity")

chart_placeholder = ui.plotly(chart)

def update_chart():
    display_measurement_data(measurement_choice.value, int(time_range_dropdown.value.split()[0]), field_choice.value)

def download_csv(x_data, y_data, field_name):
    df = pd.DataFrame({"Time": x_data, field_name: y_data})
    csv_data = df.to_csv(index=False)
    csv_filename = f"{field_name}_data.csv"

    # Specify the absolute path where the CSV file should be saved
    save_path = os.path.join(os.path.expanduser("~"), "Downloads", csv_filename)

    with open(save_path, "w") as f:
        f.write(csv_data)
    print(f"CSV file '{csv_filename}' downloaded successfully to '{save_path}'!")

csv_button = ui.button("Download CSV", on_click=lambda: download_csv(chart.data[0].x, chart.data[0].y, field_choice.value))

ui.button("Update Chart", on_click=update_chart),

ui.run(native=True)