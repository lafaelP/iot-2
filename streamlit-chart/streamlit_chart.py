import os
from influxdb_client import InfluxDBClient
import streamlit as st
import plotly.graph_objects as go
import time

INFLUX_URL = "https://us-east-1-1.aws.cloud2.influxdata.com"
ORG = "a11b07c582b5ad36"
BUCKET = "iot-2"
TOKEN = "WsDF2EMLG0BX27qAxysiWA3t4VTPka2XGp2krlne5vbD5HPjpC4PSlGqDiZf3hF6WZ5zQIOys1bdC7H93q0OLw=="

@st.cache_data
def get_weather_data(time_range=1000):
    client = InfluxDBClient(url=INFLUX_URL, token=TOKEN, org=ORG)
    query_api = client.query_api()

    time_range_str = f"{time_range}m"

    query_bme = f'from(bucket:"{BUCKET}") |> range(start: -{time_range_str}) |> filter(fn: (r) => r._measurement == "BME measurements") |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")'
    result_bme = query_api.query_data_frame(query_bme)

    query_ina = f'from(bucket:"{BUCKET}") |> range(start: -{time_range_str}) |> filter(fn: (r) => r._measurement == "INA measurements") |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")'
    result_ina = query_api.query_data_frame(query_ina)

    client.close()
    return result_bme, result_ina

def main():
    st.title("InfluxDB Data Viewer")

    col1, col2, col3 = st.columns(3)

    time_range_options = [1, 5, 10, 30, 60, 120, 180]
    time_range_choices = [f"{time} minute{'s' if time > 1 else ''}" for time in time_range_options]
    if 'time_range_minutes' not in st.session_state:
        st.session_state.time_range_minutes = time_range_choices[0]
    time_range_minutes = col1.selectbox("Select Time Range", time_range_choices, index=time_range_options.index(int(st.session_state.time_range_minutes.split()[0])))
    st.session_state.time_range_minutes = time_range_minutes
    selected_minutes = int(time_range_minutes.split()[0])

    measurement_choice = col2.selectbox("Select Measurement", ["BME measurements", "INA measurements"])
    if 'measurement_choice' not in st.session_state:
        st.session_state.measurement_choice = measurement_choice
    if st.session_state.measurement_choice != measurement_choice:
        if measurement_choice == "BME measurements":
            st.session_state.field_choice = "Humidity"
        else:
            st.session_state.field_choice = "busVoltage"
        st.session_state.measurement_choice = measurement_choice

    if 'field_choice' not in st.session_state:
        st.session_state.field_choice = "Humidity"

    if measurement_choice == "BME measurements":
        field_choice = col3.selectbox("Select Field", ["Humidity", "Pressure", "Temperature"], index=["Humidity", "Pressure", "Temperature"].index(st.session_state.field_choice))
        st.session_state.field_choice = field_choice
    elif measurement_choice == "INA measurements":
        field_choice = col3.selectbox("Select Field", ["busVoltage", "Current", "Power", "ShuntVoltage"], index=["busVoltage", "Current", "Power", "ShuntVoltage"].index(st.session_state.field_choice))
        st.session_state.field_choice = field_choice

    chart_placeholder = st.empty()

    csv_button = None

    while True:
        display_measurement_data(measurement_choice, selected_minutes, st.session_state.field_choice, chart_placeholder)

        if chart_placeholder.data:
            download_data, _ = get_weather_data(selected_minutes)
            csv_filename = f"{measurement_choice}_{st.session_state.field_choice}_data.csv"
            csv_key = f"{csv_filename}_{measurement_choice}"
            csv_data = download_data.to_csv(index=False, encoding='utf-8-sig')
            if csv_button is None:
                csv_button = st.download_button("Download CSV", data=csv_data, file_name=csv_filename, key=csv_key)

        time.sleep(1)

def display_measurement_data(measurement_choice, time_range_minutes, field, chart_placeholder):
    if field not in ["Humidity", "Pressure", "Temperature", "busVoltage", "Current", "Power", "ShuntVoltage"]:
        st.write(f"Invalid field '{field}' selected.")
        return

    client = InfluxDBClient(url=INFLUX_URL, token=TOKEN, org=ORG)
    query_api = client.query_api()

    time_range_str = f"{time_range_minutes}m"

    query = f'from(bucket:"{BUCKET}") |> range(start: -{time_range_str}) |> filter(fn: (r) => r._measurement == "{measurement_choice}") |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")'
    result = query_api.query_data_frame(query)

    client.close()

    if field in result.columns:
        fig = go.Figure()
        fig.add_trace(go.Scatter(x=result["_time"], y=result[field], mode='lines', name=field))
        fig.update_layout(title=f"{field} over Time", xaxis_title="Time", yaxis_title=field)
        chart_placeholder.plotly_chart(fig, use_container_width=True)
    else:
        st.write(f"Field '{field}' not found in the data for the selected measurement.")

if __name__ == "__main__":
    main()